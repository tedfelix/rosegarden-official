#ifndef _SOUNDFILE_H_
#define _SOUNDFILE_H_

// Abstract base class for SoundFile - both MidiFile
// and AudioFile are derived from this class.
//
//

namespace Rosegarden
{

#include <string>

class SoundFile
{
public:
    SoundFile(const std::string &fileName):m_fileName(fileName) {;}
    virtual ~SoundFile() {;}

    virtual bool open() = 0;
    virtual bool write() = 0;

    const std::string& getFilename() { return m_fileName; }
    void setFilename(const std::string &fileName) { m_fileName = fileName; }


protected:
    std::string m_fileName;

};

};


#endif // _SOUNDFILE_H_


