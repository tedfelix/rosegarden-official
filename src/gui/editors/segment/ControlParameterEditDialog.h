
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

#ifndef _RG_CONTROLPARAMETEREDITDIALOG_H_
#define _RG_CONTROLPARAMETEREDITDIALOG_H_

#include "base/ControlParameter.h"
#include <QDialog>
#include <QDialogButtonBox>


class QWidget;
class QString;
class QSpinBox;
class QLineEdit;
class QLabel;
class QComboBox;


namespace Rosegarden
{

class RosegardenGUIDoc;


class ControlParameterEditDialog : public QDialog
{
    Q_OBJECT
public:
    ControlParameterEditDialog(QDialogButtonBox::QWidget *parent,
                               ControlParameter *control,
                               RosegardenGUIDoc *doc);

    ControlParameter& getControl() { return m_dialogControl; }

public slots: 

    void slotNameChanged(const QString &);
    void slotTypeChanged(int);
    void slotDescriptionChanged(const QString &);
    void slotControllerChanged(int);
    void slotMinChanged(int);
    void slotMaxChanged(int);
    void slotDefaultChanged(int);
    void slotColourChanged(int);
    void slotIPBPositionChanged(int);

protected:
    void populate(); // populate the dialog

    RosegardenGUIDoc             *m_doc;
    ControlParameter *m_control;
    ControlParameter  m_dialogControl;

    QLineEdit                    *m_nameEdit;
    QComboBox                    *m_typeCombo;
    QLineEdit                    *m_description;
    QSpinBox                     *m_controllerBox;
    QSpinBox                     *m_minBox;
    QSpinBox                     *m_maxBox;
    QSpinBox                     *m_defaultBox;
    QComboBox                    *m_colourCombo;
    QComboBox                    *m_ipbPosition;
    QLabel                       *m_hexValue;
};



}

#endif
