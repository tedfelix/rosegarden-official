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

#define RG_MODULE_STRING "[SegmentParameterBox]"

#include "SegmentParameterBox.h"

#include "misc/Debug.h"
#include "misc/Strings.h"  // qstrtostr() etc...
#include "base/Colour.h"
#include "base/ColourMap.h"
//#include "base/NotationTypes.h"
#include "base/BasicQuantizer.h"
#include "base/RealTime.h"
#include "base/Segment.h"
#include "commands/segment/SegmentChangeQuantizationCommand.h"
#include "commands/segment/SegmentColourCommand.h"
#include "commands/segment/SegmentColourMapCommand.h"
#include "commands/segment/SegmentCommandRepeat.h"
#include "commands/segment/SegmentLabelCommand.h"
#include "commands/segment/SegmentLinkTransposeCommand.h"
#include "document/CommandHistory.h"
#include "document/RosegardenDocument.h"
#include "gui/dialogs/IntervalDialog.h"
#include "gui/editors/notation/NotationStrings.h"
#include "gui/editors/notation/NotePixmapFactory.h"
#include "gui/general/GUIPalette.h"
#include "gui/widgets/ColorCombo.h"
#include "gui/widgets/ColourTable.h"
#include "gui/widgets/TristateCheckBox.h"
#include "gui/widgets/CollapsingFrame.h"
#include "gui/widgets/LineEdit.h"
#include "gui/widgets/InputDialog.h"
#include "gui/widgets/Label.h"
#include "gui/application/RosegardenMainWindow.h"
#include "gui/application/RosegardenMainViewWidget.h"
#include "gui/editors/segment/compositionview/CompositionView.h"

#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QFontMetrics>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QString>
#include <QWidget>

#include <algorithm>  // for std::copy()


namespace Rosegarden
{


enum Tristate
{
    None,
    Some,
    All,
    NotApplicable  // no applicable segments selected
};

typedef std::vector<Segment *> SegmentVector;

namespace {
    constexpr int transposeRange = 48;
}

SegmentParameterBox::SegmentParameterBox(QWidget *parent) :
    RosegardenParameterBox(tr("Segment Parameters"), parent),
    m_standardQuantizations(BasicQuantizer::getStandardQuantizations())
{
    setObjectName("Segment Parameter Box");

    // * Label

    QLabel *label = new QLabel(tr("Label"), this);
    label->setFont(m_font);

    m_label = new Label("", this);
    // SPECIAL_LABEL => Gray background, black text.  See ThornStyle.cpp.
    // ??? Can't we just inline that here?
    m_label->setObjectName("SPECIAL_LABEL");
    m_label->setFont(m_font);
    // For horizontal, we expand to fill the layout and ignore the sizeHint()
    // which is driven by the text.  For vertical, we use the sizeHint()
    // which is based on the font.
    m_label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    m_label->setToolTip(tr("<qt>Click to edit the segment label for any selected segments</qt>"));
    connect(m_label, &Label::clicked,
            this, &SegmentParameterBox::slotEditSegmentLabel);

    // * Repeat

    QLabel *repeatLabel = new QLabel(tr("Repeat"), this);
    repeatLabel->setFont(m_font);

    m_repeat = new TristateCheckBox(this);
    m_repeat->setFont(m_font);
    m_repeat->setToolTip(tr("<qt><p>When checked,     any selected segments will repeat until they run into another segment,  "
                                 "or the end of the composition.</p><p>When viewed in the notation editor or printed via LilyPond, "
                                 "the segments will be bracketed by repeat signs.</p><p><center><img src=\":pixmaps/tooltip/repeats"
                                 ".png\"></img></center></p><br>These can be used in conjunction with special LilyPond export direct"
                                 "ives to create repeats with first and second alternate endings. See rosegardenmusic.com for a tut"
                                 "orial. [Ctrl+Shift+R] </qt>"));
    connect(m_repeat, &QCheckBox::clicked,
            this, &SegmentParameterBox::slotRepeatClicked);

    // * Transpose

    QLabel *transposeLabel = new QLabel(tr("Transpose"), this);
    transposeLabel->setFont(m_font);

    m_transpose = new QComboBox(this);
    m_transpose->setFont(m_font);
    m_transpose->setToolTip(tr("<qt><p>Raise or lower playback of any selected segments by this number of semitones</p><p>"
                                    "<i>NOTE: This control changes segments that already exist.</i></p><p><i>Use the transpose "
                                    "control in <b>Track Parameters</b> under <b>Create segments with</b> to pre-select this   "
                                    "setting before drawing or recording new segments.</i></p></qt>"));
    // QComboBox::activated() is overloaded, so we have to use SIGNAL().
    connect(m_transpose, SIGNAL(activated(int)),
            SLOT(slotTransposeSelected(int)));

    QPixmap noMap = NotePixmapFactory::makeToolbarPixmap("menu-no-note");

    for (int i = -transposeRange; i < transposeRange + 1; ++i) {
        m_transpose->addItem(noMap, QString("%1").arg(i));
    }

    // * Quantize

    QLabel *quantizeLabel = new QLabel(tr("Quantize"), this);
    quantizeLabel->setFont(m_font);

    m_quantize = new QComboBox(this);
    m_quantize->setFont(m_font);
    m_quantize->setToolTip(tr(
            "<qt><p>Quantize the selected segments using the Grid quantizer.  "
            "This quantization can be removed at any time in "
            "the future by setting it to off.</p></qt>"));
    // QComboBox::activated() is overloaded, so we have to use SIGNAL().
    connect(m_quantize, SIGNAL(activated(int)),
            SLOT(slotQuantizeSelected(int)));

    // For each standard quantization value
    for (unsigned int i = 0; i < m_standardQuantizations.size(); ++i) {
        timeT time = m_standardQuantizations[i];
        timeT error = 0;
        QString label = NotationStrings::makeNoteMenuLabel(time, true, error);
        QPixmap pmap = NotePixmapFactory::makeNoteMenuPixmap(time, error);
        // Add the icon and label to the ComboBox.
        m_quantize->addItem(error ? noMap : pmap, label);
    }
    m_quantize->addItem(noMap, tr("Off"));

    // * Delay

    QLabel *delayLabel = new QLabel(tr("Delay"), this);
    delayLabel->setFont(m_font);

    m_delay = new QComboBox(this);
    m_delay->setFont(m_font);
    m_delay->setToolTip(tr("<qt><p>Delay playback of any selected segments by this number of miliseconds</p><p><i>NOTE: "
                                "Rosegarden does not support negative delay.  If you need a negative delay effect, set the   "
                                "composition to start before bar 1, and move segments to the left.  You can hold <b>shift</b>"
                                " while doing this for fine-grained control, though doing so will have harsh effects on music"
                                " notation rendering as viewed in the notation editor.</i></p></qt>"));
    // QComboBox::activated() is overloaded, so we have to use SIGNAL().
    connect(m_delay, SIGNAL(activated(int)),
            SLOT(slotDelaySelected(int)));
    // ??? The combobox is not editable.  This will never be called.
    //     It would be a nice feature, though.
    //connect(m_delay, &QComboBox::editTextChanged,
    //        this, &SegmentParameterBox::slotDelayTextChanged);

    m_delays.clear();

    // For each note duration (timeT/ppq) delay
    for (int i = 0; i < 6; i++) {

        // extra range checks below are benign - they account for the
        // option of increasing the range of the loop beyond 0-5

        timeT time = 0;
        if (i > 0 && i < 6) {
            time = Note(Note::Hemidemisemiquaver).getDuration() << (i - 1);
        } else if (i > 5) {
            time = Note(Note::Crotchet).getDuration() * (i - 4);
        }

        m_delays.push_back(time);

        timeT error = 0;
        QString label = NotationStrings::makeNoteMenuLabel(time, true, error);
        QPixmap pmap = NotePixmapFactory::makeNoteMenuPixmap(time, error);

        // check if it's a valid note duration (it will be for the
        // time defn above, but if we were basing it on the sequencer
        // resolution it might not be) & include a note pixmap if so
        m_delay->addItem((error ? noMap : pmap), label);
    }

    // For each real-time delay (msecs)
    for (int i = 0; i < 10; i++) {
        int rtd = (i < 5 ? ((i + 1) * 10) : ((i - 3) * 50));
        m_realTimeDelays.push_back(rtd);
        m_delay->addItem(tr("%1 ms").arg(rtd));
    }

    // * Color

    QLabel *colourLabel = new QLabel(tr("Color"), this);
    colourLabel->setFont(m_font);

    m_color = new ColorCombo(this);
    m_color->setFont(m_font);
    m_color->setToolTip(tr("<qt><p>Change the color of any selected segments</p></qt>"));
    // QComboBox::activated() is overloaded, so we have to use SIGNAL().
    connect(m_color, SIGNAL(activated(int)),
            SLOT(slotColourChanged(int)));
    // slotNewDocument() will finish the initialization.

    // * Linked segment parameters (hidden)

    // Outer collapsing frame
    CollapsingFrame *linkedSegmentParametersFrame = new CollapsingFrame(
            tr("Linked segment parameters"), this, "segmentparameterslinked", false);

    // Unhide this if you want to play with the linked segment
    // transpose parameters.  I've hidden it for the time being until
    // we've decided how we're going to interact with these transpose params.
    linkedSegmentParametersFrame->hide();

    // Inner fixed widget
    QWidget *linkedSegmentParameters = new QWidget(linkedSegmentParametersFrame);
    linkedSegmentParametersFrame->setWidget(linkedSegmentParameters);
    linkedSegmentParameters->setContentsMargins(3, 3, 3, 3);

    // Transpose
    QLabel *linkTransposeLabel = new QLabel(tr("Transpose"), linkedSegmentParameters);
    linkTransposeLabel->setFont(m_font);

    // Change
    QPushButton *changeButton = new QPushButton(tr("Change"), linkedSegmentParameters);
    changeButton->setFont(m_font);
    changeButton->setToolTip(tr("<qt>Edit the relative transposition on the linked segment</qt>"));
    connect(changeButton, &QAbstractButton::released,
            this, &SegmentParameterBox::slotChangeLinkTranspose);

    // Reset
    QPushButton *resetButton = new QPushButton(tr("Reset"), linkedSegmentParameters);
    resetButton->setFont(m_font);
    resetButton->setToolTip(tr("<qt>Reset the relative transposition on the linked segment to zero</qt>"));
    connect(resetButton, &QAbstractButton::released,
            this, &SegmentParameterBox::slotResetLinkTranspose);

    // Linked segment parameters layout

    QGridLayout *groupLayout = new QGridLayout(linkedSegmentParameters);
    groupLayout->setContentsMargins(5,0,0,5);
    groupLayout->setSpacing(2);
    groupLayout->addWidget(linkTransposeLabel, 0, 0, Qt::AlignLeft);
    groupLayout->addWidget(changeButton, 0, 1);
    groupLayout->addWidget(resetButton, 0, 2);
    groupLayout->setColumnStretch(3, 1);

    // SegmentParameterBox Layout

    QGridLayout *gridLayout = new QGridLayout(this);
    gridLayout->setMargin(0);
    gridLayout->setSpacing(2);
    // Row 0: Label
    gridLayout->addWidget(label, 0, 0);
    gridLayout->addWidget(m_label, 0, 1, 1, 5);
    // Row 1: Repeat/Transpose
    gridLayout->addWidget(repeatLabel, 1, 0);
    gridLayout->addWidget(m_repeat, 1, 1);
    gridLayout->addWidget(transposeLabel, 1, 2, 1, 2, Qt::AlignRight);
    gridLayout->addWidget(m_transpose, 1, 4, 1, 2);
    // Row 2: Quantize/Delay
    gridLayout->addWidget(quantizeLabel, 2, 0);
    gridLayout->addWidget(m_quantize, 2, 1, 1, 2);
    gridLayout->addWidget(delayLabel, 2, 3, Qt::AlignRight);
    gridLayout->addWidget(m_delay, 2, 4, 1, 2);
    // Row 3: Color
    gridLayout->addWidget(colourLabel, 3, 0);
    gridLayout->addWidget(m_color, 3, 1, 1, 5);
    // Row 4: Linked segment parameters
    gridLayout->addWidget(linkedSegmentParametersFrame, 4, 0, 1, 5);

    // SegmentParameterBox

    setContentsMargins(4, 7, 4, 4);

    //RG_DEBUG << "ctor: " << this << ": font() size is " << (this->font()).pixelSize() << "px (" << (this->font()).pointSize() << "pt)";

    connect(RosegardenMainWindow::self(),
                &RosegardenMainWindow::documentChanged,
            this, &SegmentParameterBox::slotNewDocument);
}

namespace
{
    SegmentSelection
    getSelectedSegments()
    {
        // Delegates to CompositionModelImpl::getSelectedSegments().

        // ??? COPY
        return RosegardenMainWindow::self()->getView()->
                   getTrackEditor()->getCompositionView()->getModel()->
                   getSelectedSegments();
    }
}

void
SegmentParameterBox::updateLabel()
{
    SegmentSelection segmentSelection = getSelectedSegments();

    // No Segments selected?  Blank.
    if (segmentSelection.empty()) {
        m_label->setEnabled(false);
        m_label->setText("");
        return;
    }

    // One or more Segments selected

    m_label->setEnabled(true);

    SegmentSelection::const_iterator i = segmentSelection.begin();
    QString labelText = QObject::trUtf8((*i)->getLabel().c_str());

    // Just one?  Set and bail.
    if (segmentSelection.size() == 1) {
        m_label->setText(labelText);
        return;
    }

    // More than one Segment selected.

    // Skip to the second Segment.
    ++i;

    bool allSame = true;

    // For each Segment
    for (/* ...starting with the second one */;
         i != segmentSelection.end();
         ++i) {
        // If the labels do not match
        if (QObject::trUtf8((*i)->getLabel().c_str()) != labelText) {
            allSame = false;
            break;
        }
    }

    if (allSame)
        m_label->setText(labelText);
    else
        m_label->setText("*");
}

void
SegmentParameterBox::updateRepeat()
{
    SegmentSelection segmentSelection = getSelectedSegments();

    // No Segments selected?  Disable/uncheck.
    if (segmentSelection.empty()) {
        m_repeat->setEnabled(false);
        m_repeat->setCheckState(Qt::Unchecked);
        return;
    }

    // One or more Segments selected

    m_repeat->setEnabled(true);

    SegmentSelection::const_iterator i = segmentSelection.begin();

    // Just one?  Set and bail.
    if (segmentSelection.size() == 1) {
        m_repeat->setCheckState(
                (*i)->isRepeating() ? Qt::Checked : Qt::Unchecked);
        return;
    }

    // More than one Segment selected.

    std::size_t repeating = 0;

    // For each Segment, count the repeating ones
    for (/* Starting with the first... */;
         i != segmentSelection.end();
         ++i) {
        if ((*i)->isRepeating())
            ++repeating;
    }

    if (repeating == 0)  // none
        m_repeat->setCheckState(Qt::Unchecked);
    else if (repeating == segmentSelection.size())  // all
        m_repeat->setCheckState(Qt::Checked);
    else  // some
        m_repeat->setCheckState(Qt::PartiallyChecked);
}

void
SegmentParameterBox::updateTranspose()
{
    SegmentSelection segmentSelection = getSelectedSegments();

    // No Segments selected?  Disable and set to 0.
    if (segmentSelection.empty()) {
        m_transpose->setEnabled(false);
        m_transpose->setCurrentIndex(m_transpose->findText("0"));
        return;
    }

    // One or more Segments selected

    m_transpose->setEnabled(true);

    SegmentSelection::const_iterator i = segmentSelection.begin();
    int transposeValue = (*i)->getTranspose();

    // Just one?  Set and bail.
    if (segmentSelection.size() == 1) {
        m_transpose->setCurrentIndex(m_transpose->findText(
                QString("%1").arg(transposeValue)));
        return;
    }

    // More than one Segment selected.

    // Skip to the second Segment.
    ++i;

    bool allSame = true;

    // For each Segment
    for (/* ...starting with the second one */;
         i != segmentSelection.end();
         ++i) {
        if ((*i)->getTranspose() != transposeValue) {
            allSame = false;
            break;
        }
    }

    if (allSame) {
        m_transpose->setCurrentIndex(m_transpose->findText(
                QString("%1").arg(transposeValue)));
    } else {
        m_transpose->setCurrentIndex(-1);
    }
}

int
SegmentParameterBox::quantizeIndex(timeT t)
{
    for (unsigned i = 0;
         i < m_standardQuantizations.size();
         ++i) {
        if (m_standardQuantizations[i] == t)
            return i;
    }

    // Nothing?  Return one beyond the last, which is "Off" in the UI.
    return m_standardQuantizations.size();
}

void
SegmentParameterBox::updateQuantize()
{
    SegmentSelection segmentSelection = getSelectedSegments();

    // No Segments selected?  Disable and set to off.
    if (segmentSelection.empty()) {
        m_quantize->setEnabled(false);
        m_quantize->setCurrentIndex(m_quantize->count() - 1);  // Off
        return;
    }

    // One or more Segments selected

    m_quantize->setEnabled(true);

    SegmentSelection::const_iterator i = segmentSelection.begin();
    timeT quantizeValue =
            (*i)->hasQuantization() ?
                    (*i)->getQuantizer()->getUnit() :
                    0;

    // Just one?  Set and bail.
    if (segmentSelection.size() == 1) {
        m_quantize->setCurrentIndex(quantizeIndex(quantizeValue));
        return;
    }

    // More than one Segment selected.

    // Skip to the second Segment.
    ++i;

    bool allSame = true;

    // For each Segment
    for (/* ...starting with the second one */;
         i != segmentSelection.end();
         ++i) {
        timeT quantizeValue2 =
                (*i)->hasQuantization() ?
                        (*i)->getQuantizer()->getUnit() :
                        0;
        // If any of them are different from the first...
        if (quantizeValue2 != quantizeValue) {
            allSame = false;
            break;
        }
    }

    if (allSame)
        m_quantize->setCurrentIndex(quantizeIndex(quantizeValue));
    else
        m_quantize->setCurrentIndex(-1);
}

namespace
{
    // Flatten Segment::getDelay() and getRealTimeDelay() to
    // return a single delay value.  Real-time delay is returned
    // as a negative.
    // ??? This means we can't do negative delays, which could be very
    //     useful.  Might want to redo this using a vector to translate
    //     the value to an index.  Like quantizeIndex().  See m_delays
    //     and m_realTimeDelays which might be combined.  Also see
    //     comments in header on setSegmentDelay().
    long
    delay(Segment *s)
    {
        // Note duration delay (timeT/ppq: 1/4, 1/8, etc...)
        timeT delayValue = s->getDelay();
        if (delayValue != 0)
            return delayValue;

        // Millisecond delay (10ms, 20ms, etc...)
        return -(s->getRealTimeDelay().sec * 1000 +
                 s->getRealTimeDelay().msec());
    }
}

void
SegmentParameterBox::setDelay(long t)
{
    // Note duration delay (timeT/ppq: 1/4, 1/8, etc...)
    if (t >= 0) {
        timeT error = 0;

        QString label =
                NotationStrings::makeNoteMenuLabel(
                        t,  // duration
                        true,  // brief
                        error);  // errorReturn
        m_delay->setCurrentIndex(m_delay->findText(label));

        return;
    }

    // Millisecond delay (10ms, 20ms, etc...)
    m_delay->setCurrentIndex(m_delay->findText(tr("%1 ms").arg(-t)));
}

void
SegmentParameterBox::updateDelay()
{
    SegmentSelection segmentSelection = getSelectedSegments();

    // No Segments selected?  Disable and set to 0.
    if (segmentSelection.empty()) {
        m_delay->setEnabled(false);
        m_delay->setCurrentIndex(m_delay->findText("0"));
        return;
    }

    // One or more Segments selected

    m_delay->setEnabled(true);

    SegmentSelection::const_iterator i = segmentSelection.begin();
    long delayValue = delay(*i);

    // Just one?  Set and bail.
    if (segmentSelection.size() == 1) {
        setDelay(delayValue);
        return;
    }

    // More than one Segment selected.

    // Skip to the second Segment.
    ++i;

    bool allSame = true;

    // For each Segment
    for (/* ...starting with the second one */;
         i != segmentSelection.end();
         ++i) {
        if (delay(*i) != delayValue) {
            allSame = false;
            break;
        }
    }

    if (allSame)
        setDelay(delayValue);
    else
        m_delay->setCurrentIndex(-1);
}

void
SegmentParameterBox::updateColor()
{
    SegmentSelection segmentSelection = getSelectedSegments();

    // No Segments selected?  Disable and set to 0.
    if (segmentSelection.empty()) {
        m_color->setEnabled(false);
        m_color->setCurrentIndex(0);
        return;
    }

    // One or more Segments selected

    m_color->setEnabled(true);

    SegmentSelection::const_iterator i = segmentSelection.begin();

    unsigned colorIndex = (*i)->getColourIndex();

    // Just one?  Set and bail.
    if (segmentSelection.size() == 1) {
        m_color->setCurrentIndex(colorIndex);
        return;
    }

    // More than one Segment selected.

    // Skip to the second Segment.
    ++i;

    bool allSame = true;

    // For each Segment
    for (/* ...starting with the second one */;
         i != segmentSelection.end();
         ++i) {
        if ((*i)->getColourIndex() != colorIndex) {
            allSame = false;
            break;
        }
    }

    if (allSame)
        m_color->setCurrentIndex(colorIndex);
    else
        m_color->setCurrentIndex(-1);
}

void
SegmentParameterBox::updateWidgets()
{
    updateLabel();
    updateRepeat();
    updateTranspose();
    updateQuantize();
    updateDelay();
    updateColor();
}

void
SegmentParameterBox::slotSelectionChanged(
        const SegmentSelection & /*segments*/)
{
    updateWidgets();
}

void
SegmentParameterBox::slotEditSegmentLabel()
{
    SegmentSelection segmentSelection = getSelectedSegments();

    // Nothing selected?  Bail.
    if (segmentSelection.empty())
        return;

    // One or more Segments are selected.

    QString title;

    // This is too simplistic to be translated properly, but I'm leaving it
    // alone.  The right way is to use %n and all that, but we don't want the
    // number to appear in any version of the string, and I don't see a way to
    // handle plurals without a %n placemarker.

    if (segmentSelection.size() == 1)
        title = tr("Modify Segment label");
    else
        title = tr("Modify Segments label");

    bool ok = false;

    QString text = m_label->text();

    // Remove the asterisk if we're using it
    if (text == "*")
        text = "";

    QString newLabel = InputDialog::getText(
            this,  // parent
            title,  // title
            tr("Enter new label:"),  // label
            LineEdit::Normal,  // mode
            text,  // text
            &ok);  // ok

    // Canceled?  Bail.
    if (!ok)
        return;

    CommandHistory::getInstance()->addCommand(
            new SegmentLabelCommand(segmentSelection, newLabel));
}

void
SegmentParameterBox::slotRepeatClicked(bool checked)
{
    SegmentSelection segmentSelection = getSelectedSegments();

    // No selected Segments?  Bail.
    if (segmentSelection.empty())
        return;

    // SegmentCommandRepeat requires a SegmentVector.
    SegmentVector segmentVector(segmentSelection.size());
    std::copy(segmentSelection.begin(),  // source first
              segmentSelection.end(),  // source last
              segmentVector.begin());  // destination first

    CommandHistory::getInstance()->addCommand(
            new SegmentCommandRepeat(segmentVector, checked));
}

void
SegmentParameterBox::slotToggleRepeat()
{
    SegmentSelection segmentSelection = getSelectedSegments();

    if (segmentSelection.empty())
        return;

    // Compute the new state.
    bool state = !(m_repeat->checkState() == Qt::Checked);

    // SegmentCommandRepeat requires a SegmentVector.
    SegmentVector segmentVector(segmentSelection.size());
    std::copy(segmentSelection.begin(),  // source first
              segmentSelection.end(),  // source last
              segmentVector.begin());  // destination first

    CommandHistory::getInstance()->addCommand(
            new SegmentCommandRepeat(segmentVector, state));
}

void
SegmentParameterBox::slotTransposeSelected(int value)
{
    SegmentSelection segmentSelection = getSelectedSegments();

    // No Segments selected?  Bail.
    if (segmentSelection.empty())
        return;

    int transposeValue = value - transposeRange;

    // ??? This would be nice.  It would allow for undo.
    //CommandHistory::getInstance()->addCommand(
    //        new SegmentCommandChangeTransposeValue(
    //                segmentSelection, transposeValue));

    for (SegmentSelection::iterator i = segmentSelection.begin();
         i != segmentSelection.end();
         ++i) {
        (*i)->setTranspose(transposeValue);
    }

    RosegardenMainWindow::self()->getDocument()->slotDocumentModified();
}

void
SegmentParameterBox::slotQuantizeSelected(int qIndex)
{
    timeT quantization = 0;

    // If it's not "Off"...
    if (qIndex != m_quantize->count() - 1)
        quantization = m_standardQuantizations[qIndex];

    SegmentChangeQuantizationCommand *command =
        new SegmentChangeQuantizationCommand(quantization);

    SegmentSelection segmentSelection = getSelectedSegments();

    for (SegmentSelection::iterator i = segmentSelection.begin();
         i != segmentSelection.end();
         ++i) {
        command->addSegment(*i);
    }

    CommandHistory::getInstance()->addCommand(command);
}

void
SegmentParameterBox::setSegmentDelay(long delayValue)
{
    SegmentSelection segmentSelection = getSelectedSegments();

    // ??? This should use a command so that it can be undone.

    // Note duration (timeT/ppq)
    if (delayValue >= 0) {

        for (SegmentSelection::iterator it = segmentSelection.begin();
             it != segmentSelection.end();
             ++it) {
            (*it)->setDelay(delayValue);
            (*it)->setRealTimeDelay(RealTime::zeroTime);
        }

    } else {  // Negative msecs

        for (SegmentSelection::iterator it = segmentSelection.begin();
             it != segmentSelection.end();
             ++it) {
            (*it)->setDelay(0);
            (*it)->setRealTimeDelay(
                    RealTime::fromSeconds(-delayValue / 1000.0));
        }

    }

    RosegardenMainWindow::self()->getDocument()->slotDocumentModified();
}

void
SegmentParameterBox::slotDelaySelected(int value)
{
    // If it's a note-duration (timeT/ppq) delay
    if (value < static_cast<int>(m_delays.size()))
        setSegmentDelay(m_delays[value]);
    else  // It's a millisecond delay
        setSegmentDelay(-(m_realTimeDelays[value - m_delays.size()]));
}

#if 0
void
SegmentParameterBox::slotDelayTextChanged(const QString &text)
{
    // ??? The combobox is not editable.  This will never be called.
    //     It certainly would be nice to be able to set arbitrary
    //     delays, though.  Maybe this should be editable?

    if (text.isEmpty())
        return;

    int delay = text.toInt();

    // Don't allow negatives for now.
    if (delay < 0)
        delay = 0;

    // Negate to treat as msecs.
    setSegmentDelay(-delay);
}
#endif

void
SegmentParameterBox::slotColourChanged(int index)
{
    SegmentSelection segments = getSelectedSegments();
    SegmentColourCommand *command =
            new SegmentColourCommand(segments, index);

    CommandHistory::getInstance()->addCommand(command);

#if 0
// This will never happen since the "Add Color" option is never added.
    if (index == m_addColourPos) {
        ColourMap newMap = RosegardenMainWindow::self()->getDocument()->getComposition().getSegmentColourMap();
        QColor newColour;
        bool ok = false;

        QString newName = InputDialog::getText(this,
                                               tr("New Color Name"),
                                               tr("Enter new name:"),
                                               LineEdit::Normal,
                                               tr("New"), &ok);

        if ((ok == true) && (!newName.isEmpty())) {
//             QColorDialog box(this, "", true);
//             int result = box.getColor(newColour);

            //QRgb QColorDialog::getRgba(0xffffffff, &ok, this);
            QColor newColor = QColorDialog::getColor(Qt::white, this);

            if (newColor.isValid()) {
                Colour newRColour = GUIPalette::convertColour(newColour);
                newMap.addItem(newRColour, qstrtostr(newName));
                SegmentColourMapCommand *command =
                        new SegmentColourMapCommand(RosegardenMainWindow::self()->getDocument(), newMap);
                CommandHistory::getInstance()->addCommand(command);
                slotDocColoursChanged();
            }
        }
        // Else we don't do anything as they either didn't give a name·
        // or didn't give a colour
    }
#endif
}

void
SegmentParameterBox::slotDocColoursChanged()
{
    // The color combobox is handled differently from the others.  Since
    // there are 420 strings of up to 25 chars in here, it would be
    // expensive to detect changes by comparing vectors of strings.

    // For now, we'll handle the document colors changed notification
    // and reload the combobox then.

    // See the comments on RosegardenDocument::docColoursChanged()
    // in RosegardenDocument.h.

    // Note that as of this writing (June 2019) there is no way
    // to modify the document colors.  See ColourConfigurationPage
    // which was probably meant to be used by DocumentConfigureDialog.
    // See TrackParameterBox::slotDocColoursChanged().

    m_color->updateColors();
    m_color->setCurrentIndex(0);
}

void
SegmentParameterBox::slotChangeLinkTranspose()
{
    SegmentSelection segments = getSelectedSegments();

    // No Segments selected?  Bail.
    if (segments.empty())
        return;

    bool foundTransposedLinks = false;
    SegmentVector linkedSegs;
    for (SegmentSelection::iterator it = segments.begin();
         it != segments.end();
         ++it) {
        Segment *linkedSeg = *it;
        if (linkedSeg->isLinked()) {
            if (linkedSeg->getLinkTransposeParams().m_semitones == 0) {
                linkedSegs.push_back(linkedSeg);
            } else {
                foundTransposedLinks = true;
                break;
            }
        }
    }
    
    if (foundTransposedLinks) {
        QMessageBox::critical(this, tr("Rosegarden"), 
                tr("Existing transpositions on selected linked segments must be removed\nbefore new transposition can be applied."),
                QMessageBox::Ok);
        return;
    }
        
    if (linkedSegs.empty())
        return;
    
    IntervalDialog intervalDialog(this, true, true);
    int ok = intervalDialog.exec();
    
    if (!ok)
        return;

    CommandHistory::getInstance()->addCommand(
            new SegmentLinkTransposeCommand(
                    linkedSegs,  // linkedSegs
                    intervalDialog.getChangeKey(),  // changeKey
                    intervalDialog.getDiatonicDistance(),  // steps
                    intervalDialog.getChromaticDistance(),  // semitones
                    intervalDialog.getTransposeSegmentBack()));  // transposeSegmentBack
}

void
SegmentParameterBox::slotResetLinkTranspose()
{
    SegmentSelection segments = getSelectedSegments();

    // Nothing selected?  Bail.
    if (segments.empty())
        return;

    SegmentVector linkedSegs;
    for (SegmentSelection::iterator it = segments.begin();
         it != segments.end();
         ++it) {
        Segment *linkedSeg = *it;

        if (linkedSeg->isLinked())
            linkedSegs.push_back(linkedSeg);
    }

    if (linkedSegs.empty())
        return;

    int reset = QMessageBox::question(this, tr("Rosegarden"), 
                   tr("Remove transposition on selected linked segments?"));

    if (reset == QMessageBox::No)
        return;

    CommandHistory::getInstance()->addCommand(
            new SegmentLinkResetTransposeCommand(linkedSegs));
}

void
SegmentParameterBox::slotNewDocument(RosegardenDocument *doc)
{
    connect(doc, &RosegardenDocument::documentModified,
            this, &SegmentParameterBox::slotDocumentModified);

    // Connect to RosegardenMainViewWidget for selection change.
    connect(RosegardenMainWindow::self()->getView(),
                &RosegardenMainViewWidget::segmentsSelected,
            this, &SegmentParameterBox::slotSelectionChanged);

    // Detect when the document colours are updated.
    connect (doc, &RosegardenDocument::docColoursChanged,
             this, &SegmentParameterBox::slotDocColoursChanged);

    // Repopulate color combo.
    slotDocColoursChanged();

    // Make sure everything is correct.
    updateWidgets();
}

void SegmentParameterBox::slotDocumentModified(bool)
{
    updateWidgets();
}


}
