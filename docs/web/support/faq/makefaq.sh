#! /bin/bash
tmpfile=/tmp/faqdata$$.html
mydir=`dirname $0`
test -d faq || mkdir faq
perl txt2html.pl $mydir/../../../howtos/faq.txt \
    | perl $mydir/tableofcontents.pl \
    | grep -v DOCTYPE | egrep -v '(html|body)>' \
    > $tmpfile
sed "/INSERT DOCUMENT/r $tmpfile" $mydir/template.html > $mydir/faq/index.shtml
rm $tmpfile
