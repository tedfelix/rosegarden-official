#! /usr/bin/env python

import SCons.Util, os

def exists(env):
	return true

def generate(env):

	if env['HELP']:
		p=env.pprint
		p('BOLD','*** Sound options ***')
		p('BOLD','--------------------')
		p('BOLD','* nosound    ','disable sound support')
		p('BOLD','* noalsa     ','disable ALSA')
		p('BOLD','* nojack     ','disable JACK')
		p('BOLD','* nodssi     ','disable DSSI')
		p('BOLD','ie: scons configure noalsa=1 nojack=1 nodssi=1 ')
		return

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

	if not env['HELP'] and (env['_CONFIGURE'] or not env.has_key('SNDISCONFIGURED')):
		## Configure stuff
		conf = env.Configure(custom_tests = { 'Check_pkg_config' : Check_pkg_config, 'Check_package' : Check_package })

		if env.has_key('SOUND_CCFLAGS'):   env.__delitem__('SOUND_CCFLAGS')
		if env.has_key('SOUND_LDFLAGS'):   env.__delitem__('SOUND_LDFLAGS')
		if env.has_key('SNDISCONFIGURED'): env.__delitem__('SNDISCONFIGURED')

		if not conf.Check_pkg_config('0.15'):
			print 'pkg-config >= 0.15 not found.'
			env.Exit(1)

		import sys
		if 'nosound' in env['ARGS']:
			print "-> sound support disabled by user"
			haveSound = 0
		else:
			haveSound = 1

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
#		haveLibmad  = conf.Check_package('mad', '0.10')
		haveXft     = conf.Check_package('xft', '2.1.0')

		env = conf.Finish()

		if haveAlsa:   env.Append(SOUND_CCFLAGS = '-DHAVE_ALSA')
		if haveJack:   env.Append(SOUND_CCFLAGS = '-DHAVE_LIBJACK')
		if haveDssi:   env.Append(SOUND_CCFLAGS = '-DHAVE_DSSI')
		if haveLadspa: env.Append(SOUND_CCFLAGS = '-DHAVE_LADSPA')
		if haveLiblo:  env.Append(SOUND_CCFLAGS = '-DHAVE_LIBLO')
#		if haveLibmad: env.Append(SOUND_CCFLAGS = '-DHAVE_LIBMAD')
		if haveLiblrdf:
			env.Append(SOUND_CCFLAGS = '-DHAVE_LIBLRDF')
			env.AppendUnique(SOUND_LDFLAGS = '-llrdf')
		if haveXft:
		    	env.Append(SOUND_CCFLAGS = '-DHAVE_XFT')
			env.AppendUnique(SOUND_LDFLAGS = '-lXft')
		env['SNDISCONFIGURED'] = 1
		opts.Save(cachefile, env)

		#
		# Configure warnings
		#
		if not haveXft:
			print """
* Score rendering quality and performance may be
improved if Xft 2.1.0 and Freetype 2 are available, to permit
Rosegarden to override the Qt font selection mechanism.  It
may not be worth trying to install them if they aren't already
present in your distribution though.
			"""

		if not haveSound or not haveAlsa:
			print """			
* Rosegarden requires the ALSA (Advanced Linux Sound Architecture) drivers
for MIDI, and the JACK audio framework for audio sequencing.
Please see the documentation at http://www.rosegardenmusic.com/getting/
for more information about these dependencies.
			"""
		elif haveSound and not haveAlsa:
			print """			
(Rosegarden does contain some code for audio and MIDI using the KDE
aRts multimedia service.  But it's unlikely to compile, let alone work.)
			"""
		else:
			if not haveJack:
				print """
* Rosegarden uses the JACK audio server for audio recording and
sequencing.  See http://jackit.sf.net/ for more information about
getting and installing JACK.  If you want to use Rosegarden only
for MIDI, then you do not need JACK.
				"""

			if not haveLadspa:
				print """
* Rosegarden supports LADSPA audio plugins if available.  See
http://www.ladspa.org/ for more information about LADSPA.  To
build LADSPA support into Rosegarden, you need to make sure
you have ladspa.h available on your system.
				"""

			if not haveDssi:
				print """
* Rosegarden supports DSSI audio plugins if available.  See
http://dssi.sf.net/ for more information about DSSI.  To
build DSSI support into Rosegarden, you need to make sure
you have dssi.h available on your system.
				"""

			if not haveLiblo:
				print """
* Rosegarden supports custom GUIs for DSSI (and LADSPA) plugins using
the Open Sound Control protocol, if the Lite OSC library liblo is
available.  Go to http://www.plugin.org.uk/liblo/ to obtain liblo
and http://dssi.sf.net/ for more information about DSSI GUIs.
				"""
			if not haveLiblrdf:
				print """
* Rosegarden supports the LRDF metadata format for classification
of LADSPA and DSSI plugins.  This will improve the usability of
plugin selection dialogs.  You can obtain LRDF from
http://www.plugin.org.uk/lrdf/.
				"""
	if env.has_key('SOUND_CCFLAGS'):
		env.AppendUnique(CCFLAGS = env['SOUND_CCFLAGS'] )
		env.AppendUnique(CXXFLAGS = env['SOUND_CCFLAGS'] )
	if env.has_key('SOUND_LDFLAGS'):
		env.AppendUnique(LINKFLAGS = env['SOUND_LDFLAGS'] )

