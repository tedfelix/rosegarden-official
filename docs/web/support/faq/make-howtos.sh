#! /bin/bash

for howto in rgd-HOWTO i18n; do

  tmpfile=/tmp/faqdata$$.html
  mydir=`dirname $0`
  perl txt2html.pl $mydir/../../../howtos/$howto.txt \
      | perl $mydir/tableofcontents.pl \
      | grep -v DOCTYPE | egrep -v '(html|body)>' \
      > $tmpfile
  sed "/INSERT FAQ/r $tmpfile" $mydir/template.html > $mydir/../../site/$howto.html
  rm $tmpfile

done

