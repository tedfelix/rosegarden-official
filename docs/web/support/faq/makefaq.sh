#! /bin/bash
tmpfile=/tmp/faqdata$$.html
perl txt2html.pl ../../../howtos/faq.txt | perl ./tableofcontents.pl \
    | grep -v DOCTYPE | egrep -v '(html|body)>' \
    > $tmpfile
sed "/INSERT FAQ/r $tmpfile" template.html > ../../site/faq.html
rm $tmpfile
