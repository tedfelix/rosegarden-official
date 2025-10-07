/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_IDENTIFYTEXTCODECDIALOG_H
#define RG_IDENTIFYTEXTCODECDIALOG_H

#include <QDialog>
#include <QString>
#include <QStringList>

class QWidget;
class QLabel;

#include <string>


namespace Rosegarden
{


class IdentifyTextCodecDialog : public QDialog
{
    Q_OBJECT

public:

    IdentifyTextCodecDialog(QWidget *parent, const std::string &text);

    QString getCodec() const { return m_codec; }

protected slots:

    void slotCodecSelected(int);

protected:

    QString m_codec;
    QStringList m_codecs;

    std::string m_exampleText;
    // Use the selected codec (m_codec) to translate m_exampleText to unicode
    // for display.
    QString getExampleText();
    QLabel *m_example;

};


}

#endif
