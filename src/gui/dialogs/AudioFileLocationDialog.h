/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_AUDIOFILELOCATIONDIALOG_H
#define RG_AUDIOFILELOCATIONDIALOG_H

#include <QDialog>

class QCheckBox;
class QRadioButton;
class QWidget;


namespace Rosegarden
{


class LineEdit;


class AudioFileLocationDialog : public QDialog
{
    Q_OBJECT

public:
    AudioFileLocationDialog(QWidget *parent, QString documentNameDir);

    enum Location {
        AudioDir,
        DocumentNameDir,
        DocumentDir,
        CentralDir,
        CustomDir
    };

public slots:
    virtual void accept() override;

private:
    /// E.g. "./MyRosegardenDocument"
    QString m_documentNameDirStr;

    QRadioButton *m_audioDir;
    QRadioButton *m_documentNameDir;
    QRadioButton *m_documentDir;
    QRadioButton *m_centralDir;
    QRadioButton *m_customDir;
    LineEdit *m_customDirText;

    QCheckBox *m_dontShow;

    void updateWidgets();
};


}

#endif
