// -*- c-basic-offset: 4 -*-
/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
    See the AUTHORS file for more details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Strings.h"
#include "Strings.h"

#include "base/Composition.h"
#include "base/Segment.h"
#include "base/Event.h"

#include <QTextCodec>



QString strtoqstr(const std::string &str)
{
    return QString::fromUtf8(str.c_str());
}

QString strtoqstr(const Rosegarden::PropertyName &p)
{
    return QString::fromUtf8(p.c_str());
}

std::string qstrtostr(const QString &qstr)
{
    return std::string(qstr.toUtf8().data());
}

/**
 * Unlike strtod(3) or QString::toDouble(), this is locale-independent
 * and always uses '.' as a decimal point.  We use it when reading
 * things like configuration values from XML files where we want to
 * guarantee the same value is used regardless of surrounding locale.
 */
double strtodouble(const std::string &s)
{
    int dp = 0;
    int sign = 1;
    int i = 0;
    double result = 0.0;
    size_t len = s.length();

    result = 0.0;

    while (i < len && isspace(s[i]))
        ++i;

    if (i < len && s[i] == '-')
        sign = -1;

    while (i < len) {

        char c = s[i];

        if (isdigit(c)) {

            double d = c - '0';

            if (dp > 0) {
                for (int p = dp; p > 0; --p)
                    d /= 10.0;
                ++dp;
            } else {
                result *= 10.0;
            }

            result += d;

        } else if (c == '.') {
            dp = 1;
        }

        ++i;
    }

    return result * sign;
}

double qstrtodouble(const QString &s)
{
    return strtodouble(qstrtostr(s));
}

std::string
convertFromCodec(std::string text, QTextCodec *codec)
{
    if (codec)
        return qstrtostr(codec->toUnicode(text.c_str(), text.length()));
    else
        return text;
}

std::ostream &
operator<<(std::ostream &target, const QString &str)
{
    return target << str.toStdString();
}

