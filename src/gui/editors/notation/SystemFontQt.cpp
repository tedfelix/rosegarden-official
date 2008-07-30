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

#include "SystemFontQt.h"

#include "misc/Debug.h"
#include "gui/general/PixmapFunctions.h"

#include <qfont.h>
#include <qfontmetrics.h>
#include <qpainter.h>
#include <qpixmap.h>

namespace Rosegarden {

QPixmap
SystemFontQt::renderChar(CharName charName, int glyph, int code,
			 Strategy strategy, bool &success)
{
    success = false;

    if (strategy == OnlyGlyphs) {
	NOTATION_DEBUG << "SystemFontQt::renderChar: OnlyGlyphs strategy not supported by Qt renderer, can't render character " << charName.getName() << " (glyph " << glyph << ")" << endl;
	return QPixmap();
    }

    if (code < 0) {
	NOTATION_DEBUG << "SystemFontQt::renderChar: Can't render using Qt with only glyph value (" << glyph << ") for character " << charName.getName() << ", need a code point" << endl;
	return QPixmap();
    }

    QFontMetrics metrics(m_font);
    QChar qc(code);

    QPixmap map;
    map = QPixmap(metrics.width(qc), metrics.height());
    
    map.fill();
    QPainter painter;
    painter.begin(&map);
    painter.setFont(m_font);
    painter.setPen(Qt::black);
    
    NOTATION_DEBUG << "NoteFont: Drawing character code "
		   << code << " for " << charName.getName()
		   << " using QFont" << endl;

    painter.drawText(0, metrics.ascent(), qc);
    
    painter.end();
    map.setMask(PixmapFunctions::generateMask(map, Qt::white.rgb()));

    success = true;
    return map;
}

}
