/* -*- c-file-style: "bsd" -*- */

#include "gd.h"

/* Summary: Load a background image in PNG format, write some text
   onto it with a truetype font, then write out again as a PNG.
   Failures are generally catastrophic, we're not attempting to return
   anything meaningful, throw exceptions &c -- we complain and exit(1) */

/* Requires gd-1.7 (which needs libpng & zlib) and libttf from Freetype 1.2 */


class ImageWriter
{
public:
    ImageWriter(char *bgFile, char *fg /* rrr/ggg/bbb */, int flags);
    ImageWriter(int width, int height, char *fg /* rrr/ggg/bbb */,
                char *bg /* rrr/ggg/bbb */, int flags);
    ~ImageWriter();

    static const int IMG_DOUBLESIZE	 = 1<<0;
    static const int IMG_DOUBLESIZEINPUT = 1<<1; // Useless without DOUBLESIZE
    static const int IMG_CENTREX	 = 1<<2;
    static const int IMG_CENTREY	 = 1<<3;
    static const int IMG_WORDWRAP	 = 1<<4;

    void move(int x, int y);
    void setTransparent(char *colour);
    void setRightMargin(int m);
    void render(char *fontFile, int size, char *text, int leading);
    void save(char *outFile);
    int getNextY();

private:
    int m_fg;                   /* gd colour */
    int m_bg;                   /* gd colour */
    int m_width;
    int m_height;
    int m_margin;
    const int m_flags;
    gdImagePtr m_image;

    int m_x;
    int m_y;

    int m_shrinkWd;
    int m_shrinkHt;
    int m_minX, m_maxX;
    int m_minY, m_maxY;

    int parseColour(char *colour);
    void fail(char *message, char *arg);

    void extents(char *fontFile, int size, char *text,
                 int &width, int &height, int &ascent, int &descent);

    // rtn needs enough space for text:
    char *chopWords(char *text, char *&rtn, int w);
    int wordCount(char *text);

    int flag(int f) { return (m_flags & f); }
};

