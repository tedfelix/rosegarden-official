#!/bin/sh
for x in `find src \( -name \*.h -o -name \*.cpp \)`; do
    echo $x
    if ! perl ./convert.pl "$x" > "$x".$$ ; then exit ; fi
    cmp -s "$x".$$ "$x" || mv "$x".$$ "$x" && rm -f "$x".$$
done
