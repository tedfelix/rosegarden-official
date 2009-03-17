/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_ROSEGARDENTEXTFLOAT_H_
#define _RG_ROSEGARDENTEXTFLOAT_H_

#include "BaseTextFloat.h"

class QString;
class QWidget;
class QPoint;


namespace Rosegarden
{

/**

  A TextFloat is a box containing some text which can be displayed anywhere
  above the application windows, not necessarily above its parent widget.

  TextFloat implements it as a BaseTextFloat singleton shared by all widgets
  needing it.

  Currently Rosegarden uses text float to clarify some mouse actions.
  As long as usual systems only have one mouse cursor TextFloat may be used
  in place of BaseTextFloat.

  To use a TextFloat :

    1 - Get the TextFloat instance address using getTextFloat()

    2 - Call attach() to pass its current parent widget to TextFloat
        (Typically this should be done in parent enterEvent() method)

    3 - Call setText() to define which text to display

    4 - Call display() to move and show the text float

    5 - Repeatedly call setText() to change the text and/or call display()
        to move the text float

    6 - call hide() or hideAfterDelay() to hide the text float

  There should be no need to call reparent() explicitly as attach() does it to
  be called when necessary.

*/

class TextFloat : public BaseTextFloat
{

public :

    /**
     * Get the TextFloat instance address
     */
    static TextFloat *getTextFloat();

    /**
     * Attach the TextFloat to the given widget
     */
    void attach(QWidget *widget);

    /**
     * Set the text to display inside the TextFloat box.
     * (call reparent() if necessary then wrap to BaseTextFloat::setText())
     */
    virtual void setText(const QString &text);

    /**
     * Move the text float to the \a offset position relative to the top left
     * corner of the parent widget and show it.
     * (call reparent() if necessary then wrap to BaseTextFloat::display())
     */
    virtual void display(QPoint offset);

protected :
    TextFloat(QWidget *parent);
    virtual ~TextFloat() {}

    // Used to remember a parent widget change. 
    bool m_newlyAttached;

private :
    static TextFloat *m_textFloat;

};

}

#endif
