#! /bin/bash
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
IMAGEWRITER=/opt/rosegarden/docs/web/support/imagewriter

# RG2.1

#$IMAGEWRITER/imagewriter -f $IMAGEWRITER/fonts/AGaramond-Regular.pfa -s 50 -w 400 -h 83 -x 8 -y 10 -o title-rosegarden.png Rosegarden 
#$IMAGEWRITER/imagewriter -f $IMAGEWRITER/fonts/AGaramond-Regular.pfa -s 50 -w 400 -h 83 -x 8 -y 10 -o title-features.png Features
#$IMAGEWRITER/imagewriter -f $IMAGEWRITER/fonts/AGaramond-Regular.pfa -s 50 -w 400 -h 83 -x 8 -y 10 -o title-pictures.png Pictures 
#$IMAGEWRITER/imagewriter -f $IMAGEWRITER/fonts/AGaramond-Regular.pfa -s 50 -w 400 -h 83 -x 8 -y 10 -o title-list.png Mailing-list 
#$IMAGEWRITER/imagewriter -f $IMAGEWRITER/fonts/AGaramond-Regular.pfa -s 50 -w 400 -h 83 -x 8 -y 10 -o title-development.png Development 
#$IMAGEWRITER/imagewriter -f $IMAGEWRITER/fonts/AGaramond-Regular.pfa -s 50 -w 400 -h 83 -x 8 -y 10 -o title-download.png Download 
#$IMAGEWRITER/imagewriter -f $IMAGEWRITER/fonts/AGaramond-Regular.pfa -s 50 -w 400 -h 83 -x 8 -y 10 -o title-faq.png FAQ
#$IMAGEWRITER/imagewriter -f $IMAGEWRITER/fonts/AGaramond-Regular.pfa -s 50 -w 400 -h 83 -x 8 -y 10 -o title-moved.png Moved

# RG 4

$IMAGEWRITER/imagewriter -f $IMAGEWRITER/fonts/aristo.ttf -b204/212/160 -t204/212/160 -s 50 -w 400 -h 104 -x 2 -y 28 -o title-rosegarden.png "Rosegarden"
$IMAGEWRITER/imagewriter -f $IMAGEWRITER/fonts/aristo.ttf -b204/212/160 -t204/212/160 -s 50 -w 400 -h 104 -x 2 -y 28 -o title-rosegarden-4.png "Rosegarden-4"
$IMAGEWRITER/imagewriter -f $IMAGEWRITER/fonts/aristo.ttf -b204/212/160 -t204/212/160 -s 50 -w 450 -h 104 -x 2 -y 28 -o title-rosegarden-2.png "Rosegarden-2.1"
$IMAGEWRITER/imagewriter -f $IMAGEWRITER/fonts/aristo.ttf -b204/212/160 -t204/212/160 -s 50 -w 400 -h 104 -x 2 -y 28 -o title-history.png History
$IMAGEWRITER/imagewriter -f $IMAGEWRITER/fonts/aristo.ttf -b204/212/160 -t204/212/160 -s 50 -w 400 -h 104 -x 2 -y 28 -o title-development.png Development
$IMAGEWRITER/imagewriter -f $IMAGEWRITER/fonts/aristo.ttf -b204/212/160 -t204/212/160 -s 50 -w 400 -h 104 -x 2 -y 28 -o title-pictures.png Screenshots
$IMAGEWRITER/imagewriter -f $IMAGEWRITER/fonts/aristo.ttf -b204/212/160 -t204/212/160 -s 50 -w 400 -h 104 -x 2 -y 28 -o title-list.png Mailing-list
$IMAGEWRITER/imagewriter -f $IMAGEWRITER/fonts/aristo.ttf -b204/212/160 -t204/212/160 -s 50 -w 400 -h 104 -x 2 -y 28 -o title-download.png Download
$IMAGEWRITER/imagewriter -f $IMAGEWRITER/fonts/aristo.ttf -b204/212/160 -t204/212/160 -s 50 -w 400 -h 104 -x 2 -y 28 -o title-faq.png FAQ
$IMAGEWRITER/imagewriter -f $IMAGEWRITER/fonts/aristo.ttf -b204/212/160 -t204/212/160 -s 50 -w 400 -h 104 -x 2 -y 28 -o title-features.png Features
$IMAGEWRITER/imagewriter -f $IMAGEWRITER/fonts/aristo.ttf -b204/212/160 -t204/212/160 -s 50 -w 400 -h 104 -x 2 -y 28 -o title-authors.png Authors

