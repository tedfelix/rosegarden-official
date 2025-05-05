#!/bin/sh

# This script run LilyPond on all the sources created while processing
# the LilyPond versions test.
# It can't work without an up to date file "versions_test_config".
# Please, see the README file in test/lilypond.

PATH0=$PATH
DIR0=`pwd`

main () {
    . versions_test_config
}
    
function do_version() {
# Param $1 : LilyPond version
# Param $2 : LilyPond path
    echo "======= $1   ($2)"

    if [ "$2" == "" ] || [ -d "$2" ]; then
    
        export PATH=$2:$PATH0
        lilypond --version | grep LilyPond

        if [ -d "$sources/$1" ]; then
            echo "    Source found !"

            cd $sources/$1
            rm -rf tmp
            mkdir tmp

            for file in *.ly; do
                # Make it work with old LilyPond syntax
                #convert-ly $file > tmp/converted.tmp.ly

                echo -n "$file : "

                # Using --ps since it's faster
                lilypond --ps -o tmp/$file $file 2>tmp/$file.err

                if [ $? -eq 0 ]; then
                    error=""
                else
                    error="ERROR"
                fi

                key=$keywarn
                if [ "`grep $key tmp/$file.err`" == "" ]; then
                    warning="OK"
                else
                    warning="WARNING"
                fi

                if [ "$error" == "" ]; then
                    echo $warning
                else
                    echo $error
                fi

                # Cleanup
                #rm -f tmp/converted.tmp.ly
            done
        else
            echo "Directory $sources/$1 NOT FOUND !"
        fi
    else
        echo "CAN'T FOUND $2:"
        echo "The $1 LilyPond version is skipped"
    fi

    cd $DIR0
}

main "$@"


