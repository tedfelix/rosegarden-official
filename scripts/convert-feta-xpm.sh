#!/bin/bash
mydir=`dirname $0`
convert -border 2x2 -bordercolor white xpm:- xpm:- | convert -geometry 50% -colors 16 xpm:- xpm:- | perl $mydir/make-xpm-transparent.pl
