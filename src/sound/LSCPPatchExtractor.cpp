/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.
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
/////// Works for single device midi mappings!

    Device device;
    
    QFile lscpFile(fileName);
    QTextStream inStream(&lscpFile);

    QStringList splitLine;

    unsigned int bank, program;
    std::string programName;
    std::string bankName;
    std::string tempDeviceName, tempBankName;
    
    if (!lscpFile.open(QFile::ReadOnly)) {
        return device;
    } else {
        while (!inStream.atEnd()) {
            QString currentLine = inStream.readLine();
            currentLine = currentLine.simplified();
            
            Bank_Prog currentDevice;

            if (!currentLine.isEmpty() && currentLine.startsWith("add", Qt::CaseInsensitive)) {

///// Useless for now. Will be needed if someone dedicates time to implement Device import!
//                std::cout << "Usao sam u ADD!";
//
//                splitLine = splitQuotedString(currentLine);
//                tempDeviceName = splitLine.at(2).latin1();
//                //debug
//                for (int i = 0; i < splitLine.size(); i++) std::cout << splitLine.at(i) << std::endl;
//                std::cout << "  " << tempDeviceName << std::endl;
                
            } else if (!currentLine.isEmpty() && currentLine.startsWith("map", Qt::CaseInsensitive)) {

//                std::cout << "Usao sam u MAP!";

                unsigned int positionOfBankElement = 3; //position of element in QStringList if NON_MODAL element is absent
                unsigned int positionOfProgramElement = 4; //similar to above
                unsigned int positionOfPatchFileName = 6; //we extract bank name from filesystem's file name.

                splitLine = splitQuotedString(currentLine);

                if (splitLine.at(2) == "NON_MODAL") {
                    // Shifting position of elements if optional element (NON MODAL) is present
                    positionOfBankElement = 4;
                    positionOfProgramElement = 5;
                    positionOfPatchFileName = 7;
                }

                //Getting bank name HACK!!! Not in specification, so we use filename!
                QString patchFileName = splitLine.at(positionOfPatchFileName);
                QStringList splitPatchFileName = patchFileName.split("/");
                QString temp = splitPatchFileName.at(splitPatchFileName.size()-1);
                temp = temp.replace("x20"," ");
                temp = temp.remove(".gig");

                currentDevice.bankName = temp.latin1();
                currentDevice.bankNumber = splitLine.at(positionOfBankElement).toInt();
                currentDevice.programNumber = splitLine.at(positionOfProgramElement).toInt();

                QString quotedName = splitLine.at(splitLine.size() - 1);
                // Chacking for another optional element. This one is even more strange - program name may be absent as well!
                if (quotedName.isEmpty() ||
                    quotedName.contains("ON_DEMAND") ||
                    quotedName.contains("ON_DEMAND_HOLD") ||
                    quotedName.contains("PERSISTENT")) {
                    currentDevice.programName = "Unnamed instrument";
                } else {
                    currentDevice.programName = quotedName.latin1();
                }

                device.push_back(currentDevice);
            }
            
        }
        return device;
    }
}

}
