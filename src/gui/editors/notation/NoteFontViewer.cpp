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


#include "NoteFontViewer.h"

#include <klocale.h>
#include "FontViewFrame.h"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <ktoolbar.h>
#include <QLabel>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <QVBoxLayout>


namespace Rosegarden
{

void
NoteFontViewer::slotViewChanged(QDialogButtonBox::int i)
{
    m_frame->setGlyphs(i == 0);

    m_rows->clear();
    int firstRow = -1;

    for (int r = 0; r < 256; ++r) {
        if (m_frame->hasRow(r)) {
            m_rows->addItem(QString("%1").arg(r));
            if (firstRow < 0)
                firstRow = r;
        }
    }

    if (firstRow < 0) {
        m_rows->setEnabled(false);
        m_frame->setRow(0);
    } else {
        m_rows->setEnabled(true);
        m_frame->setRow(firstRow);
    }
}

void
NoteFontViewer::slotRowChanged(const QString &s)
{
    bool ok;
    int i = s.toInt(&ok);
    if (ok)
        m_frame->setRow(i);
}

void
NoteFontViewer::slotFontChanged(const QString &s)
{
    m_frame->setFont(s);
    slotViewChanged(m_view->currentIndex());
}

NoteFontViewer::NoteFontViewer(QWidget *parent, QString noteFontName,
                               QStringList fontNames, int pixelSize) :
        QDialog(parent)
{
    setModal(true);
    setWindowTitle(i18n("Note Font Viewer: %1", noteFontName));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *box = new QWidget(this);
    QVBoxLayout *boxLayout = new QVBoxLayout;
    metagrid->addWidget(box, 0, 0);

    KToolBar *controls = new KToolBar( box );
    boxLayout->addWidget(controls);
    controls->setMargin(3);

    (void) new QLabel(i18n("  Component: "), controls);
    m_font = new QComboBox(controls);

    for (QStringList::iterator i = fontNames.begin(); i != fontNames.end();
            ++i) {
        m_font->addItem(*i);
    }

    (void) new QLabel(i18n("  View: "), controls);
    m_view = new QComboBox(controls);

    m_view->addItem(i18n("Glyphs"));
    m_view->addItem(i18n("Codes"));

    (void) new QLabel(i18n("  Page: "), controls);
    m_rows = new QComboBox(controls);

    m_frame = new FontViewFrame(pixelSize, box );
    boxLayout->addWidget(m_frame);
    box->setLayout(boxLayout);

    connect(m_font, SIGNAL(activated(const QString &)),
            this, SLOT(slotFontChanged(const QString &)));

    connect(m_view, SIGNAL(activated(int)),
            this, SLOT(slotViewChanged(int)));

    connect(m_rows, SIGNAL(activated(const QString &)),
            this, SLOT(slotRowChanged(const QString &)));

    slotFontChanged(m_font->currentText());
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

}
#include "NoteFontViewer.moc"
