/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more m_information.
*/

#define RG_MODULE_STRING "[LilyPondProcessor]"
#define RG_NO_DEBUG_PRINT

#include "LilyPondProcessor.h"

#include "IconLoader.h"

#include "gui/widgets/ProgressBar.h"
#include "misc/ConfigGroups.h"
#include "misc/Debug.h"

#include <QProcess>
#include <QGridLayout>
#include <QPushButton>
#include <QSettings>
#include <QLabel>
#include <QMessageBox>
#include <QDir>
#include <QTextCodec>


namespace Rosegarden
{


LilyPondProcessor::LilyPondProcessor(
        QWidget *parent, Mode mode, const QString &filename) :
    QDialog(parent),
    m_mode(mode)
{
    // Split the file path into directory and file name.
    QFileInfo fileInfo(filename);
    m_workingDirectory = fileInfo.dir().path();
    m_filename = fileInfo.fileName();

    RG_DEBUG << "ctor:  mode: " << mode << " m_filename: " << m_filename;

    setModal(false);
    setWindowIcon(IconLoader::loadPixmap("window-lilypond"));

    QString modeStr;
    if (mode == Mode::Print)
        modeStr = tr("Print");
    else
        modeStr = tr("Preview");
    setWindowTitle(tr("Rosegarden - %1 with LilyPond...").arg(modeStr));

    QGridLayout *layout = new QGridLayout(this);

    // Icon
    QLabel *icon = new QLabel(this);
    icon->setPixmap(IconLoader::loadPixmap("rosegarden-lilypond"));
    layout->addWidget(icon, 0, 0);

    // Message
    m_info = new QLabel(this);
    m_info->setWordWrap(true);
    layout->addWidget(m_info, 0, 1);

    // Progress Bar
    m_progress = new ProgressBar(this);
    layout->addWidget(m_progress, 1, 1);

    // Cancel
    QPushButton *cancel = new QPushButton(tr("Cancel"), this);
    connect(cancel, &QAbstractButton::clicked, this, &QDialog::reject);
    layout->addWidget(cancel, 3, 1); 
   
    // Just run convert.ly without all the logic to figure out if it's needed or
    // not.  This is harmless, and adds little extra processing time if the
    // conversion isn't required.
    runConvertLy();
}

void
LilyPondProcessor::fail(const QString &error, const QString &details)
{
    delete m_process;
    m_process = nullptr;

    m_progress->setMaximum(100);
    m_progress->hide();

    m_info->setText(tr("Fatal error.  Processing aborted."));
    QMessageBox messageBox(this);
    messageBox.setIcon(QMessageBox::Critical);
    messageBox.setWindowTitle(tr("Rosegarden - Fatal processing error!"));
    messageBox.setText(error);
    messageBox.setDetailedText(details);
    messageBox.exec();

    // abort processing after a fatal error, so calls to fail() abort the whole
    // process in its tracks
    reject();

    // Well, that was the theory.  In practice it apparently isn't so easy to do
    // the bash equivalent of a spontaneous "exit 1" inside a QDialog.  Hrm.
}

void
LilyPondProcessor::runConvertLy()
{
    RG_DEBUG << "runConvertLy()";

    m_info->setText(tr("Running <b>convert-ly</b>..."));
    m_process = new QProcess;
    m_process->setWorkingDirectory(m_workingDirectory);
    m_process->start("convert-ly", QStringList() << "-e" << m_filename);
    connect(m_process, (void(QProcess::*)(int, QProcess::ExitStatus))
                    &QProcess::finished,
            this, &LilyPondProcessor::runLilyPond);

    // wait up to 30 seconds for process to start
    if (m_process->waitForStarted()) {
        m_info->setText(tr("<b>convert-ly</b> started..."));
    } else {
        fail(tr("<qt><p>Could not run <b>convert-ly</b>!</p><p>Please install LilyPond and ensure that the \"convert-ly\" and \"lilypond\" commands are available on your path.  If you perform a <b>Run Command</b> (typically <b>Alt+F2</b>) and type \"convert-ly\" into the box, you should not get a \"command not found\" error.  If you can do that without getting an error, but still see this error message, please consult <a style=\"color:gold\" href=\"mailto:rosegarden-user@lists.sourceforge.net\">rosegarden-user@lists.sourceforge.net</a> for additional help.</p><p>Processing terminated due to fatal errors.</p></qt>"));
    }

    m_progress->setValue(25);

    // runLilyPond() will take over when convert-ly is done...
}

void
LilyPondProcessor::runLilyPond(int exitCode, QProcess::ExitStatus)
{
    RG_DEBUG << "runLilyPond()";

    if (exitCode == 0) {
        m_info->setText(tr("<b>convert-ly</b> finished..."));
        delete m_process;
        m_process = nullptr;
    } else {
        fail(tr("<qt><p>Ran <b>convert-ly</b> successfully, but it terminated with errors.</p><p>Processing terminated due to fatal errors.</p></qt>"));
    }

    m_progress->setValue(50);

    m_process = new QProcess;
    m_process->setWorkingDirectory(m_workingDirectory);
    m_info->setText(tr("Running <b>lilypond</b>..."));
    m_process->start("lilypond", QStringList() << "--pdf" << m_filename);
    connect(m_process, (void(QProcess::*)(int, QProcess::ExitStatus))
                    &QProcess::finished,
            this, &LilyPondProcessor::runFinalStage);
            
    if (m_process->waitForStarted()) {
        m_info->setText(tr("<b>lilypond</b> started..."));
    } else {
        fail(tr("<qt><p>Could not run <b>lilypond</b>!</p><p>Please install LilyPond and ensure that the \"convert-ly\" and \"lilypond\" commands are available on your path.  If you perform a <b>Run Command</b> (typically <b>Alt+F2</b>) and type \"lilypond\" into the box, you should not get a \"command not found\" error.  If you can do that without getting an error, but still see this error message, please consult <a style=\"color:gold\" href=\"mailto:rosegarden-user@lists.sourceforge.net\">rosegarden-user@lists.sourceforge.net</a> for additional help.</p><p>Processing terminated due to fatal errors.</p></qt>"));
    }

    // go into Knight Rider mode when chewing on LilyPond, because it can take
    // an eternity, but I don't really want to re-create all the text stream
    // monitoring and guessing code that's easy to do in a script and hell to do
    // in real code
    m_progress->setMaximum(0);

    // runFinalStage() will take over when lilypond is done...
}

void
LilyPondProcessor::runFinalStage(int exitCode, QProcess::ExitStatus)
{
    RG_DEBUG << "runFinalStage()";

    // If LilyPond failed...
    if (exitCode != 0) {
        // read preferences from last export from QSettings to offer clues what
        // failed
        QSettings settings;
        settings.beginGroup(LilyPondExportConfigGroup);
        bool exportedBeams = settings.value("lilyexportbeamings", false).toBool();
        bool exportedBrackets = settings.value("lilyexportstaffbrackets", false).toBool();
        settings.endGroup();

        RG_DEBUG << "  finalStage: exportedBeams == " << (exportedBeams ? "true" : "false");
        RG_DEBUG << " exportedBrackets == " << (exportedBrackets ? "true" : "false");

        QString message = "<html>";
        message += tr("<p>Ran <b>lilypond</b> successfully, but it terminated with errors.</p>");

        if (exportedBeams) {
            message += tr("<p>You opted to export Rosegarden's beaming, and LilyPond could not process the file.  It is likely that you performed certain actions in the course of editing your file that resulted in hidden beaming properties being attached to events where they did not belong, and this probably caused LilyPond to fail.  The recommended solution is to either leave beaming to LilyPond (whose automatic beaming is far better than Rosegarden's) and un-check this option, or to un-beam everything and then re-beam it all manually inside Rosegarden.  Leaving the beaming up to LilyPond is probaby the best solution.</p>");
        }

        if (exportedBrackets) {
            message += tr("<p>You opted to export staff group brackets, and LilyPond could not process the file.  Unfortunately, this useful feature can be very fragile.  Please go back and ensure that all the brackets you've selected make logical sense, paying particular attention to nesting.  Also, please check that if you are working with a subset of the total number of tracks, the brackets on that subset make sense together when taken out of the context of the whole.  If you have any doubts, please try turning off the export of staff group brackets to see whether LilyPond can then successfully render the result.</p>");
        }

        message += tr("<p>Processing terminated due to fatal errors.</p>");

        message += "</html>";

        QTextCodec *codec = QTextCodec::codecForLocale();
        fail(message, codec->toUnicode(m_process->readAllStandardError()));

        // fail() doesn't actually work, so we have to return in order to avoid
        // further processing
        return;
    }

    // All's well.  LilyPond exited normally.

    m_info->setText(tr("<b>lilypond</b> finished..."));

    delete m_process;
    m_process = nullptr;

    if (m_mode == Mode::Print) {
        print();
    } else {
        // just default to preview (I always use preview anyway, as I never
        // trust the results for a direct print without previewing them first,
        // and in fact the direct print option seems somewhat dubious to me)
        preview();
    }
}

void
LilyPondProcessor::print()
{
    const QString pdfName = m_filename.replace(".ly", ".pdf");
    m_info->setText(tr("Printing %1...").arg(pdfName));

    // retrieve user preferences from QSettings
    QSettings settings;
    settings.beginGroup(ExternalApplicationsConfigGroup);
    unsigned filePrinterIndex = settings.value("fileprinter", 0).toUInt();

    QString program;

    // ??? std::vector
    switch (filePrinterIndex) {
        case 0:
            program = "gtklp";
            break;
        case 1:
            program = "lp";
            break;
        case 2:
            program = "lpr";
            break;
        case 3:
            program = "hp-print";
            break;
        default:
            program = "lpr";
            break;
    }

    m_process = new QProcess;
    connect(m_process, (void(QProcess::*)(int, QProcess::ExitStatus))
                    &QProcess::finished,
            this, &LilyPondProcessor::finishedPrinting);
    m_process->setWorkingDirectory(m_workingDirectory);
    m_process->start(program, QStringList() << pdfName);
    // If the process does not start...
    if (!m_process->waitForStarted()) {
        QString t = QString(tr("<qt><p>LilyPond processed the file successfully, but <b>%1</b> did not run!</p><p>Please configure a valid %2 under <b>Edit -> Preferences -> General -> External Applications</b> and try again.</p><p>Processing terminated due to fatal errors.</p></qt>")).
                arg(program).
                arg(tr("file printer"));
        fail(t);
    }

    m_progress->setMaximum(100);
    m_progress->setValue(100);

    // finishedPrinting() will take over when the printing is done...
}

void
LilyPondProcessor::preview()
{
    const QString pdfName = m_filename.replace(".ly", ".pdf");
    m_info->setText(tr("Previewing %1...").arg(pdfName));

    // retrieve user preferences from QSettings
    QSettings settings;
    settings.beginGroup(ExternalApplicationsConfigGroup);
    unsigned pdfViewerIndex = settings.value("pdfviewer", 0).toUInt();

    QString program;

    // ??? std::vector
    switch (pdfViewerIndex) {
        case 0:
            program = "okular";
            break;
        case 1:
            program = "evince";
            break;
        case 2:
            program = "acroread";
            break;
        case 3:
            program = "mupdf";
            break;
        case 4:
            program = "epdfview";
            break;
        case 5:
            program = "xdg-open";
            break;
        default:
            program = "xdg-open";
    }

    // This one uses QProcess::startDetached().
    QProcess::startDetached(program, QStringList() << pdfName, m_workingDirectory);

    // Dismiss the dialog.
    accept();
}

void
LilyPondProcessor::finishedPrinting(int /*exitCode*/, QProcess::ExitStatus)
{
    delete m_process;
    m_process = nullptr;

    // Dismiss the dialog.
    accept();
}


}
