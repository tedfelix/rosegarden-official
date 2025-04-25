#!/bin/sh

sources="VersionsTestOut"

PATH0=$PATH
DIR0=`pwd`
echo "DIR0 : $DIR0"
    
function do_it() {
# Param $1 : LilyPond version
# Param $2 : LilyPond path
    echo "======= $1   ($2)"
    
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

            key="Avertissement"
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

   cd $DIR0
}


do_it "2.12" "/opt/lilypond/2.12.0-1/bin"
do_it "2.14" "/opt/lilypond/2.14.2-1/bin"
do_it "2.16" "/opt/lilypond/2.16.2-1/bin"
do_it "2.18" "/opt/lilypond/2.18.2-1/bin"
do_it "2.19" "/opt/lilypond/2.19.19-1/bin"
do_it "2.20" "/opt/lilypond/2.20.0-1/bin"
do_it "2.22" "/opt/lilypond/2.22.2-1/bin"
do_it "2.24" ""

