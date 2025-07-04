#!/bin/sh
cd baseline
mkdir -p tmp
for file in *.ly; do
    # Make it work with old LilyPond syntax
    convert-ly $file > tmp/converted.tmp.ly

    # Using --ps since it's faster
    lilypond --ps -o tmp/$file tmp/converted.tmp.ly
    if [ $? -eq 0 ]; then
        echo $file is valid
    else
        echo "ERROR: $file does not compile"
        exit 1
    fi

    # Cleanup
    rm -f tmp/converted.tmp.ly
done
