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

#ifndef RG_CONFIGUREDIALOGBASE_H
#define RG_CONFIGUREDIALOGBASE_H

#include <QMessageBox>
#include <QDialog>
#include <QString>
#include <vector>


class QWidget;
class QTabWidget;
class QDialogButtonBox;
class IconStackedWidget;

namespace Rosegarden
{


class TabbedConfigurationPage;


class ConfigureDialogBase : public QDialog
{
    Q_OBJECT
public:
    ConfigureDialogBase(QWidget *parent = nullptr, const QString &label = {}, const char *name = nullptr);

    ~ConfigureDialogBase() override;

    typedef std::vector<TabbedConfigurationPage *> ConfigurationPages;

    void addPage( const QString& name, const QString& title, const QPixmap& icon, QWidget *page );

    void setPageByIndex(int index);

protected slots:
    void accept() override;
    virtual void slotApply();
    virtual void slotCancelOrClose();
    virtual void slotHelpRequested();

public slots:
    virtual void slotActivateApply();

public:
    void deactivateApply();

protected:

    ConfigurationPages m_configurationPages;
    
    QPushButton       *m_applyButton;
    QDialogButtonBox  *m_dialogButtonBox;
    IconStackedWidget *m_iconWidget;    
};


}

#endif
