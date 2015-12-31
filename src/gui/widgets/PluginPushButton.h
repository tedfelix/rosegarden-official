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

#ifndef RG_PLUGIN_PUSHBUTTON_H
#define RG_PLUGIN_PUSHBUTTON_H

#include <QPushButton>


class QWidget;
class QMouseEvent;


namespace Rosegarden
{

/** Tidy up plugin button color management code by creating a new type of
 * QPushButton that alters its style to reflect one of three possible states.
 * This consolidates all the color management code under one roof, and cleans up
 * a lot of old palette juggling nonsense with a consistent implementation in
 * just one place.
 *
 * \author D. Michael McIntyre
 */
class PluginPushButton : public QPushButton
{
Q_OBJECT

public:

    enum State {Normal, Bypassed, Active};


    PluginPushButton(QWidget *parent = 0) : QPushButton(parent)
    {
    };

    virtual ~PluginPushButton();

    /** Set the state of the PluginPushButton to one of:
     *
     *     Normal    Style like standard QPushButton
     *     Bypassed  Style in the "loaded but bypassed" scheme
     *     Active    Style in the "loaded and active" scheme
     */
    void setState(State state);

private:
};


}

#endif
