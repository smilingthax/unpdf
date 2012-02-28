#ifndef _STATEPP_HPP
#define _STATEPP_HPP

#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/blank.hpp>
#include <type_traits>
#include <functional>
#include "bindthis.h"

struct Trans;
// gcc sucks
#define TTS_BUG(S,E,Snew) Snew operator()(S &s,const E &e) const;

// NOTE: most similar to http://boost-extension.redshoelace.com/docs/boost/fsm/doc/state_machine.html

/* Usage e.g.:
struct S1 {
  void me() const { printf("S1\n"); }
};
struct S2 {
  void me() const { printf("S2\n"); }
};
STATEPP_MAKE_CALLIT(void,me(),CallMe);
STATEPP_MAKE_GETMEMBER(me,BoundMe);

boost::variant<S1,S2> state;
use_CallMe(state); // .me() must be in all instances at compile time
get_BoundMe<void(void)>(state)(); // same effect
get_BoundMe<void(void)>(&state)(); // might be empty instead (segfault at invocation, if you do not check...)
int x=get_BoundMe<int>(state)(); // if it's "int me" ... compile time check
int *x=get_BoundMe<int>(&state)(); // ptr or null

... state=S2(); ...
... state=S1(); ... etc
*/

/* Usage e.g with common base:
struct S_base { virtual void me() const=0; }; // no virtual dtor needed in variant (but better add one...)
struct S1 : S_base {...};
struct S2 : S_base {...};
boost::variant<S1,S2> state;

StatePP::get_base<S_base>(state).me();

// Now the event stuff:
struct S1 {};
struct S2 {};
struct E1 {};
struct E2 {};

struct Trans {
  TTS(S1,E1,S1);
  TTS(S1,E2,S2);
  TTS(S2,E2,S2);
  TTS(S2,E1,S1);
};

boost::variant<S1,S2> state; // initial is S1
bool success=StatePP::next_state_static<Trans>(state,E1());
StatePP::next_state_static<Trans>(state,E1());
StatePP::next_state_static<Trans>(state,E2());
...

*/


namespace StatePP {
  namespace detail {
    template <typename Base>
    struct GetBase : boost::static_visitor<Base&> {
      template <class S>
      Base &operator()(S &s) const { 
        static_assert(std::is_base_of<Base,S>::value,"Not all states have the needed base class"); // just hint the user why the next line fails
        return static_cast<Base&>(s);
      }
    }; 
  } // namespace detail

  template <typename Base,typename StateVariant>
  Base &get_base(StateVariant &v) {
    return boost::apply_visitor(detail::GetBase<Base>(),v);
  }
  // Use:  StatePP::get_base<S_base>(state).me();

} // namespace StatePP

// This is compile time 'virtual' calling of a member function present in all states (same return type!); and NOT needing a common base!
// For certain things however dynamic (virtual + common base, using StatePP::get_base<Base>()) works better
// still you will want so declare/use your own static_visitor for variant return types, state-type based overloads, etc.
#define STATEPP_MAKE_CALLIT(RetType,StateMemFunCall,TName)  \
  namespace { \
    template <typename UserDataT=boost::blank> \
    struct TName : boost::static_visitor<RetType> { \
      TName(UserDataT user) : user(user) {} \
      template <class S> \
      RetType operator()(S &s) const { return s.StateMemFunCall; } \
      UserDataT user; \
    }; \
    TName<boost::blank> make_##TName() { \
      return TName<boost::blank>(boost::blank()); \
    } \
    template <typename UserDataT> \
    TName<UserDataT &> make_##TName(UserDataT &user) { \
      return TName<UserDataT &>(user); \
    } \
    template <typename UserDataT> \
    TName<const UserDataT &> make_##TName(const UserDataT &user) { \
      return TName<const UserDataT &>(user); \
    } \
    template <typename StateVariant> \
    RetType use_##TName(StateVariant &v) { \
      return boost::apply_visitor(TName<boost::blank>(boost::blank()),v); \
    } \
    template <typename StateVariant,typename UserDataT> \
    RetType use_##TName(StateVariant &v,UserDataT &user) { \
      return boost::apply_visitor(TName<UserDataT &>(user),v); \
    } \
    template <typename StateVariant,typename UserDataT> \
    RetType use_##TName(StateVariant &v,const UserDataT &user) { \
      return boost::apply_visitor(TName<const UserDataT &>(user),v); \
    } \
  }
// Example: STATEPP_MAKE_CALLIT(void,me(),CallMe); // outside fn
//     Use:  boost::apply_visitor(make_CallMe(),state); // inside
//     Use:  use_CallMe(state);
// Example: STATEPP_MAKE_CALLIT(void,me(user.first,3),CallMe);
//     Use:  boost::apply_visitor(make_CallMe(make_pair(5,6)),state);
//     Use:  use_CallMe(state,make_pair(5,6));

namespace StatePP {
  namespace detail {
    template <typename T> struct is_std_function { static const bool value=false; };
    template <typename T> struct is_std_function<std::function<T>> { static const bool value=true; };
    template <typename T,bool C> struct with_ptr { typedef T type; };
    template <typename T>        struct with_ptr<T,true> { typedef typename std::add_pointer<T>::type type; };
  } // namespace detail
} // namespace StatePP

#define STATEPP_MAKE_GETMEMBER(MemName,TName) \
  namespace { \
    template <typename RetType,bool AsPtr> /* AsPtr only considered if !is_std_function<RetType> */ \
    struct TName { /* : boost::static_vistor<...> just addes result_type, see next line */ \
      typedef typename StatePP::detail::with_ptr<RetType,!StatePP::detail::is_std_function<RetType>::value && AsPtr>::type result_type; \
      static const bool doCall=StatePP::detail::is_std_function<RetType>::value; \
      template <class S> struct has_member { \
        typedef char (&Yes)[1]; typedef char (&No)[2]; \
        template <typename T> static Yes chk(T *t,char (*)[sizeof(&T::MemName)]=0); \
        static No chk(...); \
        static const bool value=sizeof(chk((S *)0))==sizeof(Yes); \
      }; \
      template <class S> \
      result_type operator()(S &s,char (*)[doCall && std::is_member_function_pointer<decltype(&S::MemName)>::value]=0) const { \
        return bind_this(&S::MemName,&s); \
      } \
      template <class S> \
      result_type operator()(S &s,char (*)[doCall && std::is_function<typename std::remove_pointer<decltype(&S::MemName)>::type>::value]=0) const { /* static function members */ \
        /* tip(... undefined reference): gcc requires "const S::MemName" in namespace for 'instantiation' of statics */ \
        return &S::MemName; \
      } \
      template <class S> \
      result_type operator()(S &s,char (*)[!doCall && AsPtr && std::is_member_object_pointer<decltype(&S::MemName)>::value]=0) const { \
        return &s.MemName; \
      } \
      template <class S> \
      result_type operator()(S &s,char (*)[!doCall && AsPtr && !std::is_member_pointer<decltype(&S::MemName)>::value && !std::is_function<typename std::remove_pointer<decltype(&S::MemName)>::type>::value]=0) const { /* static data members */ \
        return &S::MemName; \
      } \
      template <class S> \
      result_type operator()(S &s,char (*)[!doCall && !AsPtr && std::is_member_object_pointer<decltype(&S::MemName)>::value]=0) const { \
        return s.MemName; \
      } \
      template <class S> \
      result_type operator()(S &s,char (*)[!doCall && !AsPtr && !std::is_member_pointer<decltype(&S::MemName)>::value && !std::is_function<typename std::remove_pointer<decltype(&S::MemName)>::type>::value]=0) const { /* static data members */ \
        return S::MemName; \
      } \
      template <class S> \
      result_type operator()(S &s,char (*)[AsPtr && !has_member<S>::value]=0) const { \
        return result_type(); \
      } \
      template <class S> \
      const result_type &operator()(S &s,char (*)[!AsPtr && !has_member<S>::value]=0) const { \
        /* gcc needs something not const false to even look at S (or SFINAE): */ \
        static_assert(AsPtr,#TName ": No matching member ." #MemName " for non-ptr get in some instances (see first line of this gcc error: ... [with S= ??? ...)"); \
        return *(typename std::add_pointer<result_type>::type)0; \
      } \
    }; \
    template <typename RetType,typename StateVariant> \
    RetType get_##TName(StateVariant &v) { \
      return boost::apply_visitor(TName<RetType,false>(),v); \
    } \
    template <typename RetType,typename StateVariant> \
    typename std::enable_if<!std::is_function<RetType>::value,RetType *>::type get_##TName(StateVariant *v) { \
      return boost::apply_visitor(TName<RetType,true>(),*v); \
    } \
    template <typename RetType,typename StateVariant> \
    typename std::enable_if<std::is_function<RetType>::value,std::function<RetType>>::type get_##TName(StateVariant &v) { \
      return boost::apply_visitor(TName<std::function<RetType>,false>(),v); \
    } \
    template <typename RetType,typename StateVariant> \
    typename std::enable_if<std::is_function<RetType>::value,std::function<RetType>>::type get_##TName(StateVariant *v) { \
      return boost::apply_visitor(TName<std::function<RetType>,true>(),*v); \
    } \
  }
// Example: STATEPP_MAKE_GETMEMBER(ma,BoundMa);
//     Use: get_BoundMa<void(int)>(state)(42);
//     Use: auto b=get_BoundMa<void(int)>(&state); // std::function, actually (also in previous line, but there surely !=false)
//          if (!b) ... else b(42); 

// Example: STATEPP_MAKE_GETMEMBER(bl,BoundBl);
//     Use: const int *k=get_BoundBl<const int>(&state); // null if not there
//          printf("%p\n",k);  // ptr
//     Use: int k=get_BoundBl<int>(state); // compile time error if not everywhere
//      or: int &k=get_BoundBl<int&>(state); // beware of const int members...
//          printf("%d\n",k);

namespace StatePP {
  namespace detail {
    template <class StateVisitor,class Event,class Transitions>
    struct NextState : boost::static_visitor<bool> {
      NextState(StateVisitor &v,Event e,Transitions t) : v(v),e(e),t(t) {}

      template <class State>
      bool operator()(State &s,char (*)[sizeof(std::declval<Transitions>()(s,std::declval<Event>()))]=0) const {
        v=t(s,e);
        return true;
      }
 
      bool operator()(...) const { return false; }
    private:
      StateVisitor &v;
      Event e;
      Transitions t;
    };
  } // namespace detail
 
  // TODO: fail on ref, bool on ptr (state)
  template <class Transitions=Trans,class StateVisitor,class Event>
  bool next_state(StateVisitor &state,Event &&event,Transitions &&trans=Transitions()) {
    return boost::apply_visitor(detail::NextState<StateVisitor,Event,Transitions>(state,std::forward<Event>(event),std::forward<Transitions>(trans)),state);
  }
 
// TODO? what with const
#define TTR_0(S,E,Snew)      Snew operator()(S &s,const E &e) const { return Snew(); }
#define TTR_1(S,E,Snew,code) Snew operator()(S &s,const E &e) code
#define TTR_HLP(arg1,arg2,arg3,arg4,func,...) func
#define TTR(...) TTR_HLP(__VA_ARGS__,TTR_1,TTR_0)(__VA_ARGS__)
  
  namespace detail {
    template <class StateVisitor,class Event,class Transitions>
    struct NextStateStatic : boost::static_visitor<bool> {
      NextStateStatic(StateVisitor &v,Event e) : v(v),e(e) {}
 
      template <class State>
#ifndef TTS_BUG
      bool operator()(State &s,char (*)[sizeof(std::declval<Transitions>().next(std::declval<State&>(),std::declval<Event>()))]=0) const {
#else
      bool operator()(State &s,char (*)[sizeof(std::declval<Transitions>()(std::declval<State&>(),std::declval<Event>()))]=0) const {
#endif
        v=Transitions::next(s,e);
        return true;
      }
 
      bool operator()(...) const { return false; }
    private:
      StateVisitor &v;
      Event e;
    };
  } // namespace detail
 
  template <class Transitions=Trans,class StateVisitor,class Event>
  bool next_state_static(StateVisitor &state,Event &&event) {
    return boost::apply_visitor(detail::NextStateStatic<StateVisitor,Event,Transitions>(state,std::forward<Event>(event)),state);
  }

#ifndef TTS_BUG
#define TTS_BUG(S,E,Snew)
#endif
#define TTS_0(S,E,Snew)      TTS_BUG(S,E,Snew) static Snew next(S &s,const E &e) { return Snew(); }
#define TTS_1(S,E,Snew,code) TTS_BUG(S,E,Snew) static Snew next(S &s,const E &e) code
#define TTS_HLP(arg1,arg2,arg3,arg4,func,...) func
#define TTS(...) TTS_HLP(__VA_ARGS__,TTS_1,TTS_0)(__VA_ARGS__)

/* GCC 4.6 sucks: trick (add at end of struct, when not using TTS_BUG)
  template <class S,class E>
  auto operator()(S &s,const E &e) const -> decltype(next(s,e));
*/
} // namespace StatePP

#endif
