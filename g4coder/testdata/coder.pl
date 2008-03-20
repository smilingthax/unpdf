#!/usr/bin/perl
# Prototype g3 (only) coder

$line=<>;
if (!$line eq "P1") {
  die "Unknown Fileformat\n";
}
while (<>) {
  if (/^#/) {
  } elsif (/([0-9]+) ([0-9]+)/) {
    $width=$1;
#    $height=$2;
    last;
  }
}
$data="";
while (<>) {
  /([01]+)/;
  $data.=$1;
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

@lastline=(-1);
@curline=();

# {{{ rlecode(\$data,\@line)
sub rlecode {
  my $data=shift;
  my $line=shift;
  my $x=0,$y=$width;
  
  @$line=();
  while ( ($y>0)&&(length($$data)) ) { 
    while ( (substr($$data,$x,1) eq "0")&&($y>0) ) {
      $x++;
      $y--;
    }
    push @$line,$x;
    last if ($y<=0);
    while ( (substr($$data,$x,1) eq "1")&&($y>0) ) {
      $x++;
      $y--;
    }
    push @$line,$x;
  }
  $$data=substr($$data,$x);
}
# }}}

# {{{ writehuff(\%table,$code)
sub writehuff {
  my $table=shift;
  my $code=shift;

  print STDERR $$table{$code};
}
# }}} 

while ($data) {
  $a0=0;
  $black=0;
  print "----- ";
  rlecode(\$data,\@curline);
  foreach $it (@curline) {
    print "$it,";
  }
  print "\n";
  @saveline=@curline;
  while (defined $curline[0]) {
    print "§$a0§";
  foreach $it (@lastline) {
    print "$it,";
  }
  print "\n";
    if (defined $lastline[1]) {
      $b1=$lastline[1];
    } else {
      $b1=$width;
    }
    $x=$b1-$curline[0];
    print "$b1 - $x\n";
    if ( (defined $curline[0])&&(defined $lastline[2])&&($lastline[2]<$curline[0]) ) {
      print "P ";
      writehuff(\%opcodes,"P");
      $a0=$lastline[2];
    } elsif ( ($x>=-3)&&($x<=3) ) {
      print "V";
      if ($x<0) {
        print "R".(-$x)." ";
        writehuff(\%opcodes,"VR".(-$x));
      } elsif ($x>0) {
        print "L".$x." ";
        writehuff(\%opcodes,"VL".$x);
      } else {
        print " ";
        writehuff(\%opcodes,"V");
      }
      $a0=$curline[0];
      shift @curline;
      $black^=1;
      if ($lastline[0]<=$a0) { # either previous(color was wrong) or next
        shift @lastline;
      } else { # keep black balance
        unshift @lastline,-1;
      }
    } else {
      if ($black) {
        print "Hb";
        writehuff(\%opcodes,"H");
        writehuff(\%blackhuff,($curline[0]-$a0));
        writehuff(\%whitehuff,($curline[1]-$curline[0]));
      } else {
        print "Hw";
        writehuff(\%opcodes,"H");
        writehuff(\%whitehuff,($curline[0]-$a0));
        writehuff(\%blackhuff,($curline[1]-$curline[0]));
      }
      print "".($curline[0]-$a0).",".($curline[1]-$curline[0])." ";
      $a0=$curline[1];
      shift @curline;
      shift @curline;
    }
    while ( (defined $lastline[1])&&($lastline[1]<=$a0) ) {
      shift @lastline;
      shift @lastline;
    }
  }
  print "\n";
  @lastline=@saveline;
  unshift @lastline,-1;
}
print "EOL EOL\n";
writehuff(\%opcodes,"EOL");
writehuff(\%opcodes,"EOL");
print STDERR "\n";
