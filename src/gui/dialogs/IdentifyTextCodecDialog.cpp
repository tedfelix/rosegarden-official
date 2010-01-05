/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2010 the Rosegarden development team.
 
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
#include <QGroupBox>


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
    setWindowTitle(tr("Rosegarden"));

    QVBoxLayout *vboxLayout = new QVBoxLayout;
    setLayout(vboxLayout);

    vboxLayout->addWidget(new QLabel(tr("<qt><h3>Choose Text Encoding</h3></qt>")));

    QGroupBox *g = new QGroupBox;
    QVBoxLayout *gl = new QVBoxLayout;
    vboxLayout->addWidget(g);
    g->setLayout(gl);

    gl->addWidget(new QLabel(tr("<qt><p>This file contains text in an unknown language encoding.</p><p>Please select one of the following estimated text encodings for use with the text in this file:</p></qt>")));

    QComboBox *codecs = new QComboBox;
    gl->addWidget(codecs);

    std::string defaultCodec;
    QTextCodec *cc = 0;
    QTextCodec *codec = 0;

    std::map<std::string, QString> codecDescriptions;
    codecDescriptions["SJIS"] = tr("Japanese Shift-JIS");
    codecDescriptions["UTF-8"] = tr("Unicode variable-width");
    codecDescriptions["ISO 8859-1"] = tr("Western Europe");
    codecDescriptions["ISO 8859-15"] = tr("Western Europe + Euro");
    codecDescriptions["ISO 8859-2"] = tr("Eastern Europe");
    codecDescriptions["ISO 8859-3"] = tr("Southern Europe");
    codecDescriptions["ISO 8859-4"] = tr("Northern Europe");
    codecDescriptions["ISO 8859-5"] = tr("Cyrillic");
    codecDescriptions["ISO 8859-6"] = tr("Arabic");
    codecDescriptions["ISO 8859-7"] = tr("Greek");
    codecDescriptions["ISO 8859-8"] = tr("Hebrew");
    codecDescriptions["ISO 8859-9"] = tr("Turkish");
    codecDescriptions["ISO 8859-10"] = tr("Nordic");
    codecDescriptions["ISO 8859-11"] = tr("Thai");
    codecDescriptions["ISO 8859-13"] = tr("Baltic");
    codecDescriptions["ISO 8859-14"] = tr("Celtic");
    codecDescriptions["SJIS"] = tr("Japanese Shift-JIS");
    codecDescriptions["Big5"] = tr("Traditional Chinese");
    codecDescriptions["GB18030"] = tr("Simplified Chinese");
    codecDescriptions["KOI8-R"] = tr("Russian");
    codecDescriptions["KOI8-U"] = tr("Ukrainian");
    codecDescriptions["TSCII"] = tr("Tamil");

    int i = 0;
    int current = -1;

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

        std::string name = qstrtostr(codec->name().data());

        std::cerr << "codec " << name << " preserves " << preserves << std::endl;

        if (name == "UTF-8" && cc && preserves) {
            std::cerr << "UTF-8 is a possibility, selecting it instead of our first choice to promote global harmony" << std::endl;
            cc = codec;
        }

        QString description = codecDescriptions[name];
        if (description == "") {
            if (strtoqstr(name).left(3) == "CP ") {
                description = tr("Microsoft Code Page %1")
                              .arg(strtoqstr(name).right(name.length() - 3));
            }
        }

        if (description != "") {
            description = tr("%1 (%2)").arg(strtoqstr(name)).arg(description);
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

    new QLabel(tr("\nExample text from file:"));
    m_example = new QLabel;
    m_example->setStyleSheet("background: #fff3c3; color: black;");
    gl->addWidget(m_example);
    QFont font;
    font.setStyleHint(QFont::TypeWriter);
    m_example->setFont(font);
    if (current < 0) current = 0;
    codecs->setCurrentIndex(current);
    slotCodecSelected(current);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    vboxLayout->addWidget(buttonBox);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void
IdentifyTextCodecDialog::slotCodecSelected(int i)
{
    if (i < 0 || i >= (int)m_codecs.size()) return;
    std::string name = m_codecs[i];
    QTextCodec *codec = QTextCodec::codecForName(name.c_str());
    if (!codec) return;
    m_codec = qstrtostr( codec->name() );
    QString outText = codec->toUnicode(m_text.c_str(), m_text.length());
    if (outText.length() > 80) outText = outText.left(80);
    m_example->setText("\"" + outText + "\"");
}

}
#include "IdentifyTextCodecDialog.moc"
