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


#include "IdentifyTextCodecDialog.h"

#include "misc/Strings.h"
#include "base/NotationTypes.h"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFont>
#include <QSet>
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
    vboxLayout->addWidget(g, 20);
    g->setLayout(gl);

    QLabel *l = new QLabel(tr("<qt><p>This file contains text in an unknown language encoding.</p><p>Please select one of the following estimated text encodings for use with the text in this file:</p></qt>"));
    l->setWordWrap(true);
    gl->addWidget(l);

    QComboBox *codecs = new QComboBox;
    gl->addWidget(codecs);

    QString defaultCodec;
    // QTextCodec *codec = 0;

    QMap<QString, QString> codecDescriptions;
    codecDescriptions["UTF-8"] = tr("Unicode variable-width");
    codecDescriptions["ISO-8859-1"] = tr("Western Europe");
    codecDescriptions["ISO-8859-15"] = tr("Western Europe + Euro");
    codecDescriptions["ISO-8859-2"] = tr("Eastern Europe");
    codecDescriptions["ISO-8859-3"] = tr("Southern Europe");
    codecDescriptions["ISO-8859-4"] = tr("Northern Europe");
    codecDescriptions["ISO-8859-5"] = tr("Cyrillic");
    codecDescriptions["ISO-8859-6"] = tr("Arabic");
    codecDescriptions["ISO-8859-7"] = tr("Greek");
    codecDescriptions["ISO-8859-8"] = tr("Hebrew");
    codecDescriptions["ISO-8859-9"] = tr("Turkish");
    codecDescriptions["ISO-8859-10"] = tr("Nordic");
    codecDescriptions["ISO-8859-11"] = tr("Thai");
    codecDescriptions["ISO-8859-13"] = tr("Baltic");
    codecDescriptions["ISO-8859-14"] = tr("Celtic");
    codecDescriptions["SJIS"] = tr("Japanese Shift-JIS");
    codecDescriptions["Big5"] = tr("Traditional Chinese");
    codecDescriptions["GB18030"] = tr("Simplified Chinese");
    codecDescriptions["KOI8-R"] = tr("Russian");
    codecDescriptions["KOI8-U"] = tr("Ukrainian");
    codecDescriptions["TSCII"] = tr("Tamil");

    // int i = 0;
    int current = -1;

    QList<int> mibs = QTextCodec::availableMibs();

    m_codecs.clear();

    QSet<QTextCodec *> seen;

    QTextCodec *cc = 0;
    int currentWeight = -1;
    QMap<QString, int> codecWeights;
    codecWeights["UTF-8"] = 20;
    codecWeights["ISO-8859-1"] = 14;
    codecWeights["ISO-8859-15"] = 13;
    codecWeights["ISO-8859-2"] = 12;
    codecWeights["SJIS"] = 10;
    codecWeights["Big5"] = 10;
    codecWeights["GB18030"] = 10;
    codecWeights["KOI8-R"] = 10;

    for (int i = 0; i < mibs.size(); ++i) {
        
        int mib = mibs[i];

        QTextCodec *codec = QTextCodec::codecForMib(mib);
        if (!codec) continue;
        if (seen.contains(codec)) continue;
        seen.insert(codec);

        bool preserves = codecPreserves(*codec, m_text);

//        std::cerr << "codec " << codec->name().data() << " mib " << mib << " preserves " << preserves << std::endl;

        QStringList names;
        names.push_back(QString::fromLatin1(codec->name()));
        foreach (QByteArray ba, codec->aliases()) {
            names.push_back(QString::fromLatin1(ba));
        }

        QString goodName;
        QString description;

        foreach (QString name, names) {

            if (codecDescriptions.contains(name)) {
                goodName = name;
                description = codecDescriptions[name];
//                std::cerr << "have description " << description.toStdString() << " for name " << name.toStdString() << std::endl;
                if (description == "") {
                    if (name.left(3) == "windows-") {
                        description = tr("Microsoft Code Page %1")
                            .arg(name.right(name.length() - 8));
                    }
                }
            } else {
//                std::cerr << "have no description for name " << name.toStdString() << std::endl;
            }

            if (preserves) {
                int weight = 0;
                if (codecWeights.contains(name)) {
                    weight = codecWeights[name];
                }
                if (!cc || currentWeight < weight) {
                    cc = codec;
                    currentWeight = weight;
                }
            }

            if (name[0].isUpper() && goodName == "") {
                goodName = name;
            }
        }

        if (goodName == "") {
            goodName = QString::fromLatin1(codec->name());
        }

        if (description != "") {
            description = tr("%1 (%2)").arg(goodName).arg(description);
        } else {
            description = goodName;
        }

        m_codecs.push_back(goodName);
        codecs->addItem(description);
        if (cc == codec) current = i;
    }

    connect(codecs, SIGNAL(activated(int)),
            this, SLOT(slotCodecSelected(int)));

    gl->addWidget(new QLabel(tr("\nExample text from file:")));
    m_example = new QLabel;

    // background: #fff3c3; color: black;
    QPalette pal;
    pal.setColor(QPalette::Base, QColor(0xff, 0xf3, 0xc3));
    pal.setColor(QPalette::Text, Qt::black);
    m_example->setPalette(pal);

    gl->addWidget(m_example, 20);
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
    m_codec = m_codecs[i];
    m_example->setText(getExampleText());
}

QString
IdentifyTextCodecDialog::getExampleText()
{
    QTextCodec *codec = QTextCodec::codecForName(m_codec.toLatin1());
    if (!codec) return "";
//    std::cerr << "codec->name() returns " << codec->name().data() << std::endl;

    int offset = 0;
    for (; offset + 80 < (int)m_text.length(); ++offset) {
        if (!isascii(m_text[offset])) {
            for (int i = 0; i < 80; ++i) {
                if (offset == 0) {
                    break;
                }
                --offset;
                if (m_text[offset] == '\n') {
                    break;
                }
            }
            break;
        }
    }

    if (offset < 20) offset = 0;

    QString outText = codec->toUnicode(m_text.c_str(), m_text.length());
    outText = outText.mid(offset, 160);
    return outText;
}    

}
