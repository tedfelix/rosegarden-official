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

#define RG_MODULE_STRING "[TransportDialog]"

#include "TransportDialog.h"

#include "base/Composition.h"
#include "base/Configuration.h"  // DocumentConfiguration
#include "base/NotationTypes.h"
#include "base/Pitch.h"
#include "base/RealTime.h"
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "gui/general/ThornStyle.h"
#include "sequencer/RosegardenSequencer.h"
#include "sound/SequencerDataBlock.h"
#include "gui/application/TransportStatus.h"
#include "gui/application/RosegardenApplication.h"
#include "gui/general/IconLoader.h"
#include "gui/studio/StudioControl.h"
#include "gui/widgets/Label.h"
#include "sound/MappedEvent.h"
#include "misc/ConfigGroups.h"
#include "document/RosegardenDocument.h"
#include "base/Composition.h"
#include "gui/application/RosegardenMainWindow.h"
#include "misc/Preferences.h"

#include <QSettings>
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
#include <QPainter>
#include <QtGlobal>
#include <QScreen>


namespace  // anonymous
{
    QColor ledBlue(192, 216, 255);
}

namespace Rosegarden
{

TransportDialog::TransportDialog(QWidget *parent):
    QDialog(parent),
    ui(new Ui_RosegardenTransport()),
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
    m_midiInTimer(nullptr),
    m_midiOutTimer(nullptr),
    m_enableMIDILabels(true),
    //m_panelOpen(),
    //m_panelClosed(),
    m_isExpanded(true),
    m_isBackgroundSet(false),
    m_sampleRate(0)
    //m_modeMap()
{
    // So we can identify it in RosegardenMainWindow::awaitDialogClearance().
    // Do not change this string:
    setObjectName("Rosegarden Transport");
    setWindowTitle(tr("Rosegarden Transport"));
    setWindowIcon(IconLoader::loadPixmap("window-transport"));

    // The child application area (client area in Windows-speak).
    QFrame *frame = new QFrame(this);

    ui->setupUi(frame);

    resetFonts();

    initModeMap();

    // set the LCD frame background to black
    //
    QPalette lcdPalette = ui->LCDBoxFrame->palette();
    lcdPalette.setColor(ui->LCDBoxFrame->backgroundRole(), QColor(Qt::black));
    ui->LCDBoxFrame->setPalette(lcdPalette); // this propagates to children, but they don't autofill their background anyway.
    ui->LCDBoxFrame->setAutoFillBackground(true);

    // unset the negative sign to begin with
    ui->NegativePixmap->clear();

    // Set our toggle buttons
    //
    ui->PlayButton->setCheckable(true);
    ui->RecordButton->setCheckable(true);

    // fix and hold the size of the dialog
    //
//!!! this probably won't work -- need sizeHint() after widget is realised
//    setMinimumSize(width(), height());
//    setMaximumSize(width(), height());

    loadPixmaps();

    // Create Midi label timers
    m_midiInTimer = new QTimer(this);
    m_midiOutTimer = new QTimer(this);

    connect(m_midiInTimer, &QTimer::timeout,
            this, &TransportDialog::slotClearMidiInLabel);

    connect(m_midiOutTimer, &QTimer::timeout,
            this, &TransportDialog::slotClearMidiOutLabel);

    ui->TimeDisplayLabel->hide();
    ui->ToEndLabel->hide();

    connect(ui->TimeDisplayButton, &QAbstractButton::clicked,
            this, &TransportDialog::slotChangeTimeDisplay);

    connect(ui->ToEndButton, &QAbstractButton::clicked,
            this, &TransportDialog::slotChangeToEnd);

    connect(ui->PanelOpenButton, &QAbstractButton::clicked,
            this, &TransportDialog::slotPanelOpenButtonClicked);

    connect(ui->PanelCloseButton, &QAbstractButton::clicked,
            this, &TransportDialog::slotPanelCloseButtonClicked);

    connect(ui->PanicButton, &QAbstractButton::clicked,
            this, &TransportDialog::panic);
/*
    const QPixmap *p = ui->PanelOpenButton->pixmap();
    if (p) m_panelOpen = *p;
    p = ui->PanelCloseButton->pixmap();
    if (p) m_panelClosed = *p;
*/

    // Loop widgets.
    connect(ui->LoopButton, &QAbstractButton::clicked,
            this, &TransportDialog::slotLoopButtonClicked);
    connect(ui->SetStartLPButton, &QAbstractButton::clicked,
            this, &TransportDialog::slotSetStartLoopingPointAtMarkerPos);
    connect(ui->SetStopLPButton, &QAbstractButton::clicked,
            this, &TransportDialog::slotSetStopLoopingPointAtMarkerPos);

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

    connect(ui->TempoDisplay, &Label::doubleClicked,
            this, &TransportDialog::slotEditTempo);

    //connect(ui->TempoDisplay, &Label::scrollWheel,
    //        this, &TransportDialog::scrollTempo);

    connect(ui->TimeSigDisplay, &Label::doubleClicked,
            this, &TransportDialog::slotEditTimeSignature);

    // toil through the individual pixmaps
    connect(ui->NegativePixmap, &Label::doubleClicked,
            this, &TransportDialog::slotEditTime);
    connect(ui->TenHoursPixmap, &Label::doubleClicked,
            this, &TransportDialog::slotEditTime);
    connect(ui->UnitHoursPixmap, &Label::doubleClicked,
            this, &TransportDialog::slotEditTime);
    connect(ui->HourColonPixmap, &Label::doubleClicked,
            this, &TransportDialog::slotEditTime);
    connect(ui->TenMinutesPixmap, &Label::doubleClicked,
            this, &TransportDialog::slotEditTime);
    connect(ui->UnitMinutesPixmap, &Label::doubleClicked,
            this, &TransportDialog::slotEditTime);
    connect(ui->MinuteColonPixmap, &Label::doubleClicked,
            this, &TransportDialog::slotEditTime);
    connect(ui->TenSecondsPixmap, &Label::doubleClicked,
            this, &TransportDialog::slotEditTime);
    connect(ui->UnitSecondsPixmap, &Label::doubleClicked,
            this, &TransportDialog::slotEditTime);
    connect(ui->SecondColonPixmap, &Label::doubleClicked,
            this, &TransportDialog::slotEditTime);
    connect(ui->TenthsPixmap, &Label::doubleClicked,
            this, &TransportDialog::slotEditTime);
    connect(ui->HundredthsPixmap, &Label::doubleClicked,
            this, &TransportDialog::slotEditTime);
    connect(ui->HundredthColonPixmap, &Label::doubleClicked,
            this, &TransportDialog::slotEditTime);
    connect(ui->TenThousandthsPixmap, &Label::doubleClicked,
            this, &TransportDialog::slotEditTime);
    connect(ui->ThousandthsPixmap, &Label::doubleClicked,
            this, &TransportDialog::slotEditTime);

    // Note: For Thorn style, ThornStyle sets the transport's background
    //       to dark gray.  See AppEventFilter::polishWidget() in
    //       ThornStyle.cpp.

    loadGeo();

    connect(RosegardenMainWindow::self(),
                &RosegardenMainWindow::documentLoaded,
            this, &TransportDialog::slotDocumentLoaded);

    // Metronome
    connect(&m_metronomeTimer, &QTimer::timeout,
            this, &TransportDialog::slotMetronomeTimer);
    // No improvement.
    //m_metronomeTimer.setTimerType(Qt::PreciseTimer);

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
    // Only save if visible.  RMW::slotUpdateTransportVisibility() has already
    // saved if we are hidden.
    if (isVisible())
        saveGeo();
}

void TransportDialog::init()
{
    RosegardenDocument *doc = RosegardenDocument::currentDocument;
    if (!doc)
        return;

    Composition &comp = doc->getComposition();

    setEnabled(true);

    setTimeSignature(comp.getTimeSignatureAt(comp.getPosition()));

    // bring the transport to the front
    raise();

    // set the play metronome button
    MetronomeButton()->setChecked(comp.usePlayMetronome());

    // set the transport mode found in the configuration
    const std::string transportMode =
            doc->getConfiguration().get<String>(
                    DocumentConfiguration::TransportMode);
    setNewMode(transportMode);

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
             << m_currentMode << " to string.";
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
TransportDialog::loadPixmaps()
{
    // For each digit 0-9...
    for (int i = 0; i <= 9; ++i) {
        // Load the transparent pixmap.
        m_digitsTransparent[i] =
                IconLoader::loadPixmap(QString("led-%1").arg(i));

        // Make the opaque version.  This should be a little
        // faster to draw since it avoids alpha math.
        QImage opaqueImage(m_digitsTransparent[i].size(), QImage::Format_RGB32);
        // Fill with black.
        opaqueImage.fill(0);
        // Create a QPainter so that we can draw the pixmap on the image.
        QPainter opaquePainter(&opaqueImage);
        // Draw the digit pixmap onto the image.
        opaquePainter.drawPixmap(0, 0, m_digitsTransparent[i]);
        // Copy to the opaque pixmap array.
        m_digitsOpaque[i] = QPixmap::fromImage(opaqueImage);
    }

    // Load the "negative" sign pixmap
    //
    m_negativeSign = IconLoader::loadPixmap("led--");
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

/* unused
void
ransportDialog::setSMPTEResolution(int framesPerSecond,
                                    int bitsPerFrame)
{
    m_framesPerSecond = framesPerSecond;
    m_bitsPerFrame = bitsPerFrame;
}
*/

/* unused
void
TransportDialog::getSMPTEResolution(int &framesPerSecond,
                                    int &bitsPerFrame)
{
    framesPerSecond = m_framesPerSecond;
    bitsPerFrame = m_bitsPerFrame;
}
*/

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

    RosegardenDocument *doc = RosegardenDocument::currentDocument;
    if (!doc)
        return;

    // Save the new TimeDisplayMode in the DocumentConfiguration.
    doc->getConfiguration().set<String>(
            DocumentConfiguration::TransportMode, getCurrentModeAsString());
    doc->slotDocumentModified();

}

void
TransportDialog::displayTime()
{
    switch (m_currentMode) {
    case RealMode:
        ui->TimeDisplayLabel->hide();
        break;

    case SMPTEMode:
        ui->TimeDisplayLabel->setText("SMPTE"); // DO NOT i18n
        ui->TimeDisplayLabel->show();
        break;

    case BarMode:
        ui->TimeDisplayLabel->setText("BAR"); // DO NOT i18n
        ui->TimeDisplayLabel->show();
        break;

    case BarMetronomeMode:
        ui->TimeDisplayLabel->setText("MET"); // DO NOT i18n
        ui->TimeDisplayLabel->show();
        break;

    case FrameMode:
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

    updateMetronomeTimer();

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

    resetBackground();

    if (m_lastMode != RealMode) {
        ui->HourColonPixmap->show();
        ui->MinuteColonPixmap->show();
        ui->SecondColonPixmap->hide();
        ui->HundredthColonPixmap->hide();
        m_lastMode = RealMode;
    }

    // If time is negative then reverse the time and set the minus flag
    //
    if (st < RealTime::zero()) {
        st = RealTime::zero() - st;
        if (!m_lastNegative) {
            ui->NegativePixmap->setPixmap(m_negativeSign);
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

    resetBackground();

    if (m_lastMode != FrameMode) {
        ui->HourColonPixmap->hide();
        ui->MinuteColonPixmap->hide();
        ui->SecondColonPixmap->hide();
        ui->HundredthColonPixmap->hide();
        m_lastMode = FrameMode;
    }

    // If time is negative then reverse the time and set the minus flag
    //
    if (st < RealTime::zero()) {
        st = RealTime::zero() - st;
        if (!m_lastNegative) {
            ui->NegativePixmap->setPixmap(m_negativeSign);
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

    resetBackground();

    if (m_lastMode != SMPTEMode) {
        ui->HourColonPixmap->show();
        ui->MinuteColonPixmap->show();
        ui->SecondColonPixmap->show();
        ui->HundredthColonPixmap->show();
        m_lastMode = SMPTEMode;
    }

    // If time is negative then reverse the time and set the minus flag
    //
    if (st < RealTime::zero()) {
        st = RealTime::zero() - st;
        if (!m_lastNegative) {
            ui->NegativePixmap->setPixmap(m_negativeSign);
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
            ui->NegativePixmap->setPixmap(m_negativeSign);
            m_lastNegative = true;
        }
    } else // don't show the flag
    {
        if (m_lastNegative) {
            ui->NegativePixmap->clear();
            m_lastNegative = false;
        }
    }

    // Break bar/beat/unit into digits.

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

    // Update the digits on the display.
    updateTimeDisplay();
}

void
TransportDialog::updateTimeDisplay()
{

#define UPDATE(NEW,OLD,WIDGET)                                     \
    if (NEW != OLD) {                                              \
        if (NEW < 0  ||  NEW > 9) {                                \
            ui->WIDGET->clear();                          \
        } else if (!m_isBackgroundSet) {                           \
            /* Metronome is *not* flashing, use the opaque versions. */  \
            /* ??? Seeing the opaque bitmap during flash on occasion.  */  \
            ui->WIDGET->setPixmap(m_digitsOpaque[NEW]);  \
        } else {                                                   \
            /* Metronome *is* flashing, use the transparent versions. */  \
            ui->WIDGET->setPixmap(m_digitsTransparent[NEW]);  \
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

    const QString timeSigString =
        QString::asprintf("%d/%d", numerator, denominator);
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

        ui->InDisplay->setText(
                Pitch::toStringOctave(mE->getPitch()) +
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

    case MappedEvent::MidiRPN:
        ui->InDisplay->setText(tr("RPN"));
        break;

    case MappedEvent::MidiNRPN:
        ui->InDisplay->setText(tr("NRPN"));
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
    case MappedEvent::KeySignature:
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
    resetBackground();
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
        ui->OutDisplay->setText(
                Pitch::toStringOctave(mE->getPitch()) +
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

    case MappedEvent::MidiRPN:
        ui->OutDisplay->setText(tr("RPN"));
        break;

    case MappedEvent::MidiNRPN:
        ui->OutDisplay->setText(tr("NRPN"));
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
    case MappedEvent::KeySignature:
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
TransportDialog::closeEvent(QCloseEvent * /*closeEvent*/)
{
    emit closed();

    // We don't actually close.  Instead we hide.
    // RosegardenMainWindow::slotCloseTransport() handles that and keeping
    // the menu in sync.
    //e->accept();
}

void
TransportDialog::slotLoopButtonClicked()
{
    RosegardenDocument::currentDocument->loopButton(
            ui->LoopButton->isChecked());
}

void
TransportDialog::slotSetStartLoopingPointAtMarkerPos()
{
    RosegardenDocument *document = RosegardenDocument::currentDocument;
    Composition &composition = document->getComposition();

    const timeT loopStart = composition.getPosition();
    timeT loopEnd = composition.getLoopEnd();

    // Turn a backwards loop into an empty loop.
    if (loopStart > loopEnd)
        loopEnd = loopStart;

    if (loopStart != loopEnd)
        composition.setLoopMode(Composition::LoopOn);
    else
        composition.setLoopMode(Composition::LoopOff);

    composition.setLoopStart(loopStart);
    composition.setLoopEnd(loopEnd);

    emit document->loopChanged();
}

void
TransportDialog::slotSetStopLoopingPointAtMarkerPos()
{
    RosegardenDocument *document = RosegardenDocument::currentDocument;
    Composition &composition = document->getComposition();

    timeT loopStart = composition.getLoopStart();
    const timeT loopEnd = composition.getPosition();

    // Turn a backwards loop into an empty loop.
    if (loopEnd < loopStart)
        loopStart = loopEnd;

    if (loopStart != loopEnd)
        composition.setLoopMode(Composition::LoopOn);
    else
        composition.setLoopMode(Composition::LoopOff);

    composition.setLoopStart(loopStart);
    composition.setLoopEnd(loopEnd);

    emit document->loopChanged();
}

void
TransportDialog::slotLoopChanged()
{
    RosegardenDocument *document = RosegardenDocument::currentDocument;
    Composition &composition = document->getComposition();

    ui->LoopButton->setChecked(
            (composition.getLoopMode() != Composition::LoopOff));
}

void
TransportDialog::slotDocumentLoaded(RosegardenDocument *doc)
{
    connect(doc, &RosegardenDocument::loopChanged,
            this, &TransportDialog::slotLoopChanged);
}

void TransportDialog::slotTempoChanged(tempoT tempo)
{
    const QString tempoString =
        QString::asprintf("%4.3f", Composition::getTempoQpm(tempo));
    ui->TempoDisplay->setText(tempoString);
}

void TransportDialog::slotPlaying(bool checked)
{
    ui->PlayButton->setChecked(checked);

    // If it changed...
    if (checked != m_playing) {
        m_playing = checked;
        updateMetronomeTimer();
    }
}

void TransportDialog::slotRecording(bool checked)
{
    ui->RecordButton->setChecked(checked);

    // If it changed...
    if (checked != m_recording) {
        m_recording = checked;
        updateMetronomeTimer();
    }
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
    palette.setColor(ui->LCDBoxFrame->backgroundRole(), color);
    ui->LCDBoxFrame->setPalette(palette);

    m_isBackgroundSet = true;
}

void
TransportDialog::resetBackground()
{
    if (m_isBackgroundSet) {
        setBackgroundColor(Qt::black);
    }
    m_isBackgroundSet = false;
}

void TransportDialog::saveGeo()
{
    QSettings settings;
    settings.beginGroup(WindowGeometryConfigGroup);
    settings.setValue("Transport_Geometry", saveGeometry());
}

void TransportDialog::loadGeo()
{
    QSettings settings;
    settings.beginGroup(WindowGeometryConfigGroup);
    restoreGeometry(settings.value("Transport_Geometry").toByteArray());
}

void TransportDialog::slotMetronomeTimer()
{
    // This routine is time-critical.  Keep it short and do as
    // little work as possible.

    // ??? This is still pretty unreliable.  Either the timer isn't
    //     very reliable or the display updates aren't.  We could
    //     collect a list of the arrival times in a circular
    //     queue and then analyze the deltas.  That would give us an
    //     idea how good the timer is.

    // If we are flashing
    if (m_flashing) {
        // If we have timed out
        if (QDateTime::currentDateTime() > m_metronomeTimeout) {
            // Clear the flash.
            // ??? Inline and make less CPU-intensive?
            resetBackground();

            // No improvement.
            //repaint();
            //qApp->processEvents();

            // Indicate not flashing.
            m_flashing = false;

            // If we are neither playing nor recording, stop the timer.
            // Otherwise the flash will be stuck if we stop when it is
            // flashing.
            if (!(m_playing  ||  m_recording))
                m_metronomeTimer.stop();
        }
        return;
    }

    // Get the playback time.
    // SequencerDataBlock appears to be the right place for very
    // precise time.
    const RealTime position =
            SequencerDataBlock::getInstance()->getPositionPointer();
    const Composition &comp =
            RosegardenDocument::currentDocument->getComposition();
    // Convert RealTime to timeT (sequencer ticks).
    timeT elapsedTime = comp.getElapsedTimeForRealTime(position);
    int bar;
    int beat;
    int fraction;
    int remainder;
    comp.getMusicalTimeForAbsoluteTime(elapsedTime, bar, beat, fraction, remainder);

    // If we are on the beat.
    if (fraction == 0) {
        // Flash
        if (beat == 1) {
            // ??? Inline and make less CPU-intensive?
            setBackgroundColor(Qt::red);
            //setBackgroundColor(QColor(255,0,0));
        } else {
            // ??? Inline and make less CPU-intensive?
            setBackgroundColor(Qt::cyan);
            //setBackgroundColor(QColor(0,255,255));
        }

        // No improvement.
        //repaint();
        //qApp->processEvents();

        // Compute the timeout.
        m_metronomeTimeout = QDateTime::currentDateTime().addMSecs(90);

        // Indicate flashing.
        m_flashing = true;

        return;
    }
}

void TransportDialog::updateMetronomeTimer()
{
    // Start/Stop metronome timer as needed.
    if (m_currentMode == BarMetronomeMode  &&  (m_playing  ||  m_recording)) {
        m_metronomeTimer.start(10);
        return;
    }

    // We need to stop the timer.

    // If we aren't flashing, we can just go ahead and stop the timer.
    if (!m_flashing) {
        m_metronomeTimer.stop();
    }

    // If we are flashing, we need to let slotMetronomeTimer() clear the flash
    // for us and stop the timer.
}

void TransportDialog::keyPressEvent(QKeyEvent *keyEvent)
{
    if (!keyEvent)
        return;

    RosegardenMainWindow *rmw = RosegardenMainWindow::self();
    if (!rmw) {
        QDialog::keyPressEvent(keyEvent);
        return;
    }

    const int key = keyEvent->key();

    if (key == Qt::Key_PageUp)
        rmw->slotRewind();
    else if (key == Qt::Key_PageDown)
        rmw->slotFastforward();
    else if (key == Qt::Key_Home)
        rmw->slotRewindToBeginning();
    else if (key == Qt::Key_End)
        rmw->slotFastForwardToEnd();

    QDialog::keyPressEvent(keyEvent);
}


}
