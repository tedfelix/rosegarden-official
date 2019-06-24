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


namespace Rosegarden
{


enum Tristate
{
    None,
    Some,
    All,
    NotApplicable  // no applicable segments selected
};

SegmentParameterBox::SegmentParameterBox(RosegardenDocument* doc,
                                         QWidget *parent) :
    RosegardenParameterBox(tr("Segment Parameters"), parent),
    m_doc(doc),
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
    QFontMetrics fontMetrics(m_font);
    const int width20 = fontMetrics.width("12345678901234567890");
    m_label->setFixedWidth(width20);
    m_label->setToolTip(tr("<qt>Click to edit the segment label for any selected segments</qt>"));
    connect(m_label, &Label::clicked,
            this, &SegmentParameterBox::slotEditSegmentLabel);

    // * Edit button

    // ??? This Edit button is now no longer needed.  The user can just
    //     click on the label to edit it.
    m_editButton = new QPushButton(tr("Edit"), this);
    m_editButton->setFont(m_font);
    m_editButton->setToolTip(tr("<qt>Edit the segment label for any selected segments</qt>"));
    connect(m_editButton, &QAbstractButton::released,
            this, &SegmentParameterBox::slotEditSegmentLabel);

    // * Repeat

    QLabel *repeatLabel = new QLabel(tr("Repeat"), this);
    repeatLabel->setFont(m_font);

    m_repeatCheckBox = new TristateCheckBox(this);
    m_repeatCheckBox->setFont(m_font);
    m_repeatCheckBox->setToolTip(tr("<qt><p>When checked,     any selected segments will repeat until they run into another segment,  "
                                 "or the end of the composition.</p><p>When viewed in the notation editor or printed via LilyPond, "
                                 "the segments will be bracketed by repeat signs.</p><p><center><img src=\":pixmaps/tooltip/repeats"
                                 ".png\"></img></center></p><br>These can be used in conjunction with special LilyPond export direct"
                                 "ives to create repeats with first and second alternate endings. See rosegardenmusic.com for a tut"
                                 "orial. [Ctrl+Shift+R] </qt>"));
    connect(m_repeatCheckBox, &QAbstractButton::pressed,
            this, &SegmentParameterBox::slotRepeatPressed);

    // * Transpose

    QLabel *transposeLabel = new QLabel(tr("Transpose"), this);
    transposeLabel->setFont(m_font);

    m_transposeComboBox = new QComboBox(this);
    m_transposeComboBox->setFont(m_font);
    m_transposeComboBox->setToolTip(tr("<qt><p>Raise or lower playback of any selected segments by this number of semitones</p><p>"
                                    "<i>NOTE: This control changes segments that already exist.</i></p><p><i>Use the transpose "
                                    "control in <b>Track Parameters</b> under <b>Create segments with</b> to pre-select this   "
                                    "setting before drawing or recording new segments.</i></p></qt>"));
    // ??? QComboBox::activated() is overloaded, so we have to use SIGNAL().
    connect(m_transposeComboBox, SIGNAL(activated(int)),
            SLOT(slotTransposeSelected(int)));
    connect(m_transposeComboBox, &QComboBox::editTextChanged,
            this, &SegmentParameterBox::slotTransposeTextChanged);

    QPixmap noMap = NotePixmapFactory::makeToolbarPixmap("menu-no-note");

    constexpr int transposeRange = 48;

    for (int i = -transposeRange; i < transposeRange + 1; ++i) {
        m_transposeComboBox->addItem(noMap, QString("%1").arg(i));
    }

    // * Quantize

    QLabel *quantizeLabel = new QLabel(tr("Quantize"), this);
    quantizeLabel->setFont(m_font);

    m_quantizeComboBox = new QComboBox(this);
    m_quantizeComboBox->setFont(m_font);
    m_quantizeComboBox->setToolTip(tr(
            "<qt><p>Quantize the selected segments using the Grid quantizer.  "
            "This quantization can be removed at any time in "
            "the future by setting it to off.</p></qt>"));
    // ??? QComboBox::activated() is overloaded, so we have to use SIGNAL().
    connect(m_quantizeComboBox, SIGNAL(activated(int)),
            SLOT(slotQuantizeSelected(int)));

    // For each standard quantization value
    for (unsigned int i = 0; i < m_standardQuantizations.size(); ++i) {
        timeT time = m_standardQuantizations[i];
        timeT error = 0;
        QString label = NotationStrings::makeNoteMenuLabel(time, true, error);
        QPixmap pmap = NotePixmapFactory::makeNoteMenuPixmap(time, error);
        // Add the icon and label to the ComboBox.
        m_quantizeComboBox->addItem(error ? noMap : pmap, label);
    }
    m_quantizeComboBox->addItem(noMap, tr("Off"));

    // * Delay

    QLabel *delayLabel = new QLabel(tr("Delay"), this);
    delayLabel->setFont(m_font);

    m_delayComboBox = new QComboBox(this);
    m_delayComboBox->setFont(m_font);
    m_delayComboBox->setToolTip(tr("<qt><p>Delay playback of any selected segments by this number of miliseconds</p><p><i>NOTE: "
                                "Rosegarden does not support negative delay.  If you need a negative delay effect, set the   "
                                "composition to start before bar 1, and move segments to the left.  You can hold <b>shift</b>"
                                " while doing this for fine-grained control, though doing so will have harsh effects on music"
                                " notation rendering as viewed in the notation editor.</i></p></qt>"));
    // ??? QComboBox::activated() is overloaded, so we have to use SIGNAL().
    connect(m_delayComboBox, SIGNAL(activated(int)),
            SLOT(slotDelaySelected(int)));
    connect(m_delayComboBox, &QComboBox::editTextChanged,
            this, &SegmentParameterBox::slotDelayTextChanged);

    m_delays.clear();

    // For each note duration delay
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
        m_delayComboBox->addItem((error ? noMap : pmap), label);
    }

    // For each real-time delay (msecs)
    for (int i = 0; i < 10; i++) {
        int rtd = (i < 5 ? ((i + 1) * 10) : ((i - 3) * 50));
        m_realTimeDelays.push_back(rtd);
        m_delayComboBox->addItem(tr("%1 ms").arg(rtd));
    }

    // * Color

    QLabel *colourLabel = new QLabel(tr("Color"), this);
    colourLabel->setFont(m_font);

    m_color = new QComboBox(this);
    m_color->setEditable(false);
    m_color->setFont(m_font);
    m_color->setToolTip(tr("<qt><p>Change the color of any selected segments</p></qt>"));
    m_color->setMaxVisibleItems(20);
    connect(m_color, SIGNAL(activated(int)),
            SLOT(slotColourChanged(int)));

    connect(m_doc, &RosegardenDocument::docColoursChanged,
            this, &SegmentParameterBox::slotDocColoursChanged);
    // Populate the colours.
    slotDocColoursChanged();

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
    gridLayout->addWidget(m_label, 0, 1, 1, 4);
    gridLayout->addWidget(m_editButton, 0, 5);
    // Row 1: Repeat/Transpose
    gridLayout->addWidget(repeatLabel, 1, 0);
    gridLayout->addWidget(m_repeatCheckBox, 1, 1);
    gridLayout->addWidget(transposeLabel, 1, 2, 1, 2, Qt::AlignRight);
    gridLayout->addWidget(m_transposeComboBox, 1, 4, 1, 2);
    // Row 2: Quantize/Delay
    gridLayout->addWidget(quantizeLabel, 2, 0);
    gridLayout->addWidget(m_quantizeComboBox, 2, 1, 1, 2);
    gridLayout->addWidget(delayLabel, 2, 3, Qt::AlignRight);
    gridLayout->addWidget(m_delayComboBox, 2, 4, 1, 2);
    // Row 3: Color
    gridLayout->addWidget(colourLabel, 3, 0);
    gridLayout->addWidget(m_color, 3, 1, 1, 5);
    // Row 4: Linked segment parameters
    gridLayout->addWidget(linkedSegmentParametersFrame, 4, 0, 1, 5);

    // SegmentParameterBox

    setContentsMargins(4, 7, 4, 4);

    //RG_DEBUG << "ctor: " << this << ": font() size is " << (this->font()).pixelSize() << "px (" << (this->font()).pointSize() << "pt)";

    m_doc->getComposition().addObserver(this);

    connect(RosegardenMainWindow::self(),
                &RosegardenMainWindow::documentChanged,
            this, &SegmentParameterBox::slotNewDocument);

    // ??? commandExecuted() is overloaded so we must use SLOT().
    //     Rename to commandExecutedOrUn().
    // ??? We should subscribe for documentModified instead of this.
    connect(CommandHistory::getInstance(), SIGNAL(commandExecuted()),
            this, SLOT(slotUpdate()));
}

SegmentParameterBox::~SegmentParameterBox()
{
    if (!isCompositionDeleted()) {
        RosegardenMainWindow::self()->getDocument()->
            getComposition().removeObserver(this);
    }
}

void
SegmentParameterBox::setDocument(RosegardenDocument *doc)
{
    if (m_doc) {
        disconnect(m_doc, &RosegardenDocument::docColoursChanged,
                   this, &SegmentParameterBox::slotDocColoursChanged);
    }

    m_doc = doc;

    // Detect when the document colours are updated
    connect (m_doc, &RosegardenDocument::docColoursChanged,
             this, &SegmentParameterBox::slotDocColoursChanged);

    // repopulate combo
    slotDocColoursChanged();
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
SegmentParameterBox::useSegments(const SegmentSelection &segments)
{
    // ??? Switch this push approach to a pull approach.
    //     Use getSelectedSegments() and get rid of this.

    // Copy from segments which is a std::set to m_segments which
    // is a std::vector.
    m_segments.clear();
    m_segments.resize(segments.size());
    std::copy(segments.begin(), segments.end(), m_segments.begin());

    populateBoxFromSegments();
}

void
SegmentParameterBox::slotDocColoursChanged()
{
    RG_DEBUG << "slotDocColoursChanged()";

    // Note that as of this writing (June 2019) there is no way
    // to modify the document colors.  See ColourConfigurationPage
    // which was probably meant to be used by DocumentConfigureDialog.
    // See TrackParameterBox::slotDocColoursChanged().
    // ??? Probably should combine this and the TPB version into a ColorCombo
    //     class derived from QComboBox.

    m_color->clear();

    // ??? TrackParameterBox doesn't have this.  Can we get rid of it?
    m_colourList.clear();

    // Populate it from Composition::m_segmentColourMap
    ColourMap temp = m_doc->getComposition().getSegmentColourMap();

    // ??? TrackParameterBox doesn't have this.  Can we get rid of it?
    unsigned i = 0;

    // For each color in the segment color map
    for (RCMap::const_iterator colourIter = temp.begin();
         colourIter != temp.end();
         ++colourIter) {
        // Wrap in a tr() call in case the color is on the list of translated
        // color names we're including since 09.10.
        QString colourName(QObject::tr(colourIter->second.second.c_str()));

        QPixmap colourIcon(15, 15);
        colourIcon.fill(GUIPalette::convertColour(colourIter->second.first));

        if (colourName == "") {
            m_color->addItem(colourIcon, tr("Default"), i);
        } else {
            // truncate name to 25 characters to avoid the combo forcing the
            // whole kit and kaboodle too wide (This expands from 15 because the
            // translators wrote books instead of copying the style of
            // TheShortEnglishNames, and because we have that much room to
            // spare.)
            if (colourName.length() > 25)
                colourName = colourName.left(22) + "...";

            m_color->addItem(colourIcon, colourName, i);
        }
        m_colourList[colourIter->first] = i; // maps colour number to menu index
        ++i;
    }

    m_addColourPos = i;
    m_color->addItem(tr("Add New Color"), m_addColourPos);
    
    // remove the item we just inserted; this leaves the translation alone, but
    // eliminates the useless option
    //
    //!!! fix after release
    m_color->removeItem(m_addColourPos);

    m_color->setCurrentIndex(0);
}

void SegmentParameterBox::slotUpdate()
{
    RG_DEBUG << "slotUpdate()";

    populateBoxFromSegments();
}

void
SegmentParameterBox::segmentRemoved(const Composition *composition,
                                    Segment *segment)
{
    RG_DEBUG << "segmentRemoved()...";

    // Not our composition?  Bail.
    if (composition != &m_doc->getComposition()) {
        RG_DEBUG << "segmentRemoved(): received a delete for the wrong Composition";
        return;
    }

    // For each Segment that we are displaying...
    for (SegmentVector::const_iterator it =
             m_segments.begin();
         it != m_segments.end();
         ++it) {

        // If we found the segment in question, delete it from our list.
        if (*it == segment) {
            RG_DEBUG << "segmentRemoved(): found the segment to remove";
            m_segments.erase(it);
            return;
        }

    }

    // ??? We don't slotUpdate() here, so we still show old data.
}

void
SegmentParameterBox::populateBoxFromSegments()
{
    SegmentVector::iterator it;
    Tristate repeated = NotApplicable;
    Tristate quantized = NotApplicable;
    Tristate transposed = NotApplicable;
    Tristate delayed = NotApplicable;
    Tristate diffcolours = NotApplicable;
    Tristate highlow = NotApplicable;
    unsigned int myCol = 0;
    unsigned int myHigh = 127;
    unsigned int myLow = 0;

    timeT qntzLevel = 0;
    // At the moment we have no negative delay, so we use negative
    // values to represent real-time delay in ms
    timeT delayLevel = 0;
    int transposeLevel = 0;

    if (m_segments.size() == 0)
        m_label->setText("");
    else 
        m_label->setText(QObject::trUtf8(m_segments[0]->getLabel().c_str()));

    // I never noticed this after all this time, but it seems to go all the way
    // back to the "..." button that this was never disabled if there was no
    // segment, and therefore no label to edit.  So we disable the edit button
    // and repeat checkbox first:
    m_editButton->setEnabled(false);
    m_repeatCheckBox->setEnabled(false);


    for (it = m_segments.begin(); it != m_segments.end(); ++it) {
        // ok, first thing is we know we have at least one segment
        //
        // and since there is at least one segment, we can re-enable the edit button
        // and repeat checkbox:
        m_editButton->setEnabled(true);
        m_repeatCheckBox->setEnabled(true);

        if (repeated == NotApplicable)
            repeated = None;
        if (quantized == NotApplicable)
            quantized = None;
        if (transposed == NotApplicable)
            transposed = None;
        if (delayed == NotApplicable)
            delayed = None;
        if (diffcolours == NotApplicable)
            diffcolours = None;
        if (highlow == NotApplicable)
            highlow = None;

        // Set label to "*" when multiple labels don't match
        //
        if (QObject::trUtf8((*it)->getLabel().c_str()) != m_label->text())
            m_label->setText("*");

        // Are all, some or none of the Segments repeating?
        if ((*it)->isRepeating()) {
            if (it == m_segments.begin())
                repeated = All;
            else {
                if (repeated == None)
                    repeated = Some;
            }
        } else {
            if (repeated == All)
                repeated = Some;
        }

        // Quantization
        //
        if ((*it)->hasQuantization()) {
            if (it == m_segments.begin()) {
                quantized = All;
                qntzLevel = (*it)->getQuantizer()->getUnit();
            } else {
                // If quantize levels don't match
                if (quantized == None ||
                        (quantized == All &&
                         qntzLevel !=
                         (*it)->getQuantizer()->getUnit()))
                    quantized = Some;
            }
        } else {
            if (quantized == All)
                quantized = Some;
        }

        // Transpose
        //
        if ((*it)->getTranspose() != 0) {
            if (it == m_segments.begin()) {
                transposed = All;
                transposeLevel = (*it)->getTranspose();
            } else {
                if (transposed == None ||
                        (transposed == All &&
                         transposeLevel != (*it)->getTranspose()))
                    transposed = Some;
            }

        } else {
            if (transposed == All)
                transposed = Some;
        }

        // Delay
        //
        timeT myDelay = (*it)->getDelay();
        if (myDelay == 0) {
            myDelay = -((*it)->getRealTimeDelay().sec * 1000 +
                        (*it)->getRealTimeDelay().msec());
        }

        if (myDelay != 0) {
            if (it == m_segments.begin()) {
                delayed = All;
                delayLevel = myDelay;
            } else {
                if (delayed == None ||
                        (delayed == All &&
                         delayLevel != myDelay))
                    delayed = Some;
            }
        } else {
            if (delayed == All)
                delayed = Some;
        }

        // Colour

        if (it == m_segments.begin()) {
            myCol = (*it)->getColourIndex();
        } else {
            //!!! the following if statement had been empty since who knows
            // when, and made no logical sense, so let's see if this is what the
            // original coder was trying to say here:
            if (myCol != (*it)->getColourIndex()) diffcolours = All;
        }

        // Highest/Lowest playable
        //
        if (it == m_segments.begin()) {
            myHigh = (unsigned int)(*it)->getHighestPlayable();
            myLow = (unsigned int)(*it)->getLowestPlayable();
        } else {
            if (myHigh != (unsigned int)(*it)->getHighestPlayable() ||
                myLow != (unsigned int)(*it)->getLowestPlayable()) {
                highlow = All;
            }
        }

    }

    switch (repeated) {
    case All:
        m_repeatCheckBox->setChecked(true);
        break;

    case Some:
        m_repeatCheckBox->setCheckState(Qt::PartiallyChecked);
        break;

    case None:
    case NotApplicable:
    default:
        m_repeatCheckBox->setChecked(false);
        break;
    }

    m_repeatCheckBox->setEnabled(repeated != NotApplicable);

    switch (quantized) {
    case All: {
            for (unsigned int i = 0;
                    i < m_standardQuantizations.size(); ++i) {
                if (m_standardQuantizations[i] == qntzLevel) {
                    m_quantizeComboBox->setCurrentIndex(i);
                    break;
                }
            }
        }
        break;

    case Some:
        // Set the edit text to an unfeasible blank value meaning "Some"
        //
        m_quantizeComboBox->setCurrentIndex( -1);
        break;

        // Assuming "Off" is always the last field
    case None:
    case NotApplicable:
    default:
        m_quantizeComboBox->setCurrentIndex(m_quantizeComboBox->count() - 1);
        break;
    }

    m_quantizeComboBox->setEnabled(quantized != NotApplicable);

    switch (transposed) {
    case All:
          m_transposeComboBox->setCurrentIndex(m_transposeComboBox->findText(QString("%1").arg(transposeLevel)));
          break;

    case Some:
          m_transposeComboBox->setCurrentIndex(m_transposeComboBox->findText(QString("")));
          break;

    case None:
    case NotApplicable:
    default:
          m_transposeComboBox->setCurrentIndex(m_transposeComboBox->findText(QString("0")));
          break;
    }

    m_transposeComboBox->setEnabled(transposed != NotApplicable);

    m_delayComboBox->blockSignals(true);

    switch (delayed) {
    case All:
        if (delayLevel >= 0) {
            timeT error = 0;
            QString label = NotationStrings::makeNoteMenuLabel(delayLevel,
                            true,
                            error);
               m_delayComboBox->setCurrentIndex(m_delayComboBox->findText(label));

        } else if (delayLevel < 0) {

               m_delayComboBox->setCurrentIndex(m_delayComboBox->findText( tr("%1 ms").arg(-delayLevel) ));
          }

        break;

    case Some:
          m_delayComboBox->setCurrentIndex(m_delayComboBox->findText(QString("")));
          break;

    case None:
    case NotApplicable:
    default:
        m_delayComboBox->setCurrentIndex(m_delayComboBox->findText(QString("0")));
        break;
    }

    m_delayComboBox->setEnabled(delayed != NotApplicable);

    m_delayComboBox->blockSignals(false);

    switch (diffcolours) {
    case None:
        if (m_colourList.find(myCol) != m_colourList.end())
            m_color->setCurrentIndex(m_colourList[myCol]);
        else
            m_color->setCurrentIndex(0);
        break;


    case All:
    case NotApplicable:
    case Some:
    default:
        m_color->setCurrentIndex(0);
        break;

    }

    m_color->setEnabled(diffcolours != NotApplicable);

    // deleted a large amount of "fix after 1.3" cruft from this spot
}

void SegmentParameterBox::slotRepeatPressed()
{
    if (m_segments.size() == 0)
        return ;

    bool state = false;

    switch (m_repeatCheckBox->checkState()) {
    case Qt::Unchecked:
        state = true;
        break;

    case Qt::PartiallyChecked:
    case Qt::Checked:
    default:
        state = false;
        break;
    }

    // update the check box and all current Segments
    m_repeatCheckBox->setChecked(state);

    CommandHistory::getInstance()->addCommand(
            new SegmentCommandRepeat(m_segments, state));

    //     SegmentVector::iterator it;

    //     for (it = m_segments.begin(); it != m_segments.end(); it++)
    //         (*it)->setRepeating(state);
}

void
SegmentParameterBox::slotQuantizeSelected(int qLevel)
{
    bool off = (qLevel == m_quantizeComboBox->count() - 1);

    SegmentChangeQuantizationCommand *command =
        new SegmentChangeQuantizationCommand
        (off ? 0 : m_standardQuantizations[qLevel]);

    SegmentVector::iterator it;
    for (it = m_segments.begin(); it != m_segments.end(); ++it) {
        command->addSegment(*it);
    }

    CommandHistory::getInstance()->addCommand(command);
}

void
SegmentParameterBox::slotTransposeTextChanged(const QString &text)
{
    if (text.isEmpty() || m_segments.size() == 0)
        return ;

    int transposeValue = text.toInt();

    //CommandHistory::getInstance()->addCommand(
    //        new SegmentCommandChangeTransposeValue(m_segments, transposeValue));

    SegmentVector::iterator it;
    for (it = m_segments.begin(); it != m_segments.end(); ++it) {
        (*it)->setTranspose(transposeValue);
    }

    emit documentModified();
}

void
SegmentParameterBox::slotTransposeSelected(int value)
{
    slotTransposeTextChanged(m_transposeComboBox->itemText(value));
}

void
SegmentParameterBox::slotChangeLinkTranspose()
{
    if (m_segments.size() == 0)
        return ;

    bool foundTransposedLinks = false;
    SegmentVector linkedSegs;
    SegmentVector::iterator it;
    for (it = m_segments.begin(); it != m_segments.end(); ++it) {
        Segment *linkedSeg = *it;
        if (linkedSeg->isLinked()) {
            if (linkedSeg->getLinkTransposeParams().m_semitones==0) {
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
        
    if (linkedSegs.size()==0) {
        return;
    }
    
    IntervalDialog intervalDialog(this, true, true);
    int ok = intervalDialog.exec();
    
    if (!ok) {
        return;
    }

    bool changeKey = intervalDialog.getChangeKey();
    int steps = intervalDialog.getDiatonicDistance();
    int semitones = intervalDialog.getChromaticDistance();
    bool transposeSegmentBack = intervalDialog.getTransposeSegmentBack();
     
    CommandHistory::getInstance()->addCommand
        (new SegmentLinkTransposeCommand(linkedSegs, changeKey, steps, 
                                         semitones, transposeSegmentBack));
}

void
SegmentParameterBox::slotResetLinkTranspose()
{
    if (m_segments.size() == 0)
        return ;

    SegmentVector linkedSegs;
    SegmentVector::iterator it;
    for (it = m_segments.begin(); it != m_segments.end(); ++it) {
        Segment *linkedSeg = *it;
        if (linkedSeg->isLinked()) {
            linkedSegs.push_back(linkedSeg);
        }
    }

    if (linkedSegs.size() == 0) {
        return;
    }

    int reset = QMessageBox::question(this, tr("Rosegarden"), 
                   tr("Remove transposition on selected linked segments?"));

    if (reset == QMessageBox::No) {
        return ;
    }

    CommandHistory::getInstance()->addCommand
        (new SegmentLinkResetTransposeCommand(linkedSegs));
}

void
SegmentParameterBox::slotDelayTimeChanged(timeT delayValue)
{
    // by convention and as a nasty hack, we use negative timeT here
    // to represent positive RealTime in ms

    if (delayValue > 0) {

        SegmentVector::iterator it;
        for (it = m_segments.begin(); it != m_segments.end(); ++it) {
            (*it)->setDelay(delayValue);
            (*it)->setRealTimeDelay(RealTime::zeroTime);
        }

    } else if (delayValue < 0) {

        SegmentVector::iterator it;
        for (it = m_segments.begin(); it != m_segments.end(); ++it) {
            (*it)->setDelay(0);
            int sec = ( -delayValue) / 1000;
            int nsec = (( -delayValue) - 1000 * sec) * 1000000;
            (*it)->setRealTimeDelay(RealTime(sec, nsec));
        }
    } else {

        SegmentVector::iterator it;
        for (it = m_segments.begin(); it != m_segments.end(); ++it) {
            (*it)->setDelay(0);
            (*it)->setRealTimeDelay(RealTime::zeroTime);
        }
    }

    emit documentModified();
}

void
SegmentParameterBox::slotDelayTextChanged(const QString &text)
{
    if (text.isEmpty() || m_segments.size() == 0)
        return ;

    slotDelayTimeChanged( -(text.toInt()));
}

void
SegmentParameterBox::slotDelaySelected(int value)
{
    if (value < int(m_delays.size())) {
        slotDelayTimeChanged(m_delays[value]);
    } else {
        slotDelayTimeChanged( -(m_realTimeDelays[value - m_delays.size()]));
    }
}

void
SegmentParameterBox::slotColourChanged(int index)
{
    unsigned int colorIndex = 0;

    ColourTable::ColourList::const_iterator pos;
    for (pos = m_colourList.begin(); pos != m_colourList.end(); ++pos) {
        if (int(pos->second) == index) {
            colorIndex = pos->first;
            break;
        }
    }

    SegmentSelection segments = getSelectedSegments();
    SegmentColourCommand *command =
            new SegmentColourCommand(segments, colorIndex);

    CommandHistory::getInstance()->addCommand(command);

#if 0
// This will never happen since the "Add Color" option is never added.
    if (index == m_addColourPos) {
        ColourMap newMap = m_doc->getComposition().getSegmentColourMap();
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
                        new SegmentColourMapCommand(m_doc, newMap);
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
SegmentParameterBox::slotEditSegmentLabel()
{
    QString editLabel;

    //!!!  This is too simplistic to be translated properly, but I'm leaving it
    // alone.  The right way is to use %n and all that, but we don't want the
    // number to appear in any version of the string, and I don't see a way to
    // handle plurals without a %n placemarker.
    if (m_segments.size() == 0) return;
    else if (m_segments.size() == 1) editLabel = tr("Modify Segment label");
    else editLabel = tr("Modify Segments label");

    bool ok = false;

    // Remove the asterisk if we're using it
    //
    QString label = m_label->text();
    if (label == "*")
        label = "";

    QString newLabel = InputDialog::getText(this, 
                                            editLabel,
                                            tr("Enter new label:"),
                                            LineEdit::Normal,
                                            m_label->text(),
                                            &ok);

    if (ok) {
        SegmentSelection segments;
        SegmentVector::iterator it;
        for (it = m_segments.begin(); it != m_segments.end(); ++it)
            segments.insert(*it);

        SegmentLabelCommand *command = new
                                       SegmentLabelCommand(segments, newLabel);

        CommandHistory::getInstance()->addCommand(command);

        // fix #1776915, maybe?
        slotUpdate();
    }
}

void
SegmentParameterBox::slotNewDocument(RosegardenDocument *doc)
{
    // Connect to the new document.
    m_doc = doc;
    m_doc->getComposition().addObserver(this);

    // Make sure everything is correct.
    slotUpdate();
}


}
