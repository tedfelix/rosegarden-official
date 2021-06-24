/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

#include "sound/MidiFile.h"
#include "document/RosegardenDocument.h"

#include <QTest>
#include <QDebug>

using namespace Rosegarden;

// Unit test for command line file conversion.
// This is just a simple test that will crash if anything's wrong.
class TestConvert : public QObject
{
    Q_OBJECT

private Q_SLOTS:

    void test1();
};

void TestConvert::test1()
{
    // For now, just do the conversion like main.cpp::convert() and
    // let it crash.

    RosegardenDocument doc(
            nullptr,  // parent
            {},  // audioPluginManager
            true,  // skipAutoload
            true,  // clearCommandHistory
            false);  // m_useSequencer

    bool ok;

    ok = doc.openDocument(
            "../../data/examples/aylindaamiga.rg",
            false,  // permanent
            true,  // squelchProgressDialog
            false);  // enableLock
    QVERIFY(ok);

    QString outFilename = "aylindaamiga.mid";

    MidiFile midiFile;
    ok = midiFile.convertToMidi(&doc, outFilename);
    QVERIFY(ok);

    // Clean up.
    QFile::remove(outFilename);
}

QTEST_MAIN(TestConvert)

#include "convert.moc"

