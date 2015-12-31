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
    bool m_wantsMemory;

protected slots:
    void toggle(bool state);

public:
    /**Creates a checkable QPushButton, using IconLoader to load the named icon
     * specified in the string.  If the button is checkable (the default state)
     * then the name of its icon doubles as a key for QSettings to keep track of
     * its last state automatically.  Created for SelectDialog, be aware that if
     * you use CheckButton in some other context, and happen to use the same
     * icon as was used there, then terrible confusion could result.
     *
     * Buttons that don't take the memory effect are always created in a checked
     * state initially.
     */
    CheckButton(QString iconName, bool wantsMemory = true,  QWidget *parent = 0);

    ~CheckButton();
};


}

#endif
