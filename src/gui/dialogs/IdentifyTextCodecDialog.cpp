/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "IdentifyTextCodecDialog.h"

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

// This dialog used to use QTextCodec::codecForContent() to guess an
// appropriate codec for the string, and then call
// QTextCodec::heuristicContentMatch() to test each listed codec to
// determine whether to include it in the dialog (if it has a non-zero
// probability).

// Unfortunately, both of those functions have been removed from Qt4.
// Not that either was all that effective in the first place.

// QTextCodec::canEncode is not a replacement; it tests whether a
// given QString can be encoded using a certain codec, and our problem
// is that we haven't got a QString to start with.

// Let's try this:

static bool
codecPreserves(QTextCodec &codec, std::string encoded)
{
    QString u = codec.toUnicode(encoded.c_str(), encoded.length());
    QByteArray d = codec.fromUnicode(u);
    if (encoded == d.data()) return true;
    else return false; // not at all authoritative
}

IdentifyTextCodecDialog::IdentifyTextCodecDialog(QWidget *parent,
        std::string text) :
        QDialog(parent),
        m_text(text)
{
    setModal(true);
    setWindowTitle(QObject::tr("Choose Text Encoding"));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *vbox = new QWidget(this);
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    metagrid->addWidget(vbox, 0, 0);

    new QLabel(QObject::tr("\nThis file contains text in an unknown language encoding.\n\nPlease select one of the following estimated text encodings\nfor use with the text in this file:\n"), vbox);

    QComboBox *codecs = new QComboBox( vbox );
    vboxLayout->addWidget(codecs);

    std::string defaultCodec;
    QTextCodec *cc = 0;
    QTextCodec *codec = 0;

    std::map<std::string, QString> codecDescriptions;
    codecDescriptions["SJIS"] = QObject::tr("Japanese Shift-JIS");
    codecDescriptions["UTF-8"] = QObject::tr("Unicode variable-width");
    codecDescriptions["ISO 8859-1"] = QObject::tr("Western Europe");
    codecDescriptions["ISO 8859-15"] = QObject::tr("Western Europe + Euro");
    codecDescriptions["ISO 8859-2"] = QObject::tr("Eastern Europe");
    codecDescriptions["ISO 8859-3"] = QObject::tr("Southern Europe");
    codecDescriptions["ISO 8859-4"] = QObject::tr("Northern Europe");
    codecDescriptions["ISO 8859-5"] = QObject::tr("Cyrillic");
    codecDescriptions["ISO 8859-6"] = QObject::tr("Arabic");
    codecDescriptions["ISO 8859-7"] = QObject::tr("Greek");
    codecDescriptions["ISO 8859-8"] = QObject::tr("Hebrew");
    codecDescriptions["ISO 8859-9"] = QObject::tr("Turkish");
    codecDescriptions["ISO 8859-10"] = QObject::tr("Nordic");
    codecDescriptions["ISO 8859-11"] = QObject::tr("Thai");
    codecDescriptions["ISO 8859-13"] = QObject::tr("Baltic");
    codecDescriptions["ISO 8859-14"] = QObject::tr("Celtic");
    codecDescriptions["SJIS"] = QObject::tr("Japanese Shift-JIS");
    codecDescriptions["Big5"] = QObject::tr("Traditional Chinese");
    codecDescriptions["GB18030"] = QObject::tr("Simplified Chinese");
    codecDescriptions["KOI8-R"] = QObject::tr("Russian");
    codecDescriptions["KOI8-U"] = QObject::tr("Ukrainian");
    codecDescriptions["TSCII"] = QObject::tr("Tamil");

    int i = 0;
    int current = -1;

    int selectedProbability = 0;

    while ((codec = QTextCodec::codecForIndex(i)) != 0) {

        if (!codec) {
            ++i;
            continue;
        }

        bool preserves = codecPreserves(*codec, m_text);

        if (preserves && !cc) {
            // prefer the first codec that seems OK with what we've got
            cc = codec;
        }

/*
  we're no longer confident enough to actually eliminate codecs on the
  basis of our crappy test, though

        if (probability <= 0) {
            ++i;
            continue;
        }
*/

        std::string name = qstrtostr(codec->name().data());

        std::cerr << "codec " << name << " preserves " << preserves << std::endl;

        if (name == "UTF-8" && cc && preserves) {
            std::cerr << "UTF-8 is a possibility, selecting it instead of our first choice to promote global harmony" << std::endl;
            cc = codec;
        }

        QString description = codecDescriptions[name];
        if (description == "") {
            if (strtoqstr(name).left(3) == "CP ") {
                description = QObject::tr("Microsoft Code Page %1")
                              .arg(strtoqstr(name).right(name.length() - 3));
            }
        }

        if (description != "") {
            description = QObject::tr("%1 (%2)").arg(strtoqstr(name)).arg(description);
        } else {
            description = strtoqstr(name);
        }

        codecs->addItem(description, 0);
        m_codecs.push_front(name);
        if (current >= 0) ++current;

        if (cc && (name == qstrtostr(cc->name().data())) ) {
            current = 0;
        }

        ++i;
    }

    connect(codecs, SIGNAL(activated(int)),
            this, SLOT(slotCodecSelected(int)));

    new QLabel(QObject::tr("\nExample text from file:"), vbox);
    m_example = new QLabel("", vbox );
    vboxLayout->addWidget(m_example);
    vbox->setLayout(vboxLayout);
    QFont font;
    font.setStyleHint(QFont::TypeWriter);
    m_example->setFont(font);
    m_example->setPaletteForegroundColor(QColor(Qt::blue));
//    std::cerr << "calling slotCodecSelected(" << current << ")" << std::endl;
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
    QTextCodec *codec = QTextCodec::codecForName( qStrToCharPtrUtf8(strtoqstr(name)) );
    if (!codec) return;
    m_codec = qstrtostr( codec->name() );
//    std::cerr << "Applying codec " << m_codec << std::endl;
    QString outText = codec->toUnicode(m_text.c_str(), m_text.length());
    if (outText.length() > 80) outText = outText.left(80);
    m_example->setText("\"" + outText + "\"");
}

}
#include "IdentifyTextCodecDialog.moc"
