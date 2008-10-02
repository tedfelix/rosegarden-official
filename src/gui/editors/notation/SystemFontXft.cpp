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


#ifdef HAVE_XFT

// include this first:
#include <QTextStream>

#include "SystemFontXft.h"

#include "misc/Debug.h"
#include "gui/general/PixmapFunctions.h"



namespace Rosegarden {

/*!!! Just test code.

int
staticMoveTo(FT_Vector *to, void *)
{
    NOTATION_DEBUG << "moveTo: (" << to->x << "," << to->y << ")" << endl;
    return 0;
}

int
staticLineTo(FT_Vector *to, void *)
{
    NOTATION_DEBUG << "lineTo: (" << to->x << "," << to->y << ")" << endl;
    return 0;
}

int
staticConicTo(FT_Vector *control, FT_Vector *to, void *)
{
    NOTATION_DEBUG << "conicTo: (" << to->x << "," << to->y << ") control (" << control->x << "," << control->y << ")" << endl;
    return 0;
}

int
staticCubicTo(FT_Vector *control1, FT_Vector *control2, FT_Vector *to, void *)
{
    NOTATION_DEBUG << "cubicTo: (" << to->x << "," << to->y << ") control1 (" << control1->x << "," << control1->y << ") control2 (" << control2->x << "," << control2->y << ")" << endl;
    return 0;
}

*/

QPixmap
SystemFontXft::renderChar(CharName charName, int glyph, int code,
			  Strategy strategy, bool &success)
{
    success = false;

    if (glyph < 0 && code < 0) {
	NOTATION_DEBUG << "SystemFontXft::renderChar: Have neither glyph nor code point for character " << charName.getName() << ", can't render" << endl;
	return QPixmap();
    }

    if (code < 0 && strategy == OnlyCodes) {
	NOTATION_DEBUG << "SystemFontXft::renderChar: strategy is OnlyCodes but no code point provided for character " << charName.getName() << " (glyph is " << glyph << ")" << endl;
	return QPixmap();
    }

    if (glyph < 0 && strategy == OnlyGlyphs) {
	NOTATION_DEBUG << "SystemFontXft::renderChar: strategy is OnlyGlyphs but no glyph index provided for character " << charName.getName() << " (code is " << code << ")" << endl;
	return QPixmap();
    }

    XGlyphInfo extents;

    bool useGlyph = true;
    if (glyph < 0 || (strategy == PreferCodes && code >= 0)) useGlyph = false;
    if (glyph >= 0 && useGlyph == false && !XftCharExists(m_dpy, m_font, code)) {
	NOTATION_DEBUG << "SystemFontXft::renderChar: code " << code << " is preferred for character " << charName.getName() << ", but it doesn't exist in font!  Falling back to glyph " << glyph << endl;
	useGlyph = true;
    }

    if (useGlyph) {
	FT_UInt ui(glyph);
	XftGlyphExtents(m_dpy, m_font, &ui, 1, &extents);
	if (extents.width == 0 || extents.height == 0) {
	    NOTATION_DEBUG
		<< "SystemFontXft::renderChar: zero extents for character "
		<< charName.getName() << " (glyph " << glyph << ")" << endl;
	    return QPixmap();
	}
    } else {
	FcChar32 char32(code);
	XftTextExtents32(m_dpy, m_font, &char32, 1, &extents);
	if (extents.width == 0 || extents.height == 0) {
	    NOTATION_DEBUG
		<< "SystemFontXft::renderChar: zero extents for character "
		<< charName.getName() << " (code " << code << ")" << endl;
	    return QPixmap();
	}
    }
 
    QPixmap map(extents.width, extents.height);
    map.fill();

    Drawable drawable = (Drawable)map.handle();
    if (!drawable) {
	std::cerr << "ERROR: SystemFontXft::renderChar: No drawable in QPixmap!" << std::endl;
	return map;
    }

    XftDraw *draw = XftDrawCreate(m_dpy,
				  drawable,
				  (Visual *)map.x11Visual(),
				  map.x11Colormap());

    QColor pen(QColor(Qt::black));
    XftColor col;
    col.color.red = pen.red () | pen.red() << 8;
    col.color.green = pen.green () | pen.green() << 8;
    col.color.blue = pen.blue () | pen.blue() << 8;
    col.color.alpha = 0xffff;
    col.pixel = pen.pixel();

    if (useGlyph) {
	NOTATION_DEBUG << "NoteFont: drawing raw character glyph "
		       << glyph << " for " << charName.getName()
		       << " using Xft" << endl;
	FT_UInt ui(glyph);
	XftDrawGlyphs(draw, &col, m_font, extents.x, extents.y, &ui, 1);
    } else {
	NOTATION_DEBUG << "NoteFont: drawing character code "
		       << code << " for " << charName.getName()
		       << " using Xft" << endl;
	FcChar32 char32(code);
	XftDrawString32(draw, &col, m_font, extents.x, extents.y, &char32, 1);
    }

    XftDrawDestroy(draw);

    map.setMask(PixmapFunctions::generateMask(map, QColor(Qt::white).rgb()));
    success = true;

    
    //!!! experimental stuff
/*!!!
    FT_Face face = XftLockFace(m_font);
    if (!face) {
	NOTATION_DEBUG << "Couldn't lock face" << endl;
	return map;
    }
    // not checking return value here
    FT_Load_Glyph(face, glyph, 0);
    if (face->glyph->format != FT_GLYPH_FORMAT_OUTLINE) {
	NOTATION_DEBUG << "Glyph " << glyph << " isn't an outline" << endl;
	XftUnlockFace(m_font);
	return map;
    }
    FT_Glyph ftglyph;
    FT_Get_Glyph(face->glyph, &ftglyph);
    FT_Outline &outline = ((FT_OutlineGlyph)ftglyph)->outline;
    NOTATION_DEBUG << "Outline: " << outline.n_contours << " contours, "
		   << outline.n_points << " points" << endl;


    FT_Outline_Funcs funcs = {
	staticMoveTo, staticLineTo, staticConicTo, staticCubicTo, 0, 0
    };
    FT_Outline_Decompose(&outline, &funcs, 0);
    FT_Done_Glyph(ftglyph);
    XftUnlockFace(m_font);
*/

    return map;
}

}

#endif /* HAVE_XFT */

