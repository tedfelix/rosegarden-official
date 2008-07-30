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


#include "AudioPlayingDialog.h"

#include <klocale.h>
#include <kdialogbase.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qstring.h>
#include <qwidget.h>


namespace Rosegarden
{

AudioPlayingDialog::AudioPlayingDialog(QWidget *parent,
                                       const QString &name):
        KDialogBase(parent, 0, true,
                    i18n("Playing audio file"),
                    Cancel)
{
    QHBox *w = makeHBoxMainWidget();
    QLabel *label = new
                    QLabel(i18n("Playing audio file \"%1\"").arg(name), w);

    label->setMinimumHeight(80);


}

}
#include "AudioPlayingDialog.moc"
