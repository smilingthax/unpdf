#!/usr/bin/perl
# Decoder for debugging

$mode=shift;
if ($mode eq "-g3") {
  $x=shift;
  if ($x eq "-2d") {
    $mode.=$x;
    $width=shift;
  } else {
    $width=$x;
  }
} else {
  $width=shift;
}

%whitehuff=();
%blackhuff=();
%opcodes=();

# {{{ - Load tables
open F,"table.data" or die "Could not open Datafile: $!\n";
while (<F>) {
  if (/^([^ ]*):$/) {
    if (/^white/) {
      $black=0;
    } elsif (/^black/) {
      $black=1;
    } elsif (/^big/) {
      $black=-1;
    }
  } elsif (/^\"([^,]*)\",\"(.*)\"$/) {
    $opcodes{$1}=$2;
  } elsif (/^([^,]*),\"(.*)\"$/) {
    if ($black==-1) {
      $blackhuff{$1}=$2;
      $whitehuff{$1}=$2;
    } elsif ($black) {
      $blackhuff{$1}=$2;
    } else {
      $whitehuff{$1}=$2;
    }
  } else {
    die "Error in Datafile: $_\n";
  }
}
close(F);
# }}}

# {{{ int readsimple(\%table,\$instring,\$pad)
sub readsimple {
  my $table=shift;
  my $input=shift;
  my $pad=shift;
  my $result=0;

  while (my ($key,$val)=each %$table) {
    if (substr($$input,0,length($val)) eq $val) {
      print $key." "x(length($val)-length($key));
      $$pad.=" "x(length($val));
      $$input=substr($$input,length($val));
      if ($key > 63) { # bigmakeup
        keys %$table;
        if ($result%2560!=0) {
          print "Error with bigmakeup\n";
        }
        $result+=$key;
      } else {
        $result+=$key;
        last;
      }
    }
  }
  return $result;
}
# }}}

# {{{ handle2d($op,\@lastline,\@curline,\$a0,\$black);
sub handle2d {
  my $op=shift;
  my $lastline=shift;
  my $curline=shift;
  my $a0=shift;
  my $black=shift;
  my $x;

  if ($op eq "P") {
    $$a0=$$lastline[2];
    shift @$lastline;
    shift @$lastline;
  } elsif (substr($op,0,1) eq "V") {
    $x=$$lastline[1];
    if (!defined $x) { # synthetic lineend
      $x=$width;
    }
    my $y=substr($op,1,1),$z=substr($op,2,1);
    if ($y) {
      if ($y eq "L") {
        $z=-$z;
      }
    } else {
      $z=0;
    }
    $$a0=$x+$z;
    push @$curline,$$a0; # TODO? clamp $$a0 to [0..$width)
    $$black^=1;
    if ($$lastline[0]<=$$a0) { # either previous(color was wrong) or next
      shift @$lastline;
    } else { # keep black balance
      unshift @$lastline,-1;
    }
  }
}
# }}}

# {{{ rleshow(\@line)
sub rleshow {
  my $line=shift;
  my $x=0,$col=0;

  foreach $it (@$line) {
    for (;$x<$it;$x++) {
      print STDERR $col;
    }
    $col^=1;
  }
  print STDERR "\n";
}
# }}}

 
$input=<>;

@lastline=(-1); # sentinel
@curline=();
$black=0;
$a0=0;

# mode=-g4
#print $input;
$lastlen=length($input)+1;
$pad="";
while (length($input)<$lastlen) {
  $lastlen=length($input);
  keys %opcodes; # reset pointer
  while (($key,$val)=each %opcodes) {
    if (substr($input,0,length($val)) eq $val) {
      if ($key eq "H") {
        if ($black) {
          print "Hb ";
        } else {
          print "Hw ";
        }
        $pad.="   ";
      } else { 
        print $key." "x(length($val)-length($key));
        $pad.=" "x(length($val));
      }
      last;
    }
  }
  $input=substr($input,length($val));
  if ($key eq "H") { # horizontal: read white, black or vice versa
    keys %blackhuff; # reset pointers
    keys %whitehuff;
    if ($black) {
      $a0+=readsimple(\%blackhuff,\$input,\$pad);
      push @curline,$a0;
      $a0+=readsimple(\%whitehuff,\$input,\$pad);
      push @curline,$a0;
    } else {
      $a0+=readsimple(\%whitehuff,\$input,\$pad);
      push @curline,$a0;
      $a0+=readsimple(\%blackhuff,\$input,\$pad);
      push @curline,$a0;
    }
  } else {
    handle2d($key,\@lastline,\@curline,\$a0,\$black);
  }
  while ( (defined $lastline[1])&&($lastline[1]<=$a0) ) { # "implicit" "P"s
    shift @lastline;
    shift @lastline;
  }
  if ($a0>=$width) {
    $black=0;
    @lastline=@curline;
    @curline=();
    $a0=0;
    print "\n";
    foreach my $it (@lastline) { print "$it,"; }
    print "\n";
    rleshow(\@lastline);
    print "$pad";
    unshift @lastline,-1; # sentinel value
  }
}
print "\n";
