#!/usr/bin/perl

@ara=();

$width=8;
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
    $$data=substr($$data,$x);
    push @$line,$x; $x=0;
    last if ($y<=0);
    while ( (substr($$data,$x,1) eq "1")&&($y>0) ) {
      $x++;
      $y--;
    }
    $$data=substr($$data,$x);
    push @$line,$x; $x=0;
  }
  $$data=substr($$data,$x);
}
# }}}

@hlp=();
for ($iA=0;$iA<128;$iA++) {
  $str=unpack("B8",pack("C",$iA));
  print $str.":";
  rlecode(\$str,\@hlp);
  $x=0;$y=0;
  foreach $it (@hlp) {
    print $it.",";
    $x|=$it<<$y;
    $y+=4;
  }
  printf "0x%08x\n",$x;
#  print "\n" if ($k++%8==7) 
}
