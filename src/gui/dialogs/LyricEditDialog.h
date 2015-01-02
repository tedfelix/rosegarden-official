
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_LYRICEDITDIALOG_H
#define RG_LYRICEDITDIALOG_H

#include <QDialog>
#include <QString>
#include <vector>


class QWidget;
class QTextEdit;
class QComboBox;
class QPushButton;


namespace Rosegarden
{

class Segment;


class LyricEditDialog : public QDialog
{
    Q_OBJECT

public:
    LyricEditDialog(QWidget *parent, Segment *segment);

    int getVerseCount() const;
    QString getLyricData(int verse) const;

protected slots:
    void slotVerseNumberChanged(int);
    void slotAddVerse();
    void slotRemoveVerse();
    void slotHelpRequested();

protected:
    Segment *m_segment;

    int m_currentVerse;
    QComboBox *m_verseNumber;
    QTextEdit *m_textEdit;
    QPushButton *m_verseAddButton;
    QPushButton *m_verseRemoveButton;

    int m_verseCount;
    std::vector<QString> m_texts;
    QString m_skeleton;

    void countVerses();
    void unparse();
    void verseDialogRepopulate();
};

}

#endif
