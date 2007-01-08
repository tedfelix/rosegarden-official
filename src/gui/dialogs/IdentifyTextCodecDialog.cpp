/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2007
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


#include "IdentifyTextCodecDialog.h"

#include <klocale.h>
#include "misc/Strings.h"
#include "base/NotationTypes.h"
#include <kcombobox.h>
#include <kdialogbase.h>
#include <qfont.h>
#include <qlabel.h>
#include <qstring.h>
#include <qtextcodec.h>
#include <qvbox.h>
#include <qwidget.h>


namespace Rosegarden
{

IdentifyTextCodecDialog::IdentifyTextCodecDialog(QWidget *parent,
        std::string text) :
        KDialogBase(parent, 0, true, i18n("Choose Text Encoding"), Ok),
        m_text(text)
{
    QVBox *vbox = makeVBoxMainWidget();
    new QLabel(i18n("\nThis file contains text in an unknown language encoding.\n\nPlease select one of the following estimated text encodings\nfor use with the text in this file:\n"), vbox);

    KComboBox *codecs = new KComboBox(vbox);

    std::string defaultCodec;
    QTextCodec *cc = QTextCodec::codecForContent(text.c_str(), text.length());
    QTextCodec *codec = 0;

    std::cerr << "cc is " << (cc ? cc->name() : "null") << std::endl;

    std::map<std::string, QString> codecDescriptions;
    codecDescriptions["SJIS"] = i18n("Japanese Shift-JIS");
    codecDescriptions["UTF-8"] = i18n("Unicode variable-width");
    codecDescriptions["ISO 8859-1"] = i18n("Western Europe");
    codecDescriptions["ISO 8859-15"] = i18n("Western Europe + Euro");
    codecDescriptions["ISO 8859-2"] = i18n("Eastern Europe");
    codecDescriptions["ISO 8859-3"] = i18n("Southern Europe");
    codecDescriptions["ISO 8859-4"] = i18n("Northern Europe");
    codecDescriptions["ISO 8859-5"] = i18n("Cyrillic");
    codecDescriptions["ISO 8859-6"] = i18n("Arabic");
    codecDescriptions["ISO 8859-7"] = i18n("Greek");
    codecDescriptions["ISO 8859-8"] = i18n("Hebrew");
    codecDescriptions["ISO 8859-9"] = i18n("Turkish");
    codecDescriptions["ISO 8859-10"] = i18n("Nordic");
    codecDescriptions["ISO 8859-11"] = i18n("Thai");
    codecDescriptions["ISO 8859-13"] = i18n("Baltic");
    codecDescriptions["ISO 8859-14"] = i18n("Celtic");
    codecDescriptions["SJIS"] = i18n("Japanese Shift-JIS");
    codecDescriptions["Big5"] = i18n("Traditional Chinese");
    codecDescriptions["GB18030"] = i18n("Simplified Chinese");
    codecDescriptions["KOI8-R"] = i18n("Russian");
    codecDescriptions["KOI8-U"] = i18n("Ukrainian");
    codecDescriptions["TSCII"] = i18n("Tamil");

    int i = 0;
    int current = -1;

    while ((codec = QTextCodec::codecForIndex(i)) != 0) {
        if (codec->heuristicContentMatch(m_text.c_str(), m_text.length()) <= 0) {
            ++i;
            continue;
        }
        std::cerr << "codec " << codec->name() << " probability " << codec->heuristicContentMatch(m_text.c_str(), m_text.length()) << std::endl;
        std::string name = codec->name();
        QString description = codecDescriptions[name];
        if (description == "") {
            if (strtoqstr(name).left(3) == "CP ") {
                description = i18n("Microsoft Code Page %1").
                              arg(strtoqstr(name).right(name.length() - 3));
            }
        }
        if (description != "") {
            description = i18n("%1 (%2)").arg(strtoqstr(name)).arg(description);
        } else {
            description = strtoqstr(name);
        }
        codecs->insertItem(description, 0);
        m_codecs.push_back(name);
        if (cc && (name == cc->name())) {
            codecs->setCurrentItem(0);
            current = 0;
        } else {
            if (current >= 0)
                ++current;
        }
        ++i;
    }

    connect(codecs, SIGNAL(activated(int)),
            this, SLOT(slotCodecSelected(int)));

    new QLabel(i18n("\nExample text from file:"), vbox);
    m_example = new QLabel("", vbox);
    QFont font;
    font.setStyleHint(QFont::TypeWriter);
    m_example->setFont(font);
    m_example->setPaletteForegroundColor(Qt::blue);
    slotCodecSelected(current >= 0 ? current : 0);
}

void
IdentifyTextCodecDialog::slotCodecSelected(int i)
{
    if (i < 0 || i >= m_codecs.size())
        return ;
    std::string name = m_codecs[m_codecs.size() - i - 1];
    QTextCodec *codec = QTextCodec::codecForName(strtoqstr(name));
    if (!codec)
        return ;
    m_codec = qstrtostr(codec->name());
    std::cerr << "Applying codec " << m_codec << std::endl;
    QString outText = codec->toUnicode(m_text.c_str(), m_text.length());
    if (outText.length() > 80)
        outText = outText.left(80);
    m_example->setText("\"" + outText + "\"");
}

}
#include "IdentifyTextCodecDialog.moc"
