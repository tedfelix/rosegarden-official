/*
 * CheckForParallelsDialog.h
 *
 *  Created on: Mar 22, 2015
 *      Author: lambache
 */

#ifndef SRC_GUI_DIALOGS_CHECKFORPARALLELSDIALOG_H_
#define SRC_GUI_DIALOGS_CHECKFORPARALLELSDIALOG_H_

#include "base/Segment.h"
#include "base/Composition.h"
#include "base/Studio.h"
#include "document/RosegardenDocument.h"
#include "gui/editors/notation/NotationStaff.h"
#include "gui/editors/notation/NotationScene.h"
#include "gui/editors/notation/NotationView.h"

#include <QTextBrowser>
#include <QDialog>
#include <QString>
#include <QCheckBox>
#include <QPushButton>


// check for parallels in voices
//
// we go through all segments in the notation editor, take all events and look for progressions
//
// a progression consists of two notes with no rest in between
// the time of the progression is defined as the begin of the second note
//
// we examine all progressions that occur at the same time and check whether the progressions form a parallel movement
//
// these are shown in the textBrowser of the dialog and marked in color in the notation editor
//
// parallelism is checked by pitch values, so a diminished fifth is the same as an augmented fourth
// this means that a movement from an augmented fourth to a perfect fifth is the same as from an diminished fifth to a perfect fifth
//   and is therefore considered a parallel
//
// frankly, I have no idea how this could be figured out with sensible effort...
//
// as we go through the notation content by segment, we will not find transitions across segment boundaries
// in principle this should be possible, but is too complicated for me at the moment...


namespace Rosegarden
{


class CheckForParallelsDialog : public QDialog
{
    Q_OBJECT
public:
    typedef std::vector<Segment *> SegmentVector;

    // types of parallels

    enum ParallelType {UNISON, FIFTH, OCTAVE, HIDDEN_FIFTH, HIDDEN_OCTAVE};

    // 'Transition' represents a transition from one note to another
    //
    // this is the basic entity to check for parallels

    typedef struct {
        Segment::iterator note;             // the note on which the transition ends
        Segment::iterator predecessor;      // the note before
        Segment *segment;                   // pointer to the segment to which the transition belongs
        NotationStaff *staff;               // pointer to the staff to which the transition belongs
        int TrackPosition;                  // position of track in rosegarden gui
        QString trackLabel;
        timeT time;                         // occurrence time of the transition, i.e. start time of the note
    } Transition;

    // description of parallel

    typedef struct {
        ParallelType type;

        Segment::iterator note1;            // notes at end of parallel
        Segment::iterator note2;

        Segment::iterator predecessor1;     // notes at begin of parallel
        Segment::iterator predecessor2;

        Segment *segment1;                  // pointers to the segments
        Segment *segment2;

        NotationStaff *staff1;              // pointers to staves
        NotationStaff *staff2;

        int trackPosition1;                 // shown number of track in notation editor
        int trackPosition2;

        QString trackLabel1;                // label of track
        QString trackLabel2;

        timeT time;                         // occurrence time of the parallel, i.e. start note
    } Parallel;

    // a set of parallels that occur at a specific time
    typedef std::vector<Parallel> ParallelSet;

    // this describes where a parallel occurs in the notation view
    typedef struct {
        timeT   time;
        NotationStaff *staff;
    } parallelLocation;

    // link between segment and staff so we can position the pointer properly to the parallel
    typedef struct {
        Segment *segment;
        NotationStaff *staff;       // staff that the segment belongs to
    } SegmentStaffLink;

    // compare function for sorting the transitionList
    static bool sortByTime(const Transition &t1, const Transition &t2) { return t1.time < t2.time; }

    CheckForParallelsDialog(NotationView *parent, RosegardenDocument *document, NotationScene *ns, Composition *comp);

protected slots:
    void startCheck();
    void clear();
    void cleanUpAndLeave();
    void checkForUnisonsClicked();
    void checkForHiddenParallelsClicked();
    void exportText();
    void onTextBrowserclicked();

private:

    // add text to textBrowser
    void addText(QString text);

    // check a set of transitions for parallels
    void checkParallels(std::vector<Transition> &tSet);

    // returns true if the transition set has parallels
    bool hasParallels(std::vector<Transition> &tSet, std::vector<Parallel> &p);

    // fill the fields of a parallel with exception of type
    void populateParallel(Transition t1, Transition t2, Parallel &p);

    // returns a string that identifies a track
    // track label if present, otherwise position of track
    QString makeTrackString(int trackPosition, QString trackLabel);

    // updates the list of segments that are currently in the notationView
    void updateSegments();

    // write out the transition list (for debug purposes)
    void writeTransitionList(std::vector<Transition> transitionList);

    // write a single transition
    void writeTransition(std::vector<Transition>::iterator it);

    //-------------------------------------------------------------------------

    // where we display the results of the check
    QTextBrowser *textBrowser;

    // the notation view
    NotationView *notationView;

    // the document
    RosegardenDocument *document;

    // the current NotationScene
    NotationScene *notationScene;

    // all segments in the notation view (pointers!)
    SegmentVector segment;

    // current composition
    Composition *composition;

    // all transitions in all segments
    std::vector<Transition> transitionList;

    // all found parallels
    std::vector<ParallelSet> parallelList;

    // shall we check for unisons?
    /*static*/ bool checkForUnisons;
    QCheckBox *checkForUnisonsCheckBox;

    // shall we check for hidden parallels?
    /*static*/ bool checkForHiddenParallels;
    QCheckBox *checkForHiddenParallelsCheckBox;

    // last directory we exported the parallels list to
    static QString lastExportDirectory;

    // hack to find line when user clicks into textBrowser
    // could not find how to do this with mouse events in reasonable time
    // so we follow the cursor changes but we have to ignore this as long as we are populating the window
    bool ignoreCursor;

    // list of locations vs. line numbers in textBrowser
    std::vector<parallelLocation> locationForLine;

    // list of all links between staves and segments so we know to which staff a segement belongs
    std::vector<SegmentStaffLink> segmentStaffLinkList;

    // the button to start the search
    QPushButton *startButton;

    // a list of potential stop points of checking for each track in case of multiple notes
    std::vector<timeT> checkStopTime;
};


}



#endif /* SRC_GUI_DIALOGS_CHECKFORPARALLELSDIALOG_H_ */
/*
 * CheckForParallelsDialog.h
 *
 *  Created on: Mar 22, 2015
 *      Author: lambache
 */

#ifndef SRC_GUI_DIALOGS_CHECKFORPARALLELSDIALOG_H_
#define SRC_GUI_DIALOGS_CHECKFORPARALLELSDIALOG_H_

#include "base/Segment.h"
#include "base/Composition.h"
#include "base/Studio.h"
#include "document/RosegardenDocument.h"
#include "gui/editors/notation/NotationStaff.h"
#include "gui/editors/notation/NotationScene.h"
#include "gui/editors/notation/NotationView.h"

#include <QTextBrowser>
#include <QDialog>
#include <QString>
#include <QCheckBox>
#include <QPushButton>


// check for parallels in voices
//
// we go through all segments in the notation editor, take all events and look for progressions
//
// a progression consists of two notes with no rest in between
// the time of the progression is defined as the begin of the second note
//
// we examine all progressions that occur at the same time and check whether the progressions form a parallel movement
//
// these are shown in the textBrowser of the dialog and marked in color in the notation editor
//
// parallelism is checked by pitch values, so a diminished fifth is the same as an augmented fourth
// this means that a movement from an augmented fourth to a perfect fifth is the same as from an diminished fifth to a perfect fifth
//   and is therefore considered a parallel
//
// frankly, I have no idea how this could be figured out with sensible effort...
//
// as we go through the notation content by segment, we will not find transitions across segment boundaries
// in principle this should be possible, but is too complicated for me at the moment...


namespace Rosegarden
{


class CheckForParallelsDialog : public QDialog
{
    Q_OBJECT
public:
    typedef std::vector<Segment *> SegmentVector;

    // types of parallels

    enum ParallelType {UNISON, FIFTH, OCTAVE, HIDDEN_FIFTH, HIDDEN_OCTAVE};

    // 'Transition' represents a transition from one note to another
    //
    // this is the basic entity to check for parallels

    typedef struct {
        Segment::iterator note;             // the note on which the transition ends
        Segment::iterator predecessor;      // the note before
        Segment *segment;                   // pointer to the segment to which the transition belongs
        NotationStaff *staff;               // pointer to the staff to which the transition belongs
        int TrackPosition;                  // position of track in rosegarden gui
        QString trackLabel;
        timeT time;                         // occurrence time of the transition, i.e. start time of the note
    } Transition;

    // description of parallel

    typedef struct {
        ParallelType type;

        Segment::iterator note1;            // notes at end of parallel
        Segment::iterator note2;

        Segment::iterator predecessor1;     // notes at begin of parallel
        Segment::iterator predecessor2;

        Segment *segment1;                  // pointers to the segments
        Segment *segment2;

        NotationStaff *staff1;              // pointers to staves
        NotationStaff *staff2;

        int trackPosition1;                 // shown number of track in notation editor
        int trackPosition2;

        QString trackLabel1;                // label of track
        QString trackLabel2;

        timeT time;                         // occurrence time of the parallel, i.e. start note
    } Parallel;

    // a set of parallels that occur at a specific time
    typedef std::vector<Parallel> ParallelSet;

    // this describes where a parallel occurs in the notation view
    typedef struct {
        timeT   time;
        NotationStaff *staff;
    } parallelLocation;

    // link between segment and staff so we can position the pointer properly to the parallel
    typedef struct {
        Segment *segment;
        NotationStaff *staff;       // staff that the segment belongs to
    } SegmentStaffLink;

    // compare function for sorting the transitionList
    static bool sortByTime(const Transition &t1, const Transition &t2) { return t1.time < t2.time; }

    CheckForParallelsDialog(NotationView *parent, RosegardenDocument *document, NotationScene *ns, Composition *comp);

protected slots:
    void startCheck();
    void clear();
    void cleanUpAndLeave();
    void checkForUnisonsClicked();
    void checkForHiddenParallelsClicked();
    void exportText();
    void onTextBrowserclicked();

private:

    // add text to textBrowser
    void addText(QString text);

    // check a set of transitions for parallels
    void checkParallels(std::vector<Transition> &tSet);

    // returns true if the transition set has parallels
    bool hasParallels(std::vector<Transition> &tSet, std::vector<Parallel> &p);

    // fill the fields of a parallel with exception of type
    void populateParallel(Transition t1, Transition t2, Parallel &p);

    // returns a string that identifies a track
    // track label if present, otherwise position of track
    QString makeTrackString(int trackPosition, QString trackLabel);

    // updates the list of segments that are currently in the notationView
    void updateSegments();

    // write out the transition list (for debug purposes)
    void writeTransitionList(std::vector<Transition> transitionList);

    // write a single transition
    void writeTransition(std::vector<Transition>::iterator it);

    //-------------------------------------------------------------------------

    // where we display the results of the check
    QTextBrowser *textBrowser;

    // the notation view
    NotationView *notationView;

    // the document
    RosegardenDocument *document;

    // the current NotationScene
    NotationScene *notationScene;

    // all segments in the notation view (pointers!)
    SegmentVector segment;

    // current composition
    Composition *composition;

    // all transitions in all segments
    std::vector<Transition> transitionList;

    // all found parallels
    std::vector<ParallelSet> parallelList;

    // shall we check for unisons?
    /*static*/ bool checkForUnisons;
    QCheckBox *checkForUnisonsCheckBox;

    // shall we check for hidden parallels?
    /*static*/ bool checkForHiddenParallels;
    QCheckBox *checkForHiddenParallelsCheckBox;

    // last directory we exported the parallels list to
    static QString lastExportDirectory;

    // hack to find line when user clicks into textBrowser
    // could not find how to do this with mouse events in reasonable time
    // so we follow the cursor changes but we have to ignore this as long as we are populating the window
    bool ignoreCursor;

    // list of locations vs. line numbers in textBrowser
    std::vector<parallelLocation> locationForLine;

    // list of all links between staves and segments so we know to which staff a segement belongs
    std::vector<SegmentStaffLink> segmentStaffLinkList;

    // the button to start the search
    QPushButton *startButton;

    // a list of potential stop points of checking for each track in case of multiple notes
    std::vector<timeT> checkStopTime;
};


}



#endif /* SRC_GUI_DIALOGS_CHECKFORPARALLELSDIALOG_H_ */
/*
 * CheckForParallelsDialog.h
 *
 *  Created on: Mar 22, 2015
 *      Author: lambache
 */

#ifndef SRC_GUI_DIALOGS_CHECKFORPARALLELSDIALOG_H_
#define SRC_GUI_DIALOGS_CHECKFORPARALLELSDIALOG_H_

#include "base/Segment.h"
#include "base/Composition.h"
#include "base/Studio.h"
#include "document/RosegardenDocument.h"
#include "gui/editors/notation/NotationStaff.h"
#include "gui/editors/notation/NotationScene.h"
#include "gui/editors/notation/NotationView.h"

#include <QTextBrowser>
#include <QDialog>
#include <QString>
#include <QCheckBox>
#include <QPushButton>


// check for parallels in voices
//
// we go through all segments in the notation editor, take all events and look for progressions
//
// a progression consists of two notes with no rest in between
// the time of the progression is defined as the begin of the second note
//
// we examine all progressions that occur at the same time and check whether the progressions form a parallel movement
//
// these are shown in the textBrowser of the dialog and marked in color in the notation editor
//
// parallelism is checked by pitch values, so a diminished fifth is the same as an augmented fourth
// this means that a movement from an augmented fourth to a perfect fifth is the same as from an diminished fifth to a perfect fifth
//   and is therefore considered a parallel
//
// frankly, I have no idea how this could be figured out with sensible effort...
//
// as we go through the notation content by segment, we will not find transitions across segment boundaries
// in principle this should be possible, but is too complicated for me at the moment...


namespace Rosegarden
{


class CheckForParallelsDialog : public QDialog
{
    Q_OBJECT
public:
    typedef std::vector<Segment *> SegmentVector;

    // types of parallels

    enum ParallelType {UNISON, FIFTH, OCTAVE, HIDDEN_FIFTH, HIDDEN_OCTAVE};

    // 'Transition' represents a transition from one note to another
    //
    // this is the basic entity to check for parallels

    typedef struct {
        Segment::iterator note;             // the note on which the transition ends
        Segment::iterator predecessor;      // the note before
        Segment *segment;                   // pointer to the segment to which the transition belongs
        NotationStaff *staff;               // pointer to the staff to which the transition belongs
        int TrackPosition;                  // position of track in rosegarden gui
        QString trackLabel;
        timeT time;                         // occurrence time of the transition, i.e. start time of the note
    } Transition;

    // description of parallel

    typedef struct {
        ParallelType type;

        Segment::iterator note1;            // notes at end of parallel
        Segment::iterator note2;

        Segment::iterator predecessor1;     // notes at begin of parallel
        Segment::iterator predecessor2;

        Segment *segment1;                  // pointers to the segments
        Segment *segment2;

        NotationStaff *staff1;              // pointers to staves
        NotationStaff *staff2;

        int trackPosition1;                 // shown number of track in notation editor
        int trackPosition2;

        QString trackLabel1;                // label of track
        QString trackLabel2;

        timeT time;                         // occurrence time of the parallel, i.e. start note
    } Parallel;

    // a set of parallels that occur at a specific time
    typedef std::vector<Parallel> ParallelSet;

    // this describes where a parallel occurs in the notation view
    typedef struct {
        timeT   time;
        NotationStaff *staff;
    } parallelLocation;

    // link between segment and staff so we can position the pointer properly to the parallel
    typedef struct {
        Segment *segment;
        NotationStaff *staff;       // staff that the segment belongs to
    } SegmentStaffLink;

    // compare function for sorting the transitionList
    static bool sortByTime(const Transition &t1, const Transition &t2) { return t1.time < t2.time; }

    CheckForParallelsDialog(NotationView *parent, RosegardenDocument *document, NotationScene *ns, Composition *comp);

protected slots:
    void startCheck();
    void clear();
    void cleanUpAndLeave();
    void checkForUnisonsClicked();
    void checkForHiddenParallelsClicked();
    void exportText();
    void onTextBrowserclicked();

private:

    // add text to textBrowser
    void addText(QString text);

    // check a set of transitions for parallels
    void checkParallels(std::vector<Transition> &tSet);

    // returns true if the transition set has parallels
    bool hasParallels(std::vector<Transition> &tSet, std::vector<Parallel> &p);

    // fill the fields of a parallel with exception of type
    void populateParallel(Transition t1, Transition t2, Parallel &p);

    // returns a string that identifies a track
    // track label if present, otherwise position of track
    QString makeTrackString(int trackPosition, QString trackLabel);

    // updates the list of segments that are currently in the notationView
    void updateSegments();

    // write out the transition list (for debug purposes)
    void writeTransitionList(std::vector<Transition> transitionList);

    // write a single transition
    void writeTransition(std::vector<Transition>::iterator it);

    //-------------------------------------------------------------------------

    // where we display the results of the check
    QTextBrowser *textBrowser;

    // the notation view
    NotationView *notationView;

    // the document
    RosegardenDocument *document;

    // the current NotationScene
    NotationScene *notationScene;

    // all segments in the notation view (pointers!)
    SegmentVector segment;

    // current composition
    Composition *composition;

    // all transitions in all segments
    std::vector<Transition> transitionList;

    // all found parallels
    std::vector<ParallelSet> parallelList;

    // shall we check for unisons?
    /*static*/ bool checkForUnisons;
    QCheckBox *checkForUnisonsCheckBox;

    // shall we check for hidden parallels?
    /*static*/ bool checkForHiddenParallels;
    QCheckBox *checkForHiddenParallelsCheckBox;

    // last directory we exported the parallels list to
    static QString lastExportDirectory;

    // hack to find line when user clicks into textBrowser
    // could not find how to do this with mouse events in reasonable time
    // so we follow the cursor changes but we have to ignore this as long as we are populating the window
    bool ignoreCursor;

    // list of locations vs. line numbers in textBrowser
    std::vector<parallelLocation> locationForLine;

    // list of all links between staves and segments so we know to which staff a segement belongs
    std::vector<SegmentStaffLink> segmentStaffLinkList;

    // the button to start the search
    QPushButton *startButton;

    // a list of potential stop points of checking for each track in case of multiple notes
    std::vector<timeT> checkStopTime;
};


}



#endif /* SRC_GUI_DIALOGS_CHECKFORPARALLELSDIALOG_H_ */
