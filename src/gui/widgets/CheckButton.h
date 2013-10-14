/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2013 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_CHECK_BUTTON_H
#define RG_CHECK_BUTTON_H

#include <QPushButton>



/**This provides checkable tool buttons, originally for use by SelectDialog, to
 * set up the properties the same way on dozens of buttons.
 *
 * \author D. Michael McIntyre
 */
namespace Rosegarden
{


class CheckButton : public QPushButton
{
    Q_OBJECT
private:

    QString m_iconName;

    void toggle();

public:
    /**Creates a checkable QPushButton, using IconLoader to load the named icon
     * specified in the string.  This string is also used as an identifying key
     * in QSettings, so each time one of these buttons is created with a
     * particular icon, it will set itself to the checked state saved from the
     * last time one of these buttons was created with that icon.  This saves a
     * crap ton of settings nonsense at the dialog level, but it could get
     * confusing if something other that SelectDialog actually uses CheckButton
     * for something at some point in the future.
     */
    CheckButton(QString iconName, QWidget *parent = 0);

    ~CheckButton();
};


}

#endif
