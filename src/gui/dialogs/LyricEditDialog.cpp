/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
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


#include "LyricEditDialog.h"

#include "base/Event.h"
#include "base/BaseProperties.h"
#include <klocale.h>
#include "misc/Strings.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include <kdialogbase.h>
#include <qgroupbox.h>
#include <qregexp.h>
#include <qstring.h>
#include <qtextedit.h>
#include <qvbox.h>
#include <qwidget.h>


namespace Rosegarden
{

LyricEditDialog::LyricEditDialog(QWidget *parent,
                                 Segment *segment) :
        KDialogBase(parent, 0, true, i18n("Edit Lyrics"), Ok | Cancel | Help),
        m_segment(segment)
{
    setHelp("nv-text-lyrics");

    QVBox *vbox = makeVBoxMainWidget();

    QGroupBox *groupBox = new QGroupBox
                          (1, Horizontal, i18n("Lyrics for this segment"), vbox);

    m_textEdit = new QTextEdit(groupBox);
    m_textEdit->setTextFormat(Qt::PlainText);

    m_textEdit->setMinimumWidth(300);
    m_textEdit->setMinimumHeight(200);

    unparse();
}

void
LyricEditDialog::unparse()
{
    // This and SetLyricsCommand::execute() are opposites that will
    // need to be kept in sync with any changes in one another.  (They
    // should really both be in a common lyric management class.)

    Composition *comp = m_segment->getComposition();

    QString text;

    timeT lastTime = m_segment->getStartTime();
    int lastBarNo = comp->getBarNumber(lastTime);
    bool haveLyric = false;

    text += QString("[%1] ").arg(lastBarNo + 1);

    for (Segment::iterator i = m_segment->begin();
            m_segment->isBeforeEndMarker(i); ++i) {

        bool isNote = (*i)->isa(Note::EventType);
        bool isLyric = false;

        if (!isNote) {
            if ((*i)->isa(Text::EventType)) {
                std::string textType;
                if ((*i)->get
                        <String>(Text::TextTypePropertyName, textType) &&
                        textType == Text::Lyric) {
                    isLyric = true;
                }
            }
        } else {
            if ((*i)->has(BaseProperties::TIED_BACKWARD) &&
                    (*i)->get
                    <Bool>(BaseProperties::TIED_BACKWARD))
                continue;
        }

        if (!isNote && !isLyric)
            continue;

        timeT myTime = (*i)->getNotationAbsoluteTime();
        int myBarNo = comp->getBarNumber(myTime);

        if (myBarNo > lastBarNo) {

            while (myBarNo > lastBarNo) {
                text += " /";
                ++lastBarNo;
            }
            text += QString("\n[%1] ").arg(myBarNo + 1);
        }

        if (myTime > lastTime && isNote) {
            if (!haveLyric)
                text += " .";
            lastTime = myTime;
            haveLyric = false;
        }

        if (isLyric) {
            std::string ssyllable;
            (*i)->get
            <String>(Text::TextPropertyName, ssyllable);
            QString syllable(strtoqstr(ssyllable));
            syllable.replace(QRegExp("\\s+"), "~");
            text += " " + syllable;
            haveLyric = true;
        }
    }

    m_textEdit->setText(text);
}

QString
LyricEditDialog::getLyricData()
{
    return m_textEdit->text();
}

}
#include "LyricEditDialog.moc"
