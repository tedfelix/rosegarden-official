/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2020 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "base/XmlExportable.h"

#include <QTest>

#include <string>

using namespace Rosegarden;

/// Unit test for UTF-8
class TestUTF8 : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void test();
};

std::string binary(unsigned char c)
{
    std::string s;
    for (int i = 0; i < 8; ++i) {
        s = ((c & 0x1) ? '1' : '0') + s;
        c >>= 1;
    }
    return s;
}

void TestUTF8::test()
{
    QCOMPARE(XmlExportable::encode("ニュース").c_str(),
                                        "ニュース");
    QCOMPARE(XmlExportable::encode("주요 뉴스").c_str(),
                                        "주요 뉴스");
    QCOMPARE(XmlExportable::encode("Nyheter").c_str(),
                                   "Nyheter");
    QCOMPARE(XmlExportable::encode("天气").c_str(),
                                   "天气");
    QCOMPARE(XmlExportable::encode("Notícias").c_str(),
                                   "Notícias");

    QCOMPARE(XmlExportable::encode("ニュ&ース").c_str(),
                                        "ニュ&amp;ース");
    QCOMPARE(XmlExportable::encode("주요 <뉴스>").c_str(),
                                        "주요 &lt;뉴스&gt;");
    QCOMPARE(XmlExportable::encode("\"Nyheter\"").c_str(),
                                   "&quot;Nyheter&quot;");
    QCOMPARE(XmlExportable::encode("'Notícias'").c_str(),
                                   "&apos;Notícias&apos;");

    QCOMPARE(XmlExportable::encode("&\t<\n>'").c_str(),
             "&amp; &lt; &gt;&apos;");
    QCOMPARE(XmlExportable::encode("\"\r").c_str(),
             "&quot; ");
    QCOMPARE(XmlExportable::encode("Nyhe\004ter").c_str(),
             "Nyheter");

#if 0
    // These seem to have EF BF BD inserted in various places.
    // encode() doesn't seem to find it.
    std::string invalid[] = {
        "����ース",  // encode() doesn't find any problems with these.
        "주� � 뉴스",
        "�天气",
        "Not�cias",
    };

    qDebug() << "Testing invalid strings -- should be "
             << (sizeof(invalid)/sizeof(invalid[0]))
             << " errors here (but no fatal ones)";

    for (size_t i = 0; i < sizeof(invalid)/sizeof(invalid[0]); ++i) {
        qDebug() << "Testing " << i << " ==============================";
        std::string encoded = XmlExportable::encode(invalid[i]);
        qDebug() << encoded.c_str();
        if (encoded == invalid[i]) {
            qDebug() << "FAIL: Encoding succeeded but should have failed:";
            for (size_t j = 0; j < invalid[i].length(); ++j) {
                qDebug() << (char)invalid[i][j]
                         << " (" << binary(invalid[i][j]).c_str() << ")";
            }
            QVERIFY(false);
        }
    }
#endif
}

QTEST_MAIN(TestUTF8)

#include "utf8.moc"
