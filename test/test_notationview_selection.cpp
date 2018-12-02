#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include <QTest>
#include "gui/seqmanager/SequenceManager.h"
#include "document/RosegardenDocument.h"
#include "gui/editors/notation/NotationView.h"

int init()
{
    qputenv("QT_FATAL_WARNINGS", "1");
    return 0;
}
Q_CONSTRUCTOR_FUNCTION(init)

using namespace Rosegarden;

// This test opens data/examples/test_selection.rg and simulates using the keyboard to navigate and select in the notation view
class TestNotationViewSelection : public QObject
{
    Q_OBJECT

public:
    TestNotationViewSelection()
        : m_doc(nullptr, nullptr, true /*skip autoload*/, true, false /*no sound*/),
          m_view(nullptr) {}

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void testNavigate();
    void testKeyboardSelection_data();
    void testKeyboardSelection();
    void testSelectForwardAndBackward();

private:
    QString selectedNotes() const;

    RosegardenDocument m_doc;
    Segment *m_segment;
    NotationView *m_view;
    SequenceManager m_seqManager;
};

void TestNotationViewSelection::initTestCase()
{
    // Loading from a file
    const QString input = QFINDTESTDATA("../data/examples/test_selection.rg");
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

    m_seqManager.setDocument(&m_doc);

    // The mainwindow connects fast-forward and rewind (to intercept them when recording), so we need to do it ourselves here.
    connect(m_view, &NotationView::fastForwardPlayback,
            &m_seqManager, &SequenceManager::fastforward);
    connect(m_view, &NotationView::rewindPlayback,
            &m_seqManager, &SequenceManager::rewind);
    connect(m_view, &NotationView::fastForwardPlaybackToEnd,
            &m_seqManager, &SequenceManager::fastForwardToEnd);
    connect(m_view, &NotationView::rewindPlaybackToBeginning,
            &m_seqManager, &SequenceManager::rewindToBeginning);

}

void TestNotationViewSelection::cleanupTestCase()
{
    delete m_view;
}

// Returns the notes in the selection, as a string of note names. Ex: "ABBDG".
QString TestNotationViewSelection::selectedNotes() const
{
    EventSelection *selection = m_view->getSelection();
    if (!selection) {
        return QString();
    }
    const EventContainer &eventContainer = selection->getSegmentEvents();
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

void TestNotationViewSelection::init()
{
    // Before each test, unselect all and go back to position 0
    m_view->setSelection(nullptr, false);
    m_doc.slotSetPointerPosition(0);
}

void TestNotationViewSelection::testNavigate()
{
    // Go right one note
    m_view->slotStepForward();
    QCOMPARE(m_doc.getComposition().getPosition(), timeT(960)); // one quarter

    // Go to next bar
    m_seqManager.fastforward();
    QCOMPARE(m_doc.getComposition().getPosition(), timeT(3840)); // one quarter

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
        QCOMPARE(m_doc.getComposition().getPosition(), timeT(3840 + expectedPositions.at(i)));
    }
}

void TestNotationViewSelection::testKeyboardSelection_data()
{
    QTest::addColumn<QString>("keysPressed");
    QTest::addColumn<QStringList>("expectedSelections");

    // Syntax for keyPressed:
    // l = left, r = right, L = shift-left, R = shift right
    // n = next bar (ctrl+right), N = select until next bar (ctrl+shift+right)
    // p = prev bar (ctrl+left), P = select until previous bar (ctrl+shift+left)

    // To understand expectedSelections, note that the beginning of the file says: ABCDG[rest]...
    QTest::newRow("1-3-5") << "RrRrR" << (QStringList() << "A" << "A" << "AC" << "AC" << "ACG");
    QTest::newRow("shift_change_direction_1") << "RRLR" << (QStringList() << "A" << "AB" << "A" << "AB");
    QTest::newRow("shift_change_direction_2") << "nLLRL" << (QStringList() << "" << "D" << "CD" << "D" << "CD");
    QTest::newRow("bug_1519_testcase_2") << "RrL" << (QStringList() << "A" << "A" << "AB");
    QTest::newRow("shift_right_again_same_note") << "RRlR" << (QStringList() << "A" << "AB" << "AB" << "A");
    QTest::newRow("shift_left_again_same_note") << "nLLrL" << (QStringList() << "" << "D" << "CD" << "CD" << "D");
    QTest::newRow("select_unselect_bar") << "NprNP" << (QStringList() << "ABCD" << "ABCD" << "ABCD" << "A" << "ABCD");
}

void TestNotationViewSelection::testKeyboardSelection()
{
    QFETCH(QString, keysPressed);
    QFETCH(QStringList, expectedSelections);
    for (int i = 0 ; i < keysPressed.size(); ++i) {
        const QChar key = keysPressed.at(i);
        switch (key.toLatin1()) {
        case 'l':
            m_view->slotStepBackward();
            break;
        case 'r':
            m_view->slotStepForward();
            break;
        case 'L':
            m_view->slotExtendSelectionBackward();
            break;
        case 'R':
            m_view->slotExtendSelectionForward();
            break;
        case 'n':
            m_seqManager.fastforward();
            break;
        case 'N':
            m_view->slotExtendSelectionForwardBar();
            break;
        case 'p':
            m_seqManager.rewind();
            break;
        case 'P':
            m_view->slotExtendSelectionBackwardBar();
            break;
        }
        const QString prefix = QString("step %1, key %2: ").arg(i).arg(key); // more info in case of failure
        QCOMPARE(prefix + selectedNotes(), prefix + expectedSelections.at(i));
    }
}

void TestNotationViewSelection::testSelectForwardAndBackward()
{
    m_seqManager.fastforward();

    QStringList expectedSelections;
    expectedSelections << "G"
                       << "G" // the rest doesn't get selected, currently
                       << "GB"
                       << "GBCC" // tied notes get selected together
                       << "GBCCBB"
                       << "GBCCBBGGG"
                       << "GBCCBBGGGCC"
                       << "GBCCBBGGGCCG"
                       << "GBCCBBGGGCCGBDBD"
                       << "GBCCBBGGGCCGBDBDG"
                       << "GBCCBBGGGCCGBDBDGC"
                       ;

    // select forward
    for (int i = 0 ; i < expectedSelections.size(); ++i) {
        m_view->slotExtendSelectionForward();
        QCOMPARE(selectedNotes(), expectedSelections.at(i));
    }

    const int pos = m_doc.getComposition().getPosition();

    // unselect backward
    QStringList expectedSelectionsBack = expectedSelections;
    std::reverse(expectedSelectionsBack.begin(), expectedSelectionsBack.end());
    expectedSelectionsBack.append(QString());

    for (int i = 0 ; i < expectedSelectionsBack.size(); ++i) {
        if (i > 0)
            m_view->slotExtendSelectionBackward();
        QCOMPARE(selectedNotes(), expectedSelectionsBack.at(i));
    }
    QCOMPARE(selectedNotes(), QString());

    // select everything backward, check at end
    m_doc.slotSetPointerPosition(pos);
    for (int i = 0 ; i < expectedSelections.size(); ++i) {
        m_view->slotExtendSelectionBackward();
    }
    QCOMPARE(m_doc.getComposition().getPosition(), timeT(3840)); // one quarter
    QCOMPARE(selectedNotes(), QString("GBCCBBGGGCCGDBDBGC")); // order of notes in the chords is reversed, doesn't matter
}

QTEST_MAIN(TestNotationViewSelection)

#include "test_notationview_selection.moc"
