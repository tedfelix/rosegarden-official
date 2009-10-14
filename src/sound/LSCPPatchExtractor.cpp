/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "LSCPPatchExtractor.h"

#include <QFile>
#include <QTextStream>
#include <QStringList>

#include <iostream>

class QFile;
class QTextStream;
class QString;
class QStringList;

namespace Rosegarden
{

bool
LSCPPatchExtractor::isLSCPFile(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly)) {
        return false;
    } else {
        QTextStream check(&file);
        while (!check.atEnd()) {
            QString currentLine = check.readLine();
            if (currentLine.contains("map", Qt::CaseInsensitive)) { //CaseInsensitive - you could never be sure

                //A stupid way of determining if we can use the file, but I can't remember better one!

                std::cout << "MAP string found!" << std::endl;
                return true;
            }
        }
        std::cout << "Has extension, but it will not be useful!" << std::endl;
        return false;
    }
}

LSCPPatchExtractor::Device
LSCPPatchExtractor::extractContent(const QString& fileName)
{
    std::cout << "Usao sam u extractContent u LSCPParser-u!" << std::endl;
    Device device;
    
    QFile lscpFile(fileName);
    QTextStream inStream(&lscpFile);
    QStringList splitLine;
    
    unsigned int bank, program;
    std::string programName;
    //    std::string deviceName;
    
    if (!lscpFile.open(QFile::ReadOnly)) {
        return device;
    } else {
        while (!inStream.atEnd()) {
            QString currentLine = inStream.readLine();
            currentLine = currentLine.simplified();
            
            if (!currentLine.isEmpty() && currentLine.startsWith("add", Qt::CaseInsensitive)) {
                // Preparation for returning device's name - do nothing for now!

                // splitLine = currentLine.split(QRegExp("\\s+"));
                // deviceName = splitLine.at(2).latin1();
                
            } else if (!currentLine.isEmpty() && currentLine.startsWith("map", Qt::CaseInsensitive)) {

                unsigned int positionOfBankElement = 3; //position of element in QStringList if NON_MODAL element is absent
                unsigned int positionOfProgramElement = 4; //similar to above

                splitLine = currentLine.split(QRegExp("\\s+"));
                std::cout << splitLine.size() << std::endl;

                if (splitLine.at(2) == "NON_MODAL") {
                    positionOfBankElement = 4;
                    positionOfProgramElement = 5;
                }

                bank = splitLine.at(positionOfBankElement).toInt();
                program = splitLine.at(positionOfProgramElement).toInt();
                
                QString quotedName = splitLine.at(splitLine.size() - 1);
                if (quotedName[0] != '\'') {
                    quotedName = "Unnamed instrument";
                } else {
                    programName = quotedName.latin1();
                }

                device[bank][program] = programName;
            }
            
        }
        return device;
    }
}

}
