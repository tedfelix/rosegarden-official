#!/usr/bin/python
## scons script for building rosegarden, adapted from
## the scons script for building the kde test applications
## Thomas Nagy, 2004

## This file can be reused freely for any project (see COPYING)

import os

def Check_pkg_config(context, version):
	context.Message('Checking for pkg-config ... ')
	ret = context.TryAction('pkg-config --atleast-pkgconfig-version=%s' % version)[0]
	context.Result(ret)
	return ret

def Check_package(context, module, version):
	context.Message('Checking for %s >= %s ... ' % (module, version))
	ret = context.TryAction('pkg-config %s --atleast-version=%s' % (module, version))[0]
	if ret:
	    env.ParseConfig('pkg-config %s --cflags --libs' % module);
	context.Result(ret)
	return ret

env = Environment(tools = ['default', 'config', 'kde'], toolpath='./')
env.AppendUnique( ENV = os.environ )

# The target make dist requires the python module shutil which is in 2.3
env.EnsurePythonVersion(2,3)

# Install only if asked to
env['INSTALL_ALL'] = 0
if 'install' in COMMAND_LINE_TARGETS:
    env['INSTALL_ALL'] = 1

##
## Configure stuff    
conf = Configure(env, custom_tests = { 'Check_pkg_config' : Check_pkg_config, 
                                       'Check_package' : Check_package }) 
if not conf.Check_pkg_config('0.15'):
    print 'pkg-config >= 0.15 not found.' 
    Exit(1) 

haveAlsa    = conf.Check_package('alsa','1.0')
haveJack    = conf.Check_package('jack', '0.77')
haveLadspa  = conf.CheckHeader('ladspa.h')
haveLiblrdf = conf.CheckLibWithHeader('lrdf', ['stdio.h', 'lrdf.h'], 'C', 'lrdf_init();')
haveLiblo   = conf.Check_package('liblo', '0.7')
haveLibmad  = conf.Check_package('mad', '0.10')
haveLibdssi = conf.Check_package('dssi', '0.4')
haveXft     = conf.Check_package('xft', '2.1.0')

env = conf.Finish()

env.Append(CCFLAGS = '-DQT_THREAD_SUPPORT')
if haveAlsa:
    env.Append(CCFLAGS = '-DHAVE_ALSA')
if haveJack:
    env.Append(CCFLAGS = '-DHAVE_JACK')
if haveLadspa:
    env.Append(CCFLAGS = '-DHAVE_LADSPA')
if haveLiblo:
    env.Append(CCFLAGS = '-DHAVE_LIBLO')
if haveLibmad:
    env.Append(CCFLAGS = '-DHAVE_LIBMAD')
if haveLiblrdf:
    env.Append(CCFLAGS = '-DHAVE_LRDF')

# Export 'env' so that sconscripts in subdirectories can use it
Export( "env" )

# Cache directory
env.CacheDir('cache')

# Avoid spreading .sconsign files everywhere
env.SConsignFile('scons_signatures')

## The qt library is needed by every sub-program
env.AppendUnique(LIBS = ['qt-mt'])

## The list of install targets is populated 
## when the SConscripts are read
env['INST_TARGETS'] = []

env.SConscript("base/SConscript")
soundLibs = env.SConscript("sound/SConscript")
env.SConscript("sequencer/SConscript", 'soundLibs')
env.SConscript("gui/SConscript", 'soundLibs')
env.SConscript("doc/en/SConscript")
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

