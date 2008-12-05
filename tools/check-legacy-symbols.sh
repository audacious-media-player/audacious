#!/bin/sh

make

echo "building symbol table of legacy code."
rm symbols.list
for i in src/audacious/legacy/*.o; do
    objdump -t $i | grep "F .text" | cut -b49- >> symbols.list
done

for i in `cat symbols.list`; do
    grep $i src/audacious/*.c
done
