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

#ifndef LHPARAMDIALOG_H
#define LHPARAMDIALOG_H

#include "base/AudioPluginInstance.h"

#include <QDialog>

class QTimer;
class QLabel;
class QDialogButtonBox;
class QLineEdit;
class QComboBox;
class QPushButton;
class QIntValidator;
class QDoubleValidator;
class QGroupBox;

#include <map>

namespace Rosegarden
{

class AudioPluginGUIManager;

class AudioPluginParameterDialog : public QDialog
{
    Q_OBJECT
 public:
    AudioPluginParameterDialog(QWidget* parent,
                               InstrumentId instrument,
                               int position);

  public slots:
    void slotSetValue();
    void slotUpdate();
    void slotEditAccept();
    void slotEditReject();
    void slotSelectPath();

 private:

    void showEditor(const QString& paramId);
    void hideEditor();
    void clearEditor();

    InstrumentId m_instrument;
    int m_position;
    AudioPluginGUIManager *m_pluginGUIManager;
    QTimer* m_updateTimer;
    AudioPluginInstance::PluginParameters m_params;
    std::map<QString, QLabel*> m_valueLabels;
    AudioPluginInstance::ParameterType m_openEditorType;
    QString m_openEditorId;
    QGroupBox* m_editorBox;
    QWidget* m_editor;
    QLabel* m_editorHeader;
    QLineEdit* m_paramLineEdit;
    QIntValidator* m_intValidator;
    QDoubleValidator* m_doubleValidator;
    QComboBox* m_boolComboBox;
    QPushButton* m_pathButton;
    QDialogButtonBox* m_buttonBox;
};

}

#endif
