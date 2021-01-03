/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/



#ifndef RG_INCONSISTENCIES_H
#define RG_INCONSISTENCIES_H

#include <vector>
#include <map>

#include "base/Event.h"
#include "base/Segment.h"
#include "base/Composition.h"
#include "base/Overlaps.h"
#include "base/OverlapRange.h"
#include "gui/dialogs/ClefDialog.h"
#include "StaffHeader.h"

#include <QObject>
#include <QString>
#include <QCoreApplication>

namespace Rosegarden
{

template <class T>
class Inconsistencies : public Overlaps<T>
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::Inconsistencies)
public :

    Inconsistencies(std::vector<Segment *> segments) : Overlaps<T>(segments) {}

    ~Inconsistencies() {}

    void display(QString &str, Composition *comp, QString segLine)
    {
        timeT start = comp->getStartMarker();
        timeT end = comp->getEndMarker();

        typename std::map<timeT, OverlapRange<T> >::iterator it;
        if (this->getFirst(start, end, it)) {
            for (;;) {
                timeT t1, t2;
                if (!this->isConsistent(it)) {
                    this->getTimeRange(it, t1, t2);
                    int bar1 = comp->getBarNumber(t1) + 1;
                    int bar2 = comp->getBarNumber(t2) + 1;
                    str += QString("<blockquote>");
                    if (bar1 == bar2) {
                        str += QObject::tr("Bar %1:").arg(bar1);
                    } else {
                        str += QObject::tr("Bars %1 to %2:").arg(bar1)
                                                             .arg(bar2);
                    }

                    str += QString("<blockquote>");
                    const std::vector<Segment *> *s = this->getSegments(it);
                    std::vector<Segment *>::const_iterator sit;
                    for (sit = s->begin(); sit != s->end(); ++sit) {
                        if (sit != s->begin()) str += QString("<br>");                            
                        T pr = Overlaps<T>::getPropertyAtTime(*sit, t1);
                        str+= segLine
                                  .arg(QString::fromStdString((*sit)->getLabel()))
                                  .arg(this->getTranslatedName(pr));
                    }
                    str += QString("</blockquote></blockquote>");
                }
                if (!this->getNext(end, it)) break;
            }
        }
    }

protected :

    QString getTranslatedName(T property) const;


private :
    //--------------- Data members ---------------------------------

};


//------------- Specialized functions -----------------------------------

template <>
inline QString
Inconsistencies<Clef>::getTranslatedName(Clef clef) const
{

    return ClefDialog::translatedClefName(clef);
}

template <>
inline QString
Inconsistencies<Key>::getTranslatedName(Key key) const
{
    // Key::getName() returns strings like "F# minor" from it's detail map.
    // I started three different approaches to this one, and by far the least
    // complicated is to just perform surgery on the string.  The left part of
    // the string "F#" exists in the QObject translation context, and the
    // major/minor bit is easy to re-create, so there's no real point in
    // preserving it from the original string.  This gets it done without the
    // pointless addition of a pile of new strings.
    QString keyName = QString::fromStdString(key.getName());
    return QObject::tr("%1 %2").arg(QObject::tr(keyName.left(keyName.indexOf(" ")).toStdString().c_str(), "note name" ))
                               .arg(key.isMinor() ? QObject::tr("minor")
                                                  : QObject::tr("major"));
}

template <>
inline QString
Inconsistencies<int>::getTranslatedName(int transpose) const
{
    return StaffHeader::transposeValueToName(transpose);
}


}

#endif // RG_INCONSISTENCIES_H

