/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[TextFloat]"
#define RG_NO_DEBUG_PRINT

#include "TextFloat.h"


namespace Rosegarden
{


TextFloat *TextFloat::m_instance{nullptr};

TextFloat::TextFloat() :
        BaseTextFloat(nullptr)
{
}

TextFloat::~TextFloat()
{
    // Avoid reuse when QObject deletes us.
    m_instance = nullptr;
}

TextFloat *
TextFloat::getInstance()
{
    if (!m_instance)
        m_instance = new TextFloat;

    return m_instance;

#if 0
    // Can't do the usual as this is a QObject and it ends up being managed
    // by its parent.  It could go away at any time.  The dtor makes sure
    // we don't reuse a deleted instance.

    // Guaranteed in C++11 to be lazy initialized and thread-safe.
    // See ISO/IEC 14882:2011 6.7(4).
    static TextFloat instance;

    return &instance;
#endif
}

void
TextFloat::attach(QWidget *widget)
{
    m_widget = widget;
    m_newlyAttached = true;
}

void
TextFloat::setText(const QString &text)
{
    // Call reparent() only if we are going to use text float from a
    // newly entered widget
    if (m_newlyAttached) {
        reparent(m_widget);
        m_newlyAttached = false;
    }

    // then wrap to BaseTextFloat
    BaseTextFloat::setText(text);
}

void
TextFloat::display(QPoint offset)
{
    // Call reparent() only if we are going to use text float from a
    // newly entered widget
    if (m_newlyAttached) {
        reparent(m_widget);
        m_newlyAttached = false;
    }

    // then wrap to BaseTextFloat
    BaseTextFloat::display(offset);
}


}

