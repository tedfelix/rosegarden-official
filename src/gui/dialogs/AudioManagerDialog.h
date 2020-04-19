
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
    COPYING included with this distribution for more information.
*/

#ifndef RG_AUDIOMANAGERDIALOG_H
#define RG_AUDIOMANAGERDIALOG_H

#include "sound/AudioFile.h"
#include "misc/ConfigGroups.h"
#include "gui/general/ActionFileClient.h"
#include "gui/widgets/WarningGroupBox.h"
#include "base/Selection.h"

#include <QMainWindow>

class QWidget;
class QTimer;
class QString;
class QTreeWidget;
class QTreeWidgetItem;
class QLabel;
class QDropEvent;
class QCloseEvent;
class QShortcut;
class QUrl;

//class KURL;


namespace Rosegarden
{


class Segment;
class RosegardenDocument;
class RealTime;
class AudioPlayingDialog;
class AudioFile;
class Command;


class AudioManagerDialog : public QMainWindow, public ActionFileClient
{
    Q_OBJECT

public:
    AudioManagerDialog(QWidget *parent,
                       RosegardenDocument *doc);
    ~AudioManagerDialog() override;

    // Populate the file list from the AudioFileManager
    //

    // Return a pointer to the currently selected AudioFile -
    // returns 0 if nothing is selected
    //
    AudioFile* getCurrentSelection();

    // Scroll and expand to show this selected item
    //
    void setSelected(AudioFileId id,
                     const Segment *segment,
                     bool propagate); // if true then we tell the segmentcanvas

    // Pop down playing dialog if it's currently up
    //
    void closePlayingDialog(AudioFileId id);

    // Can we playback audio currently?
    //
    void setAudioSubsystemStatus(bool ok);

    // Return the shortcut object
    //
    QShortcut* getShortcuts() { return m_shortcuts; }

    // Add a new file to the audio file manager
    //
    bool addAudioFile(const QString &filePath);


public slots:
    void slotAdd();
    void slotPlayPreview();
    void slotRename();
    void slotInsert();
    void slotRemove();
    void slotRemoveAll();
    void slotRemoveAllUnused();
    void slotDeleteUnused();
    void slotExportAudio();
    void slotHelpRequested();
    void slotHelpAbout();
	
	//void slotItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
	
    // get selection
	void slotSelectionChanged();
//	void slotSelectionChanged(QTreeWidgetItem *);

    // Repopulate
    //
    void slotPopulateFileList();

    // Commands
    //
    void slotCommandExecuted();

    /**
     * Accept a list of Segments and highlight accordingly
     * Used to reflect a selection on the main view
     * (when the user selects an audio track, the corresponding item
     * in the audio manager should be selected in turn)
     *
     * We check for embedded audio segments and if we find exactly one
     * we highlight it.  If we don't we unselect everything.
     *
     */
    void slotSegmentSelection(const SegmentSelection &);

    /**
     * Cancel the currently playing audio file
     */
    void slotCancelPlayingAudioFile();

    void slotClose();

    /**
     * Turn a MIDI segment into a set of audio segments triggered
     * by the MIDI Note Ons
     */
    void slotDistributeOnMidiSegment();

signals:

    // Control signals so we can tell the sequencer about our changes
    // or actions.
    //
    void addAudioFile(AudioFileId);
    void deleteAudioFile(AudioFileId);
    void playAudioFile(AudioFileId,
                       const RealTime &,
                       const RealTime &);
    void cancelPlayingAudioFile(AudioFileId);
    void deleteAllAudioFiles();

    /// We've selected a segment here, make the canvas select it too
    /**
     * RosegardenMainWindow::slotAudioManager() connects this to
     * RosegardenMainViewWidget::slotPropagateSegmentSelection().
     */
    void segmentsSelected(const SegmentSelection&);

    void deleteSegments(const SegmentSelection&);
    void insertAudioSegment(AudioFileId,
                            const RealTime &,
                            const RealTime &);

    void closing();
protected slots:
    void slotDropped(QDropEvent *event, QTreeWidget*, const QList<QUrl> &sl );
    void slotCancelPlayingAudio();

protected:
    bool addFile(const QUrl& kurl);
    bool isSelectedTrackAudio();
    void selectFileListItemNoSignal(QTreeWidgetItem*);
    void updateActionState(bool haveSelection);

    void closeEvent(QCloseEvent *) override;

    //--------------- Data members ---------------------------------

    QTreeWidget        *m_fileList;         // (==AudioListView)
    WarningGroupBox    *m_wrongSampleRates;
    RosegardenDocument *m_doc;

    QShortcut           *m_shortcuts;

    AudioFileId  m_playingAudioFile;
    AudioPlayingDialog      *m_audioPlayingDialog;
    QTimer                  *m_playTimer;

    static const char* const m_listViewLayoutName;
    static const int         m_maxPreviewWidth;
    static const int         m_previewHeight;

    bool                     m_audiblePreview;
    int                      m_sampleRate;
};



}

#endif
