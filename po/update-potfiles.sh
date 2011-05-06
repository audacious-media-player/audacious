#!/bin/bash
rm POTFILES*
echo "# Please don't update this file manually - use ./update-potfiles.sh instead!" > POTFILES.in
cd ..
find src/ \( -name "*.c" -o -name "*.cxx" -o -name "*.cc" -o -name "*.glade" \) -exec grep -lE "translatable|_\(" \{\} \; | sort | uniq >> po/POTFILES.in
