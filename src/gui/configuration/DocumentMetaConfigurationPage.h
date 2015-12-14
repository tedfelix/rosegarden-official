/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_DOCUMENTMETACONFIGURATIONPAGE_H
#define RG_DOCUMENTMETACONFIGURATIONPAGE_H

#include "TabbedConfigurationPage.h"
#include <QString>
#include "base/Composition.h"


class QWidget;
class QListWidget;


namespace Rosegarden
{

class RosegardenDocument;
class HeadersConfigurationPage;
class CommentsConfigurationPage;

/**
 * Document Meta-information page
 *
 * (document-wide settings)
 */
class DocumentMetaConfigurationPage : public TabbedConfigurationPage
{
    Q_OBJECT
public:
    DocumentMetaConfigurationPage(RosegardenDocument *doc, QWidget *parent = 0);
    virtual void apply();

    static QString iconLabel() { return tr("About"); }
    static QString title() { return tr("About"); }
    static QString iconName()  { return "mm-mime-hi32-rosegarden"; }

/* hjj: WHAT TO DO WITH THIS ?
    void selectMetadata(QString name);
*/

protected:
    static QString durationToString(Composition &comp,
                                    timeT absTime,
                                    timeT duration,
                                    RealTime rt) {
        return tr("%1 minutes %2.%3%4 seconds (%5 units, %6 measures)") // TODO - PLURAL
	 .arg(rt.sec / 60).arg(rt.sec % 60)
	 .arg(rt.msec() / 100).arg((rt.msec() / 10) % 10)
	 .arg(duration).arg(comp.getBarNumber(absTime + duration) -
			   comp.getBarNumber(absTime));
    }

    //--------------- Data members ---------------------------------

    HeadersConfigurationPage *m_headersPage;
    CommentsConfigurationPage *m_commentsPage;
};



}

#endif
