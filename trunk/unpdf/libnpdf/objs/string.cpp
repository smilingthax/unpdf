#include "string.h"
#include <assert.h>
#include "../io/base.h"
#include "../io/crypt.h"

namespace PDFTools {

String::String() // {{{
  : val(),
    as_hex(false)
{
}
// }}}

String::String(const char *str) // {{{
  : val(str),
    as_hex(false)
{
}
// }}}

String::String(const char *str,int len,bool as_hex) // {{{
  : val(str,len),
    as_hex(as_hex)
{
}
// }}}

String::String(const std::string &str,const Decrypt *decrypt,bool _as_hex) // {{{
{
  if (decrypt) {
    (*decrypt)(val,str);
  } else {
    val.assign(str);
  }
  as_hex=_as_hex;
}
// }}}

// TODO: if (output==Encrypt_Output) ...
void String::print(Output &out) const // {{{
{
  if (as_hex) {
    out.put('<');
    for (int iA=0;iA<(int)val.size();iA++) {
      if ( (iA)&&(iA%40==0) ) {
        out.put('\n');
      }
      out.printf("%02x",(unsigned char)val[iA]);
    }
    out.put('>');
  } else {
    out.put('(');
    // escape special chars: \0 \\ \( \) \r     - don't bother about balanced parens
    const char *buf=val.data();
    int iA=0;
    for (int iB=0;iB<(int)val.size();iA++,iB++) {
      if ( (buf[iA]==0)||(buf[iA]=='(')||(buf[iA]==')')||(buf[iA]=='\\')||(buf[iA]=='\r') ) {
        out.write(buf,iA);
        if (buf[iA]==0) {
          out.write("\\000",4);
        } else {
          out.put('\\');
          out.put(buf[iA]);
        }
        buf+=iA+1;
        iA=-1;
      }
    }
    out.write(buf,iA);
//    out.write(val.data(),val.size()); // TODO? maybe wrap line after...
    out.put(')');
  }
}
// }}}

} // namespace PDFTools
