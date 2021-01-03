/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_PERCUSSIONMAP_H
#define RG_PERCUSSIONMAP_H

#include "base/BaseProperties.h"
#include "gui/general/ResourceFinder.h"

#include <map>

#include <QXmlDefaultHandler>

namespace Rosegarden
{
/**
 * A simple helper class for mapping percussion instruments to staff. A much
 * better solution would be if Rosegarden does support a real percussion
 * notation but for now this will do.
 */
class PercussionMap : public QXmlDefaultHandler
{
public:
    /**
     * Constructs a PercussionMap. For now it just defines a few percussion
     * instruments. Future extensions could be a separate mapping file or
     * the mapping could included in Rosegarden self (in the
     * studio/device/keymapping?)
     */
    PercussionMap() {};

    /**
     * Retrieves the MusicXML pitch for given pitch.
     * If no mapping is found, the pitch is returned
     *
     * @param pitch the performance pitch of the percussion instrument.
     */
    int getPitch(int pitch);

    /**
     * Retrieves the MusicXML notehead for given pitch. Please note this is
     * the MusicXML notehead name, note the Rosegarden name!
     * If no mapping is found "normal" is returned.
     *
     * @param pitch the performance pitch of the percussion instrument.
     */
    std::string getNoteHead(int pitch);

    /**
     * Retrieves the MusicXML voice for given pitch.
     * If no mapping is found an 1 is returned.
     *
     * @param pitch the performance pitch of the percussion instrument.
     */
    int getVoice(int pitch);

    /**
     * Load the default PercussionMap file.
     */
    bool loadDefaultPercussionMap();

    /**
     * Load a PercussionMap file.
     */
    bool loadPercussionMap(const QString &filename);

//     virtual bool startDocument();

    bool startElement(const QString& namespaceURI,
                              const QString& localName,
                              const QString& qName,
                              const QXmlAttributes& atts) override;

    bool endElement(const QString& namespaceURI,
                            const QString& localName,
                            const QString& qName) override;

//     virtual bool characters(const QString& ch);

//     virtual bool endDocument ();


protected:
    /**
     * A simple mapping table. Contains for a perfomance pitch the
     * step, octave and notehead to be used in the MusicXML file.
     * Hopefully this part can moved to the Rosegarden studio in the future.
     */
    class PMapData {
    public:
        PMapData(int pitch=1, const std::string &notehead="", int upper=true) :
                m_pitch(pitch),
                m_notehead(notehead),
                m_voice(upper?1:2)
                {};
        int         m_pitch;
        std::string m_notehead;
        int         m_voice;
    };

    /**
     * A map containing mapping data for a performance pitch.
     */
    std::map<int, PMapData> m_data;

private:
    int         m_xmlPitchIn;
    int         m_xmlPitchOut;
    std::string m_xmlNotehead;
    bool        m_xmlStemUp;
};

}
#endif
