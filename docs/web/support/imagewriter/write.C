/* -*- c-file-style: "bsd" -*- */


#include "ImageWriter.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <iostream.h>

// Options are as follows.  (These are a real mess: a GNU-style long option
// syntax would improve things, except using GNU getopt_long would complicate
// building on Solaris)
// 
// -o <out-file-name>	File to write the finished image to (default to stdout)
// -f <font-file-name>	File to load truetype font from (default "arial.ttf")
// -i <image-file-name>	Image to use as background (default none; "-" == stdin)
// -c <colour>		Colour for text, rrr/ggg/bbb decimal (default 0/0/0)
// -b <colour>		Background colour, if no bg image (default 255/255/255)
// -w <wd>		Width of image, if no background image
// -h <ht>		Height of image, if no background image
// -s <size>		Point size (default 10)
// -l <pixels>		Leading (default 0)
// -x <x>		X coordinate to start writing at (default 0)
// -y <y>		Y coordinate (of top of glyph) to start at (default 30)
// -t <colour>          Colour to make transparent in output (default none)
// 
// If there's no background image or width or height, the image will be
// shrunk to fit the text bounding box (with a maximum of 600x300).  If
// only one dimension is given, the other will be shrunk to fit.
// 
// Layout options:
// 
// -Lw			Do word-wrap
// -Lcx			Centre horizontally; ignore value of -x
// -Lcy			Centre vertically; ignore value of -y (one-liners only)
// -Lm<margin>		Margin to leave at right, if word-wrapping (default 20)
// -Lm			Leave no margin at right (equivalent to -Lm 0)
//
// Also
// -d			Write the output double-size.  If you have a processing
// 			tool that can do smooth rescaling, you may get more
// 			Web-friendly results reducing a double-size image than
//			using a single-size one (because smoothness and even
// 			colour is often more important than crisp legibility).
// -D			Expect the background image (-i option) at double-size;
//			useful for chaining writes on a pipe.  Implies -d.
// 
// Any remaining arguments on the command line are taken as text to be
// written, with each argument on a separate line.
// 
// Mostly I get the best results for text for the Web with "imagewriter -d |
// convert -geometry 200% - - | convert -despeckle -geometry 25% - out.gif",
// using ImageMagick convert, although the converts are terribly slow.  The
// results from plain imagewriter are okay, especially if you want crisp text

int main(int argc, char **argv)
{
    int c;
    extern char *optarg;
    extern int optind;
    char *outFile = 0;
    char *fontFile = "arial.ttf";
    char *bgFile = 0;
    char *fg = "0/0/0";
    char *bg = "255/255/255";
    char *trans = 0;
    int wd = -1;
    int ht = -1;
    int pt = 10;
    int ld = 0;
    int x = 0;
    int y = 0;
    int d = 0;
    int D = 0;
    int wrap = 0;
    int cx = 0;
    int cy = 0;
    int m = 20;

    while ((c = getopt(argc, argv, "o:f:i:c:b:t:w:h:s:l:x:y:L:dD")) != EOF) {
        switch (c) {
        case 'o':  outFile = optarg; break;
        case 'f': fontFile = optarg; break;
        case 'i':   bgFile = optarg; break;
        case 'c':       fg = optarg; break;
        case 'b':       bg = optarg; break;
        case 't':    trans = optarg; break;
        case 'w': wd = atoi(optarg); break;
        case 'h': ht = atoi(optarg); break;
        case 's': pt = atoi(optarg); break;
        case 'l': ld = atoi(optarg); break;
        case 'x':  x = atoi(optarg); break;
        case 'y':  y = atoi(optarg); break;
        case 'd':  d = 1; break;
        case 'D':  D = d = 1; break;
        case 'L':
            if (!strcmp(optarg, "w")) { wrap = 1; break; }
            else if (!strcmp(optarg, "cx")) { cx = 1; break; }
            else if (!strcmp(optarg, "cy")) { cy = 1; break; }
            else if (!strncmp(optarg, "m", 1)) { m = atoi(optarg+1); break; }
            // else fall through to usage:
        case '?':
        default:
            cerr << "Usage: " << argv[0]
                 << " [-o <outputfile>]\n  [-f <fontfile>] [-s <pointsize>] "
                 << " [-l <leading>] [-x <x>] [-y <y>]\n"
                 << "  [-d] [-t <transparent>] [-Lw] [-Lcx] [-Lcy] [-Lm<margin>]\n"
                 << "    {   { [-i <bgimagefile>] }\n"
                 << "      | { [-c <fg>] [-b <bg>] [-w <wd>] [-h <ht>] }   }\n"
                 << "where <fg>, <bg> are of the form rrr/ggg/bbb decimal\n";
            exit(2);
        }
    }

    ImageWriter *writer = 0;
    int flags = 0;
    if (d)    flags |= ImageWriter::IMG_DOUBLESIZE;
    if (D)    flags |= ImageWriter::IMG_DOUBLESIZEINPUT;
    if (cx)   flags |= ImageWriter::IMG_CENTREX;
    if (cy)   flags |= ImageWriter::IMG_CENTREY;
    if (wrap) flags |= ImageWriter::IMG_WORDWRAP;

    if (bgFile) {
        writer = new ImageWriter(bgFile, fg, flags);
    } else {
        writer = new ImageWriter(wd, ht, fg, bg, flags);
    }

    writer->setRightMargin(m);
    if (trans) writer->setTransparent(trans);
    writer->move(x, y);
    while (optind < argc) writer->render(fontFile, pt, argv[optind++], ld);
    writer->save(outFile);

    cerr << "Next Y: " << writer->getNextY() << endl;
    delete writer;
    return 0;
}

