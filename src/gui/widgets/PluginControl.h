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

#ifndef RG_PLUGINCONTROL_H
#define RG_PLUGINCONTROL_H

#include <QSharedPointer>
#include <QWidget>


class QWidget;
class QGridLayout;


namespace Rosegarden
{

class Rotary;
class PluginPort;
class AudioPluginManager;
class Studio;

class PluginControl : public QWidget
{
    Q_OBJECT
public:

    // ??? An enum with only one value seems kinda useless.
    enum class ControlType
    {
        Rotary
        //Slider,
        //NumericSlider
    };

    PluginControl(QWidget *parent,
                  ControlType type,
                  QSharedPointer<PluginPort> port,
                  QSharedPointer<AudioPluginManager> pluginManager,
                  int index,
                  float initialValue,
                  bool showBounds);
 
    void setValue(float value, bool emitSignals = true);
    float getValue() const;

    int getIndex() const { return m_index; }

    void show();
    void hide();

public slots:
    void slotValueChanged(float value);

signals:
    void valueChanged(float value);

private:

    ControlType          m_type;
    QSharedPointer<PluginPort> m_port;

    Rotary *m_dial;
    QSharedPointer<AudioPluginManager> m_pluginManager;

    int                  m_index;

};


}

#endif
