#! /bin/bash

test -d documents || mkdir documents

for howto in rgd-HOWTO i18n; do

  tmpfile=/tmp/faqdata$$.html
  mydir=`dirname $0`
  perl txt2html.pl $mydir/../../../howtos/$howto.txt \
      | perl $mydir/tableofcontents.pl \
      | grep -v DOCTYPE | egrep -v '(html|body)>' \
      > $tmpfile
  sed "/INSERT DOCUMENT/r $tmpfile" $mydir/template.html > $mydir/documents/$howto.shtml
  rm $tmpfile

done

