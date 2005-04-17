#!/usr/bin/python
## scons script for building rosegarden, adapted from
## the scons script for building the kde test applications
## by Thomas Nagy, 2004 (bksys)
##
##        Guillaume Laurent   <glaurent@telegraph-road.org>,
##        Chris Cannam        <cannam@all-day-breakfast.com>,
##        Richard Bown        <bownie@bownie.com>


"""
Extract from bksys doc :

To compile the project, you will then only need to launch
scons on the top-level directory, the scripts find and
cache the proper environment automatically :
-> scons
(or python scons.py)

To clean the project
-> scons -c
(or python scons.py -c)

To install the project
-> scons install
(or python scons.py scons install)

To uninstall the project
-> scons -c install

To compile while being in a subdirectory
-> cd src; scons -u

To (re)configure the project and give particular arguments, use ie :
-> scons configure debug=1
-> scons configure prefix=/tmp/ita debug=full extraincludes=/usr/local/include:/tmp/include prefix=/usr/local
or -> python scons.py configure debug=1
etc ..
The variables are saved automatically after the first run
(look at kde.cache.py, ..)

You can alternate between debug/non-debug mode very easily :

scons configure debug=1; scons; scons configure ;

"""

import os

env = Environment(TARGS=COMMAND_LINE_TARGETS, ARGS=ARGUMENTS, tools = ['default', 'generic', 'kde', 'sound'], toolpath='./')
#env.AppendUnique( ENV = os.environ )
env.AppendUnique( ENV = {'PATH' : os.environ['PATH'], 'HOME' : os.environ['HOME']} )

VERSION = "4-1.1_cvs"

# The target make dist requires the python module shutil which is in 2.3
env.EnsurePythonVersion(2,3)

# Install only if asked to
env['INSTALL_ALL'] = 0
if 'install' in COMMAND_LINE_TARGETS:
    env['INSTALL_ALL'] = 1

# Export 'env' so that sconscripts in subdirectories can use it
Export( "env" )

# Cache directory
env.CacheDir('cache')

# Avoid spreading .sconsign files everywhere
env.SConsignFile('scons_signatures')

## The qt library is needed by every sub-program
env.AppendUnique(LIBS = ['qt-mt'])

## Threading is needed
env.Append(CCFLAGS = '-DQT_THREAD_SUPPORT')

env.Append(CCFLAGS = '-DVERSION=\\"' + VERSION + '\\"')

## The list of install targets is populated 
## when the SConscripts are read
env['INST_TARGETS'] = []

env.SConscript("base/SConscript")
soundLibs = env.SConscript("sound/SConscript")
env.SConscript("sequencer/SConscript", 'soundLibs')
env.SConscript("gui/SConscript", 'soundLibs')
#env.SConscript("docs/en/SConscript")
env.SConscript("po/SConscript")

env.Alias('install', env['INST_TARGETS'])


## Debugging help
## the following variables control the files that are moc'ed or dcop'ed,
## the values are currently set like this
#    env['QT_AUTOSCAN']     = 1
#    env['QT_DEBUG']        = 0
#    env['DCOP_AUTOSCAN']   = 1
#    env['DCOP_DEBUG']      = 0


## Distribution
## 'scons dist' creates a tarball named bksys-version.tar.bz2 
## containing the source code

if 'dist' in COMMAND_LINE_TARGETS:

    APPNAME = 'rosegarden'
    VERSION = os.popen("cat VERSION").read().rstrip()
    FOLDER  = APPNAME+'-'+VERSION
    ARCHIVE = FOLDER+'.tar.bz2'

    def tarball(env, target, source):
        """
        Function called to make a tarball of the sources
        Make sure to have python 2.3 for the shutil module
        """    
        import shutil
        import glob
    
        ## check if the temporary directory already exists
        if os.path.isdir(FOLDER):
	   shutil.rmtree(FOLDER)
    
        ## create a temporary directory
        startdir = os.getcwd()
        shutil.copytree(startdir, FOLDER)
       
        ## remove the useless files
        os.popen("find "+FOLDER+" -name \"{arch}\" | xargs rm -rf")
        os.popen("find "+FOLDER+" -name \".arch-ids\" | xargs rm -rf")
        os.popen("find "+FOLDER+" -name \".arch-inventory\" | xargs rm -f")
        os.popen("find "+FOLDER+" -name \".sconsign\" | xargs rm -f")
	os.popen("find "+FOLDER+" -name \"CVS\" | xargs rm -rf")
        os.popen("rm -f "+FOLDER+"/config.py*")

        ## make the tarball
        os.popen("tar cjf "+ARCHIVE+" "+FOLDER)
    
        ## remove the temporary directory
        if os.path.isdir(FOLDER):
	    shutil.rmtree(FOLDER)

    ## Make an alias so that 'scons dist' builds the archive
    env.Command(ARCHIVE, 'VERSION', tarball)
    env.Alias('dist', ARCHIVE )
    

    ## Tell scons to rebuild the archive everytime
    env.AlwaysBuild(ARCHIVE)

