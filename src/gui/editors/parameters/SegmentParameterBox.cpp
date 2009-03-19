/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "SegmentParameterBox.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "document/ConfigGroups.h"
#include "base/Colour.h"
#include "base/ColourMap.h"
#include "base/Composition.h"
#include "base/MidiProgram.h"
#include "base/NotationTypes.h"
#include "base/BasicQuantizer.h"
#include "base/RealTime.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "commands/segment/SegmentChangeQuantizationCommand.h"
#include "commands/segment/SegmentColourCommand.h"
#include "commands/segment/SegmentColourMapCommand.h"
#include "commands/segment/SegmentCommandRepeat.h"
#include "commands/segment/SegmentLabelCommand.h"
#include "document/CommandHistory.h"
#include "document/RosegardenDocument.h"
#include "gui/dialogs/PitchPickerDialog.h"
#include "gui/editors/notation/NotationStrings.h"
#include "gui/editors/notation/NotePixmapFactory.h"
#include "gui/general/GUIPalette.h"
#include "gui/widgets/ColourTable.h"
#include "gui/widgets/TristateCheckBox.h"
#include "RosegardenParameterArea.h"
#include "RosegardenParameterBox.h"
#include "document/Command.h"
#include "gui/widgets/LineEdit.h"
#include "gui/widgets/InputDialog.h"

#include <QColorDialog>
#include <QLayout>
#include <QApplication>
#include <QComboBox>
#include <QSettings>
#include <QTabWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QColor>
#include <QDialog>
#include <QFont>
#include <QFontMetrics>
#include <QFrame>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QString>
#include <QToolTip>
#include <QWidget>
#include <QLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStackedWidget>
//#include <QLineEdit>
//#include <QInputDialog>


namespace Rosegarden
{

SegmentParameterBox::SegmentParameterBox(RosegardenDocument* doc,
        QWidget *parent)
        : RosegardenParameterBox(tr("Segment"),
                                 tr("Segment Parameters"),
                                 parent),
        m_highestPlayable(127),
        m_lowestPlayable(0),
        m_standardQuantizations(BasicQuantizer::getStandardQuantizations()),
        m_doc(doc),
        m_transposeRange(48)
{
    setObjectName("Segment Parameter Box");

    initBox();

    m_doc->getComposition().addObserver(this);

    connect(CommandHistory::getInstance(), SIGNAL(commandExecuted()),
            this, SLOT(update()));
}

SegmentParameterBox::~SegmentParameterBox()
{
    if (!isCompositionDeleted()) {
        m_doc->getComposition().removeObserver(this);
    }
}

void
SegmentParameterBox::initBox()
{
    QFont font(m_font);

    QFontMetrics fontMetrics(font);
    // magic numbers: 13 is the height of the menu pixmaps, 10 is just 10
    //int comboHeight = std::max(fontMetrics.height(), 13) + 10;
    int width = fontMetrics.width("12345678901234567890");

    //    QFrame *frame = new QFrame(this);
    setContentsMargins(4, 4, 4, 4);
    QGridLayout *gridLayout = new QGridLayout(this);
    gridLayout->setMargin(0);
    gridLayout->setSpacing(2);

    QLabel *label     = new QLabel(tr("Label"), this);
    QLabel *repeatLabel = new QLabel(tr("Repeat"), this);
    QLabel *quantizeLabel = new QLabel(tr("Quantize"), this);
    QLabel *transposeLabel = new QLabel(tr("Transpose"), this);
    QLabel *delayLabel = new QLabel(tr("Delay"), this);
    QLabel *colourLabel = new QLabel(tr("Color"), this);
//    m_autoFadeLabel = new QLabel(tr("Audio auto-fade"), this);
//    m_fadeInLabel = new QLabel(tr("Fade in"), this);
//    m_fadeOutLabel = new QLabel(tr("Fade out"), this);
//    m_rangeLabel = new QLabel(tr("Range"), this);

    // Label ..
    m_label = new QLabel(this);
    m_label->setObjectName("SPECIAL_LABEL");
    m_label->setFont(font);
    m_label->setFixedWidth(width);
    //m_label->setFixedHeight(comboHeight);
    m_label->setFrameStyle(QFrame::Panel | QFrame::Sunken);

    // .. and edit button
    m_labelButton = new QPushButton(tr("Edit"), this);
    m_labelButton->setFont(font);
    m_labelButton->setToolTip(tr("<qt>Edit the segment label for any selected segments</qt>"));
    //    m_labelButton->setFixedWidth(50);

    connect(m_labelButton, SIGNAL(released()),
            SLOT(slotEditSegmentLabel()));

    m_repeatValue = new TristateCheckBox(this);
    m_repeatValue->setFont(font);
    m_repeatValue->setToolTip(tr("<qt><p>When checked,     any selected segments will repeat until they run into another segment,  "
                                 "or the end of the composition.</p><p>When viewed in the notation editor or printed via LilyPond, "
                                 "the segments will be bracketed by repeat signs.</p><p><center><img src=\":pixmaps/tooltip/repeats"
                                 ".png\"</img></center></p><br>These can be used in conjunction with special LilyPond export direct"
                                 "ives to create repeats with first and second alternate endings. See rosegardenmusic.com for a tut"
                                 "orial.</qt>"));
    //m_repeatValue->setFixedHeight(comboHeight);

    // handle state changes
    connect(m_repeatValue, SIGNAL(pressed()), SLOT(slotRepeatPressed()));

    // non-reversing motif style read-only combo
    m_quantizeValue = new QComboBox(this);
    m_quantizeValue->setFont(font);
    m_quantizeValue->setToolTip(tr("<qt><p>This allows you to choose how you want to quantize the midi notes.This allows you to tidy up human play to make sense of the notes for notation purposes. This gives you visual quantization only, and does not affect the way the midi sounds. </p></qt>"));
    //m_quantizeValue->setFixedHeight(comboHeight);

    // handle quantize changes from drop down
    connect(m_quantizeValue, SIGNAL(activated(int)),
            SLOT(slotQuantizeSelected(int)));

    // reversing motif style read-write combo
    m_transposeValue = new QComboBox(this);
    m_transposeValue->setFont(font);
    m_transposeValue->setToolTip(tr("<qt><p>Raise or lower playback of any selected segments by this number of semitones</p><p>"
                                    "<i>NOTE: This control changes segments that already exist.</i></p><p><i>Use the transpose "
                                    "control in <b>Track Parameters</b> under <b>Create segments with</b> to pre-select this   "
                                    "setting before drawing or recording new segments.</i></p></qt>"));
    //m_transposeValue->setFixedHeight(comboHeight);

    // handle transpose combo changes
    connect(m_transposeValue, SIGNAL(activated(int)),
            SLOT(slotTransposeSelected(int)));

    // and text changes
    connect(m_transposeValue, SIGNAL(textChanged(const QString&)),
            SLOT(slotTransposeTextChanged(const QString&)));

    // reversing motif style read-write combo
    m_delayValue = new QComboBox(this);
    m_delayValue->setFont(font);
    m_delayValue->setToolTip(tr("<qt><p>Delay playback of any selected segments by this number of miliseconds</p><p><i>NOTE: "
                                "Rosegarden does not support negative delay.  If you need a negative delay effect, set the   "
                                "composition to start before bar 1, and move segments to the left.  You can hold <b>shift</b>"
                                " while doing this for fine-grained control, though doing so will have harsh effects on music"
                                " notation rendering as viewed in the notation editor.</i></p></qt>"));
    //m_delayValue->setFixedHeight(comboHeight);

    // handle delay combo changes
    connect(m_delayValue, SIGNAL(activated(int)),
            SLOT(slotDelaySelected(int)));

    // Detect when the document colours are updated
    connect(m_doc, SIGNAL(docColoursChanged()),
            this, SLOT(slotDocColoursChanged()));

    // handle text changes for delay
    connect(m_delayValue, SIGNAL(textChanged(const QString&)),
            SLOT(slotDelayTextChanged(const QString &)));

    // set up combo box for colours
    m_colourValue = new QComboBox(this);
    m_colourValue->setEditable(false);
    m_colourValue->setFont(font);
    m_colourValue->setToolTip(tr("<qt><p>Change the color of any selected segments</p></qt>"));
    //m_colourValue->setFixedHeight(comboHeight);
    //    m_colourValue->setMaximumWidth(width);
    m_colourValue->setMaxVisibleItems(20);

    // handle colour combo changes
    connect(m_colourValue, SIGNAL(activated(int)),
            SLOT(slotColourSelected(int)));

    //!!! I deleted some legacy commented out code from the automatic audio
    // crossfaders Rich never finished, and from the segment-level controls to
    // change highest/lowest playable that I decided not to add after all.

    label->setFont(font);
    repeatLabel->setFont(font);
    quantizeLabel->setFont(font);
    transposeLabel->setFont(font);
    delayLabel->setFont(font);
    colourLabel->setFont(font);

    int row = 0;

//    gridLayout->addRowSpacing(0, 12); // why??

    gridLayout->addWidget(label, row, 0); //, AlignRight);
    gridLayout->addWidget(m_label, row, 1, row- row+1, 4); //, AlignLeft);
    gridLayout->addWidget(m_labelButton, row, 5); //, AlignLeft);
    ++row;

    gridLayout->addWidget(repeatLabel, row, 0); //, AlignRight);
    gridLayout->addWidget(m_repeatValue, row, 1); //, AlignLeft);

    gridLayout->addWidget(transposeLabel, row, 2, row- row+1, 3- 2+1, Qt::AlignRight);
    gridLayout->addWidget(m_transposeValue, row, 4, row- row+1, 5- 4+1);
    ++row;

    gridLayout->addWidget(quantizeLabel, row, 0); //, AlignRight);
    gridLayout->addWidget(m_quantizeValue, row, 1, row- row+1, 2); //, AlignLeft);

    gridLayout->addWidget(delayLabel, row, 3, Qt::AlignRight);
    gridLayout->addWidget(m_delayValue, row, 4, row- row+1, 5- 4+1);
    ++row;

    gridLayout->addWidget(colourLabel, row, 0); //, AlignRight);
    gridLayout->addWidget(m_colourValue, row, 1, row- row+1, 5);
    ++row;

    // Configure the empty final row to accomodate any extra vertical space.

    gridLayout->setRowStretch(gridLayout->rowCount() - 1, 1);

    // Configure the empty final column to accomodate any extra horizontal
    // space.

//    gridLayout->setColStretch(gridLayout->numCols() - 1, 1);

    // populate the quantize combo
    //
    QPixmap noMap = NotePixmapFactory::makeToolbarPixmap("menu-no-note");

    for (unsigned int i = 0; i < m_standardQuantizations.size(); ++i) {

        timeT time = m_standardQuantizations[i];
        timeT error = 0;
        QString label = NotationStrings::makeNoteMenuLabel(time, true, error);
        QPixmap pmap = NotePixmapFactory::makeNoteMenuPixmap(time, error);
        m_quantizeValue->addItem(error ? noMap : pmap, label);
    }
    m_quantizeValue->addItem(noMap, tr("Off"));

    // default to last item
    m_quantizeValue->setCurrentIndex(m_quantizeValue->count() - 1);

    // populate the transpose combo
    //
    for (int i = -m_transposeRange; i < m_transposeRange + 1; i++) {
        m_transposeValue->addItem(noMap, QString("%1").arg(i));
        if (i == 0)
            m_transposeValue->setCurrentIndex(m_transposeValue->count() - 1);
    }

    m_delays.clear();

    for (int i = 0; i < 6; i++) {
        timeT time = 0;
        if (i > 0 && i < 6) {
            time = Note(Note::Hemidemisemiquaver).getDuration() << (i - 1);
        } else if (i > 5) {
            time = Note(Note::Crotchet).getDuration() * (i - 4);
        }

        m_delays.push_back(time);

        // check if it's a valid note duration (it will be for the
        // time defn above, but if we were basing it on the sequencer
        // resolution it might not be) & include a note pixmap if so
        //
        timeT error = 0;
        QString label = NotationStrings::makeNoteMenuLabel(time, true, error);
        QPixmap pmap = NotePixmapFactory::makeNoteMenuPixmap(time, error);
        m_delayValue->addItem((error ? noMap : pmap), label);
    }

    for (int i = 0; i < 10; i++) {
        int rtd = (i < 5 ? ((i + 1) * 10) : ((i - 3) * 50));
        m_realTimeDelays.push_back(rtd);
        m_delayValue->addItem(tr("%1 ms").arg(rtd));
    }

    // set delay blank initially
    m_delayValue->setCurrentIndex( -1);

    // populate m_colourValue
    slotDocColoursChanged();
}

void
SegmentParameterBox::setDocument(RosegardenDocument* doc)
{
    if (m_doc != 0)
        disconnect(m_doc, SIGNAL(docColoursChanged()),
                   this, SLOT(slotDocColoursChanged()));

    m_doc = doc;

    // Detect when the document colours are updated
    connect (m_doc, SIGNAL(docColoursChanged()),
             this, SLOT(slotDocColoursChanged()));

    slotDocColoursChanged(); // repopulate combo
}

void
SegmentParameterBox::useSegment(Segment *segment)
{
    m_segments.clear();
    m_segments.push_back(segment);
    populateBoxFromSegments();
}

void
SegmentParameterBox::useSegments(const SegmentSelection &segments)
{
    m_segments.clear();

    m_segments.resize(segments.size());
    std::copy(segments.begin(), segments.end(), m_segments.begin());

    populateBoxFromSegments();
}

void
SegmentParameterBox::slotDocColoursChanged()
{
    RG_DEBUG << "SegmentParameterBox::slotDocColoursChanged()" << endl;

    m_colourValue->clear();
    m_colourList.clear();
    // Populate it from composition.m_segmentColourMap
    ColourMap temp = m_doc->getComposition().getSegmentColourMap();

    unsigned int i = 0;

    for (RCMap::const_iterator it = temp.begin(); it != temp.end(); ++it) {
        QString qtrunc(strtoqstr(it->second.second));
        QPixmap colour(15, 15);
        colour.fill(GUIPalette::convertColour(it->second.first));
        if (qtrunc == "") {
            m_colourValue->addItem(colour, tr("Default"), i);
        } else {
            // truncate name to 15 characters to avoid the combo forcing the
            // whole kit and kaboodle too wide
            if (qtrunc.length() > 15)
                qtrunc = qtrunc.left(12) + "...";
            m_colourValue->addItem(colour, qtrunc, i);
        }
        m_colourList[it->first] = i; // maps colour number to menu index
        ++i;
    }

    m_addColourPos = i;
    m_colourValue->addItem(tr("Add New Color"), m_addColourPos);

    m_colourValue->setCurrentIndex(0);
}

void SegmentParameterBox::update()
{
    RG_DEBUG << "SegmentParameterBox::update()" << endl;

    populateBoxFromSegments();
}

void
SegmentParameterBox::segmentRemoved(const Composition *composition,
                                    Segment *segment)
{
    if (composition == &m_doc->getComposition()) {

        for (std::vector<Segment*>::iterator it =
                    m_segments.begin(); it != m_segments.end(); ++it) {

            if (*it == segment) {
                m_segments.erase(it);
                return ;
            }
        }
    }
}

void
SegmentParameterBox::populateBoxFromSegments()
{
    std::vector<Segment*>::iterator it;
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
        m_label->setText(strtoqstr(m_segments[0]->getLabel()));

    // I never noticed this after all this time, but it seems to go all the way
    // back to the "..." button that this was never disabled if there was no
    // segment, and therefore no label to edit.  So we disable the edit button
    // and repeat checkbox first:
    m_labelButton->setEnabled(false);
    m_repeatValue->setEnabled(false);


    for (it = m_segments.begin(); it != m_segments.end(); it++) {
        // ok, first thing is we know we have at least one segment
        //
        // and since there is at least one segment, we can re-enable the edit button
        // and repeat checkbox:
        m_labelButton->setEnabled(true);
        m_repeatValue->setEnabled(true);

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
        if (strtoqstr((*it)->getLabel()) != m_label->text())
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
            if (myCol != (*it)->getColourIndex())
                ;
            diffcolours = All;
        }

        // Highest/Lowest playable
        //
        if (it == m_segments.begin()) {
            myHigh = (*it)->getHighestPlayable();
            myLow = (*it)->getLowestPlayable();
        } else {
            if (myHigh != (*it)->getHighestPlayable() ||
                myLow != (*it)->getLowestPlayable()) {
                highlow = All;
            }
        }

    }

    switch (repeated) {
    case All:
        m_repeatValue->setChecked(true);
        break;

    case Some:
        m_repeatValue->setNoChange();
        break;

    case None:
    case NotApplicable:
    default:
        m_repeatValue->setChecked(false);
        break;
    }

    m_repeatValue->setEnabled(repeated != NotApplicable);

    switch (quantized) {
    case All: {
            for (unsigned int i = 0;
                    i < m_standardQuantizations.size(); ++i) {
                if (m_standardQuantizations[i] == qntzLevel) {
                    m_quantizeValue->setCurrentIndex(i);
                    break;
                }
            }
        }
        break;

    case Some:
        // Set the edit text to an unfeasible blank value meaning "Some"
        //
        m_quantizeValue->setCurrentIndex( -1);
        break;

        // Assuming "Off" is always the last field
    case None:
    default:
        m_quantizeValue->setCurrentIndex(m_quantizeValue->count() - 1);
        break;
    }

    m_quantizeValue->setEnabled(quantized != NotApplicable);

    switch (transposed) {
        // setCurrentIndex works with QStrings
        // 2nd arg of "true" means "add if necessary"
    case All:
        m_transposeValue->
//           setCurrentIndex(QString("%1").arg(transposeLevel), true);
          setCurrentText( QString("%1").arg(transposeLevel) );
          break;

    case Some:
//           m_transposeValue->setCurrentIndex(QString(""), true);
          m_transposeValue->setCurrentText(QString(""));
          break;

    case None:
    default:
//           m_transposeValue->setCurrentIndex("0");
          m_transposeValue->setCurrentText("0");
          break;
    }

    m_transposeValue->setEnabled(transposed != NotApplicable);

    m_delayValue->blockSignals(true);

    switch (delayed) {
    case All:
        if (delayLevel >= 0) {
            timeT error = 0;
            QString label = NotationStrings::makeNoteMenuLabel(delayLevel,
                            true,
                            error);
//                m_delayValue->setCurrentIndex(label, true);
               m_delayValue->setCurrentText(label);

        } else if (delayLevel < 0) {

//                m_delayValue->setCurrentIndex(tr("%1 ms").arg(-delayLevel),true);
               m_delayValue->setCurrentText( tr("%1 ms").arg(-delayLevel) );
          }

        break;

    case Some:
//           m_delayValue->setCurrentIndex("", true);
          m_delayValue->setCurrentText("");
          break;

    case None:
    default:
        m_delayValue->setCurrentIndex(0);
        break;
    }

    m_delayValue->setEnabled(delayed != NotApplicable);

    m_delayValue->blockSignals(false);

    switch (diffcolours) {
    case None:
        if (m_colourList.find(myCol) != m_colourList.end())
            m_colourValue->setCurrentIndex(m_colourList[myCol]);
        else
            m_colourValue->setCurrentIndex(0);
        break;


    case All:
    case NotApplicable:
    default:
        m_colourValue->setCurrentIndex(0);
        break;

    }

    m_colourValue->setEnabled(diffcolours != NotApplicable);

    //!!! this is all borked up and useless; sort out after 1.3
/*
    switch (highlow) {
    case All:
        updateHighLow();
        break;

    case Some:
    case None:
    default:
        m_highButton->setText(tr("High: ---"));
        m_lowButton->setText(tr("Low: ----"));
        highlow = NotApplicable;
        break;
    }

    m_highButton->setEnabled(highlow != NotApplicable);
    m_lowButton->setEnabled(highlow != NotApplicable);
*/

    // Enable or disable the fade in/out params
/*
    if (m_segments.size() == 1 &&
            (*(m_segments.begin()))->getType() == Segment::Audio) {
        m_autoFadeBox->blockSignals(true);
        m_fadeInSpin->blockSignals(true);
        m_fadeOutSpin->blockSignals(true);

        ... !!! No, not setting up autofade widgets.  The implementation's too
              incomplete to finish for this release.
         
              (Or for the next one after the one the previous comment referred to.)
         
              (Or for the one after the one after that.  Will we ever get those
              working, or should Rich's final legacy simply be quietly disappeared?)
         
                m_fadeInLabel->show();
                m_fadeInSpin->show();
                m_fadeOutLabel->show();
                m_fadeOutSpin->show();
         
             instead:
        
        m_fadeInLabel->hide();
        m_fadeInSpin->hide();
        m_fadeOutLabel->hide();
        m_fadeOutSpin->hide();

        m_autoFadeLabel->setEnabled(true);
        m_autoFadeBox->setEnabled(true);
        m_fadeInLabel->setEnabled(true);
        m_fadeInSpin->setEnabled(true);
        m_fadeOutLabel->setEnabled(true);
        m_fadeOutSpin->setEnabled(true);

        Segment *seg = *(m_segments.begin());

        int fadeInTime = seg->getFadeInTime().sec * 1000 +
                         seg->getFadeInTime().msec();
        m_fadeInSpin->setValue(fadeInTime);

        int fadeOutTime = seg->getFadeOutTime().sec * 1000 +
                          seg->getFadeOutTime().msec();
        m_fadeOutSpin->setValue(fadeOutTime);

        m_autoFadeBox->setChecked(seg->isAutoFading());

        m_autoFadeBox->blockSignals(false);
        m_fadeInSpin->blockSignals(false);
        m_fadeOutSpin->blockSignals(false);
    } else {
        m_autoFadeLabel->setEnabled(false);
        m_autoFadeBox->setEnabled(false);
        m_fadeInLabel->setEnabled(false);
        m_fadeInSpin->setEnabled(false);
        m_fadeOutLabel->setEnabled(false);
        m_fadeOutSpin->setEnabled(false);

        m_autoFadeLabel->hide();
        m_autoFadeBox->hide();
        m_fadeInLabel->hide();
        m_fadeInSpin->hide();
        m_fadeOutLabel->hide();
        m_fadeOutSpin->hide();

        m_autoFadeBox->setChecked(false);
        m_fadeInSpin->setValue(0);
        m_fadeOutSpin->setValue(0);
    }
*/

}

void SegmentParameterBox::slotRepeatPressed()
{
    if (m_segments.size() == 0)
        return ;

    bool state = false;

    switch (m_repeatValue->state()) {
    case QCheckBox::Off:
        state = true;
        break;

    case QCheckBox::NoChange:
    case QCheckBox::On:
    default:
        state = false;
        break;
    }

    // update the check box and all current Segments
    m_repeatValue->setChecked(state);

    addCommandToHistory(new SegmentCommandRepeat(m_segments, state));

    //     std::vector<Segment*>::iterator it;

    //     for (it = m_segments.begin(); it != m_segments.end(); it++)
    //         (*it)->setRepeating(state);
}

void
SegmentParameterBox::slotQuantizeSelected(int qLevel)
{
    bool off = (qLevel == m_quantizeValue->count() - 1);

    SegmentChangeQuantizationCommand *command =
        new SegmentChangeQuantizationCommand
        (off ? 0 : m_standardQuantizations[qLevel]);

    std::vector<Segment*>::iterator it;
    for (it = m_segments.begin(); it != m_segments.end(); it++) {
        command->addSegment(*it);
    }

    addCommandToHistory(command);
}

void
SegmentParameterBox::slotTransposeTextChanged(const QString &text)
{
    if (text.isEmpty() || m_segments.size() == 0)
        return ;

    int transposeValue = text.toInt();

    //     addCommandToHistory(new SegmentCommandChangeTransposeValue(m_segments,
    //                                                                transposeValue));

    std::vector<Segment*>::iterator it;
    for (it = m_segments.begin(); it != m_segments.end(); it++) {
        (*it)->setTranspose(transposeValue);
    }

    emit documentModified();
}

void
SegmentParameterBox::slotTransposeSelected(int value)
{
    slotTransposeTextChanged(m_transposeValue->text(value));
}

void
SegmentParameterBox::slotDelayTimeChanged(timeT delayValue)
{
    // by convention and as a nasty hack, we use negative timeT here
    // to represent positive RealTime in ms

    if (delayValue > 0) {

        std::vector<Segment*>::iterator it;
        for (it = m_segments.begin(); it != m_segments.end(); it++) {
            (*it)->setDelay(delayValue);
            (*it)->setRealTimeDelay(RealTime::zeroTime);
        }

    } else if (delayValue < 0) {

        std::vector<Segment*>::iterator it;
        for (it = m_segments.begin(); it != m_segments.end(); it++) {
            (*it)->setDelay(0);
            int sec = ( -delayValue) / 1000;
            int nsec = (( -delayValue) - 1000 * sec) * 1000000;
            (*it)->setRealTimeDelay(RealTime(sec, nsec));
        }
    } else {

        std::vector<Segment*>::iterator it;
        for (it = m_segments.begin(); it != m_segments.end(); it++) {
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
SegmentParameterBox::slotColourSelected(int value)
{
    if (value != m_addColourPos) {
        unsigned int temp = 0;

        ColourTable::ColourList::const_iterator pos;
        for (pos = m_colourList.begin(); pos != m_colourList.end(); ++pos) {
            if (pos->second == value) {
                temp = pos->first;
                break;
            }
        }

        SegmentSelection segments;
        std::vector<Segment*>::iterator it;

        for (it = m_segments.begin(); it != m_segments.end(); ++it) {
            segments.insert(*it);
        }

        SegmentColourCommand *command = new SegmentColourCommand(segments, temp);

        addCommandToHistory(command);
    } else {
        ColourMap newMap = m_doc->getComposition().getSegmentColourMap();
        QColor newColour;
        bool ok = false;
        QString newName = InputDialog::getText(this,
                                               tr("New Color Name"),
                                               tr("Enter new name"),
                                               LineEdit::Normal,
                                               tr("New"), &ok);
        if ((ok == true) && (!newName.isEmpty())) {
//             QColorDialog box(this, "", true);
               
               //QRgb QColorDialog::getRgba( 0xffffffff, &ok, this );
               QColor newColor = QColorDialog::getColor( Qt::white, this );

//             int result = box.getColor(newColour);

            if( newColor.isValid() ) {
                Colour newRColour = GUIPalette::convertColour(newColour);
                newMap.addItem(newRColour, qstrtostr(newName));
                SegmentColourMapCommand *command = new SegmentColourMapCommand(m_doc, newMap);
                addCommandToHistory(command);
                slotDocColoursChanged();
            }
        }
        // Else we don't do anything as they either didn't give a name·
        //  or didn't give a colour
    }


}

void
SegmentParameterBox::updateHighLow()
{
    // Key of C major and NoAccidental means any "black key" notes will be
    // written as sharps.
    Accidental accidental = Accidentals::NoAccidental;
    Rosegarden::Key key = Rosegarden::Key("C major");

    Pitch highest(m_highestPlayable, accidental);
    Pitch lowest(m_lowestPlayable, accidental);

    QSettings settings;
    settings.beginGroup( GeneralOptionsConfigGroup );

    int base = settings.value("midipitchoctave", -2).toInt() ;
    settings.endGroup();
    //!!! FIXME this code is broken, and needs to be fixed after the fashion of
    //the TPB, but I'm not bothering with that at this time, because they are
    //going to be hidden for 1.3 anyway
//    m_highButton->setText(QString("&High:   %1%2").arg(highest.getNoteName(key)).arg(highest.getOctave(base)));
//    m_lowButton->setText(QString("&Low:   %1%2").arg(lowest.getNoteName(key)).arg(lowest.getOctave(base)));
}

void
SegmentParameterBox::slotHighestPressed()
{
    RG_DEBUG << "SegmentParameterBox::slotHighestPressed()" << endl;

    PitchPickerDialog dialog(0, m_highestPlayable, tr("Highest playable note"));
    std::vector<Segment*>::iterator it;

    if (dialog.exec() == QDialog::Accepted) {
        m_highestPlayable = dialog.getPitch();
        updateHighLow();

        for (it = m_segments.begin(); it != m_segments.end(); it++) {
            (*it)->setHighestPlayable(m_highestPlayable);
        }

        emit documentModified();
    }
}

void
SegmentParameterBox::slotLowestPressed()
{
    RG_DEBUG << "SegmentParameterBox::slotLowestPressed()" << endl;

    PitchPickerDialog dialog(0, m_lowestPlayable, tr("Lowest playable note"));
    std::vector<Segment*>::iterator it;

    if (dialog.exec() == QDialog::Accepted) {
        m_lowestPlayable = dialog.getPitch();
        updateHighLow();

        for (it = m_segments.begin(); it != m_segments.end(); it++) {
            (*it)->setLowestPlayable(m_lowestPlayable);
        }

        emit documentModified();
    }
}

void
SegmentParameterBox::addCommandToHistory(Command *command)
{
    CommandHistory::getInstance()->addCommand(command);
}

void
SegmentParameterBox::slotEditSegmentLabel()
{
    QString editLabel;

    if (m_segments.size() == 0)
        return ;
    else if (m_segments.size() == 1)
        editLabel = tr("Modify Segment label");
    else
        editLabel = tr("Modify Segments label");

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
                                            &ok );

    if (ok) {
        SegmentSelection segments;
        std::vector<Segment*>::iterator it;
        for (it = m_segments.begin(); it != m_segments.end(); ++it)
            segments.insert(*it);

        SegmentLabelCommand *command = new
                                       SegmentLabelCommand(segments, newLabel);

        addCommandToHistory(command);

     // fix #1776915, maybe?
     update();
    }
}

void
SegmentParameterBox::slotAudioFadeChanged(int value)
{
    RG_DEBUG << "SegmentParameterBox::slotAudioFadeChanged - value = "
    << value << endl;
/*
    if (m_segments.size() == 0)
        return ;

    bool state = false;
    if (value == QCheckBox::On)
        state = true;

    std::vector<Segment*>::iterator it;
    for (it = m_segments.begin(); it != m_segments.end(); it++) {
        (*it)->setAutoFade(state);
    }
*/
}

void
SegmentParameterBox::slotFadeInChanged(int value)
{
    RG_DEBUG << "SegmentParameterBox::slotFadeInChanged - value = "
    << value << endl;
/*
    if (m_segments.size() == 0)
        return ;

    if (value == 0 && m_fadeOutSpin->value() == 0)
        slotAudioFadeChanged(QCheckBox::Off);
    else
        slotAudioFadeChanged(QCheckBox::On);

    // Convert from ms
    //
    RealTime fadeInTime(value / 1000, (value % 1000) * 1000000);

    std::vector<Segment*>::iterator it;
    for (it = m_segments.begin(); it != m_segments.end(); it++) {
        (*it)->setFadeInTime(fadeInTime);
    }

    emit documentModified();
*/
}

void
SegmentParameterBox::slotFadeOutChanged(int value)
{
    RG_DEBUG << "SegmentParameterBox::slotFadeOutChanged - value = "
    << value << endl;
/*
    if (m_segments.size() == 0)
        return ;

    if (value == 0 && m_fadeInSpin->value() == 0)
        slotAudioFadeChanged(QCheckBox::Off);
    else
        slotAudioFadeChanged(QCheckBox::On);

    // Convert from ms
    //
    RealTime fadeOutTime(value / 1000000, (value % 1000) * 10000000);

    std::vector<Segment*>::iterator it;
    for (it = m_segments.begin(); it != m_segments.end(); it++) {
        (*it)->setFadeOutTime(fadeOutTime);
    }

    emit documentModified();
*/
}

void
SegmentParameterBox::showAdditionalControls(bool showThem)
{
    //!!! disabled until after 1.3
    /*    m_highButton->setShown(showThem);
        m_lowButton->setShown(showThem);
        m_rangeLabel->setShown(showThem); */
}

QString
SegmentParameterBox::getPreviousBox(RosegardenParameterArea::Arrangement arrangement) const
{
    if (arrangement == RosegardenParameterArea::CLASSIC_STYLE) {
        return "";
    } else {
        return tr("Instrument");
    }
}

}
#include "SegmentParameterBox.moc"
