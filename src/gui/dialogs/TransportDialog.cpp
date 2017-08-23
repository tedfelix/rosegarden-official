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

#define RG_MODULE_STRING "[TransportDialog]"

#include "TransportDialog.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/RealTime.h"
#include "base/Profiler.h"
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "gui/general/ThornStyle.h"
#include "sequencer/RosegardenSequencer.h"
#include "gui/application/TransportStatus.h"
#include "gui/application/RosegardenApplication.h"
#include "gui/general/MidiPitchLabel.h"
#include "gui/general/IconLoader.h"
#include "gui/studio/StudioControl.h"
#include "gui/widgets/Label.h"
#include "sound/MappedEvent.h"
#include "misc/ConfigGroups.h"

#include <QSettings>
#include <QShortcut>
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
#include <QDesktopWidget>
#include <QPainter>
#include <QtGlobal>


namespace  // anonymous
{
    QColor ledBlue(192, 216, 255);
}

namespace Rosegarden
{

TransportDialog::TransportDialog(QWidget *parent):
    QDialog(parent, 0),
    ui(0),
    //m_lcdList(),
    //m_lcdListDefault(),
    //m_lcdNegative(),
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
    m_tenHours(0),
    m_unitHours(0),
    m_tenMinutes(0),
    m_unitMinutes(0),
    m_tenSeconds(0),
    m_unitSeconds(0),
    m_tenths(0),
    m_hundreths(0),
    m_thousandths(0),
    m_tenThousandths(0),
    m_numerator(0),
    m_denominator(0),
    m_framesPerSecond(24),
    m_bitsPerFrame(80),
    m_midiInTimer(0),
    m_midiOutTimer(0),
    m_clearMetronomeTimer(0),
    m_enableMIDILabels(true),
    //m_panelOpen(),
    //m_panelClosed(),
    m_shortcuts(0),
    m_isExpanded(true),
    m_isBackgroundSet(false),
    m_sampleRate(0)
    //m_modeMap()
{
    // So we can identify it in RosegardenMainWindow::awaitDialogClearance().
    // Do not change this string:
    setObjectName("Rosegarden Transport");

    QVBoxLayout *vboxLay = new QVBoxLayout();
    setLayout( vboxLay );
    vboxLay->setMargin(0);

    QFrame *frame = new QFrame;
    vboxLay->addWidget(frame);
    ui = new Ui_RosegardenTransport();
    ui->setupUi(frame);
	
    setWindowTitle(tr("Rosegarden Transport"));
    setWindowIcon(IconLoader().loadPixmap("window-transport"));

    resetFonts();

    initModeMap();

    // set the LCD frame background to black
    //
    QPalette backgroundPalette = ui->LCDBoxFrame->palette();
    backgroundPalette.setColor(QPalette::Window, QColor(Qt::black));
    ui->LCDBoxFrame->setPalette(backgroundPalette); // this propagates to children, but they don't autofill their background anyway.
    ui->LCDBoxFrame->setAutoFillBackground(true);

    // unset the negative sign to begin with
    ui->NegativePixmap->clear();

    // Set our toggle buttons
    //
    ui->PlayButton->setCheckable(true);
    ui->RecordButton->setCheckable(true);

// Disable the loop button if JACK transport enabled, because this
// causes a nasty race condition, and it just seems our loops are not JACK compatible
// #1240039 - DMM
//    QSettings settings ; // was: mainWindow->config()
//    settings.beginGroup(SequencerOptionsConfigGroup);
//    if ( qStrToBool( settings.value("jacktransport", "false" ) ) )
//    {
//        ui->LoopButton->setEnabled(false);
//    }
//      settings.endGroup();

    // fix and hold the size of the dialog
    //
//!!! this probably won't work -- need sizeHint() after widget is realised
//    setMinimumSize(width(), height());
//    setMaximumSize(width(), height());

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

    ui->TimeDisplayLabel->hide();
    ui->ToEndLabel->hide();

    connect(ui->TimeDisplayButton, SIGNAL(clicked()),
            SLOT(slotChangeTimeDisplay()));

    connect(ui->ToEndButton, SIGNAL(clicked()),
            SLOT(slotChangeToEnd()));

    connect(ui->LoopButton, SIGNAL(clicked()),
            SLOT(slotLoopButtonClicked()));

    connect(ui->PanelOpenButton, SIGNAL(clicked()),
            SLOT(slotPanelOpenButtonClicked()));

    connect(ui->PanelCloseButton, SIGNAL(clicked()),
            SLOT(slotPanelCloseButtonClicked()));

    connect(ui->PanicButton, SIGNAL(clicked()), SIGNAL(panic()));
/*
    const QPixmap *p = ui->PanelOpenButton->pixmap();
    if (p) m_panelOpen = *p;
    p = ui->PanelCloseButton->pixmap();
    if (p) m_panelClosed = *p;
*/
    connect(ui->SetStartLPButton, SIGNAL(clicked()), SLOT(slotSetStartLoopingPointAtMarkerPos()));
    connect(ui->SetStopLPButton, SIGNAL(clicked()), SLOT(slotSetStopLoopingPointAtMarkerPos()));

    // clear labels
    //
    slotClearMidiInLabel();
    slotClearMidiOutLabel();

    // and by default we close the lower panel
    //
//    int rfh = ui->RecordingFrame->height();
    ui->RecordingFrame->hide();
//    setFixedSize(width(), height() - rfh);
//    ui->PanelOpenButton->setPixmap(m_panelClosed);

    // and since by default we show real time (not SMPTE), by default
    // we hide the small colon pixmaps
    //
    ui->SecondColonPixmap->hide();
    ui->HundredthColonPixmap->hide();

    // We have to specify these settings in this class (copied
    // from rosegardentransport.cpp) as we're using a specialised
    // widgets for TempoDisplay.  Ugly but works - does mean that
    // if the rest of the Transport ever changes then this code
    // will have to as well.
    //
    QPalette tempoPalette = ui->TempoDisplay->palette();
    tempoPalette.setColor(
            ui->TempoDisplay->foregroundRole(),
            ledBlue);
    ui->TempoDisplay->setPalette(tempoPalette);
    ui->TempoDisplay->setAlignment( Qt::AlignVCenter | Qt::AlignRight );

    QPalette timeSigPalette = ui->TimeSigDisplay->palette();
    timeSigPalette.setColor(
            ui->TimeSigDisplay->foregroundRole(),
            ledBlue);
    ui->TimeSigDisplay->setPalette(timeSigPalette);
    ui->TimeSigDisplay->setAlignment( Qt::AlignVCenter | Qt::AlignRight );

    QFont localFont(ui->OutDisplay->font() );
    localFont.setFamily( "lucida" );
    localFont.setBold( true );

    ui->TempoDisplay->setFont( localFont );
    ui->TimeSigDisplay->setFont( localFont );

    connect(ui->TempoDisplay, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTempo()));

    connect(ui->TempoDisplay, SIGNAL(scrollWheel(int)),
            this, SIGNAL(scrollTempo(int)));

    connect(ui->TimeSigDisplay, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTimeSignature()));

    // toil through the individual pixmaps
    connect(ui->NegativePixmap, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTime()));
    connect(ui->TenHoursPixmap, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTime()));
    connect(ui->UnitHoursPixmap, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTime()));
    connect(ui->HourColonPixmap, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTime()));
    connect(ui->TenMinutesPixmap, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTime()));
    connect(ui->UnitMinutesPixmap, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTime()));
    connect(ui->MinuteColonPixmap, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTime()));
    connect(ui->TenSecondsPixmap, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTime()));
    connect(ui->UnitSecondsPixmap, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTime()));
    connect(ui->SecondColonPixmap, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTime()));
    connect(ui->TenthsPixmap, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTime()));
    connect(ui->HundredthsPixmap, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTime()));
    connect(ui->HundredthColonPixmap, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTime()));
    connect(ui->TenThousandthsPixmap, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTime()));
    connect(ui->ThousandthsPixmap, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTime()));

    // shortcut object
    //
    m_shortcuts = new QShortcut(this);

    if (ThornStyle::isEnabled()) {
        /* Give the non-LED parts of the dialog the groupbox "lighter black" background
         * for improved constrast, and set foreground color to "LED blue" as used
         * elsewhere
         */
#if 0
        // ??? This is doing nothing.  My guess is that ThornStyle is
        //     overriding this with its own palette.
        QPalette backgroundPalette = palette();
        backgroundPalette.setColor(backgroundRole(), QColor(0x40, 0x40, 0x40));
        backgroundPalette.setColor(foregroundRole(), ledBlue);
        setPalette(backgroundPalette); // this propagates to children
        setAutoFillBackground(true);
#endif
    }

    // Performance Testing

    QSettings settings;
    settings.beginGroup("Performance_Testing");

    m_enableMIDILabels =
            (settings.value("TransportDialog_MIDI_Labels", 1).toInt() != 0);

    // Write it to the file to make it easier to find.
    settings.setValue("TransportDialog_MIDI_Labels",
                      m_enableMIDILabels ? 1 : 0);

    settings.endGroup();
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
         ++iter)
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
        if (x + width() > dw) x = dw - width();
        if (y + height() > dh) y = dh - height();
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
    m_lcdListDefault.clear();

    for (int i = 0; i < 10; i++) {
        m_lcdList[i] = IconLoader().loadPixmap(QString("led-%1").arg(i));
        QImage im(m_lcdList[i].size(), QImage::Format_RGB32);
        im.fill(0);
        QPainter p(&im);
        p.drawPixmap(0, 0, m_lcdList[i]);
        m_lcdListDefault[i] = QPixmap::fromImage(im);
    }

    // Load the "negative" sign pixmap
    //
    m_lcdNegative = IconLoader().loadPixmap("led--");
}

void
TransportDialog::resetFonts()
{
    resetFont(ui->TimeSigLabel);
    resetFont(ui->TimeSigDisplay);
    resetFont(ui->TempoLabel);
    resetFont(ui->TempoDisplay);
    resetFont(ui->DivisionLabel);
    resetFont(ui->DivisionDisplay);
    resetFont(ui->InLabel);
    resetFont(ui->InDisplay);
    resetFont(ui->OutLabel);
    resetFont(ui->OutDisplay);
    resetFont(ui->ToEndLabel);
    resetFont(ui->TimeDisplayLabel);
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
        ui->TimeDisplayLabel->hide();
        break;

    case SMPTEMode:
        m_clearMetronomeTimer->stop();
        ui->TimeDisplayLabel->setText("SMPTE"); // DO NOT i18n
        ui->TimeDisplayLabel->show();
        break;

    case BarMode:
        m_clearMetronomeTimer->stop();
        ui->TimeDisplayLabel->setText("BAR"); // DO NOT i18n
        ui->TimeDisplayLabel->show();
        break;

    case BarMetronomeMode:
        m_clearMetronomeTimer->setSingleShot(false);
        m_clearMetronomeTimer->start(1700);
        ui->TimeDisplayLabel->setText("MET"); // DO NOT i18n
        ui->TimeDisplayLabel->show();
        break;

    case FrameMode:
        m_clearMetronomeTimer->stop();
        ui->TimeDisplayLabel->setText(QString("%1").arg(m_sampleRate));
        ui->TimeDisplayLabel->show();
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
    if (ui->ToEndButton->isChecked()) {
        ui->ToEndLabel->show();
    } else {
        ui->ToEndLabel->hide();
    }
}

bool
TransportDialog::isShowingTimeToEnd()
{
    return ui->ToEndButton->isChecked();
}

void
TransportDialog::displayRealTime(const RealTime &rt)
{
    RealTime st = rt;

    slotResetBackground();

    if (m_lastMode != RealMode) {
        ui->HourColonPixmap->show();
        ui->MinuteColonPixmap->show();
        ui->SecondColonPixmap->hide();
        ui->HundredthColonPixmap->hide();
        m_lastMode = RealMode;
    }

    // If time is negative then reverse the time and set the minus flag
    //
    if (st < RealTime::zeroTime) {
        st = RealTime::zeroTime - st;
        if (!m_lastNegative) {
            ui->NegativePixmap->setPixmap(m_lcdNegative);
            m_lastNegative = true;
        }
    } else // don't show the flag
    {
        if (m_lastNegative) {
            ui->NegativePixmap->clear();
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
        ui->HourColonPixmap->hide();
        ui->MinuteColonPixmap->hide();
        ui->SecondColonPixmap->hide();
        ui->HundredthColonPixmap->hide();
        m_lastMode = FrameMode;
    }

    // If time is negative then reverse the time and set the minus flag
    //
    if (st < RealTime::zeroTime) {
        st = RealTime::zeroTime - st;
        if (!m_lastNegative) {
            ui->NegativePixmap->setPixmap(m_lcdNegative);
            m_lastNegative = true;
        }
    } else // don't show the flag
    {
        if (m_lastNegative) {
            ui->NegativePixmap->clear();
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
        ui->HourColonPixmap->show();
        ui->MinuteColonPixmap->show();
        ui->SecondColonPixmap->show();
        ui->HundredthColonPixmap->show();
        m_lastMode = SMPTEMode;
    }

    // If time is negative then reverse the time and set the minus flag
    //
    if (st < RealTime::zeroTime) {
        st = RealTime::zeroTime - st;
        if (!m_lastNegative) {
            ui->NegativePixmap->setPixmap(m_lcdNegative);
            m_lastNegative = true;
        }
    } else // don't show the flag
    {
        if (m_lastNegative) {
            ui->NegativePixmap->clear();
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
        ui->HourColonPixmap->hide();
        ui->MinuteColonPixmap->show();
        ui->SecondColonPixmap->hide();
        ui->HundredthColonPixmap->hide();
        m_lastMode = BarMode;
    }

    // If time is negative then reverse the time and set the minus flag
    //
    if (bar < 0) {
        bar = -bar;
        if (!m_lastNegative) {
            ui->NegativePixmap->setPixmap(m_lcdNegative);
            m_lastNegative = true;
        }
    } else // don't show the flag
    {
        if (m_lastNegative) {
            ui->NegativePixmap->clear();
            m_lastNegative = false;
        }
    }

    if (m_currentMode == BarMetronomeMode && unit < 2) {
        if (beat == 1) {
            setBackgroundColor(Qt::red);
        } else {
            setBackgroundColor(Qt::cyan);
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
    Profiler profiler("TransportDialog::updateTimeDisplay");

#define UPDATE(NEW,OLD,WIDGET)                                     \
    if (NEW != OLD) {                                              \
        if (NEW < 0) {                                             \
            ui->WIDGET->clear();                          \
        } else if (!m_isBackgroundSet) {                           \
            ui->WIDGET->setPixmap(m_lcdListDefault[NEW]); \
        } else {                                                   \
            ui->WIDGET->setPixmap(m_lcdList[NEW]);        \
        }                                                          \
        OLD = NEW;                                                 \
    }

    UPDATE(m_tenThousandths, m_lastTenThousandths, TenThousandthsPixmap);
    UPDATE(m_thousandths,    m_lastThousandths,    ThousandthsPixmap);
    UPDATE(m_hundreths,      m_lastHundreths,      HundredthsPixmap);
    UPDATE(m_tenths,         m_lastTenths,         TenthsPixmap);
    UPDATE(m_unitSeconds,    m_lastUnitSeconds,    UnitSecondsPixmap);
    UPDATE(m_tenSeconds,     m_lastTenSeconds,     TenSecondsPixmap);
    UPDATE(m_unitMinutes,    m_lastUnitMinutes,    UnitMinutesPixmap);
    UPDATE(m_tenMinutes,     m_lastTenMinutes,     TenMinutesPixmap);
    UPDATE(m_unitHours,      m_lastUnitHours,      UnitHoursPixmap);
    UPDATE(m_tenHours,       m_lastTenHours,       TenHoursPixmap);
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
    ui->TimeSigDisplay->setText(timeSigString);
}

void
TransportDialog::slotMidiInLabel(const MappedEvent *mE)
{
    // If MIDI label updates have been turned off, bail.
    if (!m_enableMIDILabels)
        return;

    Q_CHECK_PTR(mE);

    switch (mE->getType()) {
    case MappedEvent::MidiNote:
    case MappedEvent::MidiNoteOneShot:
    {
        // don't do anything if we've got an effective NOTE OFF
        //
        if (mE->getVelocity() == 0)
            return ;

        MidiPitchLabel mPL(mE->getPitch());
        ui->InDisplay->setText
            (mPL.getQString() +
             QString("  %1").arg(mE->getVelocity()));
    }
    break;

    case MappedEvent::MidiPitchBend:
        ui->InDisplay->setText(tr("PITCH WHEEL"));
        break;

    case MappedEvent::MidiController:
        ui->InDisplay->setText(tr("CONTROLLER"));
        break;

    case MappedEvent::MidiProgramChange:
        ui->InDisplay->setText(tr("PROG CHNGE"));
        break;

    case MappedEvent::MidiKeyPressure:
    case MappedEvent::MidiChannelPressure:
        ui->InDisplay->setText(tr("PRESSURE"));
        break;

    case MappedEvent::MidiSystemMessage:
        ui->InDisplay->setText(tr("SYS MESSAGE"));
        break;

        // Pacify compiler warnings about missed cases.
    case MappedEvent::InvalidMappedEvent:
    case MappedEvent::Audio:
    case MappedEvent::AudioCancel:
    case MappedEvent::AudioLevel:
    case MappedEvent::AudioStopped:
    case MappedEvent::AudioGeneratePreview:
    case MappedEvent::Marker:
    case MappedEvent::SystemUpdateInstruments:
    case MappedEvent::SystemJackTransport:
    case MappedEvent::SystemMMCTransport:
    case MappedEvent::SystemMIDIClock:
    case MappedEvent::SystemMetronomeDevice:
    case MappedEvent::SystemAudioPortCounts:
    case MappedEvent::SystemAudioPorts:
    case MappedEvent::SystemFailure:
    case MappedEvent::Panic:
    case MappedEvent::SystemMTCTransport:
    case MappedEvent::SystemMIDISyncAuto:
    case MappedEvent::SystemAudioFileFormat:
    case MappedEvent::TimeSignature:
    case MappedEvent::Tempo:
    case MappedEvent::Text:
    default:   // do nothing
        return ;
    }

    // Reset the timer if it's already running
    //
    if (m_midiInTimer->isActive())
        m_midiInTimer->stop();

    // 1.5 second timeout for MIDI event
    //
    m_midiInTimer->setSingleShot(true);
    m_midiInTimer->start(1500);
}

void
TransportDialog::slotClearMidiInLabel()
{
    ui->InDisplay->setText(tr("NO EVENTS"));

    // also, just to be sure:
    slotResetBackground();
}

void
TransportDialog::slotMidiOutLabel(const MappedEvent *mE)
{
    // If MIDI label updates have been turned off, bail.
    if (!m_enableMIDILabels)
        return;

    Q_CHECK_PTR(mE);

    switch (mE->getType()) {
    case MappedEvent::MidiNote:
    case MappedEvent::MidiNoteOneShot:
    {
        MidiPitchLabel mPL(mE->getPitch());
        ui->OutDisplay->setText
            (mPL.getQString() +
             QString("  %1").arg(mE->getVelocity()));
    }
    break;

    case MappedEvent::MidiPitchBend:
        ui->OutDisplay->setText(tr("PITCH WHEEL"));
        break;

    case MappedEvent::MidiController:
        ui->OutDisplay->setText(tr("CONTROLLER"));
        break;

    case MappedEvent::MidiProgramChange:
        ui->OutDisplay->setText(tr("PROG CHNGE"));
        break;

    case MappedEvent::MidiKeyPressure:
    case MappedEvent::MidiChannelPressure:
        ui->OutDisplay->setText(tr("PRESSURE"));
        break;

    case MappedEvent::MidiSystemMessage:
        ui->OutDisplay->setText(tr("SYS MESSAGE"));
        break;

        // Pacify compiler warnings about missed cases.
    case MappedEvent::InvalidMappedEvent:
    case MappedEvent::Audio:
    case MappedEvent::AudioCancel:
    case MappedEvent::AudioLevel:
    case MappedEvent::AudioStopped:
    case MappedEvent::AudioGeneratePreview:
    case MappedEvent::Marker:
    case MappedEvent::SystemUpdateInstruments:
    case MappedEvent::SystemJackTransport:
    case MappedEvent::SystemMMCTransport:
    case MappedEvent::SystemMIDIClock:
    case MappedEvent::SystemMetronomeDevice:
    case MappedEvent::SystemAudioPortCounts:
    case MappedEvent::SystemAudioPorts:
    case MappedEvent::SystemFailure:
    case MappedEvent::Panic:
    case MappedEvent::SystemMTCTransport:
    case MappedEvent::SystemMIDISyncAuto:
    case MappedEvent::SystemAudioFileFormat:
    case MappedEvent::TimeSignature:
    case MappedEvent::Tempo:
    case MappedEvent::Text:
    default:   // do nothing
        return ;
    }

    // Reset the timer if it's already running
    //
    if (m_midiOutTimer->isActive())
        m_midiOutTimer->stop();

    // 200 millisecond timeout
    //
    m_midiOutTimer->setSingleShot(true);
    m_midiOutTimer->start(200);
}

void
TransportDialog::slotClearMidiOutLabel()
{
    ui->OutDisplay->setText(tr("NO EVENTS"));
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
    //    ui->LoopButton->setEnabled(false);
    //    ui->LoopButton->setOn(false);
    //        return;
    //    }
    //    settings.endGroup();

    if (ui->LoopButton->isChecked()) {
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

void TransportDialog::slotTempoChanged(tempoT tempo)
{
    QString tempoString;
    tempoString.sprintf("%4.3f", Composition::getTempoQpm(tempo));
    ui->TempoDisplay->setText(tempoString);
}

void TransportDialog::slotPlaying(bool checked)
{
    ui->PlayButton->setChecked(checked);
}

void TransportDialog::slotRecording(bool checked)
{
    ui->RecordButton->setChecked(checked);
}

void TransportDialog::slotMetronomeActivated(bool checked)
{
    ui->MetronomeButton->setChecked(checked);
}

void
TransportDialog::slotPanelOpenButtonClicked()
{
    // int rfh = ui->RecordingFrame->height();

    if (ui->RecordingFrame->isVisible()) {
        ui->RecordingFrame->hide();
//        setFixedSize(width(), height() - rfh);
//        ui->PanelOpenButton->setPixmap(m_panelClosed);
//        adjustSize();
        setFixedSize(416, 87);
//        cerr << "size hint: " << sizeHint().width() << "x" << sizeHint().height() << endl;
//        setFixedSize(sizeHint());
//        setMinimumSize(sizeHint());
        m_isExpanded = false;
    } else {
//        setFixedSize(width(), height() + rfh);
        setFixedSize(416, 132);
        ui->RecordingFrame->show();
//        ui->PanelOpenButton->setPixmap(m_panelOpen);
//        adjustSize();
//        cerr << "size hint: " << sizeHint().width() << "x" << sizeHint().height() << endl;
//        setFixedSize(sizeHint());
//        setMinimumSize(sizeHint());
        m_isExpanded = true;
    }
}

void
TransportDialog::slotPanelCloseButtonClicked()
{
    // int rfh = ui->RecordingFrame->height();

    if (ui->RecordingFrame->isVisible()) {
        ui->RecordingFrame->hide();
//        setFixedSize(width(), height() - rfh);
//        ui->PanelOpenButton->setPixmap(m_panelClosed);
        setFixedSize(416, 87);
//        adjustSize();
//        cerr << "size hint: " << sizeHint().width() << "x" << sizeHint().height() << endl;
//        setFixedSize(sizeHint());
//        setMinimumSize(sizeHint());
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
TransportDialog::setBackgroundColor(QColor color)
{
    QPalette palette = ui->LCDBoxFrame->palette();
    palette.setColor(QPalette::Window, color);
    ui->LCDBoxFrame->setPalette(palette);

    m_isBackgroundSet = true;
}

void
TransportDialog::slotResetBackground()
{
    if (m_isBackgroundSet) {
        setBackgroundColor(Qt::black);
    }
    m_isBackgroundSet = false;
}

}
