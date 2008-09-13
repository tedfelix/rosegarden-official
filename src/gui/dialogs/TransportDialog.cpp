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


#include <QDesktopWidget>
#include "TransportDialog.h"

#include <klocale.h>
#include <kstandarddirs.h>
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/RealTime.h"
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "sequencer/RosegardenSequencer.h"
#include "gui/application/TransportStatus.h"
#include "gui/application/RosegardenApplication.h"
#include "gui/general/MidiPitchLabel.h"
#include "gui/studio/StudioControl.h"
#include "gui/widgets/Label.h"
#include "sound/MappedEvent.h"
#include "document/ConfigGroups.h"
#include <QSettings>
#include <kglobal.h>
#include <qshortcut.h>
#include <QColor>
#include <QByteArray>
#include <QDataStream>
#include <QFont>
#include <QLabel>
#include <QPalette>
#include <QPixmap>
#include <QPushButton>
#include <QString>
#include <QTimer>
#include <QWidget>
#include <QHBoxLayout>


namespace Rosegarden
{

TransportDialog::TransportDialog(QWidget *parent,
                                 const char *name,
                                 WFlags flags):
    QWidget(parent, name, WType_TopLevel | WStyle_DialogBorder | WStyle_Minimize | WStyle_SysMenu | WDestructiveClose),
    m_transport(0),
    m_lastTenHours(0),
    m_lastUnitHours(0),
    m_lastTenMinutes(0),
    m_lastUnitMinutes(0),
    m_lastTenSeconds(0),
    m_lastUnitSeconds(0),
    m_lastTenths(0),
    m_lastHundreths(0),
    m_lastThousandths(0),
    m_lastTenThousandths(0),
    m_lastNegative(false),
    m_lastMode(RealMode),
    m_currentMode(RealMode),
    m_tempo(0),
    m_numerator(0),
    m_denominator(0),
    m_framesPerSecond(24),
    m_bitsPerFrame(80),
    m_isExpanded(true),
    m_haveOriginalBackground(false),
    m_isBackgroundSet(false),
    m_sampleRate(0)
{
    m_transport = new RosegardenTransport(this);

    setCaption(i18n("Rosegarden Transport"));

    resetFonts();

    initModeMap();

    // set the LCD frame background to black
    //
    m_transport->LCDBoxFrame->setBackgroundColor(QColor(Qt::black));

    // set all the pixmap backgrounds to black to avoid
    // flickering when we update
    //
    m_transport->TenThousandthsPixmap->setBackgroundColor(QColor(Qt::black));
    m_transport->ThousandthsPixmap->setBackgroundColor(QColor(Qt::black));
    m_transport->HundredthsPixmap->setBackgroundColor(QColor(Qt::black));
    m_transport->TenthsPixmap->setBackgroundColor(QColor(Qt::black));
    m_transport->UnitSecondsPixmap->setBackgroundColor(QColor(Qt::black));
    m_transport->TenSecondsPixmap->setBackgroundColor(QColor(Qt::black));
    m_transport->UnitMinutesPixmap->setBackgroundColor(QColor(Qt::black));
    m_transport->TenMinutesPixmap->setBackgroundColor(QColor(Qt::black));
    m_transport->UnitHoursPixmap->setBackgroundColor(QColor(Qt::black));
    m_transport->TenHoursPixmap->setBackgroundColor(QColor(Qt::black));
    m_transport->NegativePixmap->setBackgroundColor(QColor(Qt::black));

    // unset the negative sign to begin with
    m_transport->NegativePixmap->clear();

    // Set our toggle buttons
    //
    m_transport->PlayButton->setToggleButton(true);
    m_transport->RecordButton->setToggleButton(true);

// Disable the loop button if JACK transport enabled, because this
// causes a nasty race condition, and it just seems our loops are not JACK compatible
// #1240039 - DMM
//    QSettings settings ; // was: rgapp->config()
//    settings.beginGroup(SequencerOptionsConfigGroup);
//    if ( qStrToBool( settings.value("jacktransport", "false" ) ) )
//    {
//        m_transport->LoopButton->setEnabled(false);
//    }
//      settings.endGroup();

    // fix and hold the size of the dialog
    //
    setMinimumSize(m_transport->width(), m_transport->height());
    setMaximumSize(m_transport->width(), m_transport->height());

    loadPixmaps();

    // Create Midi label timers
    m_midiInTimer = new QTimer(this);
    m_midiOutTimer = new QTimer(this);
    m_clearMetronomeTimer = new QTimer(this);

    connect(m_midiInTimer, SIGNAL(timeout()),
            SLOT(slotClearMidiInLabel()));

    connect(m_midiOutTimer, SIGNAL(timeout()),
            SLOT(slotClearMidiOutLabel()));

    connect(m_clearMetronomeTimer, SIGNAL(timeout()),
            SLOT(slotResetBackground()));

    m_transport->TimeDisplayLabel->hide();
    m_transport->ToEndLabel->hide();

    connect(m_transport->TimeDisplayButton, SIGNAL(clicked()),
            SLOT(slotChangeTimeDisplay()));

    connect(m_transport->ToEndButton, SIGNAL(clicked()),
            SLOT(slotChangeToEnd()));

    connect(m_transport->LoopButton, SIGNAL(clicked()),
            SLOT(slotLoopButtonClicked()));

    connect(m_transport->PanelOpenButton, SIGNAL(clicked()),
            SLOT(slotPanelOpenButtonClicked()));

    connect(m_transport->PanelCloseButton, SIGNAL(clicked()),
            SLOT(slotPanelCloseButtonClicked()));

    connect(m_transport->PanicButton, SIGNAL(clicked()), SIGNAL(panic()));

    m_panelOpen = *m_transport->PanelOpenButton->pixmap();
    m_panelClosed = *m_transport->PanelCloseButton->pixmap();


    connect(m_transport->SetStartLPButton, SIGNAL(clicked()), SLOT(slotSetStartLoopingPointAtMarkerPos()));
    connect(m_transport->SetStopLPButton, SIGNAL(clicked()), SLOT(slotSetStopLoopingPointAtMarkerPos()));

    // clear labels
    //
    slotClearMidiInLabel();
    slotClearMidiOutLabel();

    // and by default we close the lower panel
    //
    int rfh = m_transport->RecordingFrame->height();
    m_transport->RecordingFrame->hide();
    setFixedSize(width(), height() - rfh);
    m_transport->PanelOpenButton->setPixmap(m_panelClosed);

    // and since by default we show real time (not SMPTE), by default
    // we hide the small colon pixmaps
    //
    m_transport->SecondColonPixmap->hide();
    m_transport->HundredthColonPixmap->hide();

    // We have to specify these settings in this class (copied
    // from rosegardentransport.cpp) as we're using a specialised
    // widgets for TempoDisplay.  Ugly but works - does mean that
    // if the rest of the Transport ever changes then this code
    // will have to as well.
    //
    QPalette pal;
    pal.setColor(QColorGroup::Foreground, QColor(192, 216, 255));

    m_transport->TempoDisplay->setPalette(pal);
    m_transport->TempoDisplay->setAlignment(int(QLabel::AlignVCenter | QLabel::AlignRight));

    m_transport->TimeSigDisplay->setPalette(pal);
    m_transport->TimeSigDisplay->setAlignment(int(QLabel::AlignVCenter | QLabel::AlignRight));

    QFont localFont(m_transport->OutDisplay->font() );
    localFont.setFamily( "lucida" );
    localFont.setBold( TRUE );

    m_transport->TempoDisplay->setFont( localFont );
    m_transport->TimeSigDisplay->setFont( localFont );

    connect(m_transport->TempoDisplay, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTempo()));

    connect(m_transport->TempoDisplay, SIGNAL(scrollWheel(int)),
            this, SIGNAL(scrollTempo(int)));

    connect(m_transport->TimeSigDisplay, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTimeSignature()));

    // toil through the individual pixmaps
    connect(m_transport->NegativePixmap, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTime()));
    connect(m_transport->TenHoursPixmap, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTime()));
    connect(m_transport->UnitHoursPixmap, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTime()));
    connect(m_transport->HourColonPixmap, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTime()));
    connect(m_transport->TenMinutesPixmap, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTime()));
    connect(m_transport->UnitMinutesPixmap, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTime()));
    connect(m_transport->MinuteColonPixmap, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTime()));
    connect(m_transport->TenSecondsPixmap, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTime()));
    connect(m_transport->UnitSecondsPixmap, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTime()));
    connect(m_transport->SecondColonPixmap, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTime()));
    connect(m_transport->TenthsPixmap, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTime()));
    connect(m_transport->HundredthsPixmap, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTime()));
    connect(m_transport->HundredthColonPixmap, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTime()));
    connect(m_transport->TenThousandthsPixmap, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTime()));
    connect(m_transport->ThousandthsPixmap, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTime()));

    // shortcuterator object
    //
    m_shortcuterators = new QShortcut(this);
}

TransportDialog::~TransportDialog()
{
    if (isVisible()) {
        QSettings settings;
        settings.beginGroup( GeneralOptionsConfigGroup );

        settings.setValue("transportx", x());
        settings.setValue("transporty", y());

        settings.endGroup();
    }
}

std::string
TransportDialog::getCurrentModeAsString()
{
    bool found = false;
    for (std::map<std::string, TimeDisplayMode>::iterator iter = m_modeMap.begin();
         iter != m_modeMap.end() && !found;
         iter++)
    {
        if (iter->second == m_currentMode) {
            return iter->first;
        }
    }

    // we shouldn't get here unless the map is not well-configured
    RG_DEBUG << "TransportDialog::getCurrentModeAsString: could not map current mode " 
        << m_currentMode << " to string." << endl;
    throw Exception("could not map current mode to string.");
}

void
TransportDialog::initModeMap()
{
    m_modeMap["RealMode"]         = RealMode;
    m_modeMap["SMPTEMode"]        = SMPTEMode;
    m_modeMap["BarMode"]          = BarMode;
    m_modeMap["BarMetronomeMode"] = BarMetronomeMode;
    m_modeMap["FrameMode"]        = FrameMode;
}

void
TransportDialog::show()
{
    QSettings settings;
    settings.beginGroup( GeneralOptionsConfigGroup );

    int x = settings.value("transportx", -1).toInt() ;
    int y = settings.value("transporty", -1).toInt() ;
    if (x >= 0 && y >= 0) {
        int dw = QApplication::desktop()->availableGeometry(QPoint(x, y)).width();
        int dh = QApplication::desktop()->availableGeometry(QPoint(x, y)).height();
        if (x + m_transport->width() > dw) x = dw - m_transport->width();
        if (y + m_transport->height() > dh) y = dh - m_transport->height();
        move(x, y);
//        std::cerr << "TransportDialog::show(): moved to " << x << "," << y << std::endl;
        QWidget::show();
//        std::cerr << "TransportDialog::show(): now at " << this->x() << "," << this->y() << std::endl;
    } else {
        QWidget::show();
    }

    settings.endGroup();
}

void
TransportDialog::hide()
{
    if (isVisible()) {
        QSettings settings;
        settings.beginGroup( GeneralOptionsConfigGroup );
        settings.setValue("transportx", x());
        settings.setValue("transporty", y());

        settings.endGroup();
    }
    QWidget::hide();
}

void
TransportDialog::loadPixmaps()
{
    m_lcdList.clear();
    QString fileName;
    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");

    for (int i = 0; i < 10; i++) {
        fileName = QString("%1/transport/led-%2.xpm").arg(pixmapDir).arg(i);
        if (!m_lcdList[i].load(fileName)) {
            std::cerr << "TransportDialog - failed to load pixmap \""
                      << fileName << "\"" << std::endl;
        }
    }

    // Load the "negative" sign pixmap
    //
    fileName = QString("%1/transport/led--.xpm").arg(pixmapDir);
    m_lcdNegative.load(fileName);

}

void
TransportDialog::resetFonts()
{
    resetFont(m_transport->TimeSigLabel);
    resetFont(m_transport->TimeSigDisplay);
    resetFont(m_transport->TempoLabel);
    resetFont(m_transport->TempoDisplay);
    resetFont(m_transport->DivisionLabel);
    resetFont(m_transport->DivisionDisplay);
    resetFont(m_transport->InLabel);
    resetFont(m_transport->InDisplay);
    resetFont(m_transport->OutLabel);
    resetFont(m_transport->OutDisplay);
    resetFont(m_transport->ToEndLabel);
    resetFont(m_transport->TimeDisplayLabel);
}

void
TransportDialog::resetFont(QWidget *w)
{
    QFont font = w->font();
    font.setPixelSize(10);
    w->setFont(font);
}

void
TransportDialog::setSMPTEResolution(int framesPerSecond,
                                    int bitsPerFrame)
{
    m_framesPerSecond = framesPerSecond;
    m_bitsPerFrame = bitsPerFrame;
}

void
TransportDialog::getSMPTEResolution(int &framesPerSecond,
                                    int &bitsPerFrame)
{
    framesPerSecond = m_framesPerSecond;
    bitsPerFrame = m_bitsPerFrame;
}

void
TransportDialog::computeSampleRate()
{
    if (m_sampleRate == 0) {
        m_sampleRate = RosegardenSequencer::getInstance()->getSampleRate();
    }
}

void
TransportDialog::cycleThroughModes()
{
    switch (m_currentMode) {

    case RealMode:
        if (m_sampleRate > 0)
            m_currentMode = FrameMode;
        else
            m_currentMode = BarMode;
        break;

    case FrameMode:
        m_currentMode = BarMode;
        break;

    case SMPTEMode:
        m_currentMode = BarMode;
        break;

    case BarMode:
        m_currentMode = BarMetronomeMode;
        break;

    case BarMetronomeMode:
        m_currentMode = RealMode;
        break;
    }
}

void
TransportDialog::displayTime()
{
    switch (m_currentMode) {
    case RealMode:
        m_clearMetronomeTimer->stop();
        m_transport->TimeDisplayLabel->hide();
        break;

    case SMPTEMode:
        m_clearMetronomeTimer->stop();
        m_transport->TimeDisplayLabel->setText("SMPTE"); // DO NOT i18n
        m_transport->TimeDisplayLabel->show();
        break;

    case BarMode:
        m_clearMetronomeTimer->stop();
        m_transport->TimeDisplayLabel->setText("BAR"); // DO NOT i18n
        m_transport->TimeDisplayLabel->show();
        break;

    case BarMetronomeMode:
        m_clearMetronomeTimer->start(1700, FALSE);
        m_transport->TimeDisplayLabel->setText("MET"); // DO NOT i18n
        m_transport->TimeDisplayLabel->show();
        break;

    case FrameMode:
        m_clearMetronomeTimer->stop();
        m_transport->TimeDisplayLabel->setText(QString("%1").arg(m_sampleRate));
        m_transport->TimeDisplayLabel->show();
        break;
    }
}

void
TransportDialog::setNewMode(const std::string& newModeAsString)
{
    TimeDisplayMode newMode = RealMode; // default value if not found
    
    std::map<std::string, TimeDisplayMode>::iterator iter =
        m_modeMap.find(newModeAsString);

    if (iter != m_modeMap.end()) {
        // value found
        newMode = iter->second;
    } else {
        // don't fail: use default value set at declaration
    }

    setNewMode(newMode);
}

void
TransportDialog::setNewMode(const TimeDisplayMode& newMode)
{
    computeSampleRate();
    
    m_currentMode = newMode;
    
    displayTime();
}


void
TransportDialog::slotChangeTimeDisplay()
{
    computeSampleRate();
    
    cycleThroughModes();
    
    displayTime();
}

void
TransportDialog::slotChangeToEnd()
{
    if (m_transport->ToEndButton->isOn()) {
        m_transport->ToEndLabel->show();
    } else {
        m_transport->ToEndLabel->hide();
    }
}

bool
TransportDialog::isShowingTimeToEnd()
{
    return m_transport->ToEndButton->isOn();
}

void
TransportDialog::displayRealTime(const RealTime &rt)
{
    RealTime st = rt;

    slotResetBackground();

    if (m_lastMode != RealMode) {
        m_transport->HourColonPixmap->show();
        m_transport->MinuteColonPixmap->show();
        m_transport->SecondColonPixmap->hide();
        m_transport->HundredthColonPixmap->hide();
        m_lastMode = RealMode;
    }

    // If time is negative then reverse the time and set the minus flag
    //
    if (st < RealTime::zeroTime) {
        st = RealTime::zeroTime - st;
        if (!m_lastNegative) {
            m_transport->NegativePixmap->setPixmap(m_lcdNegative);
            m_lastNegative = true;
        }
    } else // don't show the flag
    {
        if (m_lastNegative) {
            m_transport->NegativePixmap->clear();
            m_lastNegative = false;
        }
    }

    m_tenThousandths = ( st.usec() / 100 ) % 10;
    m_thousandths = ( st.usec() / 1000 ) % 10;
    m_hundreths = ( st.usec() / 10000 ) % 10;
    m_tenths = ( st.usec() / 100000 ) % 10;

    m_unitSeconds = ( st.sec ) % 10;
    m_tenSeconds = ( st.sec / 10 ) % 6;

    m_unitMinutes = ( st.sec / 60 ) % 10;
    m_tenMinutes = ( st.sec / 600 ) % 6;

    m_unitHours = ( st.sec / 3600 ) % 10;
    m_tenHours = (st.sec / 36000 ) % 10;

    updateTimeDisplay();
}

void
TransportDialog::displayFrameTime(const RealTime &rt)
{
    RealTime st = rt;

    slotResetBackground();

    if (m_lastMode != FrameMode) {
        m_transport->HourColonPixmap->hide();
        m_transport->MinuteColonPixmap->hide();
        m_transport->SecondColonPixmap->hide();
        m_transport->HundredthColonPixmap->hide();
        m_lastMode = FrameMode;
    }

    // If time is negative then reverse the time and set the minus flag
    //
    if (st < RealTime::zeroTime) {
        st = RealTime::zeroTime - st;
        if (!m_lastNegative) {
            m_transport->NegativePixmap->setPixmap(m_lcdNegative);
            m_lastNegative = true;
        }
    } else // don't show the flag
    {
        if (m_lastNegative) {
            m_transport->NegativePixmap->clear();
            m_lastNegative = false;
        }
    }

    long frame = RealTime::realTime2Frame(st, m_sampleRate);

    m_tenThousandths = frame % 10;
    frame /= 10;
    m_thousandths = frame % 10;
    frame /= 10;
    m_hundreths = frame % 10;
    frame /= 10;
    m_tenths = frame % 10;
    frame /= 10;
    m_unitSeconds = frame % 10;
    frame /= 10;
    m_tenSeconds = frame % 10;
    frame /= 10;
    m_unitMinutes = frame % 10;
    frame /= 10;
    m_tenMinutes = frame % 10;
    frame /= 10;
    m_unitHours = frame % 10;
    frame /= 10;
    m_tenHours = frame % 10;
    frame /= 10;

    updateTimeDisplay();
}

void
TransportDialog::displaySMPTETime(const RealTime &rt)
{
    RealTime st = rt;

    slotResetBackground();

    if (m_lastMode != SMPTEMode) {
        m_transport->HourColonPixmap->show();
        m_transport->MinuteColonPixmap->show();
        m_transport->SecondColonPixmap->show();
        m_transport->HundredthColonPixmap->show();
        m_lastMode = SMPTEMode;
    }

    // If time is negative then reverse the time and set the minus flag
    //
    if (st < RealTime::zeroTime) {
        st = RealTime::zeroTime - st;
        if (!m_lastNegative) {
            m_transport->NegativePixmap->setPixmap(m_lcdNegative);
            m_lastNegative = true;
        }
    } else // don't show the flag
    {
        if (m_lastNegative) {
            m_transport->NegativePixmap->clear();
            m_lastNegative = false;
        }
    }

    m_tenThousandths =
        (( st.usec() * m_framesPerSecond * m_bitsPerFrame) / 1000000 ) % 10;
    m_thousandths =
        (( st.usec() * m_framesPerSecond * m_bitsPerFrame) / 10000000 ) %
        (m_bitsPerFrame / 10);
    m_hundreths =
        (( st.usec() * m_framesPerSecond) / 1000000 ) % 10;
    m_tenths =
        (( st.usec() * m_framesPerSecond) / 10000000 ) % 10;

    m_unitSeconds = ( st.sec ) % 10;
    m_tenSeconds = ( st.sec / 10 ) % 6;

    m_unitMinutes = ( st.sec / 60 ) % 10;
    m_tenMinutes = ( st.sec / 600 ) % 6;

    m_unitHours = ( st.sec / 3600 ) % 10;
    m_tenHours = ( st.sec / 36000 ) % 10;

    updateTimeDisplay();
}

void
TransportDialog::displayBarTime(int bar, int beat, int unit)
{
    if (m_lastMode != BarMode) {
        m_transport->HourColonPixmap->hide();
        m_transport->MinuteColonPixmap->show();
        m_transport->SecondColonPixmap->hide();
        m_transport->HundredthColonPixmap->hide();
        m_lastMode = BarMode;
    }

    // If time is negative then reverse the time and set the minus flag
    //
    if (bar < 0) {
        bar = -bar;
        if (!m_lastNegative) {
            m_transport->NegativePixmap->setPixmap(m_lcdNegative);
            m_lastNegative = true;
        }
    } else // don't show the flag
    {
        if (m_lastNegative) {
            m_transport->NegativePixmap->clear();
            m_lastNegative = false;
        }
    }

    if (m_currentMode == BarMetronomeMode && unit < 2) {
        if (beat == 1) {
            slotSetBackground(QColor(Qt::red));
        } else {
            slotSetBackground(QColor(Qt::cyan));
        }
    } else {
        slotResetBackground();
    }

    m_tenThousandths = ( unit ) % 10;
    m_thousandths = ( unit / 10 ) % 10;
    m_hundreths = ( unit / 100 ) % 10;
    m_tenths = ( unit / 1000 ) % 10;

    if (m_tenths == 0) {
        m_tenths = -1;
        if (m_hundreths == 0) {
            m_hundreths = -1;
            if (m_thousandths == 0) {
                m_thousandths = -1;
            }
        }
    }

    m_unitSeconds = ( beat ) % 10;
    m_tenSeconds = ( beat / 10 ) % 6;

    if (m_tenSeconds == 0) {
        m_tenSeconds = -1;
    }

    m_unitMinutes = ( bar ) % 10;
    m_tenMinutes = ( bar / 10 ) % 10;

    m_unitHours = ( bar / 100 ) % 10;
    m_tenHours = ( bar / 1000 ) % 10;

    if (m_tenHours == 0) {
        m_tenHours = -1;
        if (m_unitHours == 0) {
            m_unitHours = -1;
            if (m_tenMinutes == 0) {
                m_tenMinutes = -1;
            }
        }
    }

    updateTimeDisplay();
}

void
TransportDialog::updateTimeDisplay()
{
    if (m_tenThousandths != m_lastTenThousandths) {
        if (m_tenThousandths < 0)
            m_transport->TenThousandthsPixmap->clear();
        else
            m_transport->TenThousandthsPixmap->setPixmap(m_lcdList[m_tenThousandths]);
        m_lastTenThousandths = m_tenThousandths;
    }

    if (m_thousandths != m_lastThousandths) {
        if (m_thousandths < 0)
            m_transport->ThousandthsPixmap->clear();
        else
            m_transport->ThousandthsPixmap->setPixmap(m_lcdList[m_thousandths]);
        m_lastThousandths = m_thousandths;
    }

    if (m_hundreths != m_lastHundreths) {
        if (m_hundreths < 0)
            m_transport->HundredthsPixmap->clear();
        else
            m_transport->HundredthsPixmap->setPixmap(m_lcdList[m_hundreths]);
        m_lastHundreths = m_hundreths;
    }

    if (m_tenths != m_lastTenths) {
        if (m_tenths < 0)
            m_transport->TenthsPixmap->clear();
        else
            m_transport->TenthsPixmap->setPixmap(m_lcdList[m_tenths]);
        m_lastTenths = m_tenths;
    }

    if (m_unitSeconds != m_lastUnitSeconds) {
        if (m_unitSeconds < 0)
            m_transport->UnitSecondsPixmap->clear();
        else
            m_transport->UnitSecondsPixmap->setPixmap(m_lcdList[m_unitSeconds]);
        m_lastUnitSeconds = m_unitSeconds;
    }

    if (m_tenSeconds != m_lastTenSeconds) {
        if (m_tenSeconds < 0)
            m_transport->TenSecondsPixmap->clear();
        else
            m_transport->TenSecondsPixmap->setPixmap(m_lcdList[m_tenSeconds]);
        m_lastTenSeconds = m_tenSeconds;
    }

    if (m_unitMinutes != m_lastUnitMinutes) {
        if (m_unitMinutes < 0)
            m_transport->UnitMinutesPixmap->clear();
        else
            m_transport->UnitMinutesPixmap->setPixmap(m_lcdList[m_unitMinutes]);
        m_lastUnitMinutes = m_unitMinutes;
    }

    if (m_tenMinutes != m_lastTenMinutes) {
        if (m_tenMinutes < 0)
            m_transport->TenMinutesPixmap->clear();
        else
            m_transport->TenMinutesPixmap->setPixmap(m_lcdList[m_tenMinutes]);
        m_lastTenMinutes = m_tenMinutes;
    }

    if (m_unitHours != m_lastUnitHours) {
        if (m_unitHours < 0)
            m_transport->UnitHoursPixmap->clear();
        else
            m_transport->UnitHoursPixmap->setPixmap(m_lcdList[m_unitHours]);
        m_lastUnitHours = m_unitHours;
    }

    if (m_tenHours != m_lastTenHours) {
        if (m_tenHours < 0)
            m_transport->TenHoursPixmap->clear();
        else
            m_transport->TenHoursPixmap->setPixmap(m_lcdList[m_tenHours]);
        m_lastTenHours = m_tenHours;
    }
}

void
TransportDialog::setTempo(const tempoT &tempo)
{
    if (m_tempo == tempo)
        return ;
    m_tempo = tempo;

    // Send the quarter note length to the sequencer - shouldn't
    // really hang this off here but at least it's a single point
    // where the tempo should always be consistent.  Quarter Note
    // Length is sent (MIDI CLOCK) at 24ppqn.
    //
    double qnD = 60.0 / Composition::getTempoQpm(tempo);
    RealTime qnTime =
        RealTime(long(qnD),
                 long((qnD - double(long(qnD))) * 1000000000.0));

    StudioControl::sendQuarterNoteLength(qnTime);

    QString tempoString;
    tempoString.sprintf("%4.3f", Composition::getTempoQpm(tempo));

    m_transport->TempoDisplay->setText(tempoString);
}

void
TransportDialog::setTimeSignature(const TimeSignature &timeSig)
{
    int numerator = timeSig.getNumerator();
    int denominator = timeSig.getDenominator();
    if (m_numerator == numerator && m_denominator == denominator)
        return ;
    m_numerator = numerator;
    m_denominator = denominator;

    QString timeSigString;
    timeSigString.sprintf("%d/%d", numerator, denominator);
    m_transport->TimeSigDisplay->setText(timeSigString);
}

void
TransportDialog::setMidiInLabel(const MappedEvent *mE)
{
    assert(mE > 0);

    switch (mE->getType()) {
    case MappedEvent::MidiNote:
    case MappedEvent::MidiNoteOneShot:
    {
        // don't do anything if we've got an effective NOTE OFF
        //
        if (mE->getVelocity() == 0)
            return ;

        MidiPitchLabel mPL(mE->getPitch());
        m_transport->InDisplay->setText
            (mPL.getQString() +
             QString("  %1").arg(mE->getVelocity()));
    }
    break;

    case MappedEvent::MidiPitchBend:
        m_transport->InDisplay->setText(i18n("PITCH WHEEL"));
        break;

    case MappedEvent::MidiController:
        m_transport->InDisplay->setText(i18n("CONTROLLER"));
        break;

    case MappedEvent::MidiProgramChange:
        m_transport->InDisplay->setText(i18n("PROG CHNGE"));
        break;

    case MappedEvent::MidiKeyPressure:
    case MappedEvent::MidiChannelPressure:
        m_transport->InDisplay->setText(i18n("PRESSURE"));
        break;

    case MappedEvent::MidiSystemMessage:
        m_transport->InDisplay->setText(i18n("SYS MESSAGE"));
        break;

    default:   // do nothing
        return ;
    }

    // Reset the timer if it's already running
    //
    if (m_midiInTimer->isActive())
        m_midiInTimer->stop();

    // 1.5 second timeout for MIDI event
    //
    m_midiInTimer->start(1500, true);
}

void
TransportDialog::slotClearMidiInLabel()
{
    m_transport->InDisplay->setText(i18n(QString("NO EVENTS")));

    // also, just to be sure:
    slotResetBackground();
}

void
TransportDialog::setMidiOutLabel(const MappedEvent *mE)
{
    assert(mE > 0);

    switch (mE->getType()) {
    case MappedEvent::MidiNote:
    case MappedEvent::MidiNoteOneShot:
    {
        MidiPitchLabel mPL(mE->getPitch());
        m_transport->OutDisplay->setText
            (mPL.getQString() +
             QString("  %1").arg(mE->getVelocity()));
    }
    break;

    case MappedEvent::MidiPitchBend:
        m_transport->OutDisplay->setText(i18n("PITCH WHEEL"));
        break;

    case MappedEvent::MidiController:
        m_transport->OutDisplay->setText(i18n("CONTROLLER"));
        break;

    case MappedEvent::MidiProgramChange:
        m_transport->OutDisplay->setText(i18n("PROG CHNGE"));
        break;

    case MappedEvent::MidiKeyPressure:
    case MappedEvent::MidiChannelPressure:
        m_transport->OutDisplay->setText(i18n("PRESSURE"));
        break;

    case MappedEvent::MidiSystemMessage:
        m_transport->OutDisplay->setText(i18n("SYS MESSAGE"));
        break;

    default:   // do nothing
        return ;
    }

    // Reset the timer if it's already running
    //
    if (m_midiOutTimer->isActive())
        m_midiOutTimer->stop();

    // 200 millisecond timeout
    //
    m_midiOutTimer->start(200, true);
}

void
TransportDialog::slotClearMidiOutLabel()
{
    m_transport->OutDisplay->setText(i18n(QString("NO EVENTS")));
}

void
TransportDialog::closeEvent (QCloseEvent * /*e*/)
{
    //e->accept();  // accept the close event here
    emit closed();
}

void
TransportDialog::slotLoopButtonClicked()
{
    // disable if JACK transport has been set #1240039 - DMM
    //    QSettings settings;
    //    settings.beginGroup( SequencerOptionsConfigGroup );
    // 
    //    if ( qStrToBool( settings.value("jacktransport", "false" ) ) )
    //    {
    //    //!!! - this will fail silently
    //    m_transport->LoopButton->setEnabled(false);
    //    m_transport->LoopButton->setOn(false);
    //        return;
    //    }
    //    settings.endGroup();

    if (m_transport->LoopButton->isOn()) {
        emit setLoop();
    } else {
        emit unsetLoop();
    }
}

void
TransportDialog::slotSetStartLoopingPointAtMarkerPos()
{
    emit setLoopStartTime();
}

void
TransportDialog::slotSetStopLoopingPointAtMarkerPos()
{
    emit setLoopStopTime();
}

void
TransportDialog::slotPanelOpenButtonClicked()
{
    int rfh = m_transport->RecordingFrame->height();

    if (m_transport->RecordingFrame->isVisible()) {
        m_transport->RecordingFrame->hide();
        setFixedSize(width(), height() - rfh);
        m_transport->PanelOpenButton->setPixmap(m_panelClosed);
        m_isExpanded = false;
    } else {
        setFixedSize(width(), height() + rfh);
        m_transport->RecordingFrame->show();
        m_transport->PanelOpenButton->setPixmap(m_panelOpen);
        m_isExpanded = true;
    }
}

void
TransportDialog::slotPanelCloseButtonClicked()
{
    int rfh = m_transport->RecordingFrame->height();

    if (m_transport->RecordingFrame->isVisible()) {
        m_transport->RecordingFrame->hide();
        setFixedSize(width(), height() - rfh);
        m_transport->PanelOpenButton->setPixmap(m_panelClosed);
        m_isExpanded = false;
    }
}

bool
TransportDialog::isExpanded()
{
    return m_isExpanded;
}

void
TransportDialog::slotEditTempo()
{
    emit editTempo(this);
}

void
TransportDialog::slotEditTimeSignature()
{
    emit editTimeSignature(this);
}

void
TransportDialog::slotEditTime()
{
    emit editTransportTime(this);
}

void
TransportDialog::slotSetBackground(QColor c)
{
    if (!m_haveOriginalBackground) {
        m_originalBackground = m_transport->LCDBoxFrame->paletteBackgroundColor();
        m_haveOriginalBackground = true;
    }

    m_transport->LCDBoxFrame->setPaletteBackgroundColor(c);
    m_transport->NegativePixmap->setPaletteBackgroundColor(c);
    m_transport->TenHoursPixmap->setPaletteBackgroundColor(c);
    m_transport->UnitHoursPixmap->setPaletteBackgroundColor(c);
    m_transport->TimeDisplayLabel->setPaletteBackgroundColor(c);

    /* this is a bit more thorough, but too slow and flickery:
     
    const QObjectList *children = m_transport->LCDBoxFrame->children();
    QObjectListIt it(*children);
    QObject *obj;
     
    while ((obj = it.current()) != 0) {
     
    QWidget *w = dynamic_cast<QWidget *>(obj);
    if (w) {
    w->setPaletteBackgroundColor(c);
    }
    ++it;
    }
     
    */

    m_isBackgroundSet = true;
}

void
TransportDialog::slotResetBackground()
{
    if (m_isBackgroundSet) {
        slotSetBackground(m_originalBackground);
    }
    m_isBackgroundSet = false;
}

}
#include "TransportDialog.moc"
