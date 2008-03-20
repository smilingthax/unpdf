#!/bin/sh
#MODE=-g3
#MODE="-g3 -2d"
MODE=-g4

mkdir -p tiffs
for i in *.pbm; do
  echo $i
  j=tiffs/`basename $i .pbm`.tiff
  k=`basename $i .pbm`.out
  pnmtotiff $MODE $i > $j
  tiffinfo -dr $j > $k
  tail -1 $k | printbin.pl | tee tiffs/tmp >> $k
  showhuff.pl $MODE `cat $k | head -2 | tail -1 | sed "s/[^0-9]*\([0-9]*\) .*/\1/"` tiffs/tmp >> $k 2> tiffs/tm2
  echo -en "\n\n" >> $k
  cat $i >> $k
#  tiffinfo -d $j | tail +13 >> $k
#  diff $i tiffs/tm2 >> $k
  echo "decode:" >> $k
  cat tiffs/tm2 >> $k
  echo -en "\n\n" >> $k
  coder.pl $i >> $k 2> tiffs/tm2
  cat tiffs/tm2 >> $k
  cat tiffs/tmp >> $k
  ../g4coder -g3 -b $i >> $k
done
rm -rf tiffs
