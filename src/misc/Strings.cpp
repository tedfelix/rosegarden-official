/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
    See the AUTHORS file for more details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Strings.h"

#include "base/Composition.h"
#include "base/Segment.h"
#include "base/Event.h"

#include <QTextCodec>
#include <QVariant>
#include <QString>

namespace Rosegarden
{

ROSEGARDENPRIVATE_EXPORT QString strtoqstr(const std::string &str)
{
    return QString::fromUtf8(str.c_str());
}

ROSEGARDENPRIVATE_EXPORT QString strtoqstr(const Rosegarden::PropertyName &p)
{
    return QString::fromUtf8(p.c_str());
}

ROSEGARDENPRIVATE_EXPORT std::string qstrtostr(const QString &qstr)
{
    return std::string(qstr.toUtf8().data());
}

ROSEGARDENPRIVATE_EXPORT std::string qStrToStrUtf8(const QString &qstr)
{
    return qstrtostr(qstr);
}

ROSEGARDENPRIVATE_EXPORT std::string qStrToStrLocal8(const QString &qstr)
{
    return std::string(qstr.toLocal8Bit().data());
}


/**
 * Unlike strtod(3) or QString::toDouble(), this is locale-independent
 * and always uses '.' as a decimal point.  We use it when reading
 * things like configuration values from XML files where we want to
 * guarantee the same value is used regardless of surrounding locale.
 */
ROSEGARDENPRIVATE_EXPORT double strtodouble(const std::string &s)
{
    int dp = 0;
    int sign = 1;
    size_t i = 0;  //@@@
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

ROSEGARDENPRIVATE_EXPORT double qstrtodouble(const QString &s)
{
    return strtodouble(qstrtostr(s));
}


ROSEGARDENPRIVATE_EXPORT bool qStrToBool(const QString &s)
{
    QString tt = s.toLower();
    tt = tt.trimmed();
    if ( (tt == "1") || (tt == "true") || (tt == "yes") || (tt == "on") ){
        return true;
    }
    return false;
}

ROSEGARDENPRIVATE_EXPORT bool qStrToBool(const QVariant &v)
{
    return qStrToBool( v.toString() );
}




ROSEGARDENPRIVATE_EXPORT std::string convertFromCodec(std::string text, QTextCodec *codec)
{
    if (codec)
        return qstrtostr(codec->toUnicode(text.c_str(), text.length()));
    else
        return text;
}

ROSEGARDENPRIVATE_EXPORT std::ostream &operator<<(std::ostream &target, const QString &str)
{
    return target << str.toLocal8Bit().data();
}

ROSEGARDENPRIVATE_EXPORT QTextStream &operator<<(QTextStream &target, const std::string &str)
{
    return target << QString(str.c_str());
}

ROSEGARDENPRIVATE_EXPORT QStringList splitQuotedString(QString s)
{
    QStringList tokens;
    QString tok;

    enum { sep, unq, q1, q2 } mode = sep;

    for (int i = 0; i < s.length(); ++i) {
	
	QChar c = s[i];

	if (c == '\'') {
	    switch (mode) {
	    case sep: mode = q1; break;
	    case unq: case q2: tok += c; break;
	    case q1: mode = sep; tokens << tok; tok = ""; break;
	    }

	} else if (c == '"') {
	    switch (mode) {
	    case sep: mode = q2; break;
	    case unq: case q1: tok += c; break;
	    case q2: mode = sep; tokens << tok; tok = ""; break;
	    }

	} else if (c.isSpace()) {
	    switch (mode) {
	    case sep: break;
	    case unq: mode = sep; tokens << tok; tok = ""; break;
	    case q1: case q2: tok += c; break;
	    }

	} else if (c == '\\') {
	    if (++i < s.length()) {
		c = s[i];
		switch (mode) {
		case sep: mode = unq; tok += c; break;

                    // All fall thru to default.
                case unq:
                case q1:
                case q2:
		default: tok += c; break;
		}
	    }

	} else {
	    switch (mode) {
	    case sep: mode = unq; tok += c; break;

                // All fall thru to default.
            case unq:
            case q1:
            case q2:
	    default: tok += c; break;
	    }
	}
    }

    if (tok != "" || mode != sep) tokens << tok;
    return tokens;
}

/*

void testSplit()
{
    QStringList tests;
    tests << "a b c d";
    tests << "a \"b c\" d";
    tests << "a 'b c' d";
    tests << "a \"b c\\\" d\"";
    tests << "a 'b c\\' d'";
    tests << "a \"b c' d\"";
    tests << "a 'b c\" d'";
    tests << "aa 'bb cc\" dd'";
    tests << "a'a 'bb' \\\"cc\" dd\\\"";
    tests << "  a'a \\\'	 'bb'	 \'	\\\"cc\" ' dd\\\" '";

    for (int j = 0; j < tests.size(); ++j) {
	cout << endl;
	cout << tests[j].toStdString() << endl;
	cout << "->" << endl << "(";
	QStringList l = splitQuoted(tests[j]);
	for (int i = 0; i < l.size(); ++i) {
	    if (i > 0) cout << ";";
	    cout << l[i].toStdString();
	}
	cout << ")" << endl;
    }
}

*/

/* 
   Results:

a b c d
->     
(a;b;c;d)

a "b c" d
->       
(a;b c;d)

a 'b c' d
->       
(a;b c;d)

a "b c\" d"
->         
(a;b c" d) 

a 'b c\' d'
->         
(a;b c' d) 

a "b c' d"
->        
(a;b c' d)

a 'b c" d'
->        
(a;b c" d)

aa 'bb cc" dd'
->            
(aa;bb cc" dd)

a'a 'bb' \"cc" dd\"
->                 
(a'a;bb;"cc";dd")  

  a'a \'         'bb'    '      \"cc" ' dd\" '
->                                            
(a'a;';bb;      "cc" ;dd";)

*/

}

