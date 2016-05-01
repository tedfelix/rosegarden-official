#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include <QTest>
#include "document/RosegardenDocument.h"
#include "gui/editors/notation/NotationView.h"

int init()
{
    qputenv("QT_FATAL_WARNINGS", "1");
    return 0;
}
Q_CONSTRUCTOR_FUNCTION(init)

using namespace Rosegarden;

class TestNotationViewSelection : public QObject
{
    Q_OBJECT

public:
    TestNotationViewSelection()
        : m_doc(0, 0, true /*skip autoload*/, true, false /*no sound*/),
          m_view(0) {}

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void testNavigate();
    void testSelectForward();

private:
    RosegardenDocument m_doc;
    Segment *m_segment;
    NotationView *m_view;
};

// Qt5 has a nice QFINDTESTDATA, but to support Qt4 we'll have our own function
static QString findFile(const QString &fileName) {
    QString attempt = QFile::decodeName(SRCDIR) + '/' + fileName;
    if (QFile::exists(attempt))
        return attempt;
    qWarning() << fileName << "NOT FOUND";
    return QString();
}

void TestNotationViewSelection::initTestCase()
{
    // Loading from a file
    const QString input = findFile("../data/examples/test_selection.rg");
    QVERIFY(!input.isEmpty()); // file not found
    m_doc.openDocument(input, false /*not permanent, i.e. don't create midi devices*/, true /*no progress dlg*/);

    const SegmentMultiSet segments = m_doc.getComposition().getSegments();
    QVERIFY(!segments.empty());
    m_segment = *segments.begin();
    std::vector<Segment *> segmentsVec;
    segmentsVec.push_back(m_segment);

    m_view = new NotationView(&m_doc, segmentsVec);

    QVERIFY(!m_view->getSelection());
    QCOMPARE(m_view->getCurrentSegment(), m_segment);
    QCOMPARE(m_segment->getStartTime(), timeT(0));
}

void TestNotationViewSelection::cleanupTestCase()
{
    delete m_view;
}

void TestNotationViewSelection::testNavigate()
{
    m_doc.slotSetPointerPosition(0);

    QVector<timeT> expectedPositions;
    expectedPositions << 960       // one quarter
                      << 960 * 2   // one rest
                      << 960 * 2.5 // one eighth
                      << 960 * 3   // another
                      << 960 * 3.5 // another
                      << 3840      // second bar
                      << 3840 + 480
                      << 3840 + 960
                      << 3840 + 960 * 2
                      << 7680
                      << 7680 + 3840
                      << 15360
                      << 15360 + 480
                      << 15360 + 480 * 2
                      << 15360 + 480 * 3
                      << 15360 + 480 * 4
                         ;
    for (int i = 0 ; i < expectedPositions.size(); ++i) {
        m_view->slotStepForward();
        QCOMPARE(m_doc.getComposition().getPosition(), expectedPositions.at(i));
    }
}

// Returns the notes in the selection, as a string of note names. Ex: "ABBDG".
static QString selectionNotes(const EventContainer &eventContainer)
{
    QString ret;
    Key defaultKey;
    for (EventContainer::const_iterator it = eventContainer.begin(); it != eventContainer.end(); ++it) {
        Event *ev = *it;
        if (ev->isa(Note::EventType)) {
            ret += Pitch(*ev).getNoteName(defaultKey);
        } else if (ev->isa(Note::EventRestType)) {
            ret += 'R';
        }
    }
    return ret;
}

void TestNotationViewSelection::testSelectForward()
{
    m_doc.slotSetPointerPosition(0);

    QStringList expectedSelections;
    expectedSelections << "A"
                       << "A" // the rest doesn't get selected
                       << "AB"
                       << "ABCC" // tied notes get selected together
                       << "ABCCBB"
                       << "ABCCBBGGG"
                       << "ABCCBBGGGCC"
                       << "ABCCBBGGGCCG"
                       << "ABCCBBGGGCCGDBDB"
                       //<< "ABCCBBGGGCCGDBDBG" // BUG!
                       //<< "ABCCBBGGGCCGDBDBGC"
                       ;

    for (int i = 0 ; i < expectedSelections.size(); ++i) {
        m_view->slotExtendSelectionForward();
        QCOMPARE(selectionNotes(m_view->getSelection()->getSegmentEvents()), expectedSelections.at(i));
    }

    return; // ## the rest of this test is disabled for now

    QStringList expectedSelectionsBack = expectedSelections;
    std::reverse(expectedSelectionsBack.begin(), expectedSelectionsBack.end());
    expectedSelectionsBack.append(QString());

    for (int i = 0 ; i < expectedSelections.size(); ++i) {
        m_view->slotExtendSelectionBackward();
        QCOMPARE(selectionNotes(m_view->getSelection()->getSegmentEvents()), expectedSelectionsBack.at(i));
    }
}

QTEST_MAIN(TestNotationViewSelection)

#include "test_notationview_selection.moc"
