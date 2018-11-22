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

/*
 * CheckForParallelsDialog.cpp
 *
 *  Created on: Mar 22, 2015
 *      Author: lambache
 */

#include "CheckForParallelsDialog.h"
#include "commands/notation/MarkParallelCommand.h"
#include "document/CommandHistory.h"
#include "base/Segment.h"
#include "base/Event.h"
#include "base/BaseProperties.h"
#include "gui/widgets/FileDialog.h"
#include "misc/ConfigGroups.h"


#include <QDialog>
#include <QDialogButtonBox>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDebug>
#include <QCheckBox>
#include <QSettings>

#include <iterator>
#include <iostream>
#include <cstdio>
#include <typeinfo>
#include <algorithm>

#define DEBUG_CHECK_PARALLELS

namespace Rosegarden
{

//bool CheckForParallelsDialog::checkForUnisons = false;

CheckForParallelsDialog::CheckForParallelsDialog(NotationView *p, RosegardenDocument *doc, NotationScene *ns, Composition *comp) :
        QDialog(p)
{
    setWindowTitle(tr("Check for Parallels"));

    document = doc;
    composition = comp;
    notationScene = ns;

    notationView = p;

    // the text browser to print display the parallels in text format

    textBrowser = new QTextBrowser(this);

    // background-color: rgb(200, 200, 200);
    QPalette pal;
    pal.setColor(QPalette::Base, QColor(0xc8, 0xc8, 0xc8));
    pal.setColor(QPalette::Text, Qt::black);
    textBrowser->setPalette(pal);

    connect(textBrowser, SIGNAL(cursorPositionChanged()), this, SLOT(onTextBrowserclicked()));

    ignoreCursor = true;

    QVBoxLayout *vboxLayout = new QVBoxLayout;
    setLayout(vboxLayout);

    vboxLayout->addWidget(textBrowser);

    // we make the check for unisons optional
    //
    // if, e.g., a piece for SATB has accompanying voices in strings that go just with the sung voices
    // we will have parallels throughout the piece that do not count

    checkForUnisonsCheckBox = new QCheckBox(tr("Check for Unisons"));
    vboxLayout->addWidget(checkForUnisonsCheckBox);
    checkForUnisonsCheckBox->setChecked( checkForUnisons );

    connect(checkForUnisonsCheckBox, SIGNAL(clicked()), this, SLOT(checkForUnisonsClicked()));

    // we make the check for hidden parallels optional
    //
    // question is whether they should be on by default or off by default
    // I choose 'off'

    checkForHiddenParallelsCheckBox = new QCheckBox(tr("Check for Hidden Parallels"));
    vboxLayout->addWidget(checkForHiddenParallelsCheckBox);
    checkForHiddenParallelsCheckBox->setChecked( checkForHiddenParallels );

    connect(checkForHiddenParallelsCheckBox, SIGNAL(clicked()), this, SLOT(checkForHiddenParallelsClicked()));

    // buttons

    startButton = new QPushButton(tr("Start"));  // is not local as we want to change text after first start
    QPushButton *clearButton = new QPushButton(tr("Clear"));
    QPushButton *exportButton = new QPushButton(tr("Export"));
    QPushButton *okButton = new QPushButton(tr("Ok"));

    QHBoxLayout *hBoxLayout = new QHBoxLayout;

    hBoxLayout->addWidget(startButton);
    hBoxLayout->addWidget(clearButton);
    hBoxLayout->addWidget(exportButton);
    hBoxLayout->addWidget(okButton);

    vboxLayout->addLayout(hBoxLayout);

    QSettings settings;
    settings.beginGroup( GeneralOptionsConfigGroup );

    checkForUnisons = qStrToBool( settings.value("checkForUnisonsinParallels", "false" ));
    checkForUnisonsCheckBox->setChecked(checkForUnisons);

    settings.endGroup();


    connect(startButton, SIGNAL(clicked()), this, SLOT(startCheck()));
    connect(clearButton, SIGNAL(clicked()), this, SLOT(clear()));
    connect(exportButton, SIGNAL(clicked()), this, SLOT(exportText()));
    connect(okButton, SIGNAL(clicked()), this, SLOT(cleanUpAndLeave()));

    transitionList.clear();
}

// set the position in notationview to point to the clicked parallel

void
CheckForParallelsDialog::onTextBrowserclicked()
{
    if (ignoreCursor)
        return;

    updateSegments();

    unsigned int currentLine = textBrowser->textCursor().blockNumber();

    if (currentLine<locationForLine.size())
        if (locationForLine[currentLine].time != -1) {

            // at this stage the list of staves corresponds to the content of the note editor
            // if the list of parallels is from an older run and the user meanwhile changed the segments/staves in the note editor
            // the pointer may go to a wrong positions
            // I consider this tolerable. It's a situation comparable to a development environment in programming where you
            // can click on an error message of the compiler in the console output to go to the corresponding source line.
            // If you insert or delete lines, a click on the next error message will go to the wrong line.
            // at least this should not crash, as the list of segments was updated at the beginning of the function.

            notationScene->setCurrentStaff(locationForLine[currentLine].staff);
            document->slotSetPointerPosition(locationForLine[currentLine].time);
        }
}

// export text window in readable format, so the user can keep the list

void
CheckForParallelsDialog::exportText()
{
    QString label = "Export Parallels";

    // last directory we exported the parallels list to
    static QString lastExportDirectory;
    QString name = FileDialog::getSaveFileName(
            this, label, lastExportDirectory,
            QString(""), "*.txt", nullptr,
            FileDialog::DontConfirmOverwrite);

    if (name == "")
        return;

    //qDebug() << "got export file name '" + name + "'";

    if (name.right(4).toLower() != ".txt") {
        name += ".txt";
    }

    QFile file(name);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);

    qDebug() << "exporting parallels";
    out << textBrowser->toPlainText();

    file.close();
}

// clear parallels and text window

void
CheckForParallelsDialog::clear()
{
    textBrowser->clear();

    Segment::iterator ia;

    updateSegments();

    // now the segment list corresponds to the current content of the note editor
    //
    // if the parallels list is from an older run and the user changed the segments it may happen that not all notes are cleared.
    // I consider this tolerable. I think that usually, when looking for parallels, the user will go through the current music content
    // where he/she looked for the parallels. At least this should not crash the dialog.
    //
    // to get everything synced again you always can open all segments in the notation editor and clear that

    int numberOfSegments = segment.size();

    if (numberOfSegments == 0)
        return;

    // we clear all notes open in the notation editor

    for (int i=0; i<numberOfSegments; ++i)
    {
        ia = segment[i]->begin();

        while (ia != segment[i]->end()) {

            // if it's a note, clear it

            if ((*ia)->isa("note")) (*ia)->set<Bool>(BaseProperties::MEMBER_OF_PARALLEL, false);

            ++ia;

            if (ia == segment[i]->end()) {
                continue; // go to next segment
            }
        }

        // mark segment as modified
        segment[i]->updateRefreshStatuses(segment[i]->getStartTime(), segment[i]->getEndTime());
    }

    // trigger repaint
    MarkParallelCommand *command = new MarkParallelCommand(*segment[0], 0, 0);
    CommandHistory::getInstance()->addCommand(command);
}

void
CheckForParallelsDialog::updateSegments()
{
    SegmentStaffLink l;
    segmentStaffLinkList.clear();

    std::vector<NotationStaff *> *staff = notationScene->getStaffs();

    unsigned int numberOfStaves = staff->size();

    segment.clear();

    //qDebug() << "number of staves" << numberOfStaves;

    for (unsigned int i=0; i<numberOfStaves; ++i) {
        l.staff = (*staff)[i];
        l.segment = &((*staff)[i]->getSegment());
        segmentStaffLinkList.push_back(l);

        segment.push_back(l.segment);
    }
}

void
CheckForParallelsDialog::cleanUpAndLeave()
{
    // we do not clear so the user still sees the parallels in the editor

    this->close();
}

void
CheckForParallelsDialog::checkForUnisonsClicked()
{
    checkForUnisons = checkForUnisonsCheckBox->isChecked();

    // as far as I understand qCheckBox functionality it will always change upon click
    // so no need for an extra change whether content of checkForUnisions has changed

    startCheck();
}

void
CheckForParallelsDialog::checkForHiddenParallelsClicked()
{
    checkForHiddenParallels = checkForHiddenParallelsCheckBox->isChecked();

    startCheck();
}

void
CheckForParallelsDialog::writeTransition(std::vector<Transition>::iterator it)
{
    QString text;

    int bar;
    int beat;
    int fraction;
    int remainder;

    text += ", track " + makeTrackString((*it).TrackPosition, (*it).trackLabel);

    composition->getMusicalTimeForAbsoluteTime((*it).time, bar, beat, fraction, remainder);

    text += ", " + tr("bar") + tr(" ") + QString("%1").arg(bar) + ", " + tr("beat") + " " + QString("%1").arg(beat) + ", " + tr("fraction") + " " + QString("%1").arg(fraction);
    qDebug() << text;
}

void
CheckForParallelsDialog::writeTransitionList(std::vector<Transition> list)
{
    std::vector<Transition>::iterator it = list.begin();

    QString text;

    int count = 0;

    while (it!=list.end()) {
        text = "transition #" + QString("%1").arg(count++);
        writeTransition(it);
        ++it;
    }
}

void
CheckForParallelsDialog::startCheck()
{
    QSettings settings;

    settings.beginGroup( GeneralOptionsConfigGroup );

    settings.setValue("checkForUnisonsinParallels", checkForUnisons);
    settings.setValue("checkForHiddenParallels", checkForHiddenParallels);

    settings.endGroup();

    // after the first run, the StartButton should show the text 'Restart'

    startButton->setText(tr("Restart"));

    // clear found parallels
    clear();

    // if we append a parallel to the textBrowser it will emit a cursorPositionChanged signal
    // as long as the search is running we ignore these signals, so the notationView will not change it's pointer position
    //
    // upon end of search ignoreCursor will be set to false, from then on the cursorPositionChanged signals will occur
    // when the user clicks on a line in the list
    // these signals then are handled by the onTextBrowserclicked() slot
    // maybe someone else finds a better way of handling this by using the mouseEvents...
    ignoreCursor = true;

    // we build the list of segments on each start, as the user may edit the music while the dialog is still open
    // segments may be deleted or created in between runs

    // additionally we build a list of links between segment and staff so we can find the staff to which a segment belongs

    updateSegments();

    int numberOfSegments = segment.size();

    // we build a list of all transitions in all segments
    // this list is then sorted in ascending order according to time
    // then we group the transitions that occur at the same time into transition sets
    // these are then checked for parallels
    //
    // transitions are found from the end, i.e. we check whether an event is a note
    // then we look whether a note occurred before the current note without a rest in between
    //
    // for a transition we need the following things:
    // - the previous note (stored in 'currentPredecessor' must have ended
    // - the current note must be single
    // - there is no rest between the previous note and the current one
    //
    // if we encounter a situation where two notes sound at the same time we stop checking
    // it can be really complicated to find the next period of time where only a single note sounds again so that we could
    // go on with the search
    // so the user gets a warning that for a certain track the search was not done due to the fact that multiple notes occurred

    transitionList.clear();

    Segment::iterator ia;  // this goes through all events in all segments

    Transition t;

    Segment::iterator currentNote;
    Segment::iterator currentPredecessor;

    TrackId tId;
    Track *currentTrack;
    int currentTrackPosition;
    QString currentTrackLabel;

    NotationStaff *currentStaff = nullptr;

    parallelList.clear();

    timeT currentTime;

    QString text;

    locationForLine.clear();

    // we add the document name in the beginning to help the user keeping track which exported parallels text file
    // belongs to which rosegarden file
    //
    // we have to place an invalid parallels location for each line of text that does not belong to a parallel

    parallelLocation pl;

    pl.time = -1;
    pl.staff = nullptr;

    locationForLine.push_back(pl);
    locationForLine.push_back(pl);

    text = QString("File: ") + document->getTitle() + QString("\n\n");

    addText(text);


    for (int i=0; i<numberOfSegments; ++i)
    {
        // look up to which staff the current segment belongs

        for (unsigned int j=0; j<segmentStaffLinkList.size(); ++j) {
            if (segmentStaffLinkList[j].segment == segment[i]) {
                currentStaff = segmentStaffLinkList[j].staff;
            }
        }

        ia = segment[i]->begin();

        tId = segment[i]->getTrack();

        currentTrack = composition->getTrackById(tId);
        currentTrackPosition = currentTrack->getPosition();
        currentTrackLabel = QString::fromUtf8(currentTrack->getLabel().c_str());;

        currentPredecessor = segment[i]->end(); // this is used as flag for "we don't have a predecessor"

        while (ia != segment[i]->end()) {

            ++ia;

            if (ia == segment[i]->end()) continue; // go to next segment

            // if we have a predecessor and the current event is a note then we have a potential transition
            // but we need to check whether we have multiple notes at the same time

            if ((*ia)->isa("note")) {

                if (currentTrackLabel == QString("v4") )
                        text = "";

                qDebug() << "found note at track" << makeTrackString(currentTrackPosition, currentTrackLabel)
                        << "at time" << (*ia)->getAbsoluteTime()
                        << "duration" << (*ia)->getDuration();

                if (currentPredecessor == segment[i]->end()) {

                    // if we do not have a predecessor the current note becomes the predecessor and we are finished for this turn

                    currentPredecessor = ia;

                } else {

                    // we need to check whether we have multiple notes at the current time point
                    //
                    // this occurs if the predecessor ends after the current note starts
                    // if this happens we stop checking for the current track and give a warning to the user

                    timeT predecessorEnd = (*currentPredecessor)->getAbsoluteTime() + (*currentPredecessor)->getDuration();
                    currentTime = (*ia)->getAbsoluteTime();

                    // for the messages about multiple notes we have to add invalid locations into the list of parallel locations so that later on
                    // the line numbers for the real parallels in the text browser are correct

                    pl.time = -1;
                    pl.staff = nullptr;

                    if ( predecessorEnd > currentTime ) {
                        int bar;
                        int beat;
                        int fraction;
                        int remainder;

                        composition->getMusicalTimeForAbsoluteTime(currentTime, bar, beat, fraction, remainder);
                        ++bar;  //seems to start counting at 0

                        text = tr("found multiple notes for") + " " + makeTrackString(currentTrackPosition, currentTrackLabel);

                        text += ", " + tr("bar") + tr(" ") + QString("%1").arg(bar) + ", " + tr("beat") + " " + QString("%1").arg(beat) + ", " + tr("fraction") + " " + QString("%1").arg(fraction) + "\n";

                        text += "    stopped checking for parallels in the current segment.\n\n";

                        // each message consists of three lines
                        locationForLine.push_back(pl);
                        locationForLine.push_back(pl);
                        locationForLine.push_back(pl);

                        addText(text);

                        break; // go to next segment
                    } else {
                        // we have a transition
                        t.note = ia;
                        t.predecessor = currentPredecessor;
                        t.time = (*ia)->getAbsoluteTime();
                        t.segment = segment[i];
                        t.staff = currentStaff;
                        t.TrackPosition = currentTrackPosition + 1;
                        t.trackLabel = currentTrackLabel;

                        transitionList.push_back(t);
                        currentPredecessor = ia;
                    }
                }
            } else {
                // if we have a rest in between notes, this breaks the transition

                // are there other events that also should break the transition?

                if ((*ia)->isa("rest"))
                    currentPredecessor = segment[i]->end();
            }
        }
    }

    if (transitionList.size() == 0) return;

    // we sort the transition list by time
    // so all transitions that occur at the same time are consecutive in the vector

    std::sort(transitionList.begin(), transitionList.end(), sortByTime);

    writeTransitionList(transitionList);

    // now we check for parallels

    // all transition the occur at a specific time
    std::vector<Transition> tSet;

    std::vector<Transition>::iterator it = transitionList.begin();

    tSet.clear();

    tSet.push_back(transitionList[0]);

    currentTime = transitionList[0].time;

    ++it;

    while (it != transitionList.end()) {

        // we collect all transitions that occur at currentTime

        if (it->time == currentTime){
            tSet.push_back(*it);
        } else {
            // we need to check the current set and start the next one

            checkParallels(tSet);

            tSet.clear();

            tSet.push_back(*it);
            currentTime = it->time;
        }

        ++it;

        if (it == transitionList.end()) {
            // if we reached the end of the list we need to check the last set as well

            checkParallels(tSet);
        }
    }

    // when we arrive here the list of parallel has been populated by checkParallels

    if (parallelList.size() > 0) {

        // we trigger a repaint with an empty command, that does nothing else than to make CommandHistory to emit the repaint signal

        MarkParallelCommand *command = new MarkParallelCommand( *parallelList[0][0].segment1, 0, 0);
        CommandHistory::getInstance()->addCommand(command);
    }

    ignoreCursor = false;

}

void
CheckForParallelsDialog::checkParallels(std::vector<Transition> &tSet)
{
    ParallelSet p;

    parallelLocation pl;

    qDebug() << "checking set:";

    writeTransitionList(tSet);

    if (hasParallels(tSet, p)) {

        parallelList.push_back(p);

        int bar;
        int beat;
        int fraction;
        int remainder;

        composition->getMusicalTimeForAbsoluteTime(p[0].time, bar, beat, fraction, remainder);
        ++bar;  //seems to start counting at 0

        QString text;

        if (p.size() == 1) {
            text = tr("parallel at bar");
        } else {
            text = tr("parallels at bar");
        }

        text += " " + QString("%1").arg(bar) + ", " + tr("beat") + " " + QString("%1").arg(beat) + ", " + tr("fraction") + " " + QString("%1").arg(fraction) + "\n";

        addText(text);

        // we generate an invalid location for this line, as there is no specific segment

        pl.time = -1;
        pl.staff = p[0].staff1;
        locationForLine.push_back(pl);

        for (unsigned int i=0; i<p.size(); ++i) {

            switch (p[i].type) {

            case UNISON:
                text = "  " + tr("unisons");
                break;

            case FIFTH:
                text = "  " + tr("fifths");
                break;

            case OCTAVE:
                text = "  " + tr("octaves");
                break;

            case HIDDEN_FIFTH:
                text = "  " + tr("hidden fifths");
                break;

            case HIDDEN_OCTAVE:
                text = "  " + tr("hidden octaves");
                break;
            }

            text += " " + tr("for") + " " + makeTrackString(p[i].trackPosition1, p[i].trackLabel1)
                    + " <-> " + makeTrackString(p[i].trackPosition2, p[i].trackLabel2) + "\n";

            addText(text);

            // this line has a valid location

            pl.time = p[i].time;
            pl.staff = p[i].staff1;
            locationForLine.push_back(pl);

            // mark all RefreshStatuses so that the notaionView knows what to update
            // repaint is done by caller when all parallels are marked

            p[i].segment1->updateRefreshStatuses((*p[i].predecessor1)->getAbsoluteTime(), (*p[i].note1)->getAbsoluteTime());
            p[i].segment2->updateRefreshStatuses((*p[i].predecessor2)->getAbsoluteTime(), (*p[i].note2)->getAbsoluteTime());
        }

        addText(QString("\n"));

        // we generate an invalid location for this line, as there is no specific segment and no specific time

        pl.time = -1;
        pl.staff = p[0].staff1;
        locationForLine.push_back(pl);
    }
}

QString
CheckForParallelsDialog::makeTrackString(int trackPosition, QString trackLabel)
{
    // if the track has a label, return the label otherwise return the position as String

    if (trackLabel == "") {
        // using a length of two makes it a bit better readable if we have more than 9 tracks
        return QString("%1").arg(trackPosition, 2);
    } else {
        return trackLabel;
    }
}

// populate parallel fields with exception of type, this is done in caller

void
CheckForParallelsDialog::populateParallel(Transition t1, Transition t2, Parallel &p)
{
    p.note1 = t1.note;
    p.note2 = t2.note;

    (*p.note1)->set<Bool>(BaseProperties::MEMBER_OF_PARALLEL, true);
    (*p.note2)->set<Bool>(BaseProperties::MEMBER_OF_PARALLEL, true);

    p.predecessor1 = t1.predecessor;
    p.predecessor2 = t2.predecessor;

    (*p.predecessor1)->set<Bool>(BaseProperties::MEMBER_OF_PARALLEL, true);
    (*p.predecessor2)->set<Bool>(BaseProperties::MEMBER_OF_PARALLEL, true);

    p.segment1 = t1.segment;
    p.segment2 = t2.segment;

    p.staff1 = t1.staff;
    p.staff2 = t2.staff;

    p.trackPosition1 = t1.TrackPosition;
    p.trackPosition2 = t2.TrackPosition;

    p.trackLabel1 = t1.trackLabel;
    p.trackLabel2 = t2.trackLabel;

    p.time = t1.time;
}

bool
CheckForParallelsDialog::hasParallels(std::vector<Transition> &tSet, std::vector<Parallel> &parVec)
{
    int intervalBegin;
    int intervalEnd;

    if (tSet.size()<2)  // we need at least two voices for a parallel
        return false;

    parVec.clear();

    Parallel p;

    for (unsigned int i=0; i<tSet.size(); ++i) {
        for (unsigned int j=i+1; j<tSet.size(); ++j) {
            std::string s;

            // interval at end of transition

            int pitch1End = QString::fromUtf8((*(tSet[i].note))->getAsString("pitch").c_str()).toInt();
            int pitch2End = QString::fromUtf8((*(tSet[j].note))->getAsString("pitch").c_str()).toInt();

            // pitch2End shall be the upper voice
            // we need this later when testing for hidden parallels

            int pitch1Begin = QString::fromUtf8((*(tSet[i].predecessor))->getAsString("pitch").c_str()).toInt();
            int pitch2Begin = QString::fromUtf8((*(tSet[j].predecessor))->getAsString("pitch").c_str()).toInt();

            if (pitch1End>pitch2End) {

            	// we need to switch the voices

                int h = pitch2End;
                pitch2End = pitch1End;
                pitch1End = h;

                h = pitch2Begin;
                pitch2Begin = pitch1Begin;
                pitch1Begin = h;
            }

            qDebug() << "p1Begin=" << pitch1Begin << ", p2Begin=" << pitch2Begin << ", p1End=" << pitch1End << ", p2End=" << pitch2End;

            // flag whether both voices move in the same direction

            bool moveSameDirection;

            if ((pitch1Begin>pitch1End &&  pitch2Begin>pitch2End) || (pitch1Begin<pitch1End &&  pitch2Begin<pitch2End)) {
                moveSameDirection = true;
            } else {
                moveSameDirection = false;
            }

            intervalEnd = pitch2End-pitch1End;
            intervalBegin = pitch2Begin-pitch1Begin;

            // we bring the intervals into the same octave

            int reducedIntervalEnd = intervalEnd;

            while (reducedIntervalEnd>12)
                reducedIntervalEnd -= 12;

            int reducedIntervalBegin = intervalBegin;

            while (reducedIntervalBegin>12)
                reducedIntervalBegin -= 12;

//            qDebug() << "Begin: pitch1 is " << pitch1Begin << ", pitch2 is " << pitch2Begin << ", interval is " << intervalBegin;
//            qDebug() << "End:   pitch1 is " << pitch1End << ", pitch2 is " << pitch2End << ", interval is " << intervalEnd;

            // here we do the tests

            if (reducedIntervalEnd == 0 && reducedIntervalBegin == 0 && pitch1Begin != pitch1End && checkForUnisons) {

                // this is a simple unison parallel

                p.type = UNISON;
                populateParallel(tSet[i], tSet[j], p);
                parVec.push_back(p);

            } else if (reducedIntervalEnd == 12 && reducedIntervalBegin == 12 && pitch1Begin != pitch1End && moveSameDirection) {

                // this is a simple octave parallel

                p.type = OCTAVE;
                populateParallel(tSet[i], tSet[j], p);
                parVec.push_back(p);

            } else if (reducedIntervalEnd == 7 && reducedIntervalBegin == 7 && pitch1Begin != pitch1End && moveSameDirection) {

                // this is a simple fifth parallel, perfect -> perfect

                p.type = FIFTH;
                populateParallel(tSet[i], tSet[j], p);

                parVec.push_back(p);
            } else if (reducedIntervalEnd == 7 && reducedIntervalBegin == 6 && moveSameDirection) {

                // this is a fifth parallel, diminished -> perfect
                //
                // perfect -> diminished is allowed

                p.type = FIFTH;
                populateParallel(tSet[i], tSet[j], p);
                parVec.push_back(p);

            } else if (intervalEnd == 7
                    && moveSameDirection
                    && (abs(pitch2End-pitch2Begin) > 2)
                    && checkForHiddenParallels) {

                // this is a hidden fifth parallel
                //
                // conditions:
                //
                // second interval is a perfect fifth
                // both voices move in the same direction
                // upper voice does not move in semitone or whole tone

                p.type = HIDDEN_FIFTH;
                populateParallel(tSet[i], tSet[j], p);
                parVec.push_back(p);

            } else if (intervalEnd == 12
                    && moveSameDirection
                    && (abs(pitch2End-pitch2Begin) > 2)
                    && checkForHiddenParallels) {

                // this is a hidden octave parallel
                //
                // conditions:
                //
                // second interval is an octave (or unison)
                // both voices move in the same direction
                // upper voice does not move in semitone or whole tone

                p.type = HIDDEN_OCTAVE;
                populateParallel(tSet[i], tSet[j], p);
                parVec.push_back(p);
            }
        }
    }

    if (parVec.size() > 0) {
        return true;
    } else {
        return false;
    }
}

void
CheckForParallelsDialog::addText(QString text)
{
    QTextCursor cursor = textBrowser->textCursor();
    cursor.movePosition(QTextCursor::End);
    textBrowser->setTextCursor(cursor);
    textBrowser->setTextColor(QColor("black"));
    textBrowser->insertPlainText(text);

    qDebug() << "added text:" << text;
}


}

