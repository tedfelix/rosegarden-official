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

#ifndef RG_STRINGS_H
#define RG_STRINGS_H

#include <string>
#include <QString>
#include <iostream>
#include <QVariant>
#include <QTextStream>
#include <QStringList>

#include "base/PropertyName.h"
#include "base/Exception.h"

class QTextCodec;

namespace Rosegarden
{

extern QString strtoqstr(const std::string &);
extern QString strtoqstr(const Rosegarden::PropertyName &);

extern std::string qstrtostr(const QString &);

extern double strtodouble(const std::string &);
extern double qstrtodouble(const QString &);

extern bool qStrToBool(const QString &s);
extern bool qStrToBool(const QVariant &v);

extern std::string qStrToStrLocal8(const QString &qstr);
extern std::string qStrToStrUtf8(const QString &qstr);

extern std::string convertFromCodec(std::string, QTextCodec *);

std::ostream &
operator<<(std::ostream &, const QString &);

QTextStream &
operator<<(QTextStream &, const std::string &);

/// Split a string at whitespace, allowing for quoted substring sections
extern QStringList splitQuotedString(QString s);

}


#endif
