/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2022 the Rosegarden development team.

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
#include <QSharedPointer>

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
class QDialog;

namespace Rosegarden
{


class RosegardenDocument;
class TimeSignature;
class RealTime;
class MappedEvent;


class TransportDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TransportDialog(QWidget *parent = nullptr);
    ~TransportDialog() override;

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

    // RosegardenTransport member accessors
    QPushButton* MetronomeButton()   { return ui->MetronomeButton; }
    QPushButton* SoloButton()        { return ui->SoloButton; }
    QPushButton* PlayButton()        { return ui->PlayButton; }
    QPushButton* StopButton()        { return ui->StopButton; }
    QPushButton* FfwdButton()        { return ui->FfwdButton; }
    QPushButton* RewindButton()      { return ui->RewindButton; }
    QPushButton* RecordButton()      { return ui->RecordButton; }
    QPushButton* RewindEndButton()   { return ui->RewindEndButton; }
    QPushButton* FfwdEndButton()     { return ui->FfwdEndButton; }
    QPushButton* TimeDisplayButton() { return ui->TimeDisplayButton; }
    QPushButton* ToEndButton()       { return ui->ToEndButton; }

    /// Save geometry to .conf.
    /**
     * Making these public in an attempt to fix a bug where the window will
     * crawl upward each time it is hidden and re-shown.  It's moving up by
     * exactly the height of the titlebar.  It's as if the child window
     * position is being used for the parent window.  Or perhaps an assumption
     * is being made that the geometry stored by hide() needs to be adjusted
     * by the titlebar height.
     *
     * At any rate, using these when hiding and showing almost fixes the
     * problem.  The last failing test case is when you hide the transport,
     * close rg, launch rg, and show the transport.  It will move up.
     * Examining the geometry at each point might provide a clue.
     */
    void saveGeo();
    /// Load geometry from .conf.
    void loadGeo();

protected:
    // QDialog override.
    void closeEvent(QCloseEvent * e) override;

    void computeSampleRate();
    void cycleThroughModes();
    void displayTime();

public slots:
    void slotDocumentLoaded(RosegardenDocument *doc);

    // These two slots are activated by QTimers
    //
    void slotClearMidiInLabel();
    void slotClearMidiOutLabel();

    // These just change the little labels that say what
    // mode we're in, nothing else
    //
    void slotChangeTimeDisplay();
    void slotChangeToEnd();

    void slotPanelOpenButtonClicked();
    void slotPanelCloseButtonClicked();

    void slotEditTempo();
    void slotEditTimeSignature();
    void slotEditTime();

    void setBackgroundColor(QColor color);
    void slotResetBackground();


    // Connected to SequenceManager
    void slotTempoChanged(tempoT);
    void slotMidiInLabel(const MappedEvent *mE); // show incoming MIDI events on the Transport
    void slotMidiOutLabel(const MappedEvent *mE); // show outgoing  MIDI events on the Transport
    void slotPlaying(bool checked);
    void slotRecording(bool checked);
    void slotMetronomeActivated(bool checked);

signals:
    void closed();

    void editTempo(QWidget *);
    void editTimeSignature(QWidget *);
    void editTransportTime(QWidget *);
    //void scrollTempo(int);
    void panic();

private slots:

    // Loop Widgets.
    void slotLoopButtonClicked();
    void slotSetStartLoopingPointAtMarkerPos();
    void slotSetStopLoopingPointAtMarkerPos();
    /// From RosegardenDocument.
    void slotLoopChanged();

private:
    void loadPixmaps();
    void resetFonts();
    void resetFont(QWidget *);
    void initModeMap();

    //--------------- Data members ---------------------------------

    QSharedPointer<Ui_RosegardenTransport> ui;

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

    bool    m_isExpanded;

    bool m_isBackgroundSet;

    int m_sampleRate;

    std::map<std::string, TimeDisplayMode> m_modeMap;
};





}

#endif
