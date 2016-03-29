/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SOUNDFILE_H
#define RG_SOUNDFILE_H

// SoundFile is an abstract base class defining behaviour for both
// MidiFiles and AudioFiles.  The getBytes routine is buffered into
// suitably sized chunks to prevent excessive file reads.
// 
//

#include <fstream>
#include <string>

#include "base/Exception.h"
#include "misc/Strings.h"

#include <QCoreApplication>

namespace Rosegarden
{

typedef unsigned char FileByte;

class SoundFile
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::SoundFile)

public:
    SoundFile(const QString &fileName);
    virtual ~SoundFile();

    class BadSoundFileException : public Exception
    {
    public:
        BadSoundFileException(QString path) :
            Exception(QObject::tr("Bad sound file ") + path), m_path(path) { }
        BadSoundFileException(QString path, std::string message) :
            Exception(QObject::tr("Bad sound file ") + path + ": " + strtoqstr(message)), m_path(path) { }
        BadSoundFileException(QString path, QString file, int line) :
            Exception(QObject::tr("Bad sound file ") + path, file, line), m_path(path) { }

        ~BadSoundFileException() throw() { }

        QString getPath() const { return m_path; }

    private:
        QString m_path;
    };

    // All files should be able open, write and close
    virtual bool open() = 0;
    virtual bool write() = 0;
    virtual void close() = 0;

    QString getShortFilename() const;
    QString getFilename() const { return m_fileName; }
    void setFilename(const QString &fileName) { m_fileName = fileName; }

    // Useful methods that operate on our file data
    //
    int getIntegerFromLittleEndian(const std::string &s);
    std::string getLittleEndianFromInteger(unsigned int value,
                                           unsigned int length);

    int getIntegerFromBigEndian(const std::string &s);
    std::string getBigEndianFromInteger(unsigned int value,
                                        unsigned int length);

    // Buffered read - allow this to be public
    //
    std::string getBytes(unsigned int numberOfBytes);

    // Return file size
    //
    size_t getSize() const { return m_fileSize; }

    void resetStream() { m_inFile->seekg(0); m_inFile->clear(); }

    // check EOF status
    //
    bool isEof() const 
        { if (m_inFile) return m_inFile->eof(); else return true; }

protected:
    QString m_fileName;

    // get some bytes from an input stream - unbuffered as we can
    // modify the file stream
    std::string getBytes(std::ifstream *file, unsigned int numberOfBytes);

    // Get n bytes from an input stream and write them into buffer.
    // Return the actual number of bytes read.
    size_t getBytes(std::ifstream *file, char *buffer, size_t n);

    // write some bytes to an output stream
    void putBytes(std::ofstream *file, const std::string outputString);

    // write some bytes to an output stream
    void putBytes(std::ofstream *file, const char *buffer, size_t n);

    // Read buffering - define chunk size and buffer file reading
    //
    int            m_readChunkPtr;
    int            m_readChunkSize;
    std::string    m_readBuffer;

    std::ifstream *m_inFile;
    std::ofstream *m_outFile;
    
    bool           m_loseBuffer; // do we need to dump the read buffer
                                 // and re-fill it?

    size_t         m_fileSize;

};

}


#endif // RG_SOUNDFILE_H


