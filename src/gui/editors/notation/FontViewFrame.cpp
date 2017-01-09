/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
 
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

#include <QMessageBox>
#include <QFontMetrics>
#include <QFrame>
#include <QSize>
#include <QString>
#include <QWidget>
#include <QPainter>
#include <QChar>

#include "misc/Strings.h"

#include <iostream>

namespace Rosegarden
{

FontViewFrame::FontViewFrame( int pixelSize, QWidget* parent) :
    QFrame(parent),
    m_fontSize(pixelSize),
    m_tableFont(0),
    m_ascent(0)
{
//    setBackgroundRole( QPalette::Base );
    setFrameStyle(Panel | Sunken);
    setContentsMargins( 8,8,8,8 );
    setAttribute(Qt::WA_PaintOnScreen);
    setRow(0);
}

FontViewFrame::~FontViewFrame()
{
    delete m_tableFont;
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
    delete m_tableFont;
    QFont *qf = new QFont(m_fontName);
    qf->setPixelSize(m_fontSize);
    qf->setWeight(QFont::Normal);
    qf->setItalic(false);
    QFontInfo fi(*qf);
    std::cerr << "Loaded Qt font \"" << fi.family() << "\" (exactMatch = " << (fi.exactMatch() ? "true" : "false") << ")" << std::endl;
    m_tableFont = qf;

    m_ascent = QFontMetrics(font()).ascent(); // ascent of numbering font, not notation font
}

void FontViewFrame::setGlyphs(bool glyphs)
{
    m_glyphs = glyphs;
    update();
}

QSize FontViewFrame::sizeHint() const
{
    int top,right,left,bottom;
    getContentsMargins( &left, &top, &right, &bottom );
	
    return QSize((16 * m_fontSize * 3) / 2 + left + right + 2 * frameWidth(),
                 (16 * m_fontSize * 3) / 2 + top + bottom + 2 * frameWidth());
	
// 	old: return QSize(16 * m_fontSize * 3 / 2 + margin() + 2 * frameWidth(),
// 				 16 * m_fontSize * 3 / 2 + margin() + 2 * frameWidth());
}

QSize FontViewFrame::cellSize() const
{
    QFontMetrics fm = fontMetrics();
    return QSize( fm.maxWidth(), fm.lineSpacing() + 1 );
}

void FontViewFrame::paintEvent( QPaintEvent* e )
{
    if (!m_tableFont) return;

    QFrame::paintEvent(e);
    QPainter p(this);

    int top, right, left, bottom;
    getContentsMargins(&left, &top, &right, &bottom);
    p.setClipRect(left, top, right-left, bottom-top);
	
    int ll = 25;
    int lt = m_ascent + 5;
    int ml = frameWidth() + left + ll + 1;
    int mt = frameWidth() + top + lt;
    QSize cell((width() - 16 - ml) / 17, (height() - 16 - mt) / 17);

    if ( !cell.width() || !cell.height() )
        return ;

    p.setPen(Qt::black);

    for (int j = 0; j <= 16; j++) {
        for (int i = 0; i <= 16; i++) {

            int x = i * cell.width();
            int y = j * cell.height();

            x += ml;
            y += mt;

            if (i == 0) {
                if (j == 0) continue;
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
        }
    }

    p.setFont(*m_tableFont);

    for (int j = 1; j <= 16; j++) {
        for (int i = 1; i <= 16; i++) {

            int x = i * cell.width();
            int y = j * cell.height();

            x += ml;
            y += mt;

            QChar c(m_row * 256 + (j - 1) * 16 + i - 1);
            p.drawText(x, y, QString(c));
        }
    }
}

bool
FontViewFrame::hasRow(int r) const
{
    if (r < 256) return true;
    return false;
}

void FontViewFrame::setRow(int row)
{
    m_row = row;
    update();
}

}
