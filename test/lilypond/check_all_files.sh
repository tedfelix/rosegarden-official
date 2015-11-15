#!/bin/sh
cd baseline
mkdir -p tmp
for file in *.ly; do

    # Using --ps since it's faster
    lilypond --ps -o tmp/$file $file
    if [ $? -eq 0 ]; then
        echo $file is valid
    else
        echo "ERROR: $file does not compile"
        exit 1
    fi

done
