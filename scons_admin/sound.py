#! /usr/bin/env python

import SCons.Util, os

def exists(env):
	return true

def generate(env):

	if env['HELP']:
		p=env.pprint
		p('BOLD','*** Sound options ***')
		p('BOLD','--------------------')
		p('BOLD','* noalsa     ','disable ALSA')
		p('BOLD','* nojack     ','disable JACK')
		p('BOLD','* nodssi     ','disable DSSI')
		p('BOLD','ie: scons configure noalsa=1 nojack=1 nodssi=1 ')

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
		( 'SNDISCONFIGURED', 'debug level for the project : full or just anything' ),
		( 'SOUND_CCFLAGS', 'additional compilation flags' ),
		( 'SOUND_LDFLAGS', 'additional link flags' )
		)
	opts.Update(env)

	if not env['HELP'] and (env['_CONFIGURE'] or not env.has_key('SNDISCONFIGURED') or not os.path.isfile('config.h')):
		## Configure stuff
		conf = env.Configure(custom_tests = { 'Check_pkg_config' : Check_pkg_config, 'Check_package' : Check_package })

		if env.has_key('SOUND_CCFLAGS'):   env.__delitem__('SOUND_CCFLAGS')
		if env.has_key('SOUND_LDFLAGS'):   env.__delitem__('SOUND_LDFLAGS')
		if env.has_key('SNDISCONFIGURED'): env.__delitem__('SNDISCONFIGURED')

		if not conf.Check_pkg_config('0.15'):
			print 'pkg-config >= 0.15 not found.'
			env.Exit(1)

		#os.popen(">config.h")

		import sys
		if 'noalsa' in env['ARGS']:
			print "-> ALSA support disabled by user"
			haveAlsa = 0
		else:
			haveAlsa = conf.Check_package('alsa','1.0')

		if 'nojack' in env['ARGS']:
			print "-> JACK support disabled by user"
			haveJack = 0
		else:
			haveJack = conf.Check_package('jack', '0.77')

		if 'nodssi' in env['ARGS']:
		    	print "-> DSSI support disabled by user"
			haveDssi = 0
		else:
		    	haveDssi = conf.Check_package('dssi', '0.4')

		haveLadspa  = conf.CheckHeader('ladspa.h')
		haveLiblrdf = conf.CheckLibWithHeader('lrdf', ['stdio.h', 'lrdf.h'], 'C', 'lrdf_init();')
		haveLiblo   = conf.Check_package('liblo', '0.7')
		haveLibmad  = conf.Check_package('mad', '0.10')
		haveXft     = conf.Check_package('xft', '2.1.0')

		env = conf.Finish()

		if haveAlsa:   env.Append(SOUND_CCFLAGS = '-DHAVE_ALSA')
		if haveJack:   env.Append(SOUND_CCFLAGS = '-DHAVE_LIBJACK')
		if haveDssi:   env.Append(SOUND_CCFLAGS = '-DHAVE_DSSI')
		if haveLadspa: env.Append(SOUND_CCFLAGS = '-DHAVE_LADSPA')
		if haveLiblo:  env.Append(SOUND_CCFLAGS = '-DHAVE_LIBLO')
		if haveLibmad: env.Append(SOUND_CCFLAGS = '-DHAVE_LIBMAD')
		if haveLiblrdf:
			env.Append(SOUND_CCFLAGS = '-DHAVE_LIBLRDF')
			env.AppendUnique(SOUND_LDFLAGS = '-llrdf')
		if haveXft:
		    	env.Append(SOUND_CCFLAGS = '-DHAVE_XFT')
			env.AppendUnique(SOUND_LDFLAGS = '-lXft')
		env['SNDISCONFIGURED'] = 1
		opts.Save(cachefile, env)

	if env.has_key('SOUND_CCFLAGS'):
		env.AppendUnique(CCFLAGS = env['SOUND_CCFLAGS'] )
		env.AppendUnique(CXXFLAGS = env['SOUND_CCFLAGS'] )
	if env.has_key('SOUND_LDFLAGS'):
		env.AppendUnique(LINKFLAGS = env['SOUND_LDFLAGS'] )

