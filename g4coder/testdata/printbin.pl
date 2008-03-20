#!/usr/bin/perl

while (<>) {
  $i=$_;
  while ($i=~/^ +([0-9A-Fa-f]{2})(.*)/) {
#    print "$1\n";
    $val=$1;
    $i=$2;
    $hv=pack("H*",$val);
    $str=unpack("B8",$hv);
#    $str=~s/^(.{4})/$1 /;
    print "$str";
  }
  print "\n";
}
