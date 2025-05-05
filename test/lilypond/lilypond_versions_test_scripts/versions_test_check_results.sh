#!/bin/sh

# This script read the LilyPond output messages stored while processing
# the LilyPond versions test.
# It can't work without an up to date file "versions_test_config".
# Please, see the README file in test/lilypond.

PATH0=$PATH
DIR0=`pwd`

main () {
    . versions_test_config
}

do_version() {
# Param $1 : LilyPond version name
# Param $2 : LilyPond path

# Note: $2 is not used in this script.
# It only is here because it exists in the included common configuration file

    echo "======= $1   ($2)"

    if [ -d "$sources/$1/tmp" ]; then
        echo "    Source found !"

        cd $sources/$1/tmp
#         rm -rf tmp
#         mkdir tmp

        echo "WORKING IN "`pwd`

        for file in *.ly.err; do
            name=`basename $file ".err"`

            echo -n `basename $file ".err"`": "

            count=`grep $keywarn $file | wc -l`
            if [ "$count" == "0" ]; then
                warning="OK"
            else
                warning="WARNING $count"
            fi

            count=`grep $keyerr $file | wc -l`
            if [ "$count" == "0" ]; then
                error="OK"
            else
                error="ERROR $count"
            fi

            if [ "$error" == "OK" ]; then
                    echo $warning
                else
                    echo $error
            fi

        done
    else
        echo "Directory $sources/$1 NOT FOUND !"
   fi

   cd $DIR0
}

main "$@"

