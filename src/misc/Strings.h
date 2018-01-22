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

#ifndef RG_STRINGS_H
#define RG_STRINGS_H

#include <string>
#include <QString>
#include <QVariant>
#include <QTextStream>
#include <QStringList>

#include "base/PropertyName.h"
#include "base/Exception.h"

#include <rosegardenprivate_export.h>

class QTextCodec;

namespace Rosegarden
{

extern ROSEGARDENPRIVATE_EXPORT QString strtoqstr(const std::string &);
extern ROSEGARDENPRIVATE_EXPORT QString strtoqstr(const Rosegarden::PropertyName &);

extern ROSEGARDENPRIVATE_EXPORT std::string qstrtostr(const QString &);

extern ROSEGARDENPRIVATE_EXPORT double strtodouble(const std::string &);
extern ROSEGARDENPRIVATE_EXPORT double qstrtodouble(const QString &);

extern ROSEGARDENPRIVATE_EXPORT bool qStrToBool(const QString &s);
extern ROSEGARDENPRIVATE_EXPORT bool qStrToBool(const QVariant &v);

extern ROSEGARDENPRIVATE_EXPORT std::string qStrToStrLocal8(const QString &qstr);
extern ROSEGARDENPRIVATE_EXPORT std::string qStrToStrUtf8(const QString &qstr);

extern ROSEGARDENPRIVATE_EXPORT std::string convertFromCodec(std::string, QTextCodec *);

ROSEGARDENPRIVATE_EXPORT std::ostream &operator<<(std::ostream &, const QString &);

ROSEGARDENPRIVATE_EXPORT QTextStream &operator<<(QTextStream &, const std::string &);

/// Split a string at whitespace, allowing for quoted substring sections
extern ROSEGARDENPRIVATE_EXPORT QStringList splitQuotedString(QString s);

}


#endif
