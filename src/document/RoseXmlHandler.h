/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_ROSEXMLHANDLER_H
#define RG_ROSEXMLHANDLER_H

#include "base/Device.h"
#include "base/MidiProgram.h"
#include "base/Event.h"
#include "document/io/XMLHandler.h"

#include <QString>
#include <QtCore/QSharedPointer>
#include <QPointer>
#include <QProgressDialog>
#include <QSharedPointer>

#include <map>
#include <set>
#include <vector>
#include <string>


namespace Rosegarden
{

class XmlStorableEvent;
class XmlSubHandler;
class Studio;
class Segment;
class SegmentLinker;
class RosegardenDocument;
class Instrument;
class Device;
class Composition;
class ColourMap;
class Buss;
class AudioPluginManager;
class AudioPluginInstance;
class AudioFileManager;


/**
 * Handler for the Rosegarden XML format
 */
class RoseXmlHandler : public QObject, public XMLHandler
{
    Q_OBJECT
public:

    typedef enum
    {
        NoSection,
        InComposition,
        InSegment,
        InStudio,
        InInstrument,
        InBuss,
        InAudioFiles,
        InPlugin,
        InAppearance
    } RosegardenFileSection;

    /**
     * Construct a new RoseXmlHandler which will put the data extracted
     * from the XML file into the specified composition
     */
    RoseXmlHandler(RosegardenDocument *doc,
                   unsigned int elementCount,
                   QPointer<QProgressDialog> progressDialog,
                   bool createNewDevicesWhenNeeded);

    ~RoseXmlHandler() override;

    /// overloaded handler functions
    bool startDocument() override;
    bool startElement(const QString& namespaceURI,
                              const QString& localName,
                              const QString& qName,
                              const QXmlStreamAttributes& atts) override;

    bool endElement(const QString& namespaceURI,
                            const QString& localName,
                            const QString& qName) override;

    bool characters(const QString& chars) override;

    bool endDocument() override; // [rwb] - for tempo element catch

    bool isDeprecated() { return m_deprecation; }

    /// Return the error string set during the parsing (if any)
    QString errorString() const override;

    bool hasActiveAudio() const { return m_hasActiveAudio; }

    const std::set<QString> &pluginsNotFound() const
            { return m_pluginsNotFound; }

    bool fatalError(int lineNumber, int columnNumber,
                    const QString& msg) override;


protected:

    // just for convenience -- just call to the document
    //
    Composition& getComposition();
    Studio& getStudio();
    AudioFileManager& getAudioFileManager();
    QSharedPointer<AudioPluginManager> getAudioPluginManager();

    void setSubHandler(XmlSubHandler* sh);
    XmlSubHandler* getSubHandler() { return m_subHandler; }

    void addMIDIDevice(const QString& name,
                       bool createAtSequencer,
                       const QString& dir);  // dir = play|record
    void setMIDIDeviceConnection(const QString &connection);
    void setMIDIDeviceName(const QString &name);
    // unused void skipToNextPlayDevice();
    InstrumentId mapToActualInstrument(InstrumentId oldId);

    RosegardenDocument    *m_doc;
    Segment *m_currentSegment;
    XmlStorableEvent    *m_currentEvent;
    typedef std::map<int, SegmentLinker *> SegmentLinkerMap;
    SegmentLinkerMap m_segmentLinkers;

    timeT m_currentTime;
    timeT m_chordDuration;
    timeT *m_segmentEndMarkerTime;

    bool m_inChord;
    bool m_inGroup;
    bool m_inComposition;
    bool m_inColourMap;
    bool m_inMatrix;
    bool m_inNotation;
    std::string m_groupType;
    int m_groupId;
    int m_groupTupletBase;
    int m_groupTupledCount;
    int m_groupUntupledCount;
    std::map<long, long> m_groupIdMap;

    bool m_foundTempo;

    QString m_errorString;

    // Using a std::set to avoid a potentially very long list
    // of duplicate missing plugins across tracks.
    std::set<QString> m_pluginsNotFound;

    RosegardenFileSection             m_section;

    Device                           *m_device;
    DeviceId                          m_deviceRunningId;
    InstrumentId                      m_deviceInstrumentBase;
    InstrumentId                      m_deviceReadInstrumentBase;
    std::map<InstrumentId, InstrumentId> m_actualInstrumentIdMap;
    bool                              m_percussion;
    bool                              m_sendBankSelect;
    MidiByte                          m_msb;
    MidiByte                          m_lsb;
    Instrument                       *m_instrument;

    /// We have a volume CC.  Ignore old-style <volume> tags.
    bool m_haveVolumeCC;
    /// We have a pan CC.  Ignore old-style <pan> tags.
    bool m_havePanCC;

    Buss                             *m_buss;
    AudioPluginInstance              *m_plugin;
    bool                              m_pluginInBuss;
    ColourMap                        *m_colourMap;
    QSharedPointer<MidiKeyMapping> m_keyMapping;
    MidiKeyMapping::KeyNameMap        m_keyNameMap;
    unsigned int                      m_totalElements;
    unsigned int                      m_elementsSoFar;

    XmlSubHandler                    *m_subHandler;
    bool                              m_deprecation;
    bool                              m_createDevices;
    bool                              m_haveControls;

    bool m_hasActiveAudio;

    bool locateAudioFile(const QString& id,
                         const QString& file,
                         const QString& label);
    /// Whether the audio skip warning has been issued.
    bool m_audioSkipWarning;

    // In case we encounter an old solo attribute at the composition level,
    // hold onto it and use it to set the solo for the proper track.
    bool m_oldSolo;

    QPointer<QProgressDialog> m_progressDialog;
};


}

#endif
