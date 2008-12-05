#!/bin/sh

make > /dev/null

rm symbols.list
for i in src/audacious/legacy/*.o; do
    objdump -t $i | grep "F .text" | cut -b49- >> symbols.list
done

for i in `cat symbols.list`; do
    grep -n $i src/audacious/*.c
done
