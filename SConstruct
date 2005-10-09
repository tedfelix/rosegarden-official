#!/usr/bin/python
## scons script for building rosegarden, adapted from
## the scons script for building the kde test applications
## by Thomas Nagy, 2004 (bksys)
##
##        Guillaume Laurent   <glaurent@telegraph-road.org>,
##        Chris Cannam        <cannam@all-day-breakfast.com>,
##        Richard Bown        <bownie@bownie.com>
##        Stephen Torri       <storri@torri.org>

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
VERSION = "4-1.2_cvs"

import os
import glob
import fnmatch
import re
import string
import sys

env = Environment(TARGS=COMMAND_LINE_TARGETS, ARGS=ARGUMENTS, 
		tools = ['default', 'generic', 'kde', 'sound', 'test', 'rosegarden'], 
		toolpath=['./', 'scons_admin/'])

#-----------------------------
#        Variables
#-----------------------------

# Setup the default build directory to c_reldir
c_builddir = 'RGbuild'

## Exit if configuration requested (scons configure)
if 'configure' in COMMAND_LINE_TARGETS:
	env.Exit(0)
#-----------------------------
#        Environment
#-----------------------------
env.AppendUnique( ENV = os.environ )
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

##!!! WTF? This is supposed to be defined if NDEBUG is set, which is supposed
## to happen if debug is not actively requested
##env.Append(CCFLAGS = '-DNO_TIMING')

#-----------------------------
#        Build
#-----------------------------
def givedir(dir):
	return env.join(c_builddir, dir)

SetOption('duplicate', 'soft-copy')
dirs = env.Split("""
	sound
	sequencer
	gui
	base
""")

bdirs=[]
for d in dirs:
	tmpdir = givedir(d)
	env.BuildDir(tmpdir, d)
	bdirs.append(tmpdir)
env.subdirs(bdirs)

# disable object cache, unless you want it (define the SCONS_CACHE env. var)
if not os.environ.has_key('SCONS_CACHE'):
	env.CacheDir(None)

#-----------------------------
# Process the translations
#-----------------------------

## They are usually located in the po/ directory
env.KDElang('po/', 'rosegarden')


#-----------------------------
#        Installation
#-----------------------------

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
	env.KDEinstall('KDEDATA','/rosegarden/pixmaps/toolbar', glob.glob("gui/pixmaps/toolbar/*.png"))
	
	#for s in glob.glob("gui/pixmaps/transport/*.xpm"):
	env.KDEinstall('KDEDATA','/rosegarden/pixmaps/transport', glob.glob("gui/pixmaps/transport/*.xpm"))
	env.KDEinstall('KDEDATA','/rosegarden/pixmaps/transport', glob.glob("gui/pixmaps/transport/*.png"))

	env.KDEinstall('KDEDATA','/rosegarden/pixmaps', "gui/pixmaps/splash.png")


	# examples
	examples="""
	gui/testfiles/glazunov.rg
	gui/testfiles/notation-for-string-orchestra-in-D-minor.rg
	gui/testfiles/ravel-pc-gmaj-adagio.rg
	gui/testfiles/perfect-moment.rg
	gui/testfiles/bwv-1060-brass-band-1-allegro.rg
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
	env.KDEinstall('KDEBIN', '', "gui/rosegarden-project-package" )

	# rosegarden-lilypondview script
	env.KDEinstall('KDEBIN', '', "gui/rosegarden-lilypondview" )

	# version.txt file
	versionFile = open("version.txt", "w")
	versionFile.write(VERSION + '\n')
	versionFile.close()
	env.KDEinstall('KDEDATA', '/rosegarden', "version.txt")

env.dist('rosegarden', VERSION)

