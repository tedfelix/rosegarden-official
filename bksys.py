#
# Copyright 2005 Julien Antille
#

## This file can be reused freely for any project (see COPYING)

VERSION='1.1.0'

BOLD="\033[1m"
RED="\033[91m"
GREEN="\033[92m"
YELLOW="\033[93m"
CYAN="\033[96m"
NORMAL="\033[0m"

class bksysModule:

    def __init__(self):
        if self.__class__ is bksysModule:
            raise NotImplementedError #pure virtual class, please subclass
        self.name = None
        self.version = None
        self.author = None
        

    #this function returns a list of python lines to be written in the configuration file
    #We do _not_ create the file, we only return the content (list of lines)
    #
    #the returned program is converted to a SCons tools by bksys (that is, bksys adds a 
    #generate(env) and exists() function).
    def configure(self, args):
        if self.__class__ is bksysModule:
            raise NotImplementedError #pure virtual class, please subclass

#            
#a configuration option
#
#if "name" is specified, the option is handled in --option=value form.
#if name is None, bksys will only display the description (descr)
class bksysOption:
    
    def __init__(self, name = None , descr = None , display = True):
        self.name = name
        self.descr = descr
        self.display = display
        
    def __str__(self):
        if self.name:
            return "--" + self.name + ' ' * (14 - len(self.name)) + ': ' + self.descr
        else:
            return self.descr
