// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef ROSEGARDENGUIDOC_H
#define ROSEGARDENGUIDOC_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif 

// include files for QT
#include <qobject.h>
#include <qstring.h>
#include <qlist.h>
#include <qxml.h>

#include "Composition.h"
#include "MappedComposition.h"
#include "multiviewcommandhistory.h"
#include "AudioFileManager.h"

// forward declaration of the RosegardenGUI classes
class RosegardenGUIView;
class ViewElementsManager;
class SegmentItem;

/**
  * RosegardenGUIDoc provides a document object for a document-view model.
  *
  * The RosegardenGUIDoc class provides a document object that can be
  * used in conjunction with the classes RosegardenGUIApp and
  * RosegardenGUIView to create a document-view model for standard KDE
  * applications based on KApplication and KTMainWindow. Thereby, the
  * document object is created by the RosegardenGUIApp instance and
  * contains the document structure with the according methods for
  * manipulation of the document data by RosegardenGUIView
  * objects. Also, RosegardenGUIDoc contains the methods for
  * serialization of the document data from and to files.
  *
  * RosegardenGUIDoc owns the Composition in the document.
  */

class RosegardenGUIDoc : public QObject
{
    Q_OBJECT
public:

    /**
     * Constructor for the fileclass of the application
     */
    RosegardenGUIDoc(QWidget *parent, const char *name=0);

    /**
     * Destructor for the fileclass of the application
     */
    ~RosegardenGUIDoc();

    /**
     * adds a view to the document which represents the document
     * contents. Usually this is your main view.
     */
    void addView(RosegardenGUIView *view);

    /**
     * removes a view from the list of currently connected views
     */
    void removeView(RosegardenGUIView *view);

    /**
     * sets the modified flag for the document after a modifying
     * action on the view connected to the document.
     */
    void setModified(bool _m=true){ m_modified=_m; };

    /**
     * returns if the document is modified or not. Use this to
     * determine if your document needs saving by the user on closing.
     */
    bool isModified(){ return m_modified; };

    /**
     * "save modified" - asks the user for saving if the document is
     * modified
     */
    bool saveIfModified();	

    /**
     * deletes the document's contents
     */
    void deleteContents();

    /**
     * initializes the document generally
     */
    bool newDocument();

    /**
     * closes the actual document
     */
    void closeDocument();

    /**
     * loads the document by filename and format and emits the
     * updateViews() signal
     */
    bool openDocument(const QString &filename, const char *format=0);

    /**
     * saves the document under filename and format.
     */	
    bool saveDocument(const QString &filename, const char *format=0);

    /**
     *   sets the path to the file connected with the document
     */
    void setAbsFilePath(const QString &filename);

    /**
     * returns the pathname of the current document file
     */
    const QString &getAbsFilePath() const;

    /**
     * sets the filename of the document
     */
    void setTitle(const QString &_t);

    /**
     * returns the title of the document
     */
    const QString &getTitle() const;

    /**
     * returns the global command history
     */
    MultiViewCommandHistory *getCommandHistory() {
	return &m_commandHistory;
    }

    /**
     * deletes the document views
     */
    void deleteViews();

    Rosegarden::Composition&       getComposition()       { return m_composition; }
    const Rosegarden::Composition& getComposition() const { return m_composition; }

    /**
     * insert some recorded MIDI events into our recording Segment
     */
    void insertRecordedMidi(const Rosegarden::MappedComposition &mc);

    /**
     * Tidy up the recording Segment
     */
    void stopRecordingMidi();

    /**
     * Register audio samples at the sequencer
     */
    void prepareAudio();

    /**
     * Cause the pointerPositionChanged signal to be emitted and any
     * associated internal work in the document to happen
     */
    void setPointerPosition(Rosegarden::timeT);

    /**
     * Cause the playPositionChanged signal to be emitted and any
     * associated internal work in the document to happen
     */
    void setPlayPosition(Rosegarden::timeT);

    /**
     * Cause the loopChanged signal to be emitted and any
     * associated internal work in the document to happen
     */
    void setLoop(Rosegarden::timeT, Rosegarden::timeT);

public slots:
    /**
     * calls repaint() on all views connected to the document object
     * and is called by the view by which the document has been
     * changed.  As this view normally repaints itself, it is excluded
     * from the paintEvent.
     */
    void slotUpdateAllViews(RosegardenGUIView *sender);

    void slotDocumentModified();
    void slotDocumentRestored();

    void slotSetPointerPosition(Rosegarden::timeT t) { setPointerPosition(t); }
    void slotSetPlayPosition(Rosegarden::timeT t) { setPlayPosition(t); }
    void slotSetLoop(Rosegarden::timeT s, Rosegarden::timeT e) {setLoop(s,e);}

signals:
    void pointerPositionChanged(Rosegarden::timeT);
    void playPositionChanged(Rosegarden::timeT);
    void loopChanged(Rosegarden::timeT, Rosegarden::timeT);

protected:

    /**
     * Parse the Rosegarden file \a file
     *
     * \errMsg will contains the error messages
     * if parsing failed.
     *
     * @return false if parsing failed
     * @see RoseXmlHandler
     */
    bool xmlParse(QString& fileContents, QString &errMsg);

    /**
     * Write the given string to the given file, compressed.
     * @return false for failure
     */
    bool writeToFile(const QString &fileName, const QString &text);

    /**
     * Read the contents of the given string into the given text stream,
     * uncompressing as you go.
     * @return false for failure
     */
    bool readFromFile(const QString &fileName, QString &text);
 	
public:	
    /**
     * the list of the views currently connected to the document
     */
    static QList<RosegardenGUIView> *pViewList;	

private:
    //--------------- Data members ---------------------------------

    /**
     * the modified flag of the current document
     */
    bool m_modified;

    /**
     * the title of the current document
     */
    QString m_title;

    /**
     * absolute file path of the current document
     */
    QString m_absFilePath;

    /**
     * the composition this document is wrapping
     */
    Rosegarden::Composition m_composition;

    /**
     * stores AudioFile mappings
     */
    Rosegarden::AudioFileManager m_audioFileManager;

    /**
     * a Segment onto which we can record events
     */
    Rosegarden::Segment *m_recordSegment;
    Rosegarden::timeT m_endOfLastRecordedNote;  // we use this for rest filling

    MultiViewCommandHistory m_commandHistory;
};

#endif // ROSEGARDENGUIDOC_H
