/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/



#ifndef _INCONSISTENCIES_H_
#define _INCONSISTENCIES_H_

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

namespace Rosegarden
{

template <class T>
class Inconsistencies : public Overlaps<T>
{

public :

    Inconsistencies(std::vector<Segment *> segments) : Overlaps<T>(segments) {}

    ~Inconsistencies() {}

    void display(QString &str, Composition *comp, QString segLine)
    {
        timeT start = comp->getStartMarker();
        timeT end = comp->getEndMarker();

        typename std::map<timeT, OverlapRange<T> >::iterator it;
        if (getFirst(start, end, it)) {
            for (;;) {
                timeT t1, t2;
                if (!isConsistent(it)) {
                    getTimeRange(it, t1, t2);
                    int bar1 = comp->getBarNumber(t1) + 1;
                    int bar2 = comp->getBarNumber(t2) + 1;
                    str += QString("<blockquote>");
                    if (bar1 == bar2) {
                        str += QObject::tr("Bar %1 :").arg(bar1);
                    } else {
                        str += QObject::tr("Bars %1 to %2 :").arg(bar1)
                                                             .arg(bar2);
                    }

                    str += QString("<blockquote>");
                    const std::vector<Segment *> *s = getSegments(it);
                    std::vector<Segment *>::const_iterator sit;
                    for (sit = s->begin(); sit != s->end(); ++sit) {
                        if (sit != s->begin()) str += QString("<br>");                            
                        T pr = Overlaps<T>::getPropertyAtTime(*sit, t1);
                        str+= segLine
                                  .arg(QString::fromStdString((*sit)->getLabel()))
                                  .arg(getTranslatedName(pr));
                    }
                    str += QString("</blockquote></blockquote>");
                }
                if (!getNext(end, it)) break;
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

// TODO : The function is named getTranslatedName, but the name
//        is never translated !!!
//    Still not done because not so easy as clefs and transpositions
//    cases were. 
    return QString::fromStdString(key.getName());
}

template <>
inline QString
Inconsistencies<int>::getTranslatedName(int transpose) const
{
    return StaffHeader::transposeValueToName(transpose);
}


}

#endif // _INCONSISTENCIES_H_

