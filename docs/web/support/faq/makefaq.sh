#! /bin/bash
tmpfile=/tmp/faqdata$$.html
mydir=`dirname $0`
perl txt2html.pl $mydir/../../../howtos/faq.txt \
    | perl $mydir/tableofcontents.pl \
    | grep -v DOCTYPE | egrep -v '(html|body)>' \
    > $tmpfile
sed "/INSERT FAQ/r $tmpfile" $mydir/template.html > $mydir/../../site/faq.html
rm $tmpfile
