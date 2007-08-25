#!/bin/bash
rm POTFILES*
cd ..
find src/ \( -name "*.c*" -o -name "*.glade" \) -exec grep -lE "translatable|_\(" \{\} \; | sort | uniq > po/POTFILES.in
