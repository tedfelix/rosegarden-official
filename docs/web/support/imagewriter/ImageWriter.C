/* -*- c-file-style: "bsd" -*- */

#include "ImageWriter.h"
#include <iostream.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

ImageWriter::ImageWriter(char *bgFile, char *fg, int flags) :
    m_flags(flags), m_x(0), m_y(0), m_margin(20), m_shrinkWd(0), m_shrinkHt(0)
{
    assert(bgFile);

    FILE *bgFd;
    if (!strcmp(bgFile, "-")) bgFd = stdin;
    else bgFd = fopen(bgFile, "rb");

    if (!bgFd) fail("Couldn't open background image file ", bgFile);
    m_image = gdImageCreateFromPng(bgFd);
    if (!m_image) fail("Couldn't load PNG file (wrong format?) ", bgFile);
    if (strcmp(bgFile, "-")) fclose(bgFd);

    m_width = gdImageSX(m_image);
    m_height = gdImageSY(m_image);

    if (flag(IMG_DOUBLESIZE)) {
        if (!flag(IMG_DOUBLESIZEINPUT)) {
            int wd = m_width * 2;
            int ht = m_height * 2;
            gdImagePtr newi = gdImageCreate(wd, ht);
            gdImageCopyResized(newi, m_image, 0, 0, 0, 0,
                               wd, ht, m_width, m_height);
            gdImageDestroy(m_image);
            m_image = newi;
            m_width = wd;
            m_height = ht;
        }
        m_margin *= 2;
    }

    if (m_margin >= m_width) m_margin = 0;

    // requires m_image
    m_fg = parseColour(fg);
}

ImageWriter::ImageWriter(int width, int height, char *fg, char *bg, int flags) :
    m_flags(flags), m_x(0), m_y(0), m_margin(20)
{
    if (width < 0)  { m_shrinkWd = 1; m_width  = 600;    }
    else            { m_shrinkWd = 0; m_width  = width;  }
    if (height < 0) { m_shrinkHt = 1; m_height = 300;    }
    else            { m_shrinkHt = 0; m_height = height; }

    if (flag(IMG_DOUBLESIZE)) {
        m_width *= 2;
        m_height *= 2;
        m_margin *= 2;
    }
    m_image = gdImageCreate(m_width, m_height);

    if (m_shrinkWd) m_minX = m_maxX = -1;
    if (m_shrinkHt) m_minY = m_maxY = -1;
    
    if (m_margin >= m_width) m_margin = 0;

    // requires m_image
    m_fg = parseColour(fg);
    int bgc = parseColour(bg);
    gdImageFilledRectangle(m_image, 0, 0, m_width, m_height, bgc);
}

ImageWriter::~ImageWriter()
{
    gdImageDestroy(m_image);
}

int ImageWriter::parseColour(char *spec) // requires existance of m_image
{
    assert(m_image);
    assert(spec);
    char *orig = spec;
    int r, g, b;

    r = atoi(spec);
    while (*spec && *spec != '/') ++spec;
    if (*spec) ++spec;
    if (!*spec) fail("Invalid colour specification: ", orig);

    g = atoi(spec);
    while (*spec && *spec != '/') ++spec;
    if (*spec) ++spec;
    if (!*spec) fail("Invalid colour specification: ", orig);

    b = atoi(spec);
    int colour = gdImageColorResolve(m_image, r, g, b);
    cerr << "Parsed colour spec " << orig << " into index " << colour << endl;
    return colour;
}

void ImageWriter::fail(char *message, char *arg)
{
    assert(message);
    cerr << "ImageWriter: error: " << message << (arg ? arg : "") << endl;
    exit(1);
}

void ImageWriter::setTransparent(char *colour)
{
    assert(colour);
    gdImageColorTransparent(m_image, parseColour(colour));
}

void ImageWriter::setRightMargin(int margin)
{
    m_margin = margin;
    if (flag(IMG_DOUBLESIZE)) m_margin *= 2;
    if (m_margin >= m_width) m_margin = 0;
}

int ImageWriter::wordCount(char *text)
{
    int i, j;
    for (i = 0, j = 1; text[i]; ++i) {
        if ((text[i] == ' ') && text[i+1]) ++j;
    }
    return j;
}

char *ImageWriter::chopWords(char *text, char *&rtn, int w)
{
    char *rhs = text;
    int i;

    for (i = strlen(text)-2; i >= 0; --i) {
        if (text[i] == ' ') --w;
        if (w == 0) { rhs = text + i; break; }
    }

    if (rhs > text) {
        strncpy(rtn, text, i);
        rtn[i] = '\0';
        return rhs + 1;
    } else {
        return 0;
    }
}

void ImageWriter::move(int x, int y)
{
    if (flag(IMG_DOUBLESIZE)) {
        m_x = x*2;
        m_y = y*2;
    } else {
        m_x = x;
        m_y = y;
    }
}

int ImageWriter::getNextY()
{
    if (flag(IMG_DOUBLESIZE)) return m_y/2;
    else return m_y;
}

void ImageWriter::extents(char *fontFile, int size, char *text,
                          int &width, int &height, int &ascent, int &descent)
{
    char *message;
    int brect[8];
    message = gdImageStringTTF(NULL, brect, m_fg, fontFile, (double)size,
                               0.0, 0, 0, text);
    if (message) {
        cerr << "Failing to write with font-file \"" << fontFile
             << "\"" << endl;
        fail("Couldn't write text -- GD error: ", message);
    }

    width   =  brect[2] - brect[0];
    height  =  brect[1] - brect[7];
    ascent  = -brect[7];
    descent =  brect[1];
    message = gdImageStringTTF(NULL, brect, m_fg, fontFile, (double)size,
                               0.0, 0, 0, "|^,_~Zgl"); // high & low letters
    if (message) return;
    int ht = brect[1] - brect[7];
    if (ht > height) {
        descent += (ht - height);
        height = ht;
    }
}

void ImageWriter::render(char *fontFile, int size, char *text, int lead)
{
    assert(fontFile);

    int brect[8];
    char *message;
    int px, py;
    int wd, ht;
    int asc, dsc;
    int maxw = m_width;
    int maxh = m_height;

    int asize = size, alead = lead;
    if (flag(IMG_DOUBLESIZE)) { asize *= 2; alead *= 2; }

    extents(fontFile, asize, text, wd, ht, asc, dsc);

    if (flag(IMG_WORDWRAP) && ((m_x + wd + m_margin) > maxw) &&
        (wordCount(text) > 1)) {
        char *left = new char[strlen(text)];
        char *right;
        int w = 1;
        strcpy(left, text);
        while ((m_x + wd + m_margin) > maxw && (wordCount(left) > 1)) {
            right = chopWords(text, left, w);
            extents(fontFile, asize, left, wd, ht, asc, dsc);
            ++w;
        }
        render(fontFile, size, left, lead);
        render(fontFile, size, right, lead);
        delete[] left;
        return;
    }

    if (flag(IMG_CENTREX)) px = (maxw - wd) / 2;
    else px = m_x;
    if (m_shrinkWd) px += m_margin;

    if (flag(IMG_CENTREY)) py = (maxh - ht) / 2 + asc + alead;
    else py = m_y + asc + alead;

    if (m_shrinkHt) {
        if (m_minY < 0 || py - asc - alead < m_minY) m_minY = py - asc - alead;
    }

    message = gdImageStringTTF(m_image, brect, m_fg, fontFile,
                               (double)asize, 0.0, px, py, text);

    if (m_shrinkWd) {
        if (m_minX < 0 || brect[0] < m_minX) m_minX = brect[0];
        if (m_maxX < 0 || brect[2] > m_maxX) m_maxX = brect[2];
    }
    if (m_shrinkHt) {
        if (m_maxY < 0 || py + dsc > m_maxY) m_maxY = py + dsc;
    }

    if (message) fail("Couldn't write text -- GD error: ", message);
    cerr << "Wrote text \"" << text << "\" at (" << px << "," << py << ")"
         << endl;

    m_y = py + dsc;
}

void ImageWriter::save(char *outFile)
{
    gdImagePtr saveImage;

    if (m_shrinkWd || m_shrinkHt) {
        if ((m_shrinkWd && (m_maxX < 0)) || (m_shrinkHt && (m_maxY < 0))) {
            fail("No text, can't shrink to fit: specify size with -w, -h", "");
        }
        int wd = m_shrinkWd ? (m_maxX - m_minX) : m_width;
        int ht = m_shrinkHt ? (m_maxY - m_minY) : m_height;
        if (m_shrinkWd) {
            wd += (flag(IMG_DOUBLESIZE) ? 1 : 0) + m_margin * 2; 
            m_minX -= (m_margin + 1);
        }
        if (m_shrinkHt) {
            ht += (flag(IMG_DOUBLESIZE) ? 3 : 2); // and why?
            m_minY -= 1;
        }
        saveImage = gdImageCreate(wd, ht);
        gdImageCopy(saveImage, m_image, 0, 0,
                    m_shrinkWd ? m_minX : 0,
                    m_shrinkHt ? m_minY : 0, wd, ht);
    } else {
        saveImage = m_image;
    }

    if (outFile) {
        FILE *outFd = fopen(outFile, "wb");
        if (!outFd) fail("Couldn't open file for writing: ", outFile);
        gdImagePng(saveImage, outFd);
        fclose(outFd);
        cerr << "Wrote output to file \"" << outFile << "\"" << endl;
    } else {
        gdImagePng(saveImage, stdout);
        cerr << "Wrote output to stdout" << endl;
    }

    if (m_shrinkWd || m_shrinkHt) {
        gdImageDestroy(saveImage);
    }
}

