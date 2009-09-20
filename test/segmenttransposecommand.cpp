/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

#include "commands/segment/SegmentTransposeCommand.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/Selection.h"

using namespace Rosegarden;
using std::cout;

/**
 * Bb in Bb major became E# in F major, due to segment 
 * transposition 
 *
 * Should be F
 */
int testSegmentBbtoF()
{
    Segment * segment1 = new Segment();
    Note * n = new Note(Note::QuarterNote);
    Event * bes = n->getAsNoteEvent(1, 10);
    segment1->insert(bes);
    segment1->insert(Key("Bb major").getAsEvent(0));
    SegmentTransposeCommand * mockCommand = 
        new SegmentTransposeCommand(*segment1,
            true, -3, -5, true);
    mockCommand->execute();
	
    EventSelection m_selection(*segment1, segment1->getStartTime(), segment1->getEndMarkerTime());
    EventSelection::eventcontainer::iterator i;
    for (i = m_selection.getSegmentEvents().begin();
            i != m_selection.getSegmentEvents().end(); ++i) {
	if ((*i)->isa(Note::EventType)) {
            Pitch resultPitch(**i);
            std::cout << "Resulting pitch is: " << resultPitch.getPerformancePitch() << std::endl;
            std::cout << "accidental: " << resultPitch.getDisplayAccidental(Key("F major")) << std::endl;
            std::cout << "DisplayAccidental: " << resultPitch.getDisplayAccidental(Key("F major")) << std::endl;
            if (resultPitch.getDisplayAccidental(Key("F major")) != Accidentals::NoAccidental)
            {
                return -1;
            }
        }
    }
    
    return 0;
}

/**
 * G# in E major became Bb in F major, due to segment 
 * transposition (by using the 'segment transposition' combobox)
 *
 * Should be A#
 */
int testGistoAis()
{
    Segment * segment1 = new Segment();
    Note * n = new Note(Note::QuarterNote);
    Event * gis = n->getAsNoteEvent(1, 8);
    segment1->insert(gis);
    segment1->insert(Key("E major").getAsEvent(0));
    SegmentTransposeCommand * mockCommand = 
        new SegmentTransposeCommand(*segment1,
            true, 1, 2, true);
    mockCommand->execute();
	
    EventSelection m_selection(*segment1, segment1->getStartTime(), segment1->getEndMarkerTime());
    EventSelection::eventcontainer::iterator i;
    for (i = m_selection.getSegmentEvents().begin();
            i != m_selection.getSegmentEvents().end(); ++i) {
	if ((*i)->isa(Note::EventType)) {
            Pitch resultPitch(**i);
            std::cout << "Resulting pitch is: " << resultPitch.getPerformancePitch() << std::endl;
            std::cout << "accidental: " << resultPitch.getDisplayAccidental(Key("F# major")) << std::endl;
            std::cout << "DisplayAccidental: " << resultPitch.getDisplayAccidental(Key("F# major")) << std::endl;
            if (resultPitch.getDisplayAccidental(Key("F# major")) != Accidentals::NoAccidental)
            {
                std::cout << "Gis in E major does not become A#-in-F#-major (no-accidental) when transposed upwards by a small second" << std::endl;
                return -1;
            }
        }
    }
    
    return 0;
}

/**
 * A C# in the key of C# major somehow became a B# in the key of C
 */
int testSegmentCisToC()
{
    Segment * segment1 = new Segment();
    Note * n = new Note(Note::QuarterNote);
    Event * cis = n->getAsNoteEvent(1, 13);
    segment1->insert(cis);
    segment1->insert(Key("C# major").getAsEvent(0));
    SegmentTransposeCommand * mockCommand = 
        new SegmentTransposeCommand(*segment1,
            true, 0, -1, true);
    mockCommand->execute();
	
    EventSelection m_selection(*segment1, segment1->getStartTime(), segment1->getEndMarkerTime());
    EventSelection::eventcontainer::iterator i;
    for (i = m_selection.getSegmentEvents().begin();
            i != m_selection.getSegmentEvents().end(); ++i) {
	if ((*i)->isa(Note::EventType)) {
            Pitch resultPitch(**i);
            std::cout << "Resulting pitch is: " << resultPitch.getPerformancePitch() << std::endl;
            std::cout << "accidental: " << resultPitch.getDisplayAccidental(Key("C major")) << std::endl;
            std::cout << "DisplayAccidental: " << resultPitch.getDisplayAccidental(Key("C major")) << std::endl;
            if (resultPitch.getDisplayAccidental(Key("C major")) != Accidentals::NoAccidental)
            {
                std::cout << "C# in C# major does not lose accidental when transposed downwards by 1 semitone" << std::endl;
                return -1;
            }
        }
    }
    
    return 0;
}

int testUndo()
{
    Segment * segment1 = new Segment();
    Segment * segment2 = new Segment();

    // transpose once
    SegmentTransposeCommand * mockCommand1a = 
        new SegmentTransposeCommand(*segment1,
            true, -1, -2, true);
    mockCommand1a->execute();
    SegmentTransposeCommand * mockCommand1b = 
        new SegmentTransposeCommand(*segment2,
            true, -1, -2, true);
    mockCommand1b->execute();

    // transpose twice
    SegmentTransposeCommand * mockCommand2a = 
        new SegmentTransposeCommand(*segment1, 
            true, -1, -2, true);
    mockCommand2a->execute();
    SegmentTransposeCommand * mockCommand2b = 
        new SegmentTransposeCommand(*segment2, 
            true, -1, -2, true);
    mockCommand2b->execute();

    mockCommand2b->unexecute();
    mockCommand2a->unexecute();
    mockCommand1b->unexecute();
    mockCommand1a->unexecute();
    
    return 0;
}

int test_segmenttransposecommand(int argc, char** argv)
{
    return 
    	testGistoAis() +
    	testSegmentCisToC() +
    	testUndo() + 
        testSegmentBbtoF();
}
