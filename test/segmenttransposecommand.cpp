/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

#include "commands/segment/SegmentTransposeCommand.h"
#include "base/NotationTypes.h"
#include "base/Pitch.h"
#include "base/Segment.h"
#include "base/Selection.h"

#include <QTest>

using namespace Rosegarden;
using std::cout;

class TestSegmentTransposeCommand : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testSegmentBbtoF();
    void testGistoAis();
    void testSegmentCisToC();
    void testUndo();
};

/**
 * Bb in Bb major became E# in F major, due to segment 
 * transposition 
 *
 * Should be F
 */
void TestSegmentTransposeCommand::testSegmentBbtoF()
{
    Segment segment1;
    Note note(Note::QuarterNote);
    Event *bes = note.getAsNoteEvent(1, 10);
    segment1.insert(bes);
    segment1.insert(Key("Bb major").getAsEvent(0));

    SegmentTransposeCommand mockCommand(segment1,
            true, -3, -5, true);
    mockCommand.execute();

    EventSelection selection(
            segment1,
            segment1.getStartTime(),
            segment1.getEndMarkerTime());

    for (EventContainer::iterator i = selection.getSegmentEvents().begin();
         i != selection.getSegmentEvents().end();
         ++i) {
        if ((*i)->isa(Note::EventType)) {
            Pitch resultPitch(**i);
            std::cout << "Resulting pitch is: " << resultPitch.getPerformancePitch() << std::endl;
            std::cout << "accidental: " << resultPitch.getDisplayAccidental(Key("F major")) << std::endl;
            std::cout << "DisplayAccidental: " << resultPitch.getDisplayAccidental(Key("F major")) << std::endl;
            QCOMPARE(resultPitch.getDisplayAccidental(Key("F major")), Accidentals::NoAccidental);
        }
    }
}

/**
 * G# in E major became Bb in F major, due to segment 
 * transposition (by using the 'segment transposition' combobox)
 *
 * Should be A#
 */
void TestSegmentTransposeCommand::testGistoAis()
{
    Segment segment1;
    Note note(Note::QuarterNote);
    Event *gis = note.getAsNoteEvent(1, 8);
    segment1.insert(gis);
    segment1.insert(Key("E major").getAsEvent(0));

    SegmentTransposeCommand mockCommand(segment1,
            true, 1, 2, true);
    mockCommand.execute();
	
    EventSelection selection(
            segment1,
            segment1.getStartTime(),
            segment1.getEndMarkerTime());

    for (EventContainer::iterator i = selection.getSegmentEvents().begin();
         i != selection.getSegmentEvents().end();
         ++i) {
        if ((*i)->isa(Note::EventType)) {
            Pitch resultPitch(**i);
            std::cout << "Resulting pitch is: " << resultPitch.getPerformancePitch() << std::endl;
            std::cout << "accidental: " << resultPitch.getDisplayAccidental(Key("F# major")) << std::endl;
            std::cout << "DisplayAccidental: " << resultPitch.getDisplayAccidental(Key("F# major")) << std::endl;
            if (resultPitch.getDisplayAccidental(Key("F# major")) != Accidentals::NoAccidental)
            {
                std::cout << "Gis in E major does not become A#-in-F#-major (no-accidental) when transposed upwards by a small second" << std::endl;
                QVERIFY(false);
            }
        }
    }
}

/**
 * A C# in the key of C# major somehow became a B# in the key of C
 */
void TestSegmentTransposeCommand::testSegmentCisToC()
{
    Segment segment1;
    Note note(Note::QuarterNote);
    Event *cis = note.getAsNoteEvent(1, 13);
    segment1.insert(cis);
    segment1.insert(Key("C# major").getAsEvent(0));

    SegmentTransposeCommand mockCommand(segment1,
            true, 0, -1, true);
    mockCommand.execute();
	
    EventSelection selection(
            segment1,
            segment1.getStartTime(),
            segment1.getEndMarkerTime());

    for (EventContainer::iterator i = selection.getSegmentEvents().begin();
         i != selection.getSegmentEvents().end();
         ++i) {
        if ((*i)->isa(Note::EventType)) {
            Pitch resultPitch(**i);
            std::cout << "Resulting pitch is: " << resultPitch.getPerformancePitch() << std::endl;
            std::cout << "accidental: " << resultPitch.getDisplayAccidental(Key("C major")) << std::endl;
            std::cout << "DisplayAccidental: " << resultPitch.getDisplayAccidental(Key("C major")) << std::endl;
            if (resultPitch.getDisplayAccidental(Key("C major")) != Accidentals::NoAccidental)
            {
                std::cout << "C# in C# major does not lose accidental when transposed downwards by 1 semitone" << std::endl;
                QVERIFY(false);
            }
        }
    }
}

void TestSegmentTransposeCommand::testUndo()
{
    Segment segment1;
    Segment segment2;

    // transpose once
    SegmentTransposeCommand mockCommand1a(segment1,
            true, -1, -2, true);
    mockCommand1a.execute();
    SegmentTransposeCommand mockCommand1b(segment2,
            true, -1, -2, true);
    mockCommand1b.execute();

    // transpose twice
    SegmentTransposeCommand mockCommand2a(segment1,
            true, -1, -2, true);
    mockCommand2a.execute();
    SegmentTransposeCommand mockCommand2b(segment2,
            true, -1, -2, true);
    mockCommand2b.execute();

    mockCommand2b.unexecute();
    mockCommand2a.unexecute();
    mockCommand1b.unexecute();
    mockCommand1a.unexecute();
}

QTEST_MAIN(TestSegmentTransposeCommand)

#include "segmenttransposecommand.moc"
