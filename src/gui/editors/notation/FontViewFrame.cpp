/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "FontViewFrame.h"
#include <QApplication>

#include <klocale.h>
#include <QMessageBox>
#include <QFontMetrics>
#include <QFrame>
#include <QSize>
#include <QString>
#include <QWidget>
#include <QPainter>

#ifdef HAVE_XFT
#include <ft2build.h>
#include FT_FREETYPE_H 
#include FT_OUTLINE_H
#include FT_GLYPH_H
#include <X11/Xft/Xft.h>
#endif

namespace Rosegarden
{

FontViewFrame::FontViewFrame( int pixelSize, QWidget* parent, const char* name ) :
        QFrame(parent, name),
        m_fontSize(pixelSize),
        m_tableFont(0)
{
    setBackgroundMode(PaletteBase);
    setFrameStyle(Panel | Sunken);
    setMargin(8);
    setRow(0);
}

FontViewFrame::~FontViewFrame()
{
    // empty
}

void
FontViewFrame::setFont(QString font)
{
    m_fontName = font;
    loadFont();
    update();
}

void
FontViewFrame::loadFont()
{
#ifdef HAVE_XFT
    if (m_tableFont) {
        XftFontClose(x11AppDisplay(), (XftFont *)m_tableFont);
    }
    m_tableFont = 0;

    static bool haveDir = false;
    if (!haveDir) {
        FcConfigAppFontAddDir(FcConfigGetCurrent(),
                              (const FcChar8 *)"/opt/kde3/share/apps/rosegarden/fonts");
        haveDir = true;
    }

    FcPattern *pattern = FcPatternCreate();
    FcPatternAddString(pattern, FC_FAMILY, (FcChar8 *)m_fontName.toLatin1().data());
    FcPatternAddInteger(pattern, FC_PIXEL_SIZE, m_fontSize);

    FcConfigSubstitute(FcConfigGetCurrent(), pattern, FcMatchPattern);

    FcResult result = FcResultMatch;
    FcPattern *match = FcFontMatch(FcConfigGetCurrent(), pattern, &result);
    FcPatternDestroy(pattern);

    if (!match || result != FcResultMatch) {
        QMessageBox::error(this, i18n("Error: Unable to match font name %1", m_fontName));
        return ;
    }

    FcChar8 *matchFamily;
    FcPatternGetString(match, FC_FAMILY, 0, &matchFamily);

    if (QString((const char *)matchFamily).toLower() != m_fontName.toLower()) {
        QMessageBox::sorry(this, i18n("Warning: No good match for font name %1 (best is %2)", 
                           m_fontName, QString((const char *)matchFamily)));
        m_fontName = (const char *)matchFamily;
    }

    m_tableFont = XftFontOpenPattern(x11AppDisplay(), match);

    if (!m_tableFont) {
        QMessageBox::error(this, i18n("Error: Unable to open best-match font %1", 
                           QString((const char *)matchFamily)));
    }
#endif
}

void FontViewFrame::setGlyphs(bool glyphs)
{
    m_glyphs = glyphs;
    update();
}

QSize FontViewFrame::sizeHint() const
{
    return QSize(16 * m_fontSize * 3 / 2 + margin() + 2 * frameWidth(),
                 16 * m_fontSize * 3 / 2 + margin() + 2 * frameWidth());
}

QSize FontViewFrame::cellSize() const
{
    QFontMetrics fm = fontMetrics();
    return QSize( fm.maxWidth(), fm.lineSpacing() + 1 );
}

void FontViewFrame::paintEvent( QPaintEvent* e )
{
#ifdef HAVE_XFT
    if (!m_tableFont)
        return ;

    QFrame::paintEvent(e);
    QPainter p(this);

    int ll = 25;
    int ml = frameWidth() + margin() + ll + 1;
    int mt = frameWidth() + margin();
    QSize cell((width() - 16 - ml) / 17, (height() - 16 - mt) / 17);

    if ( !cell.width() || !cell.height() )
        return ;

    QColor body(255, 255, 192);
    QColor negative(255, 192, 192);
    QColor positive(192, 192, 255);
    QColor rnegative(255, 128, 128);
    QColor rpositive(128, 128, 255);

    Drawable drawable = (Drawable)handle();
    XftDraw *draw = XftDrawCreate(x11AppDisplay(), drawable,
                                  (Visual *)x11Visual(), x11Colormap());

    QColor pen(QColor(Qt::black));
    XftColor col;
    col.color.red = pen.red () | pen.red() << 8;
    col.color.green = pen.green () | pen.green() << 8;
    col.color.blue = pen.blue () | pen.blue() << 8;
    col.color.alpha = 0xffff;
    col.pixel = pen.pixel();

    for (int j = 0; j <= 16; j++) {
        for (int i = 0; i <= 16; i++) {

            int x = i * cell.width();
            int y = j * cell.height();

            x += ml;
            y += mt; // plus ascent

            if (i == 0) {
                if (j == 0)
                    continue;
                p.setFont(qApp->font());
                QFontMetrics afm(qApp->font());
                QString s = QString("%1").arg(m_row * 256 + (j - 1) * 16);
                p.drawText(x - afm.width(s), y, s);
                p.setPen(QColor(190, 190, 255));
                p.drawLine(0, y, width(), y);
                p.setPen(QColor(Qt::black));
                continue;
            } else if (j == 0) {
                p.setFont(qApp->font());
                QString s = QString("%1").arg(i - 1);
                p.drawText(x, y, s);
                p.setPen(QColor(190, 190, 255));
                p.drawLine(x, 0, x, height());
                p.setPen(QColor(Qt::black));
                continue;
            }

            p.save();

            if (m_glyphs) {
                FT_UInt ui = m_row * 256 + (j - 1) * 16 + i - 1;
                XftDrawGlyphs(draw, &col, (XftFont *)m_tableFont, x, y, &ui, 1);
            } else {
                FcChar32 ch = m_row * 256 + (j - 1) * 16 + i - 1;
                if (XftCharExists(x11AppDisplay(), (XftFont *)m_tableFont, ch)) {
                    XftDrawString32(draw, &col, (XftFont *)m_tableFont, x, y, &ch, 1);
                }
            }

            p.restore();
        }
    }
#endif
}

bool
FontViewFrame::hasRow(int r) const
{
#ifdef HAVE_XFT
    if (m_glyphs) {

        if (r < 256)
            return true;

    } else {

        for (int c = 0; c < 256; ++c) {
            FcChar32 ch = r * 256 + c;
            if (XftCharExists(x11AppDisplay(), (XftFont *)m_tableFont, ch)) {
                return true;
            }
        }
    }
#endif
    return false;
}

void FontViewFrame::setRow(int row)
{
    m_row = row;
    update();
}

}
#include "FontViewFrame.moc"
