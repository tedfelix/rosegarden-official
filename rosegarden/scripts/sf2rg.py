#!/usr/bin/env python
"""

sf2rg.py a script that dumps soundfonts data in rosegarden xml data format 

depends on :

 * sf2cfg, an utity included in kde-multimedia package

 * sfxload, the soundfont loader included in awesfx package

 * alsa to get /proc/asound/card0/wavetableD1
"""
__revision__ = "0.1"

import os
import os.path
import sys
import logging
import getopt
from xml.dom.minidom import getDOMImplementation
import gzip

##############################################################################
class WaveTableStat(object):
    """

    a object that maps informations from /proc/asound/cardx/wavetableDy

    'addresses', 'allocated_blocks', 'allocated_voices', 'device', 
    'instruments', 'locked_instruments', 'locked_samples', 'max_voices',
    'memory_available', 'memory_size', 'parse', 'ports', 'samples',
    'soundfonts', 'use_counter'
    """
    ##########################################################################
    def __init__(self, procFile = "/proc/asound/card0/wavetableD1"):
        """

        @param procFile: path to /proc wavetable info path
        """
        self.procFile = procFile
        self.memory_available = None
        self.memory_size = None        
        self.device = None
        self.soundfonts = None
        self.parse()

    ##########################################################################
    def parse(self):
        """

        set attributes 
        """
        lines = open(self.procFile, 'r').readlines()
        for line in lines:
            attrName, value = line.split(':', 1)
            attrName = attrName.replace(' ', '_').lower()
            values = [ value.strip() for value in value.strip().split(' ') ]
            value = len(values) == 1 and values[0] or values
            setattr(self, attrName, value)
    
    ##########################################################################
    def __str__(self):
        toMo = lambda x: round(float(x) / ( 1024 ** 2 ))
        freepercent = round(int(self.memory_available) * 100
                / int(self.memory_size))
        return """%s : %s soundfonts in memory.
Total : %i Mo
Used : %i Mo (%i%%)
Free : %i Mo (%i%%)
""" % (
                self.device,
                self.soundfonts,
                toMo(self.memory_size),
                toMo(int(self.memory_size) - int(self.memory_available)),
                100 - freepercent,
                toMo(self.memory_available),
                freepercent,
                )
        
##############################################################################
class Sf2Rg:
    """

    a soundfont data dumper to rosegarden xml data    
    """
    ##########################################################################
    def __init__(self, sf2files = None, **kwargs):
        """
        
        @param sf2files: C{list} of soundfont file paths
        """
        self.sf2files = sf2files or []
        self.bankCount = 0
        self.doc = None
        self.drums = None
        self._lsbCount = None
        self._msbCount = None
        self._curFile = None
        self.device_element = None

        self.prettyNameFormat = True
        self.loadSoundFont = False 
        self.inputFile = None
        self.outputFile = None

    ##########################################################################
    def loadSoundFontList(self, path):
        """

        load a list of soundfont from an plain ascii with one path per line
        
        @param path: path to the soundfont list
        """
        inFile = open(path, 'r')
        self.sf2files = [ os.path.expanduser(line.strip()) \
                    for line in inFile.readlines() if line.strip() ]

    ##########################################################################
    def process(self):
        """
        
        do the job : get cfg data from each soundfont and launch parsing
        """
        # creating xml document
        impl = getDOMImplementation()
        self.doc = impl.createDocument(None, "rosegarden-data", None)
        
        studio_element = self.doc.createElement('studio')
        studio_element.setAttribute('thrufilter', '0' )
        studio_element.setAttribute('recordfilter', '0' )
        
        self.doc.documentElement.appendChild(studio_element)
        
        device_element = self.doc.createElement('device')
        device_element.setAttribute('name', 'Emu10k1' )
        device_element.setAttribute('id', '0' )
        device_element.setAttribute('direction', 'play' )
        device_element.setAttribute('type', 'midi' )
        studio_element.appendChild(device_element)
        
        self.device_element = device_element

        # stepping through soundfount path list
        for sf2file in self.sf2files:
            self._curFile = sf2file
            cfg = self.getCfg(sf2file)
            if cfg:
                self.device_element.appendChild(
                        self.doc.createComment(" soundfont : %s " % sf2file))
                bankId = self.parseCfg(cfg)
                if bankId is not None:
                    if self.loadSoundFont:
                        loadCmd =  "sfxload -b%s %s" % (
                                bankId, sf2file.replace(' ', '\ ')
                                )
                        loadReturn = os.popen(loadCmd, 'r')
                        if loadReturn.readlines():                            
                            logging.error(loadCmd)
                else:
                    logging.error('No Bank In SoundFont File : %s' % sf2file)
            else:
                logging.error( 'Bad SoundFont : sf2cfg %s returns nothing. '\
                        % sf2file)
    
    ##########################################################################
    def getCfg(self, path):
        """
        
        @return: the output of the C{sf2cfg path} command
        """
        cfgCmd = "sf2cfg %s" % os.path.abspath(
                path).replace(' ', '\ ').replace('(', '\(').replace(')', '\)')
        logging.debug(cfgCmd)
        cfg = os.popen(cfgCmd , 'r')
        return cfg.readlines()
        
    ##########################################################################
    def getNextMsb(self):
        """
        
        @return: the next avalaible msb
        """
        if self._msbCount is None:
            self._msbCount = 0
        else:
            self._msbCount += 1
            self._lsbCount = None
        return self._msbCount
        
    ##########################################################################
    def getNextLsb(self):
        """
        
        @return: the next avalaible lsb
        @warning: not used at the moment
        """        
        if  self._lsbCount is None:
            self._lsbCount = 0
        else:
            self._lsbCount += 1
        return self._lsbCount    
        
    ##########################################################################
    def addBank(self, name = 'Unknow bank', msb = 0, lsb = 0, 
            percussion=False):
        """
        
        create a bank node in device subtree
        @return: the bank dom element node
        """
        bank_element = self.doc.createElement('bank')
        bank_element.setAttribute('name', name )
        bank_element.setAttribute('msb', str(msb))
        bank_element.setAttribute('lsb', str(lsb))
        bank_element.setAttribute('percussion',
                percussion and 'true' or 'false')
        self.device_element.appendChild(bank_element)
        self.bankCount += 1
        return bank_element
        
    ##########################################################################
    def addProgram(self, bank_element, progId, name):
        """
        
        create a program node in bank_element subtree
        @return: the program dom element node
        """        
        prog_element = self.doc.createElement('program')
        prog_element.setAttribute('id', str(progId))
        prog_element.setAttribute('name', name)
        bank_element.appendChild(prog_element)
        return prog_element
        
    ##########################################################################
    def addDrumset(self, bankName, progId, name):
        """
        
        create a drum program node, creating drum bank if needed
        @return: the percussion program dom element node
        """        
        if not self.drums:
            self.drums = self.addBank('%s Drums' % bankName, 
                    self.getNextMsb(), 0, True)
        return self.addProgram(self.drums, progId, name)

    ##########################################################################
    def parseCfg(self, lines):
        """
        
        parse sf2cfg output to build bank / program rosegarden xml data
        """
        i = 0
        drumsCount = 1
        instrCount = 1
        inBank = False
        currentBank = None
        lsbCount = 0
        self.drums = None
        bankNames = {}
        currentBankName = None
        bankCount = 0
        bankId = None
        while i < len(lines) :
            # skipping comment
            if lines[i].startswith('#'):
                i += 1
                continue
            # bank
            elif lines[i].startswith('bank'):
                logging.debug("new bank")
                inBank = True
                tokens = lines[i].strip().split()
                name = self.prettyName(
                        os.path.basename(
                            lines[-1].replace('sf ', '')
                            ).split('.')[0]
                        )
                msb = self.getNextMsb()
                currentBankName = name
                if bankNames.has_key(name):
                    name += ' #%02i' % bankCount
                else:
                    bankId = msb
                bankNames[name] = None
                currentBank = self.addBank(name, msb)
                bankCount += 1
            # drumset
            elif lines[i].startswith('drumset'):
                tokens = lines[i].strip().split()
                inBank = False
                self.addDrumset(currentBankName, tokens[1], 
                        len(tokens) > 3 and self.prettyName(tokens[3]) \
                                or 'DrumSet %s' % drumsCount)
                drumsCount += 1
            # program
            elif inBank and lines[i].startswith("\t"):
                tokens = lines[i].strip().split()                
                self.addProgram(currentBank, tokens[0],
                        self.prettyName(tokens[1]))
            i += 1
        return bankId
    
    ##########################################################################
    def prettyName(self, name):
        """
        
        try to well format any string (bank name, program name...)
        """
        return self.prettyNameFormat and ' '.join([ token.title() for token \
            in name.replace('_', ' ').strip().split() ]) or name.strip()
    
    ##########################################################################
    def __str__(self):
        """
        
        @returns: rosegarden xml device
        """
        if not self.doc:
            self.process()
        return self.doc and self.doc.documentElement.toprettyxml('  ') \
                or None
        
    ##########################################################################
    def mergeToRg(self, rgPath, device):
        """

        merge bank set to rosegarden project file
        @todo: implementation :o)
        """
        pass
        
    ##########################################################################
    def saveToRg(self, path):
        """

        write xml dump to a gzipped file
        """
        output = gzip.GzipFile(path, 'w')
        output.writelines(str(self))

##############################################################################
def usage():
    """

    @return: the minimalistic help for command-line usage
    """
    return """%s [OPTION]... [FILE]...\n   
-i --input FILE\t\tpath to a soundfont list file. format : one sf2 \
path per line. 
-l --load\t\tload soundfonts in wavetable memory 
-o --output FILE\tpath to rosegarden data export file (gzipped xml)
-s --stat\t\tshows synth memory usage (experimental!)
-q --quiet\t\tno stdout dump
        """ % (sys.argv[0]) 

##############################################################################
if __name__ == '__main__':
    optlist, args = getopt.getopt(sys.argv[1:], 
            "hqlo:si:" , 
            [ "help", "quiet", "load", "output=", "stat", "input=" ]
            )
    
    sf2rg = Sf2Rg(args)
    
    outputFile = None
    quiet = False
   
    for o, a in optlist:
        if o == "--help" or o == "-h":
            print usage()
            sys.exit(0)

        if o == "--load" or o == "-l":
            sf2rg.loadSoundFont = True
            
        elif o == "--output" or o == "-o":
            outputFile = a
            
        elif o == "--input" or o == "-i":
            # getting list from file if needed
            sf2rg.loadSoundFontList(a)

        elif o == "--quiet" or o == "-q":
            quiet = True

        elif o == "--stat" or o == "-s":
            wtstat = WaveTableStat()
            print str(wtstat)
            sys.exit(0)

    if not sf2rg.sf2files:
        print usage()
        sys.exit(1)

    if outputFile is not None:
        sf2rg.saveToRg(outputFile)
    elif not quiet:
        print str(sf2rg)
        
        

