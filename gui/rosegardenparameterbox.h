// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
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

#ifndef _ROSEGARDENPARAMETERBOX_H_
#define _ROSEGARDENPARAMETERBOX_H_

#include <qframe.h>
#include <qstring.h>
#include <qfont.h>

#include "rosegardenparameterarea.h"

/**
 * A flat QFrame, in which a group of parameters can be laid out.
 * Virtual method functions are defined for for requesting a layout
 * style, and returning the single-word to use for labelling the
 * box.
 */

class RosegardenParameterBox : public QFrame
{
    Q_OBJECT
public:
    RosegardenParameterBox(const QString &label, QWidget *parent = 0,
			   const char *name = 0);

    // Ask for a one-word string that can be used to label the widget.
    QString getLabel() const;

    // Get the label of the following parameter box (to establish an ordering)
    virtual QString getPreviousBox(RosegardenParameterArea::Arrangement) const;

    virtual void showAdditionalControls(bool) = 0; 

protected:
    void init();

    // List the layout styles that may be requested via a call to setStyle().

    enum LayoutMode {
	LANDSCAPE_MODE,  // Optimize the layout for a tall and narrow parent.
	PORTRAIT_MODE    // Optimize the layout for a short and wide parent.
    };

    void setLayoutMode(LayoutMode mode);

    QFont m_font;
    QString m_label;    // The string that containers can use for labelling.
    LayoutMode m_mode;  // The current layout mode.
};

#endif
