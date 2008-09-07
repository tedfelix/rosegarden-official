/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
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
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFont>
#include <QLabel>
#include <QString>
#include <QTextCodec>
#include <QWidget>
#include <QVBoxLayout>


namespace Rosegarden
{

IdentifyTextCodecDialog::IdentifyTextCodecDialog(QDialogButtonBox::QWidget *parent,
        std::string text) :
        QDialog(parent),
        m_text(text)
{
    setModal(true);
    setWindowTitle(i18n("Choose Text Encoding"));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *vbox = new QWidget(this);
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    metagrid->addWidget(vbox, 0, 0);

    new QLabel(i18n("\nThis file contains text in an unknown language encoding.\n\nPlease select one of the following estimated text encodings\nfor use with the text in this file:\n"), vbox);

    QComboBox *codecs = new QComboBox( vbox );
    vboxLayout->addWidget(codecs);

    std::string defaultCodec;
    QTextCodec *cc = QTextCodec::codecForContent(text.c_str(), text.length());
    QTextCodec *codec = 0;

    std::cerr << "cc is " << (cc ? cc->objectName() : "null") << std::endl;

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

    int selectedProbability = 0;
    if (cc) {
        selectedProbability = cc->heuristicContentMatch
            (m_text.c_str(), m_text.length());
    }

    while ((codec = QTextCodec::codecForIndex(i)) != 0) {

        int probability = codec->heuristicContentMatch
            (m_text.c_str(), m_text.length());

        if (probability <= 0) {
            ++i;
            continue;
        }

        std::string name = codec->objectName();

        std::cerr << "codec " << name << " probability " << probability << std::endl;

        if (name == "UTF-8" && 
            (!cc || (cc->objectName() != name)) &&
            probability > selectedProbability/2) {
            std::cerr << "UTF-8 has a decent probability, selecting it instead to promote global harmony" << std::endl;
            cc = codec;
        }

        QString description = codecDescriptions[name];
        if (description == "") {
            if (strtoqstr(name).left(3) == "CP ") {
                description = i18n("Microsoft Code Page %1", 
                              strtoqstr(name).right(name.length() - 3));
            }
        }

        if (description != "") {
            description = i18n("%1 (%2)", strtoqstr(name), description);
        } else {
            description = strtoqstr(name);
        }

        codecs->addItem(description, 0);
        m_codecs.push_front(name);
        if (current >= 0) ++current;

        if (cc && (name == cc->objectName())) {
            current = 0;
        }

        ++i;
    }

    connect(codecs, SIGNAL(activated(int)),
            this, SLOT(slotCodecSelected(int)));

    new QLabel(i18n("\nExample text from file:"), vbox);
    m_example = new QLabel("", vbox );
    vboxLayout->addWidget(m_example);
    vbox->setLayout(vboxLayout);
    QFont font;
    font.setStyleHint(QFont::TypeWriter);
    m_example->setFont(font);
    m_example->setPaletteForegroundColor(QColor(Qt::blue));
    std::cerr << "calling slotCodecSelected(" << current << ")" << std::endl;
    if (current < 0) current = 0;
    codecs->setCurrentIndex(current);
    slotCodecSelected(current);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void
IdentifyTextCodecDialog::slotCodecSelected(int i)
{
//    std::cerr << "codec index = " << i << std::endl;
    if (i < 0 || i >= m_codecs.size()) return;
    std::string name = m_codecs[i];
//    std::cerr << "codecs: ";
//    for (int j = 0; j < m_codecs.size(); ++j) std::cerr << m_codecs[j] << " ";
//    std::cerr << std::endl;
    QTextCodec *codec = QTextCodec::codecForName(strtoqstr(name));
    if (!codec) return;
    m_codec = qstrtostr(codec->objectName());
    std::cerr << "Applying codec " << m_codec << std::endl;
    QString outText = codec->toUnicode(m_text.c_str(), m_text.length());
    if (outText.length() > 80) outText = outText.left(80);
    m_example->setText("\"" + outText + "\"");
}

}
#include "IdentifyTextCodecDialog.moc"
