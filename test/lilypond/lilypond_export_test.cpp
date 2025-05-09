/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

#include "base/NotationTypes.h"
#include "document/io/LilyPondExporter.h"
#include "document/RosegardenDocument.h"
#include "misc/ConfigGroups.h"

#include <QTest>
#include <QDebug>
#include <QProcess>
#include <QSettings>

#include <unistd.h>

using namespace Rosegarden;

// Unit test for lilypond export
class TestLilypondExport : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    // QTest special functions...

    // Called once.
    void initTestCase();

    // Our test functions.
    
    void testEmptyDocument();

    // Data-driven testing against the example .rg files.
    // https://doc.qt.io/qt-5/qttestlib-tutorial2-example.html
    // Generate the test data.
    void testExamples_data();
    // Called for each row of test data.
    void testExamples();
};

void TestLilypondExport::initTestCase()
{
    // Make sure settings end up in the right place.
    QCoreApplication::setOrganizationName("rosegardenmusic");

    // Change the settings so that they are consistent and not affected
    // by defaults.
    QSettings settings;
    settings.beginGroup(LilyPondExportConfigGroup);
    settings.setValue("lilyfontsize", 12); // the default of 26 is really huge!
    settings.setValue("lilyexportbeamings", true);
}

// Read an entire file line-by-line.
// This is used by checkFile() to read the expected file and the
// actual file for comparison.
QList<QByteArray> readLines(const QString &fileName)
{
    QList<QByteArray> lines;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Couldn't open" << fileName;
        return lines;
    }

    // Read the entire file into lines.
    while (!file.atEnd()) {
        lines.append(file.readLine());
    }

    return lines;
}

const char header1[] =
        "This LilyPond file was generated by Rosegarden";
const char header2[] =
        "Created using Rosegarden ";

// Check the generated file (fileName) against the baseline file.
void checkFile(const QString &fileName, const QString &baseline)
{
    // Read in the files.
    const QList<QByteArray> expected = readLines(baseline);
    QList<QByteArray> actual = readLines(fileName);
    
    // For each line in the files...
    for (int i = 0; i < expected.count() && i < actual.count(); ++i) {
        const QByteArray line = actual.at(i);
        const QByteArray expectedLine = expected.at(i);
        
        // Skip header lines which have rosegarden versions in them.
        if (line.contains(header1) &&
                expectedLine.contains(header1))
            continue;
        if (line.contains(header2) &&
                expectedLine.contains(header2))
            continue;
        
        // Compare the remaining lines.
        QCOMPARE(QString(line), QString(expectedLine));
    }
    
    // Make sure the same number of lines were compared.
    QCOMPARE(actual.count(), expected.count());
}

void TestLilypondExport::testEmptyDocument()
{
    // GIVEN a document and a lilypond exporter
    RosegardenDocument doc(
            nullptr,  // parent
            {},  // audioPluginManager
            true,  // skipAutoload
            true,  // clearCommandHistory
            false);  // enableSound
    const QString fileName = "out.ly";
    LilyPondExporter exporter(&doc, SegmentSelection(), qstrtostr(fileName));

    // WHEN generating lilypond
    bool result = exporter.write();

    // THEN it should produce the file but return false and a warning message
    QVERIFY(!result);
    QCOMPARE(exporter.getMessage(),
             QString("Export succeeded, but the composition was empty."));

    // ... and the output file should match "empty.ly"
    checkFile(fileName, QFINDTESTDATA("baseline/empty.ly"));

    // Clean up.
    QFile::remove(fileName);
}

void TestLilypondExport::testExamples_data()
{
    // Usually "examples", but one "regression".
    QTest::addColumn<QString>("baseDir");

    QTest::newRow("aveverum") << "examples";
    QTest::newRow("aylindaamiga") << "examples";
    QTest::newRow("bogus-surf-jam") << "examples";
    QTest::newRow("beaming") << "examples";
    QTest::newRow("Brandenburg_No3-BWV_1048") << "examples";

    // ??? This one creates a directory in the user's home directory!?
    //     The directory's name is simply a space (' ').  Why?
    //     Otherwise this passes.  Please fix this one day.
    // ??? This should work fine now.  The original issue was an audio path
    //     that was set to a single space.  I went through all of the examples
    //     and fixed these sorts of issues.  See [26b26e6e] from May 15, 2024.
    //     Feel free to re-enable this.
    //QTest::newRow("bwv-1060-trumpet-duet-excerpt") << "examples";

    QTest::newRow("children") << "examples";
    QTest::newRow("Chopin-Prelude-in-E-minor-Aere") << "examples";
    QTest::newRow("Djer-Fire") << "examples";
    QTest::newRow("doodle-q") << "examples";
    QTest::newRow("exercise_notation") << "examples";
    QTest::newRow("glazunov-for-solo-and-piano-with-cue") << "examples";
    QTest::newRow("glazunov") << "examples";
    QTest::newRow("Hallelujah_Chorus_from_Messiah") << "examples";
    QTest::newRow("headers-and-unicode-lyrics") << "examples";
    QTest::newRow("himno_de_riego") << "examples";
    QTest::newRow("interpretation-example") << "examples";
    QTest::newRow("let-all-mortal-flesh") << "examples";
    QTest::newRow("lilypond-alternative-endings_new-way") << "examples";
    QTest::newRow("lilypond-alternative-endings") << "examples";
    QTest::newRow("lilypond-directives") << "examples";
    QTest::newRow("lilypond-up-down") << "examples";
    QTest::newRow("lilypond-staff-groupings") << "examples";
    QTest::newRow("lilypond-tied-grace-notes") << "examples";
    QTest::newRow("logical-segments-4") << "examples";
    QTest::newRow("mandolin-sonatina") << "examples";
    QTest::newRow("marks-test") << "examples";
    QTest::newRow("mozart-quartet") << "examples";
    QTest::newRow("notation-for-string-orchestra-in-D-minor") << "examples";
    QTest::newRow("perfect-moment") << "examples";
    QTest::newRow("ravel-pc-gmaj-adagio") << "examples";
    QTest::newRow("Romanza") << "examples";

    // THIS ONE FAILS
    // sicut-locutus.ly:107:47: Erreur : syntax error, unexpected '}'
    //                 < f g > 2 _\markup { \italic  
    //                                               } _\markup { \italic Masked and substituted }  _~ f _~  |
    //QTest::newRow("sicut-locutus") << "examples";

    QTest::newRow("stormy-riders") << "examples";
    QTest::newRow("test_tuplets") << "examples";
    QTest::newRow("the-rose-garden") << "examples";
    QTest::newRow("vivaldi-cs3mv2") << "examples";
    QTest::newRow("vivaldi_op44_11_1") << "examples";

    // data/regression
    QTest::newRow("export_hidden_key_signatures") << "regression";
}

void TestLilypondExport::testExamples()
{
    QString baseName{QTest::currentDataTag()};
    QFETCH(QString, baseDir);

    // GIVEN
    const QString input =
            QFINDTESTDATA("../../data/" + baseDir + "/" + baseName + ".rg");
    QVERIFY(!input.isEmpty()); // file not found

    const QString expected = QFINDTESTDATA("baseline/" + baseName + ".ly");

    const QString fileName = baseName + ".ly";
    qDebug() << "Loading" << input << "and exporting to" << fileName;

    // Load the .rg file.

    RosegardenDocument doc(
            nullptr,  // parent
            {},  // audioPluginManager
            true,  // skipAutoload
            true,  // clearCommandHistory
            false);  // enableSound

    doc.openDocument(
            input,  // filename
            false,  // permanent (false => no MIDI devices)
            true);  // squelchProgressDialog

    // Export the .ly file.

    LilyPondExporter exporter(
            &doc,  // doc
            SegmentSelection(),  // selection
            qstrtostr(fileName));  // fileName

    // WHEN
    QVERIFY(exporter.write());

    // THEN

    // If there is no baseline yet, create it.
    if (expected.isEmpty()) {
        // Use lilypond to check this file compiles before we add it to
        // our baseline
        QProcess proc;
        proc.start("lilypond", QStringList() << "--ps" << fileName);
        proc.waitForStarted();
        proc.waitForFinished();
        QCOMPARE(proc.exitStatus(), QProcess::NormalExit);
        if (proc.exitCode() != 0) {
            qWarning() << "Generated file" << fileName << "does NOT compile!";
        }

        std::cout << "*********** GENERATING NEW BASELINE FILE (remember to add it to git) ***********" << std::endl;
        QFile in(fileName);
        QVERIFY(in.open(QIODevice::ReadOnly));
        QFile out(QFile::decodeName(SRCDIR) + "/baseline/" + baseName + ".ly");
        QVERIFY(!out.exists());
        QVERIFY(out.open(QIODevice::WriteOnly));
        while (!in.atEnd()) {
            out.write(in.readLine());
        }

        // Make the test fail.  So developers add the baseline to git
        // and try again.
        QVERIFY(false);
    }

    // Compare the .ly file that was generated with the baseline file.
    checkFile(fileName, expected);

    // Clean up.
    QFile::remove(fileName);
}

QTEST_MAIN(TestLilypondExport)

#include "lilypond_export_test.moc"

