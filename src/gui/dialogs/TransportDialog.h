/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_TRANSPORTDIALOG_H
#define RG_TRANSPORTDIALOG_H

#include <QHash>
#include <QColor>
#include <QPixmap>
#include <QtCore>
#include <QDialog>

#include "base/Composition.h" // for tempoT

#include "ui_RosegardenTransportUi.h"

/* NOTE: In a stylesheet world, we can no longer flash the LCD background through
 * the palette, so we have to use spot stylesheets instead.  I've changed this
 * from passing around QColor to a QString, and use named colors.  This is
 * simplistic, but this part of the transport has always been hard coded black
 * since time immemorial, and it's quicker to pass around "black," "red," and
 * "cyan" than to convert a QColor into a a string.
 *
 * dmm
 */

class QWidget;
class QTimer;
class QPushButton;
class QCloseEvent;
class QShortcut;
class QDialog;

namespace Rosegarden
{

class TimeSignature;
class RealTime;
class MappedEvent;


class TransportDialog : public QDialog
{
    Q_OBJECT
public:
    TransportDialog(QWidget *parent = 0);
    ~TransportDialog();

    enum TimeDisplayMode { RealMode, SMPTEMode, BarMode, BarMetronomeMode, FrameMode };

    std::string getCurrentModeAsString();
    TimeDisplayMode getCurrentMode() { return m_currentMode; }
    void setNewMode(const std::string& newModeAsString);
    void setNewMode(const TimeDisplayMode& newMode);
    bool isShowingTimeToEnd();
    bool isExpanded();

    void displayRealTime(const RealTime &rt);
    void displaySMPTETime(const RealTime &rt);
    void displayFrameTime(const RealTime &rt);
    void displayBarTime(int bar, int beat, int unit);

    void setTimeSignature(const TimeSignature &timeSig);

    void setSMPTEResolution(int framesPerSecond, int bitsPerFrame);
    void getSMPTEResolution(int &framesPerSecond, int &bitsPerFrame);

    // Return the shortcut object
    //
    QShortcut* getShortcuts() { return m_shortcuts; }

    // RosegardenTransport member accessors
    QPushButton* MetronomeButton()   { return ui->MetronomeButton; }
    QPushButton* SoloButton()        { return ui->SoloButton; }
    QPushButton* LoopButton()        { return ui->LoopButton; }
    QPushButton* PlayButton()        { return ui->PlayButton; }
    QPushButton* StopButton()        { return ui->StopButton; }
    QPushButton* FfwdButton()        { return ui->FfwdButton; }
    QPushButton* RewindButton()      { return ui->RewindButton; }
    QPushButton* RecordButton()      { return ui->RecordButton; }
    QPushButton* RewindEndButton()   { return ui->RewindEndButton; }
    QPushButton* FfwdEndButton()     { return ui->FfwdEndButton; }
    QPushButton* TimeDisplayButton() { return ui->TimeDisplayButton; }
    QPushButton* ToEndButton()       { return ui->ToEndButton; }

    virtual void show();
    virtual void hide();

protected:
    virtual void closeEvent(QCloseEvent * e);
    void computeSampleRate();
    void cycleThroughModes();
    void displayTime();

public slots:

    // These two slots are activated by QTimers
    //
    void slotClearMidiInLabel();
    void slotClearMidiOutLabel();

    // These just change the little labels that say what
    // mode we're in, nothing else
    //
    void slotChangeTimeDisplay();
    void slotChangeToEnd();

    void slotLoopButtonClicked();

    void slotPanelOpenButtonClicked();
    void slotPanelCloseButtonClicked();

    void slotEditTempo();
    void slotEditTimeSignature();
    void slotEditTime();

    void setBackgroundColor(QColor color);
    void slotResetBackground();

    void slotSetStartLoopingPointAtMarkerPos();
    void slotSetStopLoopingPointAtMarkerPos();

    // Connected to SequenceManager
    void slotTempoChanged(tempoT);
    void slotMidiInLabel(const MappedEvent *event); // show incoming MIDI events on the Transport
    void slotMidiOutLabel(const MappedEvent *event); // show outgoing  MIDI events on the Transport
    void slotPlaying(bool checked);
    void slotRecording(bool checked);
    void slotMetronomeActivated(bool checked);

signals:
    void closed();

    // Set and unset the loop at the RosegardenMainWindow
    //
    void setLoop();
    void unsetLoop();
    void setLoopStartTime();
    void setLoopStopTime();

    void editTempo(QWidget *);
    void editTimeSignature(QWidget *);
    void editTransportTime(QWidget *);
    void scrollTempo(int);
    void panic();

private:
    void loadPixmaps();
    void resetFonts();
    void resetFont(QWidget *);
    void initModeMap();

    //--------------- Data members ---------------------------------

    Ui_RosegardenTransport* ui;

    QHash<int, QPixmap> m_lcdList;
    QHash<int, QPixmap> m_lcdListDefault;
    QPixmap m_lcdNegative;

    int m_lastTenHours;
    int m_lastUnitHours;
    int m_lastTenMinutes;
    int m_lastUnitMinutes;
    int m_lastTenSeconds;
    int m_lastUnitSeconds;
    int m_lastTenths;
    int m_lastHundreths;
    int m_lastThousandths;
    int m_lastTenThousandths;

    bool m_lastNegative;
    TimeDisplayMode m_lastMode;
    TimeDisplayMode m_currentMode;

    int m_tenHours;
    int m_unitHours;
    int m_tenMinutes;
    int m_unitMinutes;
    int m_tenSeconds;
    int m_unitSeconds;
    int m_tenths;
    int m_hundreths;
    int m_thousandths;
    int m_tenThousandths;

    int m_numerator;
    int m_denominator;

    int m_framesPerSecond;
    int m_bitsPerFrame;

    QTimer *m_midiInTimer;
    QTimer *m_midiOutTimer;
    QTimer *m_clearMetronomeTimer;

    bool m_enableMIDILabels;

    QPixmap m_panelOpen;
    QPixmap m_panelClosed;

    void updateTimeDisplay();

    QShortcut *m_shortcuts;
    bool    m_isExpanded;

    bool m_isBackgroundSet;

    int m_sampleRate;

    std::map<std::string, TimeDisplayMode> m_modeMap;
};

 



}

#endif
