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
    COPYING included with this distribution for more m_information.
*/

#define RG_MODULE_STRING "[ProjectPackager]"

#include "ProjectPackager.h"

#include "document/RosegardenDocument.h"
#include "base/Composition.h"
#include "base/Track.h"
#include "gui/general/IconLoader.h"
#include "gui/widgets/FileDialog.h"
#include "gui/widgets/ProgressBar.h"
#include "misc/ConfigGroups.h"
#include "misc/Strings.h"
#include "misc/Debug.h"
#include "sound/AudioFile.h"
#include "sound/AudioFileManager.h"
#include "document/GzipFile.h"

#include <QDialog>
#include <QProcess>
#include <QGridLayout>
#include <QPushButton>
#include <QSettings>
#include <QLabel>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDirIterator>
#include <QSet>

namespace Rosegarden
{


ProjectPackager::ProjectPackager(QWidget *parent, RosegardenDocument *document,  int mode, QString filename) :
        QDialog(parent),
        m_doc(document),
        m_mode(mode),
        m_filename(filename),
        m_trueFilename(filename),
        m_packTmpDirName("fatal error"),
        m_packDataDirName("fatal error"),
        m_abortText(tr("<p>Processing aborted</p>"))

{
RG_DEBUG << "ProjectPackager::ProjectPackager():  mode: " << mode << 
    " m_filename: " << m_filename;

    this->setModal(false);

    setWindowIcon(IconLoader().loadPixmap("window-packager"));

    QGridLayout *layout = new QGridLayout;
    this->setLayout(layout);

    QLabel *icon = new QLabel(this);
    icon->setPixmap(IconLoader().loadPixmap("rosegarden-packager"));
    layout->addWidget(icon, 0, 0);

    QString modeStr;
    switch (mode) {
        case ProjectPackager::Unpack:  modeStr = tr("Unpack"); break;
        case ProjectPackager::Pack:    modeStr = tr("Pack");   break;
    }
    this->setWindowTitle(tr("Rosegarden - %1 Project Package...").arg(modeStr));

    m_info = new QLabel(this);
    m_info->setWordWrap(true);
    layout->addWidget(m_info, 0, 1);

    m_progress = new ProgressBar(100, this);
    layout->addWidget(m_progress, 1, 1);

    QPushButton *ok = new QPushButton(tr("Cancel"), this);
    connect(ok, SIGNAL(clicked()), this, SLOT(reject()));
    layout->addWidget(ok, 3, 1);

    sanityCheck();
}

QString
ProjectPackager::getTrueFilename()
{
    // get the path from the original m_filename, which is wherever the unpacked
    // .rgp file sat on disk, eg. /home/melvin/Documents/
    QFileInfo origFI(m_filename);
    QString dirname = origFI.path();

RG_DEBUG << "ProjectPackager::getTrueFilename() - directory component is: " <<
    dirname;

    // get the filename component from the true m_trueFilename discovered while
    // unpacking the .rgp + extension (eg. foo.rgp yields bar.rg here)
    QFileInfo trueFI(m_trueFilename);
    QString basename = QString("%1.%2").arg(trueFI.baseName()).arg(trueFI.completeSuffix());

RG_DEBUG << "                                          name component is: " <<
    basename;

    return QString("%1/%2").arg(dirname).arg(basename);
}

void
ProjectPackager::puke(QString error)
{
    m_progress->setMaximum(100);
    m_progress->hide();

    m_info->setText(tr("<qt><p>Fatal error.</p>%1</qt>").arg(m_abortText));
    QMessageBox::critical(this, tr("Rosegarden - Fatal Processing Error"), error, QMessageBox::Ok, QMessageBox::Ok);

    // abort processing after a fatal error, so calls to puke() abort the whole
    // process in its tracks
    reject();

    // Well, that was the theory.  In practice it apparently isn't so easy to do
    // the bash equivalent of a spontaneous "exit 1" inside a QDialog.  Hrm.
}

bool
ProjectPackager::rmdirRecursive(QString dirName)
{
    QDir dir(dirName);

    // If the directory is already gone, bail.
    if (!dir.exists())
        return true;

    bool success = true;

    // *** Delete all the files

    QDirIterator fileIter(dir.path(), QDir::Files | QDir::Hidden,
                          QDirIterator::Subdirectories);
    while (fileIter.hasNext()) {
        // Create a temp to avoid calling next() twice if we want debug
        // output.
        QString currentFile = fileIter.next();
        //qDebug() << "rm" << currentFile;
        // Remove the file
        if (!QFile::remove(currentFile)) {
            success = false;
        }
    }

    // *** Delete the empty directories

    // QDirIterator iterates through the directory tree in reverse order
    // from that required to properly remove the directories recursively;
    // it iterates from root to leaf.  So we need to do this in two steps.
    // First, gather the directories into a vector, then go through the
    // vector in reverse (from leaf to root) and remove them.

    // It might be better to implement our own recursion so that we can
    // guarantee that it will always be from leaf to root.  Otherwise a
    // change to the algorithm in QDirIterator could render this code
    // useless.

    // Iterate through the directories recursively and collect the names
    // into a vector.
    QDirIterator dirIter(dir.path(), QDir::Dirs | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);

    typedef std::vector<QString> QStringVector;
    QStringVector v;

    v.push_back(dirName);

    while (dirIter.hasNext()) {
        // Create a temp to avoid calling next() twice if we want debug
        // output.
        QString currentDir = dirIter.next();
        //qDebug() << "push_back" << currentDir;
        v.push_back(currentDir);
    }

    // Have to move back one as rmdir() is relative to QDir's directory.
    dir.cdUp();

    // Now go through the directories in reverse and remove them...
    for (QStringVector::const_reverse_iterator I = v.rbegin();
         I != v.rend();
         ++I) {
        //qDebug() << "rmdir" << *I;
        if (!dir.rmdir(*I)) {
            success = false;
        }
    }

    return success;
}

void
ProjectPackager::reject()
{
RG_DEBUG << "User pressed cancel";

    rmdirRecursive(m_packTmpDirName);
    QDialog::reject();
}

QStringList
ProjectPackager::getAudioFiles()
{
    QStringList list;

    // get the Composition from the document, so we can iterate through it
    Composition *comp = &m_doc->getComposition();

    // We don't particularly care about tracks here, so just iterate through the
    // entire Composition to find the audio segments and get the associated
    // file IDs from which to obtain a list of actual files.  This could
    // conceivably pick up audio segments that are residing on MIDI tracks and
    // wouldn't otherwise be functional, but the important thing is to never
    // miss a single file that has any chance of being worth preserving.
    for (Composition::iterator i = comp->begin(); i != comp->end(); ++i) {
        if ((*i)->getType() == Segment::Audio) {

            AudioFileManager *manager = &m_doc->getAudioFileManager();

            unsigned int id = (*i)->getAudioFileId();

            AudioFile *file = manager->getAudioFile(id);

            // some polite sanity checking to avoid possible crashes
            if (!file) continue;

            list << file->getFilename();
        }
    }

    // QStringList::removeDuplicates() would have been easy, but it's only in Qt
    // 4.5.0 and up.  So here's the algorithm from Qt 4.5.0, courtesy of (and
    // originally Copyright 2009) Nokia

    QStringList *that = &list;

    int n = that->size();
    int j = 0;
    QSet<QString> seen;
    seen.reserve(n);
    for (int i = 0; i < n; ++i) {
        const QString &s = that->at(i);
        if (seen.contains(s))
            continue;
        seen.insert(s);
        if (j != i)
            (*that)[j] = s;
        ++j;
    }
    if (n != j)
        that->erase(that->begin() + j, that->end());
//    return n - j;

    return list;
}

QStringList
ProjectPackager::getPluginFilesAndRewriteXML(const QString fileToModify, const QString newPath)
{
    // yet another miserable wrinkle in this whole wretched thing: we
    // automatically ignore audio files not actually used by segments, but
    // Rosegarden wants to hunt for the missing (but useless) files when we load
    // the result, so we have to strip them out of the XML too
    //
    // ARGH!!!
    QStringList usedAudioFiles;
    if (m_mode == ProjectPackager::Pack) usedAudioFiles = getAudioFiles();

    QStringList list;

    // read the input file
    QString inText;

    bool readOK = GzipFile::readFromFile(fileToModify, inText);
    if (!readOK) {
        puke(tr("<qt><p>Unable to read %1.</p>%2</qt>").arg(fileToModify).arg(m_abortText));
        return QStringList();
    }

    // the pre-process input stream
    QString preText = inText;
    QTextStream preIn(&preText, QIODevice::ReadOnly);

    // the pre-process output steram
    QString postText;
    QTextStream preOut(&postText, QIODevice::WriteOnly);

    // insert \n between tags
    do {
        QString l = preIn.readLine();
        l.replace(QRegExp("><"), ">\n<");
        preOut << l << endl;
    } while (!preIn.atEnd());

    // the input stream
    QTextStream inStream(&postText, QIODevice::ReadOnly);

    // the output stream
    QString outText;
    QTextStream outStream(&outText, QIODevice::WriteOnly);
    outStream.setCodec("UTF-8");

    // synth plugin XML:
    //
    //  <synth identifier="dssi:/usr/lib/dssi/fluidsynth-dssi.so:FluidSynth-DSSI" bypassed="false" >
    //       <configure key="__ROSEGARDEN__:__RESERVED__:ProjectDirectoryKey" value="/home/michael/rosegarden/"/>
    //       <configure key="load" value="/home/michael/data/soundfonts/PC51f.sf2"/>
    //  </synth>    
    QString pluginAudioPathKey("<configure key=\"__ROSEGARDEN__:__RESERVED__:ProjectDirectoryKey\" value=\"");
    QString pluginAudioDataKey("<configure key=\"load\" value=\"");

    // audio path XML:
    //
    //  <audiofiles>
    //       <audioPath value="~/rosegarden/"/>
    //  </audiofiles>
    QString audioPathKey("<audioPath value=\"");

    // audio file XML:
    //  <audio id="4" file="rg-20071210-214212-5.wav" label="rg-20071210-214212-5.wav"/>
    //  -or-
    //  <audio file="asdfasdf" id="0" label="asdfasdf"/>
    //
    //  only common thread is "audio "
    QString audioFileKey("<audio ");

    QString valueTagEndKey("\"/>");

    // process the input line by line and stream it all back out, making any
    // necessary modifications along the way
    QString line;

    int c = 0;

    do {

        line = inStream.readLine();
        // don't flood the console
        if (c < 10) {
            RG_DEBUG << "LINE: " << ++c << " BUFFER SIZE: " << line.size();
        }

        if (line.contains(pluginAudioPathKey)) {

RG_DEBUG << "rewriting plugin audio path tag...";

            // we don't care what the old path was at all, this was the bug;
            // just write the new path straight up
            QString extract = newPath;
            extract.prepend(pluginAudioPathKey);
            extract.append(valueTagEndKey);

RG_DEBUG << "old line: " << line;

            line = extract;

RG_DEBUG << "new line: " << line;

        } else if (line.contains(pluginAudioDataKey)) {

            // note that "plugin audio data" is a bit of a misnomer, as this
            // could contain a soundfont or who knows what else; they're handled
            // the same way regardless, as "extra files" to add to the package

RG_DEBUG << "rewriting the path for a plugin data item...";

            int s = line.indexOf(pluginAudioDataKey) + pluginAudioDataKey.length();
            int e = line.indexOf(valueTagEndKey);

            QString extract = line.mid(s, e - s);
            
RG_DEBUG << "extracted value string:  value=\"" << extract << "\"";

            // save the extracted path to the list of extra files (and this is
            // the one part of these three block copied implementations that
            // differs significantly--really should refactor this into some
            // function, but I decided just not to bother)
            list << extract;

            // alter the path component (note that we added extract to files
            // BEFORE changing its path)
            QFileInfo fi(extract);
            extract = QString("%1/%2.%3").arg(newPath).arg(fi.baseName()).arg(fi.completeSuffix());

            // construct a new line around the altered substring
            extract.prepend(pluginAudioDataKey);
            extract.append(valueTagEndKey);

RG_DEBUG << "old line: " << line;

            line = extract;

RG_DEBUG << "new line: " << line;

        } else if (line.contains(audioPathKey)) {

RG_DEBUG << "rewriting document audio path...";

            // we don't care what the old path was at all, this was the bug;
            // just write the new path straight up
            //
            QString extract = newPath;
            extract.prepend(audioPathKey);
            extract.append(valueTagEndKey);

RG_DEBUG << "old line: " << line;

            line = extract;

RG_DEBUG << "new line: " << line;

        } else if (line.contains(audioFileKey) && 
                   m_mode == ProjectPackager::Pack) {

            // sigh... more and more brittle, on the pack, but only the PACK
            // step, we have to strip the unused audio files out of the XML,
            // since we're not including them
            //
            // RG doesn't write the tags in the same order (I have files with
            // the tags in different orders) so all we can do is iterate through
            // the list of used files and see if this line contains that string
            // somewhere, and if so, keep it, else ditch it

            QStringList::const_iterator si;
            bool keep = false;
            QFileInfo fileInfo;
                    
            for (si = usedAudioFiles.constBegin(); 
                 si != usedAudioFiles.constEnd(); 
                 ++si) {

                fileInfo.setFile(*si);

RG_DEBUG << "\"" << line << "\" contains? \"" << (fileInfo.baseName()) << "\"";
RG_DEBUG << "Qt says " << (line.contains(fileInfo.baseName()) ? "yes" : "no");
RG_DEBUG;

                if (line.contains(fileInfo.baseName())) {
                    keep = true;
                    break;
                }
            }

            if (!keep) {

RG_DEBUG << "Removed the following line referring to unused audio file: ";
RG_DEBUG << "  " << line;

                continue;
            }
        }

        outStream << line << endl;

    } while (!inStream.atEnd());


    // write the modified data to the output file
    QString ofileName = QString("%1.tmp").arg(fileToModify);
    bool writeOK = GzipFile::writeToFile(ofileName, outText);
    if (!writeOK) {
        puke(tr("<qt><p>Could not write<br>%1.</p>%2</qt>").arg(ofileName).arg(m_abortText));
        return QStringList();
    }

    // swap the .tmp modified copy back to the original filename
    if (!QFile::remove(fileToModify)) {
        puke(tr("<qt>Could not remove<br>%1<br><br>%2</qt>").arg(fileToModify).arg(m_abortText));
        return QStringList();
    }

    if (!QFile::copy(ofileName, fileToModify)) {
        puke(tr("<qt>Could not copy<br>%1<br>  to<br>%2<br><br>%3</qt>").arg(ofileName).arg(fileToModify).arg(m_abortText));
        return QStringList();
    }
    if (!QFile::remove(ofileName)) {
        puke(tr("<qt><p>Could not remove<br>%1.</p>%2</qt>").arg(ofileName).arg(m_abortText));
        return QStringList();
    }

    return list;
}


// check for flac and wavpack on every run, rather than doing this as part of
// Rosegarden's startup tester.
//
// we also use tar, gzip, and bash, but these very commonly exist on Linux, and
// we'll deal with those whenever we're looking at a broader audience than just
// Linux
void
ProjectPackager::sanityCheck() {
    // check for flac
    m_process = new QProcess;
    m_process->start("flac", QStringList() << "--help");

    // highest compression; can't cope with our default IEEE 32-bit floating
    // point (type 3) wav files
    m_info->setText(tr("Checking for flac..."));
    if (!m_process->waitForStarted()) {
        puke(tr("<qt><p>The <b>flac</b> command was not found.</p><p>FLAC is a lossless audio compression format used to reduce the size of Rosegarden project packages with no loss of audio quality.  Please install FLAC and try again.  This utility is typically available to most distros as a package called \"flac\".</p></qt>"));
        return;
    }
    // should only have to wait less than a second, so go ahead and block
    m_process->waitForFinished();
    delete m_process;

    // second highest compression, comparable availability, does not require
    // pre-conversion to a FLAC-compatible format which would make the trip
    // through the project packager a one-way transformation for the original
    // files (even if the difference is trivial, I'd rather not do anything
    // permanent without an "are you sure" process, and we don't want to ask
    // questions, we want to just do something sensible by default)
    m_process = new QProcess;
    m_process->start("wavpack", QStringList() << "--help");

    m_info->setText(tr("Checking for wavpack..."));
    if (!m_process->waitForStarted()) {
        puke(tr("<qt><p>The <b>wavpack</b> command was not found.</p><p>WavPack is an audio compression format used to reduce the size of Rosegarden project packages with no loss of audio quality.  Please install WavPack and try again.  This utility is typically available to most distros as part of a package called \"wavpack\".</p>"));
        return;
    }
    m_process->waitForFinished();
    delete m_process;

    m_process = new QProcess;
    m_process->start("wvunpack", QStringList() << "--help");

    m_info->setText(tr("Checking for wvunpack..."));
    if (!m_process->waitForStarted()) {
        puke(tr("<qt><p>The <b>wvunpack</b> command was not found.</p><p>WavPack is an audio compression format used to reduce the size of Rosegarden project packages with no loss of audio quality.  Please install WavPack and try again.  This utility is typically available to most distros as part of a package called \"wavpack\".</p>"));
        return;
    }
    connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(runPackUnpack(int, QProcess::ExitStatus)));
}

void
ProjectPackager::runPackUnpack(int exitCode, QProcess::ExitStatus) {

    // We won't get here unless waitForStarted() worked, and if the last command
    // we tested existed, that's good enough.  Ignore the error code.  (It
    // happens that wvunpack --help returns error code 1.  Odd, but not
    // important so long as the damn thing started.)

RG_DEBUG << "ProjectPackager::runPackUnpack() - " <<
    "sanity check passed, last process exited " << exitCode;

    delete m_process;

    switch (m_mode) {
        case ProjectPackager::Unpack:  runUnpack(); break;
        case ProjectPackager::Pack:    runPack();   break;
    }
}

///////////////////////////
//                       //
//  PACKING OPERATIONS   //
//                       //
///////////////////////////
void
ProjectPackager::runPack()
{

RG_DEBUG << "ProjectPackager::runPack()";

    m_info->setText(tr("Packing project..."));

    // go into spinner mode
    m_progress->setMaximum(0);

    QStringList audioFiles = getAudioFiles();

    // the base tmp directory where we'll assemble all the files
    m_packTmpDirName = QString("%1/rosegarden-project-packager-tmp").arg(QDir::homePath());

    // the data directory where audio and other files will go
    QFileInfo fi(m_filename);
    m_packDataDirName = fi.baseName();

RG_DEBUG << "using tmp data directory: " << m_packTmpDirName << "/" << 
    m_packDataDirName;

    QDir tmpDir(m_packTmpDirName);

    // get the original filename saved by RosegardenMainWindow and the name of
    // the new one we'll be including in the bundle (name isn't changing, path
    // component changes from one to the other)
    // QFileInfo::baseName() given /tmp/foo/bar/rat.rgp returns rat
    //
    // m_filename comes in already having an .rgp extension, but the file
    // was saved .rg
    QString oldName = QString("%1/%2.rg").arg(fi.path()).arg(fi.baseName());
    QString newName = QString("%1/%2.rg").arg(m_packTmpDirName).arg(fi.baseName());

    // if the tmp directory already exists, just hose it
    rmdirRecursive(m_packTmpDirName);

    // make the temporary working directory
    if (tmpDir.mkdir(m_packTmpDirName)) {

    } else {
        puke(tr("<qt><p>Could not create temporary working directory.</p>%1</qt>").arg(m_abortText));
        return;
    }

    m_info->setText(tr("Copying audio files..."));

    // leave spinner mode
    m_progress->setMaximum(100);
    m_progress->setValue(0);

    // count total audio files
    int af = 0;
    QStringList::const_iterator si;
    for (si = audioFiles.constBegin(); si != audioFiles.constEnd(); ++si)
        af++;
    int afStep = ((af == 0) ? 1 : (100 / af));

    // make the data subdir
    tmpDir.mkdir(m_packDataDirName);    

    // copy the audio files (do not remove the originals!)
    af = 0;
    for (si = audioFiles.constBegin(); si != audioFiles.constEnd(); ++si) {

        // comes in with full path and filename
        QString srcFile = (*si);
        QString srcFilePk = QString("%1.pk").arg(srcFile);

        // needs the filename split away from the path, so we can replace the
        // path with the new one
        QFileInfo fi(*si);
        QString filename = QString("%1.%2").arg(fi.baseName()).arg(fi.completeSuffix());

        QString dstFile = QString("%1/%2/%3").arg(m_packTmpDirName).arg(m_packDataDirName).arg(filename);
        QString dstFilePk = QString("%1.pk").arg(dstFile);

RG_DEBUG << "cp " << srcFile << " " << dstFile;
RG_DEBUG << "cp " << srcFilePk << " " << dstFilePk;

        if (!QFile::copy(srcFile, dstFile)) {
            puke(tr("<qt>Could not copy<br>%1<br>  to<br>%2<br><br>Processing aborted.</qt>").arg(srcFile).arg(dstFile));
            return;
        }

        // Try to copy the .wav.pk file derived from transforming the name of
        // the .wav file.  We don't trap the fail condition for this one and
        // allow it to fail silently without complaining or aborting.  If the
        // .wav.pk files are missing, they will be generated again as needed.
        //
        // Legacy .rgp files ship with improperly named .wav.pk files, from
        // a bug in the original rosegarden-project-package script.  You'd wind
        // up with an .rgp that contained, for example:
        //
        //   emergence-rg-0014.wav.pk
        //   RG-AUDIO-0014.wav.pk
        //
        // That is why this version of the project packager doesn't screw around
        // with the original filenames!
        QFile::copy(srcFilePk, dstFilePk);

        m_progress->setValue(afStep * ++af);
    }

    // deal with adding any extra files
    QStringList extraFiles;

    // first, if the composition includes synth plugins, there may be assorted
    // random audio files, soundfonts, and who knows what else in use by these
    // plugins
    //
    // obtain a list of these files, and rewrite the XML to update the referring
    // path from its original source to point to our bundled copy instead
    QString newPath = QString("%1/%2").arg(m_packTmpDirName).arg(m_packDataDirName);
    extraFiles = getPluginFilesAndRewriteXML(oldName, newPath);

    // If we do the above here and add it to extraFiles then if the user has any
    // other extra files to add by hand, it all processes out the same way with
    // no extra bundling code required (unless we want to flac any random extra
    // .wav files, and I say no, let's not get that complicated)

    // Copy the modified .rg file to the working tmp dir

RG_DEBUG << "cp " << oldName << " " << newName;

    // copy m_filename(.rgp) as $tmp/m_filename.rg
    if (!QFile::copy(oldName, newName)) {
        puke(tr("<qt>Could not copy<br>%1<br>  to<br>%2<br><br>Processing aborted.</qt>").arg(oldName).arg(newName));
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::information(this,
            tr("Rosegarden"),
            tr("<qt><p>Rosegarden can add any number of extra files you may desire to a project package.  For example, you may wish to include an explanatory text file, a soundfont, a bank definition for ZynAddSubFX, or perhaps some cover art.</p><p>Would you like to include any additional files?</p></qt>"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    while (reply == QMessageBox::Yes) {

        // it would take some trouble to make the last used paths thing work
        // here, where we're building a list of files from potentially anywhere,
        // so we'll just use the open_file path as it was last set elsewhere,
        // and leave it at that until somebody complains
        QSettings settings;
        settings.beginGroup(LastUsedPathsConfigGroup);
        QString directory = settings.value("open_file", QDir::homePath()).toString();
        settings.endGroup();

        // must iterate over a copy of the QStringList returned by
        // (Q)FileDialog::getOpenFileNames for some reason
        //
        // NOTE: This still doesn't work.  I can only add one filename.
        // Something broken in the subclass of QFileDialog?  Bad code?  I'm just
        // leaving it unresolved for now. One file at a time at least satisfies
        // the bare minimum requirements
        QStringList files =  FileDialog::getOpenFileNames(this, "Open File", directory, tr("All files") + " (*)", nullptr, nullptr);
        extraFiles << files;
       
        //!!!  It would be nice to show the list of files already chosen and
        // added, in some nice little accumulator list widget, but this would
        // require doing something more complicated than using QMessageBox
        // static convenience functions, and it's probably just not worth it
        reply =  QMessageBox::information(this,
                tr("Rosegarden"),
                tr("<qt><p>Would you like to include any additional files?</p></qt>"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    }

    m_info->setText(tr("Copying plugin data and extra files..."));

    // reset progress bar
    m_progress->setValue(0);

    // count total audio files
    int ef = 0;
    for (si = extraFiles.constBegin(); si != extraFiles.constEnd(); ++si)
        ef++;
    int efStep = ((ef == 0) ? 1 : (100 / ef));

    // copy the extra files (do not remove the originals!)
    // (iterator previously declared)
    ef = 0;
    for (si = extraFiles.constBegin(); si != extraFiles.constEnd(); ++si) {
    
        // each QStringList item from the FileDialog will include the full path
        QString srcFile = (*si);

        // so we cut it up to swap the source dir for the dest dir while leaving
        // the complete filename stuck on the end
        QFileInfo efi(*si);
        QString basename = QString("%1.%2").arg(efi.baseName()).arg(efi.completeSuffix());
        QString dstFile = QString("%1/%2/%3").arg(m_packTmpDirName).arg(m_packDataDirName).arg(basename);

RG_DEBUG << "cp " << srcFile << " " << dstFile;

        if (!QFile::copy(srcFile, dstFile)) {
            puke(tr("<qt>Could not copy<br>%1<br>  to<br>%2<br><br>Processing aborted.</qt>").arg(srcFile).arg(dstFile));
            return;
        }

        m_progress->setValue(efStep * ++ef);
    }

    // and now we have everything discovered, uncovered, added, smothered,
    // scattered and splattered, and we're ready to pack the files and
    // get the hell out of here!
    startAudioEncoder(audioFiles);
}

void
ProjectPackager::startAudioEncoder(QStringList files)
{
    m_info->setText(tr("Packing project..."));

    // (we could do some kind of QProcess monitoring, but I'm feeling lazy at
    // the moment and this will at least make us look busy while we chew)
    // go into spinner mode
    m_progress->setMaximum(0);

    // we can't do a oneliner bash script straight out of a QProcess command
    // line, so we'll have to create a purpose built script and run that
    QString scriptName("/tmp/rosegarden-audio-encoder-backend");
    m_script.setFileName(scriptName);

    // remove any lingering copy from a previous run
    if (m_script.exists()) m_script.remove();

    if (!m_script.open(QIODevice::WriteOnly | QIODevice::Text)) {
        puke(tr("<qt><p>Unable to write to temporary backend processing script %1.</p>%2</qt>").arg(m_abortText));
        return;
    }
    
    // build the script
    QTextStream out(&m_script);
    out << "# This script was generated by Rosegarden to combine multiple external processing"      << endl
        << "# operations so they could be managed by a single QProcess.  If you find this script"   << endl
        << "# it is likely that something has gone terribly wrong. See http://rosegardenmusic.com" << endl;

    QStringList::const_iterator si;
    int errorPoint = 1;
    for (si = files.constBegin(); si != files.constEnd(); ++si) {
        QFileInfo fi(*si);
        QString filename = QString("%1.%2").arg(fi.baseName()).arg(fi.completeSuffix());
        QString o = QString("%1/%2").arg(m_packDataDirName).arg(filename);

        // we'll eschew anything fancy or pretty in this disposable script and
        // just write a command on each line, terminating with an || exit n
        // which can be used to figure out at which point processing broke, for
        // cheap and easy error reporting without a lot of fancy stream wiring
        out << "wavpack -d \"" << o << "\" || exit " << errorPoint << endl;
        errorPoint++;
    }

    // Throw tar on the ass end of this script and save an extra processing step
    //
    // first cheap trick, m_packDataDirName.rg is our boy and we know it
    QString rgFile = QString("%1.rg").arg(m_packDataDirName);

    // second cheap trick, don't make a tarball in tmpdir and move it, just
    // write it at m_filename and shazam, nuke the tmpdir behind us and peace out
    out << "tar czf \"" << m_filename << "\" " << rgFile.toLocal8Bit() << " " <<  m_packDataDirName.toLocal8Bit() <<  "/ || exit " << errorPoint++ << endl;

    m_script.close();

    // run the assembled script
    m_process = new QProcess;
    m_process->setWorkingDirectory(m_packTmpDirName);
    m_process->start("bash", QStringList() << scriptName);
    connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(finishPack(int, QProcess::ExitStatus)));
}

void
ProjectPackager::finishPack(int exitCode, QProcess::ExitStatus) {

RG_DEBUG << "ProjectPackager::finishPack - exit code: " << exitCode;

    if (exitCode == 0) {
        delete m_process;
    } else {
        puke(tr("<qt><p>Encoding and compressing files failed with exit status %1. Checking %2 for the line that ends with \"exit %1\" may be useful for diagnostic purposes.</p>%3</qt>").arg(exitCode).arg(m_script.fileName()).arg(m_abortText));
        return;
    }

    m_script.remove();
    
    // remove the original file which is now safely in a package
    //
    // Well.  Oops.  No, m_filename is the .rgp version, so we need to remove
    // the .rg file that is now safely in a package, which was saved by
    // RosegardenMainWindow at the start of all this
    QFileInfo fi(m_filename);
    QString dirname = fi.path();
    QString basename = QString("%1/%2.rg").arg(dirname).arg(fi.baseName());
    if (!QFile::remove(basename)) {
        puke(tr("<qt>Could not remove<br>%1<br><br>%2</qt>").arg(basename).arg(m_abortText));
        return;
    }

    rmdirRecursive(m_packTmpDirName);
    accept();
    exitCode++; // break point
}


///////////////////////////
//                       //
// UNPACKING OPERATIONS  //
//                       //
///////////////////////////
void
ProjectPackager::runUnpack()
{

RG_DEBUG << "ProjectPackager::runUnpack() - unpacking " << m_filename;
    
    m_info->setText(tr("Unpacking project..."));

    // go into spinner mode, and we'll just leave it there for the duration of
    // the unpack too, because the operations are either too fast or too hard to
    // divide into discrete steps, and I'm bored with progress bars
    m_progress->setMaximum(0);

    m_process = new QProcess;

    // We can't assume foo.rgp actually contains foo.rg, it could
    // contain bar.rg and bar/ if the user was evil, and users tend to be.
    //
    // So while there are other ways to get here, Ilan had already written all
    // of this code to process a text file, and we'll just go that route.
    QString ofile("/tmp/rosegarden-project-package-filelist");

    // merge stdout and sterr for laziness of debugging (any errors in here mean
    // bad news)
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    // equivalent of [command] > ofile
    m_process->setStandardOutputFile(ofile, QIODevice::Truncate);

    // This is a very fast operation, just listing the files in a tarball
    // straight to a file on disk without involving a terminal, so we
    // will do this one as a waitForFinished() and risk blocking here
    //
    // (note that QProcess apparently handles escaping any spaces &c. in
    // m_filename here)
    m_process->start("tar", QStringList() << "tf" << m_filename);
    m_process->waitForStarted();

RG_DEBUG << "process started: tar tf " << m_filename;

    m_process->waitForFinished();

    if (m_process->exitCode() == 0) {
       delete m_process;
    } else {
        puke(tr("<qt><p>Unable to obtain list of files using tar.</p><p>Process exited with status code %1</p></qt>").arg(m_process->exitCode()));
        return;
    }

    QFile contents(ofile);

    if (!contents.open(QIODevice::ReadOnly | QIODevice::Text)) {
        puke(tr("<qt><p>Unable to create file list.</p>%1</qt>").arg(m_abortText));
        return;
    }

    QTextStream in1(&contents);
    QString line;
    QStringList flacFiles, wavpackFiles;

    // rude but effective hack, the primary and interesting .rg file in the
    // package is always the first one listed, so we grab that and avoid trouble
    // in the event the user was idiotic enough to include other .rg files as
    // extra files in the package data dir
    bool haveRG = false;

    while (true) {
        line = in1.readLine(1000);
        if (line.isEmpty()) break;
        // find .flac (FLAC) files
        if (line.indexOf(".flac", 0) > 0) {
            flacFiles << line;

RG_DEBUG << "Discovered FLAC for decoding:    " << line;

        // find .wv (WavPack) files
        } else if (line.indexOf(".wv", 0) > 0) {
            wavpackFiles << line;

RG_DEBUG << "Discovered WavPack for decoding: " << line;

        // find the true name of the .rg file contained in the package (foo.rgp
        // contains bar.rg and bar/)
        } else if ((line.indexOf(".rg", 0) > 0) && !haveRG) {
            m_trueFilename = line;

RG_DEBUG << "Discovered true filename: " << m_trueFilename;

            haveRG = true;
        }

    }
    contents.remove();

    QString completeTrueFilename = getTrueFilename();

    QFileInfo fi(completeTrueFilename);
    if (fi.exists()) {
        QMessageBox::StandardButton reply =  QMessageBox::warning(this,
                tr("Rosegarden"),
                tr("<qt><p>It appears that you have already unpacked this project package.</p><p>Would you like to load %1 now?</p></qt>").arg(completeTrueFilename),
                QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel);

        if (reply == QMessageBox::Ok) {
            // If they choose Ok, we'll accept() here to abort processing and
            // tell RosegardenMainWindow to load m_trueFilename
            accept();
        } else {
            reject();
        }
     } else {
         startAudioDecoder(flacFiles, wavpackFiles);
     }
}


void
ProjectPackager::startAudioDecoder(QStringList flacFiles, QStringList wavpackFiles)
{
    // we can't do a oneliner bash script straight out of a QProcess command
    // line, so we'll have to create a purpose built script and run that
    QString scriptName("/tmp/rosegarden-audio-decoder-backend");
    m_script.setFileName(scriptName);

    // remove any lingering copy from a previous run
    if (m_script.exists()) m_script.remove();

    if (!m_script.open(QIODevice::WriteOnly | QIODevice::Text)) {
        puke(tr("<qt><p>Unable to write to temporary backend processing script %1.</p>%2</qt>").arg(scriptName).arg(m_abortText));
        return;
    }

    QTextStream out(&m_script);
    out << "# This script was generated by Rosegarden to combine multiple external processing"      << endl
        << "# operations so they could be managed by a single QProcess.  If you find this script"   << endl
        << "# it is likely that something has gone terribly wrong. See http://rosegardenmusic.com" << endl;

    int errorPoint = 1;

    // The working directory must be the key to why tar is not failing, but
    // failing to do anything detectable.  Let's cut apart m_filename...
    QFileInfo fi(m_filename);
    QString dirname = fi.path();
    QString basename = QString("%1.%2").arg(fi.baseName()).arg(fi.completeSuffix());

    // There were mysterious stupid problems running tar xf in a separate
    // QProcess step, so screw it, let's just throw it into this script!
    out << "tar xzf \"" << basename << "\" || exit " << errorPoint++ << endl;

    // Decode FLAC files
    QStringList::const_iterator si;
    for (si = flacFiles.constBegin(); si != flacFiles.constEnd(); ++si) {
        QString o1 = (*si);

        // the file strings are things like xxx.wav.rgp.flac
        // without specifying the output file they will turn into xxx.wav.rgp.wav
        // thus it is best to specify the output as xxx.wav
        //
        // files from new project packages have rg-23324234.flac files, files
        // from old project packages have rg-2343242.wav.rgp.flac files, so we
        // want a robust solution to this one... QFileInfo::baseName() should
        // get it
        QFileInfo fi(o1);
        QString o2 = QString("%1/%2.wav").arg(fi.path()).arg(fi.baseName());

        // we'll eschew anything fancy or pretty in this disposable script and
        // just write a command on each line, terminating with an || exit n
        // which can be used to figure out at which point processing broke, for
        // cheap and easy error reporting without a lot of fancy stream wiring
        //
        // (let's just try escaping spaces &c. with surrounding " and see if
        // that is good enough)

RG_DEBUG << "flad -d " << o1 << " -o " << o2;

        out << "flac -d \"" <<  o1 << "\" -o \"" << o2 << "\" && rm \"" << o1 <<  "\" || exit " << errorPoint << endl;
        errorPoint++;
    }

    // Decode WavPack files
    for (si = wavpackFiles.constBegin(); si != wavpackFiles.constEnd(); ++si) {
        QString o = (*si);

        // NOTE: wvunpack -d means "delete the file if successful" not "decode"
        out << "wvunpack -d \"" <<  o << "\" || exit " << errorPoint << endl;
        errorPoint++;
    }

    m_script.close();

    // run the assembled script
    m_process = new QProcess;

    // set to the working directory extracted from m_filename above, as this is
    // was apparently the reason why tar always failed to do anything
    m_process->setWorkingDirectory(dirname);
    m_process->start("bash", QStringList() << scriptName);
    connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(finishUnpack(int, QProcess::ExitStatus)));

    // wait up to 30 seconds for process to start
    m_info->setText(tr("Decoding audio files..."));
    if (!m_process->waitForStarted()) {
        puke(tr("<qt>Could not start backend processing script %1.</qt>").arg(scriptName));
        return;
    }
}


// Finish up, and then hack the document audio path, the audio path associated
// with any plugins, and the path to any data the plugins refer to, so these
// will all be pointing at where the file resides now that we have unpacked it,
// and we leave nothing to chance.
//
// NOTE: along the way we're taking a typical audio path like "~/rosegarden" and
// writing out something hard coded for the "~".  We assume users in other
// locations will be working with the .rgp file, and we'll adapt it to their
// surroundings when they unpack it in those surroundings.  Also, the plugin
// audio path was already hard coded to "/home/$(whoami)/wherever" anyway.
void
ProjectPackager::finishUnpack(int exitCode, QProcess::ExitStatus) {

RG_DEBUG << "ProjectPackager::finishUnpack - exit code: " << exitCode;

    if (exitCode == 0) {
        delete m_process;
    } else {
        puke(tr("<qt><p>Extracting and decoding files failed with exit status %1. Checking %2 for the line that ends with \"exit %1\" may be useful for diagnostic purposes.</p>%3</qt>").arg(exitCode).arg(m_script.fileName()).arg(m_abortText));
        return;
    }

    // we don't need to do anything with the extra files in the unpack step, so
    // we ignore the list returned (tar already extracted the files, and they're
    // there)
    QFileInfo fi(m_filename);
    QString newPath = QString("%1/%2").arg(fi.path()).arg(fi.baseName());
    QString oldName = QString("%1.rg").arg(newPath);
    getPluginFilesAndRewriteXML(oldName, newPath);

    m_script.remove();
    accept();
    exitCode++; // break point
}


}

