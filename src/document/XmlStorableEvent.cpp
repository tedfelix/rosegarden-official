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

#define RG_MODULE_STRING "[XmlStorableEvent]"

#include "XmlStorableEvent.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Event.h"
#include "base/NotationTypes.h"
//#include "base/BaseProperties.h"
#include "gui/editors/notation/NotationStrings.h"

#include <QString>


namespace Rosegarden
{

XmlStorableEvent::XmlStorableEvent(const QXmlAttributes &attributes,
                                   timeT &absoluteTime)
{
    setDuration(0);

    for (int i = 0; i < attributes.length(); ++i) {

        QString attrName(attributes.qName(i)),
        attrVal(attributes.value(i));

        if (attrName == "package") {

            RG_DEBUG << "XmlStorableEvent::XmlStorableEvent: Warning: XML still uses deprecated \"package\" attribute";

        } else if (attrName == "type") {

            setType(qstrtostr(attrVal));

        } else if (attrName == "subordering") {

            bool isNumeric = true;
            int o = attrVal.toInt(&isNumeric);

            if (!isNumeric) {
                RG_DEBUG << "XmlStorableEvent::XmlStorableEvent: Bad subordering: " << attrVal;
            } else {
                if (o != 0)
                    setSubOrdering(o);
            }

        } else if (attrName == "duration") {

            bool isNumeric = true;
            timeT d = attrVal.toInt(&isNumeric);

            if (!isNumeric) {
                try {
                    Note n(NotationStrings::getNoteForName(attrVal));
                    setDuration(n.getDuration());
                } catch (const NotationStrings::MalformedNoteName &m) {
                    RG_DEBUG << "XmlStorableEvent::XmlStorableEvent: Bad duration: " << attrVal << " (" << m.getMessage() << ")";
                }
            } else {
                setDuration(d);
            }

        } else if (attrName == "absoluteTime") {

            bool isNumeric = true;
            timeT t = attrVal.toInt(&isNumeric);

            if (!isNumeric) {
                RG_DEBUG << "XmlStorableEvent::XmlStorableEvent: Bad absolute time: " << attrVal;
            } else {
                absoluteTime = t;
            }

        } else if (attrName == "timeOffset") {

            bool isNumeric = true;
            timeT t = attrVal.toInt(&isNumeric);

            if (!isNumeric) {
                RG_DEBUG << "XmlStorableEvent::XmlStorableEvent: Bad time offset: " << attrVal;
            } else {
                absoluteTime += t;
            }

        } else {

            // set generic property
            //
            QString val(attrVal);

            // Check if boolean val
            QString valLowerCase(val.toLower());
            bool isNumeric;
            int numVal;

            if (valLowerCase == "true" || valLowerCase == "false") {

                set
                    <Bool>(qstrtostr(attrName), valLowerCase == "true");

            } else {

                // Not a bool, check if integer val
                numVal = val.toInt(&isNumeric);
                if (isNumeric) {
                    set
                        <Int>(qstrtostr(attrName), numVal);
                } else {
                    // not an int either, default to string
                    set
                        <String>(qstrtostr(attrName), qstrtostr(attrVal));
                }
            }
        }
    }
/*
    if (isa(Note::EventType)
        && getDuration() < 1
        && !has(BaseProperties::IS_GRACE_NOTE)) {

        setDuration(1);
    }
*/
    setAbsoluteTime(absoluteTime);
}

XmlStorableEvent::XmlStorableEvent(Event &e) :
        Event(e)
{}

void
XmlStorableEvent::setPropertyFromAttributes(const QXmlAttributes &attributes,
        bool persistent)
{
    bool have = false;
    QString name = attributes.value("name");
    if (name == "") {
        RG_DEBUG << "XmlStorableEvent::setProperty: no property name found, ignoring";
        return ;
    }

    for (int i = 0; i < attributes.length(); ++i) {
        QString attrName(attributes.qName(i)),
        attrVal(attributes.value(i));

        if (attrName == "name") {
            continue;
        } else if (have) {
            RG_DEBUG << "XmlStorableEvent::setProperty: multiple values found, ignoring all but the first";
            continue;
        } else if (attrName == "bool") {
            set<Bool>(qstrtostr(name), attrVal.toLower() == "true", persistent);
            have = true;
        } else if (attrName == "int") {
            set<Int>(qstrtostr(name), attrVal.toInt(), persistent);
            have = true;
        } else if (attrName == "string") {
            set<String>(qstrtostr(name), qstrtostr(attrVal), persistent);
            have = true;
        } else {
            RG_DEBUG << "XmlStorableEvent::setProperty: unknown attribute name \"" << name << "\", ignoring";
        }
    }

    if (!have)
        RG_DEBUG << "XmlStorableEvent::setProperty: Warning: no property value found for property " << name;
}

}
