/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SYSTEMFONTXFT_H
#define RG_SYSTEMFONTXFT_H

#ifdef HAVE_XFT

#include "SystemFont.h"

#include <ft2build.h>
#include FT_FREETYPE_H 
#include FT_OUTLINE_H
#include FT_GLYPH_H
#include <X11/Xft/Xft.h>

namespace Rosegarden {

class SystemFontXft : public SystemFont
{
public:
    SystemFontXft(Display *dpy, XftFont *font) : m_dpy(dpy), m_font(font) { }
    virtual ~SystemFontXft() { if (m_font) XftFontClose(m_dpy, m_font); }
    
    virtual QPixmap renderChar(CharName charName, int glyph, int code,
			       Strategy strategy, bool &success);

private:
    Display *m_dpy;
    XftFont *m_font;
};

}

#endif

#endif
