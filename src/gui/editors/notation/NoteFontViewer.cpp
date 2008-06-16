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
#include <kcombobox.h>
#include <kdialogbase.h>
#include <ktoolbar.h>
#include <qlabel.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qvbox.h>
#include <qwidget.h>


namespace Rosegarden
{

void
NoteFontViewer::slotViewChanged(int i)
{
    m_frame->setGlyphs(i == 0);

    m_rows->clear();
    int firstRow = -1;

    for (int r = 0; r < 256; ++r) {
        if (m_frame->hasRow(r)) {
            m_rows->insertItem(QString("%1").arg(r));
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
    slotViewChanged(m_view->currentItem());
}

NoteFontViewer::NoteFontViewer(QWidget *parent, QString noteFontName,
                               QStringList fontNames, int pixelSize) :
        KDialogBase(parent, 0, true,
                    i18n("Note Font Viewer: %1").arg(noteFontName), Close)
{
    QVBox *box = makeVBoxMainWidget();
    KToolBar* controls = new KToolBar(box);
    controls->setMargin(3);

    (void) new QLabel(i18n("  Component: "), controls);
    m_font = new KComboBox(controls);

    for (QStringList::iterator i = fontNames.begin(); i != fontNames.end();
            ++i) {
        m_font->insertItem(*i);
    }

    (void) new QLabel(i18n("  View: "), controls);
    m_view = new KComboBox(controls);

    m_view->insertItem(i18n("Glyphs"));
    m_view->insertItem(i18n("Codes"));

    (void) new QLabel(i18n("  Page: "), controls);
    m_rows = new KComboBox(controls);

    m_frame = new FontViewFrame(pixelSize, box);

    connect(m_font, SIGNAL(activated(const QString &)),
            this, SLOT(slotFontChanged(const QString &)));

    connect(m_view, SIGNAL(activated(int)),
            this, SLOT(slotViewChanged(int)));

    connect(m_rows, SIGNAL(activated(const QString &)),
            this, SLOT(slotRowChanged(const QString &)));

    slotFontChanged(m_font->currentText());
}

}
#include "NoteFontViewer.moc"
