/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "ShowSequencerStatusDialog.h"

#include <klocale.h>
#include <kdialogbase.h>
#include <qcstring.h>
#include <qdatastream.h>
#include <qlabel.h>
#include <qstring.h>
#include <qtextedit.h>
#include <qvbox.h>
#include <qwidget.h>
#include "gui/application/RosegardenApplication.h"


namespace Rosegarden
{

ShowSequencerStatusDialog::ShowSequencerStatusDialog(QWidget *parent) :
        KDialogBase(parent, 0, true, i18n("Sequencer status"), Close)
{
    QVBox *vbox = makeVBoxMainWidget();

    new QLabel(i18n("Sequencer status:"), vbox);

    QString status(i18n("Status not available."));

    QCString replyType;
    QByteArray replyData;
    QByteArray data;

    if (!rgapp->sequencerCall("getStatusLog()", replyType, replyData)) {
        status = i18n("Sequencer is not running or is not responding.");
    }

    QDataStream streamIn(replyData, IO_ReadOnly);
    QString result;
    streamIn >> result;
    if (!result) {
        status = i18n("Sequencer is not returning a valid status report.");
    } else {
        status = result;
    }

    QTextEdit *text = new QTextEdit(vbox);
    text->setTextFormat(Qt::PlainText);
    text->setReadOnly(true);
    text->setMinimumWidth(500);
    text->setMinimumHeight(200);

    text->setText(status);
}

}
#include "ShowSequencerStatusDialog.moc"
