#ifndef _SOUNDFILE_H_
#define _SOUNDFILE_H_

// Abstract base class for SoundFile - both MidiFile
// and AudioFile are derived from this class.
//
//

#include <iostream>
#include <fstream>
#include <string>

namespace Rosegarden
{


typedef unsigned char FileByte; 

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

    // get some bytes from an input stream
    const std::string getBytes(std::ifstream *file,
                               const unsigned int &numberOfBytes);
 
    // write some bytes to an output stream
    void putBytes(std::ofstream *file,
                  const std::string outputString);


};

};


#endif // _SOUNDFILE_H_


