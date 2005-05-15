#! /usr/bin/env python

BOLD   ="\033[1m"
RED    ="\033[91m"
GREEN  ="\033[92m"
YELLOW ="\033[93m"
CYAN   ="\033[96m"
NORMAL ="\033[0m"

def exists(env):
	return true

def generate(env):
	import SCons.Util, os

	if env['help']:
		print """
"""+BOLD+"""*** Sound options ***
-----------------------"""+NORMAL+"""
"""+BOLD+"""* noalsa  """+NORMAL+""": disable alsa
"""+BOLD+"""* nojack """+NORMAL+""": disable jack
ie: """+BOLD+"""scons configure noalsa=1 nojack=1
"""+NORMAL

	def Check_pkg_config(context, version):
		context.Message('Checking for pkg-config ... ')
		pkg_config_command = 'pkg-config'
		if os.environ.has_key("PKG_CONFIG_PATH"):
			pkg_config_command = "PKG_CONFIG_PATH="+os.environ["PKG_CONFIG_PATH"]+" pkg-config "
		ret = context.TryAction(pkg_config_command+' --atleast-pkgconfig-version=%s' % version)[0]
		context.Result(ret)
		return ret

	def Check_package(context, module, version):
		context.Message('Checking for %s >= %s ... ' % (module, version))
		pkg_config_command = 'pkg-config'
		if os.environ.has_key("PKG_CONFIG_PATH"):
			pkg_config_command = "PKG_CONFIG_PATH="+os.environ["PKG_CONFIG_PATH"]+" pkg-config "
		ret = context.TryAction(pkg_config_command+' %s --atleast-version=%s' % (module, version))[0]
		if ret:
			env.ParseConfig(pkg_config_command+' %s --cflags --libs' % module);
			env.AppendUnique( SOUND_CCFLAGS = 
				SCons.Util.CLVar( 
					os.popen(pkg_config_command+" %s --cflags 2>/dev/null" % module).read().strip() ));
			env.AppendUnique( SOUND_LDFLAGS = 
				SCons.Util.CLVar( 
					os.popen(pkg_config_command+" %s --libs 2>/dev/null" % module).read().strip() ));
		context.Result(ret)
		return ret

	# load the options
	from SCons.Options import Options, PathOption
	cachefile = env['CACHEDIR']+'/sound.cache.py'
	opts = Options(cachefile)
	opts.AddOptions(
		( 'ISCONFIGURED', 'debug level for the project : full or just anything' ),
		( 'SOUND_CCFLAGS', 'additional compilation flags' ),
		( 'SOUND_LDFLAGS', 'additional link flags' )
		)
	opts.Update(env)

	if 'configure' in env['TARGS'] or not env.has_key('ISCONFIGURED') or not os.path.isfile('config.h'):
		## Configure stuff    
		conf = env.Configure(custom_tests = { 'Check_pkg_config' : Check_pkg_config, 'Check_package' : Check_package }) 

		if env.has_key('SOUND_CCFLAGS'):
			env.__delitem__('SOUND_CCFLAGS')
		if env.has_key('SOUND_LDFLAGS'):
			env.__delitem__('SOUND_LDFLAGS')

		if not conf.Check_pkg_config('0.15'):
			print 'pkg-config >= 0.15 not found.' 
			env.Exit(1) 

		#os.popen(">config.h")

		import sys
		if 'noalsa' in env['TARGS']:
			print "-> Alsa module disabled by user"
			haveAlsa = 0
		else:
			haveAlsa = conf.Check_package('alsa','1.0')

		if 'nojack' in env['TARGS']:
			print "-> Jack module disabled by user"
			haveJack = 0
		else:
			haveJack = conf.Check_package('jack', '0.77')

		haveLadspa  = conf.CheckHeader('ladspa.h')
		haveLiblrdf = conf.CheckLibWithHeader('lrdf', ['stdio.h', 'lrdf.h'], 'C', 'lrdf_init();')
		haveLiblo   = conf.Check_package('liblo', '0.7')
		haveLibmad  = conf.Check_package('mad', '0.10')
		haveLibdssi = conf.Check_package('dssi', '0.4')
		haveXft     = conf.Check_package('xft', '2.1.0')

		env = conf.Finish()

		if haveAlsa:
			env.Append(SOUND_CCFLAGS = '-DHAVE_ALSA')
			#os.popen('echo "#define HAVE_ALSA">>config.h')
		if haveJack:
			env.Append(SOUND_CCFLAGS = '-DHAVE_LIBJACK')
			#os.popen('echo "#define HAVE_LIBJACK">>config.h')
		if haveLadspa:
			env.Append(SOUND_CCFLAGS = '-DHAVE_LADSPA')
			#os.popen('echo "#define HAVE_LADSPA">>config.h')
		if haveLiblo:
			env.Append(SOUND_CCFLAGS = '-DHAVE_LIBLO')
			#os.popen('echo "#define HAVE_LIBLO">>config.h')
		if haveLibmad:
			env.Append(SOUND_CCFLAGS = '-DHAVE_LIBMAD')
			#os.popen('echo "#define HAVE_LIBMAD">>config.h')
		if haveLiblrdf:
			env.Append(SOUND_CCFLAGS = '-DHAVE_LIBLRDF')
			env.AppendUnique(SOUND_LDFLAGS = '-llrdf')
			#os.popen('echo "#define HAVE_LIBLRDF">>config.h')

		env['ISCONFIGURED'] = 1
		opts.Save(cachefile, env)

	if env.has_key('SOUND_CCFLAGS'):
		env.AppendUnique(CCFLAGS = env['SOUND_CCFLAGS'] )
		env.AppendUnique(CXXFLAGS = env['SOUND_CCFLAGS'] )
	if env.has_key('SOUND_LDFLAGS'):
		env.AppendUnique(LINKFLAGS = env['SOUND_LDFLAGS'] )

