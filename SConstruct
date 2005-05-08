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

To clean the project
-> scons -c

To install the project
-> scons install

To uninstall the project
-> scons -c install

To compile while being in a subdirectory
-> cd src; scons -u

To (re)configure the project and give particular arguments, use ie :
-> scons configure debug=1
-> scons configure prefix=/tmp/ita debug=full extraincludes=/usr/local/include:/tmp/include prefix=/usr/local
The variables are saved automatically after the first run
(look at cache/kde.cache.py, ..)

You can alternate between debug/non-debug mode very easily :

scons configure debug=1; scons; scons configure ;
"""

# Define the version
VERSION = "4-1.1_cvs"

import os
import glob

env = Environment(TARGS=COMMAND_LINE_TARGETS, ARGS=ARGUMENTS, 
		tools = ['default', 'generic', 'kde', 'sound'], 
		toolpath=['./', 'scons_admin/'])
## Exit if configuration requested (scons configure)
if 'configure' in COMMAND_LINE_TARGETS:
	env.Exit(0)
#env.AppendUnique( ENV = os.environ )
env.AppendUnique( ENV = {'PATH' : os.environ['PATH'], 'HOME' : os.environ['HOME']} )

## Use this to set rpath - this may cause trouble if folders are moved (chrpath)
env.Append( RPATH = [env['QTLIBPATH'], env['KDELIBPATH'], env['KDEMODULE']] )

# Export 'env' so that sconscripts in subdirectories can use it
Export( "env" )

## The qt library is needed by every sub-program
env.AppendUnique(LIBS = ['qt-mt'])

## Threading is needed
env.Append(CCFLAGS = '-DQT_THREAD_SUPPORT')

env.Append(CCFLAGS = '-DVERSION=\\"' + VERSION + '\\"')

env.SConscript("base/SConscript")
soundLibs = env.SConscript("sound/SConscript")
env.SConscript("sequencer/SConscript", 'soundLibs')
env.SConscript("gui/SConscript", 'soundLibs')
#env.SConscript("gui/docs/en/SConscript")
## We are now using one top-level file for the documentation instead of several almost empty scripts
env.SConscript("gui/doc/SConscript")

#env.SConscript("po/SConscript")
env.KDElang('po/', 'rosegarden') # one script to remove

# disable object cache
env.CacheDir(None)

#
## Installation
##

if 'install' in COMMAND_LINE_TARGETS:

	# .rc files
	env.KDEinstall('KDEDATA','/rosegarden', glob.glob("gui/*.rc"))

	# .desktop file
	env.KDEinstall('KDEMENU','/Applications', 'gui/rosegarden.desktop')

	# mime files
	mimefiles = """
	gui/x-rosegarden21.desktop
	gui/x-rosegarden.desktop
	gui/x-rosegarden-device.desktop
	gui/x-soundfont.desktop""".split()

	env.KDEinstall('KDEMIME','/audio', mimefiles)

	# icons
	env.KDEinstallas('KDEICONS','/locolor/16x16/apps/x-rosegarden.xpm', "gui/pixmaps/icons/cc-hi16-rosegarden.xpm")
	env.KDEinstallas('KDEICONS','/hicolor/16x16/apps/x-rosegarden.xpm', "gui/pixmaps/icons/rg-rwb-rose3-16x16.png")

	env.KDEinstallas('KDEICONS','/hicolor/48x48/apps/rosegarden.png', "gui/pixmaps/icons/rg-rwb-rose3-48x48.png")
	env.KDEinstallas('KDEICONS','/hicolor/64x64/apps/rosegarden.png', "gui/pixmaps/icons/rg-rwb-rose3-64x64.png")

	env.KDEinstallas('KDEICONS','/hicolor/128x128/apps/rosegarden.png', "gui/pixmaps/icons/rg-rwb-rose3-128x128.png")

	env.KDEinstallas('KDEICONS','/locolor/32x32/apps/rosegarden.xpm', "gui/pixmaps/icons/cc-hi32-rosegarden.xpm")
	env.KDEinstallas('KDEICONS','/hicolor/32x32/apps/rosegarden.png', "gui/pixmaps/icons/rg-rwb-rose3-32x32.png")

	env.KDEinstallas('KDEICONS','/hicolor/16x16/mimetypes/x-rosegarden.png', "gui/pixmaps/icons/mm-mime-hi16-rosegarden.png")
	env.KDEinstallas('KDEICONS','/locolor/16x16/mimetypes/x-rosegarden.png', "gui/pixmaps/icons/mm-mime-hi16-rosegarden.png")

	env.KDEinstallas('KDEICONS','/hicolor/32x32/mimetypes/x-rosegarden.png', "gui/pixmaps/icons/mm-mime-hi32-rosegarden.png")
	env.KDEinstallas('KDEICONS','/locolor/32x32/mimetypes/x-rosegarden.png', "gui/pixmaps/icons/mm-mime-hi32-rosegarden.png")

	# styles
	#for s in glob.glob("gui/styles/*.xml"):
	env.KDEinstall('KDEDATA','/rosegarden/styles', glob.glob("gui/styles/*.xml"))

	# fonts
	#for s in glob.glob("gui/fonts/*.pfa"):
	env.KDEinstall('KDEDATA','/rosegarden/fonts', glob.glob("gui/fonts/*.pfa"))

	#for s in glob.glob("gui/fonts/mappings/*.xml"):
	env.KDEinstall('KDEDATA','/rosegarden/fonts/mappings',glob.glob("gui/fonts/mappings/*.xml"))

	#for s in glob.glob("gui/pixmaps/rg21/4/*.xpm"):
	env.KDEinstall('KDEDATA','/rosegarden/fonts/rg21/4', glob.glob("gui/pixmaps/rg21/4/*.xpm"))
	#for s in glob.glob("gui/pixmaps/rg21/8/*.xpm"):
	env.KDEinstall('KDEDATA','/rosegarden/fonts/rg21/8', glob.glob("gui/pixmaps/rg21/8/*.xpm"))

	#for s in glob.glob("gui/pixmaps/feta/4/*.xpm"):
	env.KDEinstall('KDEDATA','/rosegarden/fonts/feta/4', glob.glob("gui/pixmaps/feta/4/*.xpm"))
	#for s in glob.glob("gui/pixmaps/feta/6/*.xpm"):
	env.KDEinstall('KDEDATA','/rosegarden/fonts/feta/6', glob.glob("gui/pixmaps/feta/6/*.xpm"))
	#for s in glob.glob("gui/pixmaps/feta/8/*.xpm"):
	env.KDEinstall('KDEDATA','/rosegarden/fonts/feta/8', glob.glob("gui/pixmaps/feta/8/*.xpm"))
	#for s in glob.glob("gui/pixmaps/feta/10/*.xpm"):
	env.KDEinstall('KDEDATA','/rosegarden/fonts/feta/10', glob.glob("gui/pixmaps/feta/10/*.xpm"))
	#for s in glob.glob("gui/pixmaps/feta/12/*.xpm"):
	env.KDEinstall('KDEDATA','/rosegarden/fonts/feta/12', glob.glob("gui/pixmaps/feta/12/*.xpm"))

	# pixmaps
	#for s in glob.glob("gui/pixmaps/misc/*.xpm"):
	env.KDEinstall('KDEDATA','/rosegarden/pixmaps/misc', glob.glob("gui/pixmaps/misc/*.xpm"))

	#for s in glob.glob("gui/pixmaps/toolbar/*.xpm"):
	env.KDEinstall('KDEDATA','/rosegarden/pixmaps/toolbar', glob.glob("gui/pixmaps/toolbar/*.xpm"))

	#for s in glob.glob("gui/pixmaps/transport/*.xpm"):
	env.KDEinstall('KDEDATA','/rosegarden/pixmaps/transport', glob.glob("gui/pixmaps/transport/*.xpm"))

	env.KDEinstall('KDEDATA','/rosegarden/pixmaps', "gui/pixmaps/splash.png")


	# examples
	examples="""
	gui/testfiles/glazunov.rg
	gui/testfiles/notation-for-string-orchestra-in-D-minor.rg
	gui/testfiles/navel-pc-gmaj-adagio.rg
	gui/testfiles/perfect-moment.rg
	gui/testfiles/bogus-surf-jam.rg
	gui/testfiles/the-rose-garden.rg
	gui/testfiles/children.rg
	gui/testfiles/stormy-riders.rg
	gui/testfiles/Djer-Fire.rg""".split()

	#for ex in examples:
	env.KDEinstall('KDEDATA','/rosegarden/examples', examples)

	# autoload.rg
	env.KDEinstall('KDEDATA', '/rosegarden', "gui/testfiles/autoload.rg")

	# tips
	env.KDEinstall('KDEDATA', '/rosegarden', "gui/docs/en/tips")

	# library files
	#for l in glob.glob("gui/library/*.rgd"):
	env.KDEinstall('KDEDATA', '/rosegarden/library', glob.glob("gui/library/*.rgd"))

	# rosegarden-project-package script
	env.KDEinstall('KDEBIN', "gui/rosegarden-project-package" )


	# version.txt file
	versionFile = open("version.txt", "w")
	versionFile.write(VERSION + '\n')
	versionFile.close()
	env.KDEinstall('KDEDATA', '/rosegarden', "version.txt")

### Emulate "make distclean"
if 'distclean' in COMMAND_LINE_TARGETS:
	
	## The target scons distclean requires the python module shutil which is in 2.3
	env.EnsurePythonVersion(2, 3)

	## Remove the cache directory
	import shutil
	if os.path.isdir(env['CACHEDIR']):
		shutil.rmtree(env['CACHEDIR'])

	env.Default(None)
	env.Exit(0)

### To make a tarball of your masterpiece, use 'scons dist'
if 'dist' in COMMAND_LINE_TARGETS:

	## The target scons dist requires the python module shutil which is in 2.3
	env.EnsurePythonVersion(2, 3)

	APPNAME = 'rosegarden'
	VERSION = os.popen("cat VERSION").read().rstrip()
	FOLDER  = APPNAME+'-'+VERSION
	ARCHIVE = FOLDER+'.tar.bz2'

	## If your app name and version number are defined in 'version.h', use this instead:
	## (contributed by Dennis Schridde devurandom@gmx@net)
	#import re
	#INFO = dict( re.findall( '(?m)^#define\s+(\w+)\s+(.*)(?<=\S)', open(r"version.h","rb").read() ) )
	#APPNAME = INFO['APPNAME']
	#VERSION = INFO['VERSION']

	import shutil
	import glob

	## check if the temporary directory already exists
	if os.path.isdir(FOLDER):
		shutil.rmtree(FOLDER)
	if os.path.isfile(ARCHIVE):
		os.remove(ARCHIVE)

	## create a temporary directory
	startdir = os.getcwd()
	shutil.copytree(startdir, FOLDER)

	## remove our object files first
	os.popen("find "+FOLDER+" -name \"*cache*\" | xargs rm -rf")
	os.popen("find "+FOLDER+" -name \"*.pyc\" | xargs rm -f")
	#os.popen("pushd %s && scons -c " % FOLDER) # TODO

	## CVS cleanup
	os.popen("find "+FOLDER+" -name \"CVS\" | xargs rm -rf")
	os.popen("find "+FOLDER+" -name \".cvsignore\" | xargs rm -rf")

	## Subversion cleanup
	os.popen("find %s -name .svn -type d | xargs rm -rf" % FOLDER)

	## Create the tarball (coloured output)
	print "\033[92m"+"Writing archive "+ARCHIVE+"\033[0m"
	os.popen("tar cjf "+ARCHIVE+" "+FOLDER)

	## Remove the temporary directory
	if os.path.isdir(FOLDER):
		shutil.rmtree(FOLDER)

	env.Default(None)
	env.Exit(0)
