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

#ifndef RG_WARNING_GROUP_BOX_H
#define RG_WARNING_GROUP_BOX_H

#include <QGroupBox>

/**This subclass of QGroupBox uses a spot stylesheet to give itself a look
 * suitable for warning messages.
 *
 * It was originally used for the sample rate warning in the audio file dialog.
 * I factored it out into a separate class in case we ever have some need to use
 * this elsewhere.
 *
 * \author D. Michael McIntyre
 */
namespace Rosegarden
{

class WarningGroupBox : public QGroupBox
{
    Q_OBJECT
public:
    WarningGroupBox(QWidget *parent=0);
    ~WarningGroupBox();
};

}

#endif
