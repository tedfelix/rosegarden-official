// -*- c-basic-offset: 4 -*-
/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

/*
    This file was originally based on fontdisplayer.cpp from the Qt
    example program "qfd", copyright 1992-2000 TrollTech AS.  The
    original file said:

** This file is part of an example program for Qt.  This example
** program may be used, distributed and modified without limitation.

*/

#include <qframe.h>
#include <qpainter.h>
#include <qlabel.h>
#include <qvbox.h>
#include <kcombobox.h>
#include <ktoolbar.h>
#include <klocale.h>
#include <kapplication.h>
#include <kmessagebox.h>

#include "notefontviewer.h"

#include "config.h"

#ifdef HAVE_XFT
#include <X11/Xft/Xft.h>
#endif

FontViewFrame::FontViewFrame( int pixelSize, QWidget* parent, const char* name ) :
    QFrame(parent,name),
    m_fontSize(pixelSize),
    m_tableFont(0)
{
    setBackgroundMode(PaletteBase);
    setFrameStyle(Panel|Sunken);
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
    FcPatternAddString(pattern, FC_FAMILY, (FcChar8 *)m_fontName.latin1());
    FcPatternAddInteger(pattern, FC_PIXEL_SIZE, m_fontSize);
    
    FcConfigSubstitute(FcConfigGetCurrent(), pattern, FcMatchPattern);

    FcResult result = FcResultMatch;
    FcPattern *match = FcFontMatch(FcConfigGetCurrent(), pattern, &result);
    FcPatternDestroy(pattern);

    if (!match || result != FcResultMatch) {
	KMessageBox::error(this, i18n("Error: Unable to match font name %1").arg(m_fontName));
	return;
    }

    FcChar8 *matchFamily;
    FcPatternGetString(match, FC_FAMILY, 0, &matchFamily);

    if (QString((const char *)matchFamily).lower() != m_fontName.lower()) {
	KMessageBox::sorry(this, i18n("Warning: No good match for font name %1 (best is %2)").
			     arg(m_fontName).arg(QString((const char *)matchFamily)));
	m_fontName = (const char *)matchFamily;
    }

    m_tableFont = XftFontOpenPattern(x11AppDisplay(), match);

    if (!m_tableFont) {
	KMessageBox::error(this, i18n("Error: Unable to open best-match font %1").
			   arg(QString((const char *)matchFamily)));
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
    return QSize( fm.maxWidth(), fm.lineSpacing()+1 );
}

void FontViewFrame::paintEvent( QPaintEvent* e )
{
#ifdef HAVE_XFT
    if (!m_tableFont) return;

    QFrame::paintEvent(e);
    QPainter p(this);

    int ll = 25;
    int ml = frameWidth()+margin() + ll + 1;
    int mt = frameWidth()+margin();
    QSize cell((width()-16-ml)/17,(height()-16-mt)/17);

    if ( !cell.width() || !cell.height() )
        return;

    QColor body(255,255,192);
    QColor negative(255,192,192);
    QColor positive(192,192,255);
    QColor rnegative(255,128,128);
    QColor rpositive(128,128,255);

    Drawable drawable = (Drawable)handle();
    XftDraw *draw = XftDrawCreate(x11AppDisplay(), drawable,
				  (Visual *)x11Visual(), x11Colormap());

    QColor pen(Qt::black);
    XftColor col;
    col.color.red = pen.red () | pen.red() << 8;
    col.color.green = pen.green () | pen.green() << 8;
    col.color.blue = pen.blue () | pen.blue() << 8;
    col.color.alpha = 0xffff;
    col.pixel = pen.pixel();

    for (int j = 0; j <= 16; j++) {
        for (int i = 0; i <= 16; i++) {

	    int x = i*cell.width();
	    int y = j*cell.height();
	    
	    x += ml;
	    y += mt; // plus ascent
	    
	    if (i == 0) {
		if (j == 0) continue;
		p.setFont(kapp->font());
		QFontMetrics afm(kapp->font());
		QString s = QString("%1").arg(m_row * 256 + (j-1) * 16);
		p.drawText(x - afm.width(s), y, s);
		p.setPen(QColor(190, 190, 255));
		p.drawLine(0, y, width(), y);
		p.setPen(Qt::black);
		continue;
	    } else if (j == 0) {
		p.setFont(kapp->font());
		QString s = QString("%1").arg(i - 1);
		p.drawText(x, y, s);
		p.setPen(QColor(190, 190, 255));
		p.drawLine(x, 0, x, height());
		p.setPen(Qt::black);
		continue;
	    }
	    
	    p.save();
	    
	    if (m_glyphs) {
		FT_UInt  ui = m_row * 256 + (j-1) * 16 + i - 1;
		XftDrawGlyphs(draw, &col, (XftFont *)m_tableFont, x, y, &ui, 1);
	    } else {
		FcChar32 ch = m_row * 256 + (j-1) * 16 + i - 1;
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

	if (r < 256) return true;

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

void
NoteFontViewer::slotViewChanged(int i)
{
    m_frame->setGlyphs(i == 0);

    m_rows->clear();
    int firstRow = -1;

    for (int r = 0; r < 256; ++r) {
	if (m_frame->hasRow(r)) {
	    m_rows->insertItem(QString("%1").arg(r));
	    if (firstRow < 0) firstRow = r;
	}
    }

    if (firstRow < 0) {
	m_rows->setEnabled(false);
	m_frame->setRow(0);
    } else {
	m_rows->setEnabled(true);
	m_frame->setRow(firstRow);
    }
}

void
NoteFontViewer::slotRowChanged(const QString &s)
{
    bool ok;
    int i = s.toInt(&ok);
    if (ok) m_frame->setRow(i);
}

void
NoteFontViewer::slotFontChanged(const QString &s)
{
    m_frame->setFont(s);
    slotViewChanged(m_view->currentItem());
}

NoteFontViewer::NoteFontViewer(QWidget *parent, QString noteFontName,
			       QStringList fontNames, int pixelSize) :
    KDialogBase(parent, 0, true,
		i18n("Note Font Viewer: %1").arg(noteFontName), Close)
{
    QVBox *box = makeVBoxMainWidget();
    KToolBar* controls = new KToolBar(box);
    controls->setMargin(3);

    (void) new QLabel(i18n("  Component: "), controls);
    m_font = new KComboBox(controls);

    for (QStringList::iterator i = fontNames.begin(); i != fontNames.end();
	 ++i) {
	m_font->insertItem(*i);
    }

    (void) new QLabel(i18n("  View: "), controls);
    m_view = new KComboBox(controls);

    m_view->insertItem("Glyphs");
    m_view->insertItem("Codes");

    (void) new QLabel(i18n("  Page: "), controls);
    m_rows = new KComboBox(controls);

    m_frame = new FontViewFrame(pixelSize, box);

    connect(m_font, SIGNAL(activated(const QString &)),
	    this, SLOT(slotFontChanged(const QString &)));

    connect(m_view, SIGNAL(activated(int)),
	    this, SLOT(slotViewChanged(int)));

    connect(m_rows,SIGNAL(activated(const QString &)),
	    this, SLOT(slotRowChanged(const QString &)));

    slotFontChanged(m_font->currentText());
}

