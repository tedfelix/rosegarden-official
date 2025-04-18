/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

#include "base/NotationTypes.h"
#include "document/io/LilyPondExporter.h"
#include "document/RosegardenDocument.h"
#include "misc/ConfigGroups.h"

#include <QDir>
#include <QTest>
#include <QDebug>
#include <QProcess>
#include <QSettings>

#include <unistd.h>

using namespace Rosegarden;

// Unit test for lilypond export
class TestLilypondVersion : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    // QTest special functions...
    
    // Called once.
    void initTestCase();

    // Our test functions.

    // Data-driven testing against the example .rg files.
    // https://doc.qt.io/qt-5/qttestlib-tutorial2-example.html
    // Generate the test data.
    void testExamples_data();
    // Called for each row of test data.
    void testExamples();

private:
    const char * destination;   // Directory where LilyPond files will be written
};

void TestLilypondVersion::initTestCase()
{
    // Make sure settings end up in the right place.
    QCoreApplication::setOrganizationName("rosegardenmusic");

    // Change the settings so that they are consistent and not affected
    // by defaults.
    QSettings settings;
    settings.beginGroup(LilyPondExportConfigGroup);
    settings.setValue("lilyfontsize", 12); // the default of 26 is really huge!
    settings.setValue("lilyexportbeamings", true);

    // YGYGYG SET HERE OTHER LILYPOND RELATIVE SETTIGS
    // settings.setValue("lilypapersize", PAPER_A4);
    settings.setValue("lilypaperlandscape", "false");
    settings.setValue("lilyraggedbottom", "false");

    // YGYGYG etc... TODO
    // m_exportEmptyStaves = qStrToBool(settings.value("lilyexportemptystaves", "false"));
    // m_useShortNames = qStrToBool(settings.value("lilyuseshortnames", "true"));
    // m_exportSelection = settings.value("lilyexportselection", EXPORT_NONMUTED_TRACKS).toUInt();
    // if (settings.value("lilyexporteditedsegments", "false").toBool()) {
    //     m_exportSelection = EXPORT_EDITED_SEGMENTS;
    // }
    // m_exportLyrics = settings.value("lilyexportlyrics", EXPORT_LYRICS_LEFT).toUInt();
    // m_exportTempoMarks = settings.value("lilyexporttempomarks", EXPORT_NONE_TEMPO_MARKS).toUInt();
    // m_exportBeams = qStrToBool(settings.value("lilyexportbeamings", "false"));
    // m_exportStaffGroup = qStrToBool(settings.value("lilyexportstaffbrackets", "true"));
    //
    // m_languageLevel = settings.value("lilylanguage", LILYPOND_VERSION_2_12).toUInt();
    // m_exportMarkerMode = settings.value("lilyexportmarkermode", EXPORT_NO_MARKERS).toUInt();
    // m_exportNoteLanguage = settings.value("lilyexportnotelanguage", LilyPondLanguage::NEDERLANDS).toUInt();
    // m_chordNamesMode = qStrToBool(settings.value("lilychordnamesmode", "false"));
    // m_useVolta = settings.value("lilyexportrepeat", "true").toBool();
    // m_altBar = settings.value("lilydrawbaratvolta", "true").toBool();
    // m_cancelAccidentals = settings.value("lilycancelaccidentals", "false").toBool();
    // m_fingeringsInStaff = settings.value("lilyfingeringsinstaff", "true").toBool();
    // settings.endGroup();

    // Define the destination directory and empty it
    destination = "VersionsTestOut";

    // Remove possible already existing directories
    QDir dir(destination);
    if (dir.exists()) {
        //  Destination directory found: delete it\n";
        if (!dir.removeRecursively()) {
            std::cerr << "ERROR: Can't remove \""
                      << destination << "\" directory"
                      << std::endl;
            exit(-1);
        }
    }

    // Create a directory for each LilyPond version
    for (int lv = LILYPOND_VERSION_TOO_OLD + 1;
                    lv < LILYPOND_VERSION_TOO_NEW; lv++) {
        QString path(LilyPond_Version_Names[lv]);
        if (!dir.mkpath(path)) {
            std::cerr << "ERROR: Can't create \"" << destination << "/"
                      << LilyPond_Version_Names[lv] << "\" directory"
                      << std::endl;
            exit(-1);
        }
    }
}


void TestLilypondVersion::testExamples_data()
{
    std::cout << "TESTEXAMPLES_DATA\n";
    // Usually "examples", but one "regression".
    QTest::addColumn<QString>("baseDir");
    QTest::addColumn<int>("lilyVersion");

    for (int lv = LILYPOND_VERSION_TOO_OLD + 1;
                lv < LILYPOND_VERSION_TOO_NEW; lv++) {

        QTest::newRow("aveverum") << "examples" << lv;
        QTest::newRow("aylindaamiga") << "examples" << lv;
        QTest::newRow("bogus-surf-jam") << "examples" << lv;
        QTest::newRow("beaming") << "examples" << lv;
        QTest::newRow("Brandenburg_No3-BWV_1048") << "examples" << lv;

        // ??? This one creates a directory in the user's home directory!?
        //     The directory's name is simply a space (' ').  Why?
        //     Otherwise this passes.  Please fix this one day.
        //QTest::newRow("bwv-1060-trumpet-duet-excerpt") << "examples" << lv;

        QTest::newRow("children") << "examples" << lv;
        QTest::newRow("Chopin-Prelude-in-E-minor-Aere") << "examples" << lv;
        QTest::newRow("Djer-Fire") << "examples" << lv;
        QTest::newRow("doodle-q") << "examples" << lv;
        QTest::newRow("exercise_notation") << "examples" << lv;
        QTest::newRow("glazunov-for-solo-and-piano-with-cue") << "examples" << lv;
        QTest::newRow("glazunov") << "examples" << lv;
        QTest::newRow("Hallelujah_Chorus_from_Messiah") << "examples" << lv;
        QTest::newRow("headers-and-unicode-lyrics") << "examples" << lv;
        QTest::newRow("himno_de_riego") << "examples" << lv;
        QTest::newRow("interpretation-example") << "examples" << lv;
        QTest::newRow("let-all-mortal-flesh") << "examples" << lv;
        QTest::newRow("lilypond-alternative-endings_new-way") << "examples" << lv;
        QTest::newRow("lilypond-alternative-endings") << "examples" << lv;
        QTest::newRow("lilypond-directives") << "examples" << lv;
        QTest::newRow("lilypond-up-down") << "examples" << lv;
        QTest::newRow("lilypond-staff-groupings") << "examples" << lv;
        QTest::newRow("lilypond-tied-grace-notes") << "examples" << lv;
        QTest::newRow("logical-segments-4") << "examples" << lv;
        QTest::newRow("mandolin-sonatina") << "examples" << lv;
        QTest::newRow("marks-test") << "examples" << lv;
        QTest::newRow("mozart-quartet") << "examples" << lv;
        QTest::newRow("notation-for-string-orchestra-in-D-minor") << "examples" << lv;
        QTest::newRow("perfect-moment") << "examples" << lv;
        QTest::newRow("ravel-pc-gmaj-adagio") << "examples" << lv;
        QTest::newRow("Romanza") << "examples" << lv;

        // THIS ONE FAILS
        // sicut-locutus.ly:107:47: Erreur : syntax error, unexpected '}'
        //                 < f g > 2 _\markup { \italic
        //                                               } _\markup { \italic Masked and substituted }  _~ f _~  |
        //QTest::newRow("sicut-locutus") << "examples" << lv;

        QTest::newRow("stormy-riders") << "examples" << lv;
        QTest::newRow("test_tuplets") << "examples" << lv;
        QTest::newRow("the-rose-garden") << "examples" << lv;
        QTest::newRow("vivaldi-cs3mv2") << "examples" << lv;
        QTest::newRow("vivaldi_op44_11_1") << "examples" << lv;

        // data/regression
        QTest::newRow("export_hidden_key_signatures") << "regression" << lv;
    }
}

// void TestLilypondVersion::init()
// {
//     std::cout << "INIT\n";
// }

void TestLilypondVersion::testExamples()
{
    std::cout << "TESTEXAMPLES\n";
    QString baseName{QTest::currentDataTag()};
    QFETCH(QString, baseDir);
    QFETCH(int, lilyVersion);

    std::cout << "baseName : " << baseName.toLocal8Bit().data() << "\n";
    std::cout << "baseDir : " << baseDir.toLocal8Bit().data() << "\n";
    std::cout << "lilyVersion : " << lilyVersion << "\n";

    // GIVEN
    const QString input =
            QFINDTESTDATA("../../data/" + baseDir + "/" + baseName + ".rg");
    if (input.isEmpty()) {
        std::cerr << "ERROR: File \"" << baseName << ".rg\" not found"
        << std::endl;
        exit(-1);
    }

    const QString fileName = QString(destination) + "/"
                                + LilyPond_Version_Names[lilyVersion]
                                + "/" + baseName + ".ly";
    std::cout << "Loading " << input << "\n"
              << "and exporting to " << fileName << "\n";

    // Load the .rg file.

    // Set LilyPond version
    QSettings settings;
    settings.beginGroup(LilyPondExportConfigGroup);
    settings.setValue("lilylanguage", lilyVersion);

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

    // Create the exporter
    LilyPondExporter exporter(
            &doc,  // doc
            SegmentSelection(),  // selection
            qstrtostr(fileName));  // fileName

    // Export the .ly file.
    exporter.write();

    // Don't clean up anything: we need the newly created file.
}


QTEST_MAIN(TestLilypondVersion)

#include "lilypond_versions_test.moc"

