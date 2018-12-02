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

#define RG_MODULE_STRING "[ConfigureDialogBase]"

#include "ConfigureDialogBase.h"

#include "gui/configuration/ConfigurationPage.h"
#include "gui/widgets/IconStackedWidget.h"
#include "misc/Debug.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QString>
#include <QWidget>
#include <QTabWidget>
#include <QPushButton>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QUrl>
#include <QDesktopServices>

namespace Rosegarden
{


ConfigureDialogBase::ConfigureDialogBase(QWidget *parent, const QString &label, const char *name  )
: QDialog(parent)
{
    this->setAttribute( Qt::WA_DeleteOnClose );

    this->setWindowTitle(label);
    this->setObjectName( (name) );

    QVBoxLayout *dlgLayout = new QVBoxLayout(this);
//    dlgLayout->setMargin(0);

    m_iconWidget = new IconStackedWidget(this);
    dlgLayout->addWidget(m_iconWidget);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Apply  |
                                                       QDialogButtonBox::Ok     |
                                                       QDialogButtonBox::Cancel |
                                                       QDialogButtonBox::Help);
    dlgLayout->addWidget(buttonBox);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ConfigureDialogBase::slotCancelOrClose);
    connect(buttonBox, &QDialogButtonBox::helpRequested, this, &ConfigureDialogBase::slotHelpRequested );

    m_applyButton = buttonBox->button(QDialogButtonBox::Apply);
    m_applyButton->setEnabled(false);
    connect(m_applyButton, &QAbstractButton::clicked, this, &ConfigureDialogBase::slotApply);

}

// ConfigureDialogBase::addPage(const QString& name, const QString& title, const QPixmap& icon, QWidget *page)
// where:
// name - The name written in the page select icon button
// title - The title placed at the top of the configuration page
// icon - The icon shown on the page select icon button
// page - pointer to a configuration page widget
void ConfigureDialogBase::addPage(const QString& name, const QString& title, const QPixmap& icon, QWidget *page)
{
    // New widget to hold the title, dividing line and configuration page
    QWidget * titledPage = new QWidget(this);
    QLayout * pageLayout = new QVBoxLayout(titledPage);
    pageLayout->setMargin(0);

    // Create the title label widget for the configration page
    QLabel * titleLabel = new QLabel(title, titledPage);
    QFont font;
    font.setBold(true);
    font.setPixelSize(12);
    titleLabel->setFont(font);
    
    // Create the dividing line
    QFrame * divideLine = new QFrame(titledPage);
    divideLine->setFrameStyle(QFrame::HLine | QFrame::Sunken);
    divideLine->setLineWidth(2);

    // Add these widgets to the layout
    pageLayout->addWidget(titleLabel);
    pageLayout->addWidget(divideLine);
    pageLayout->addWidget(page);
    
    // Add the page to the IconStackedWidgget (creating the select button)
    m_iconWidget->addPage(name, titledPage, icon);
}


ConfigureDialogBase::~ConfigureDialogBase()
{}


void
ConfigureDialogBase::slotApply()
{
    RG_DEBUG << "CONFIGUREDIALOGBASE SLOTAPPLY()";
    for (configurationpages::iterator i = m_configurationPages.begin();
            i != m_configurationPages.end(); ++i)
        (*i)->apply();

    m_applyButton->setEnabled(false);
}

void
ConfigureDialogBase::slotActivateApply()
{
    m_applyButton->setEnabled(true);
}

void
ConfigureDialogBase::deactivateApply()
{
    m_applyButton->setEnabled(false);
}

void
ConfigureDialogBase::accept()
{
    slotApply();
    QDialog::accept();
    close();
}

void
ConfigureDialogBase::slotCancelOrClose()
{
    close();
}

void
ConfigureDialogBase::slotHelpRequested()
{
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:configureDialogBase-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.

// Hrmmm...  What to do with this one?  Anything?  Obviously there's not going
// to be any specific help for a base class.
//    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:configureDialogBase-en");
//    QDesktopServices::openUrl(QUrl(helpURL));
}

void
ConfigureDialogBase::setPageByIndex(int index)
{
    m_iconWidget->setPageByIndex(index);
}


}
