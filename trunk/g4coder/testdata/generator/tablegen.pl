#!/usr/bin/perl

%whitehuff=();
%blackhuff=();
%opcodes=();

# {{{ - Load tables
open F,"../testdata/table.data" or die "Could not open Datafile: $!\n";
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

#$bitlen=16;
#print "short table[64]={\n";
#for ($x=0;$x<64;$x++) {
#  $val=$blackhuff{$x};
#  $val=$blackhuff{$x*64+64};
#  printf "{%2d,0x%04x},",length($val),bitval($val);
#  if ($x%8==7) {
#    print "\n";
#  }
#}
#print "}";
#exit;

$whitehuff{-3}="000000000000"; # potential EOL
$blackhuff{-3}="000000000000"; # potential EOL
$whitehuff{-2}="000000000001"; # EOL
$blackhuff{-2}="000000000001"; # EOL

$bitlen=4; # 4,8 bit
$bitsize=1<<$bitlen;

# {{{ int bitval($bits)
sub bitval {
  my $bits=shift;
  my $ret=0;
  my $x=1<<($bitlen-1);

  foreach my $it (split(//,$bits)) {
    if ($it eq "1") {
      $ret+=$x;
    }
    $x>>=1;
  }

  return $ret;
}
# }}}

# build array
@ara=();
for ($x=0;$x<$bitsize;$x++) {
  $ara[$x]=-1;
}
$arasize=$bitsize;
#while (($key,$val)=each %whitehuff) {
#while (($key,$val)=each %blackhuff) {
while (($key,$val)=each %opcodes) {
  $more=0;
  $y=0;
  do {
    $tmp=substr($val,$bitlen*$more,$bitlen);
#    print "#".$tmp."#".bitval($tmp)."#".$key."\n";
    $y+=bitval($tmp);
    if (length($val)>(1+$more)*$bitlen) {
      $more++;
    } else {
      $more=length($val)-(1+$more)*$bitlen;
    }
    if ($ara[$y]==-1) { # new entry
      if ($more>0) {
        for ($x=0;$x<$bitsize;$x++) {
          $ara[$x+$arasize]=-1;
        }
        $ara[$y]=-$arasize;
        $y=$arasize;
        $arasize+=$bitsize;
      } else { # can insert here
        for ($x=1<<-$more;$x>0;$x--,$y++) {
#          $ara[$y]=$key+length($val)*0x1000;
          $ara[$y]="OP_".$key."+".(sprintf "0x%04x",length($val)*0x1000);
        }
        # will leave loop, set $y=0
      }
    } elsif ($ara[$y]<0) { # go to subentry-start
      $y=-$ara[$y];
    } else {
      print "error: $y: $ara[$y]\n";
    }
  } while ($more>0);
}
print "short table[$arasize]={\n";
for ($x=0;$x<$arasize;$x++) {
#  if ($ara[$x]<0) {
#    $y=sprintf "-%d",-$ara[$x]; # without -
#    print " "x(6-length($y)).$y.",";
#  } else {
#    printf "0x%04x,",$ara[$x];
#  }
  print $ara[$x].",";
  if ($x%16==15) {
    print "\n";
  }
}
print "}";
