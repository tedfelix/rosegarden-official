#!/bin/sh
(
cd docs/en
export XSLDIR=/usr/share/apps/ksgmltools2/customization/
meinproc --stylesheet $XSLDIR/kde-chunk.xsl index.docbook
#for x in *.html; do perl -i -p -e 's/kde-default.css/rosegarden-help.css/g' $x ; done
)
