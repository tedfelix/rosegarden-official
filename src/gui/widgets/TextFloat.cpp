/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "TextFloat.h"
#include "gui/general/GUIPalette.h"

#include <QPaintEvent>
#include <QPoint>
#include <QString>
#include <QWidget>

#include <iostream>


namespace Rosegarden
{


TextFloat *TextFloat::m_textFloat = nullptr;

TextFloat::TextFloat(QWidget *parent):
    BaseTextFloat(parent),
    m_newlyAttached(false)
{
}

TextFloat::~TextFloat()
{
    // m_textFloat is static
    m_textFloat = nullptr;
}

TextFloat *
TextFloat::getTextFloat()
{
    if (!m_textFloat) {
        m_textFloat = new TextFloat(nullptr);
    }

    return m_textFloat;
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

