#! /usr/bin/env python

#
# Rosegarden-specific options
# (for now, only lirc)
#

def exists(env):
	return true

def generate(env):
	if env['HELP']:
		p=env.pprint
		p('BOLD','*** other options ***')
		p('BOLD','--------------------')
		p('BOLD','* nolirc     ','disable LIRC support')
		p('BOLD','ie: scons configure nolirc=1')
		return

	from SCons.Options import Options
	cachefile=env['CACHEDIR']+'misc.cache.py'
	opts = Options(cachefile)
	opts.AddOptions(( 'OPTLIB_CCFLAGS', 'optional libraries compilation flags' ),
                        ( 'OPTLIB_LDFLAGS', 'optional libraries link flags' ),
                        ( 'LIRC_SOURCES',   'compile in LIRC sources' ),
                        )
        opts.Update(env)
	if not env['HELP'] and (env['_CONFIGURE'] or not env.has_key('LIRC_SOURCES')):
            # Configure stuff for optional libraries
            conf = env.Configure()

            if env.has_key('OPTLIB_CCFLAGS'):
                env.__delitem__('OPTLIB_CCFLAGS')
            if env.has_key('OPTLIB_LDFLAGS'):
                env.__delitem__('OPTLIB_LDFLAGS')

            if 'nolirc' in env['ARGS']:
                print "-> LIRC support disabled by user"
                haveLirc = 0
            else:
                haveLirc = conf.CheckLibWithHeader('lirc_client', 'lirc/lirc_client.h', 'C', 'int lirc_init();')

                env = conf.Finish()

            if haveLirc:
                env.Append(LIRC_SOURCES = 1)
                env.Append(OPTLIB_CCFLAGS = '-DHAVE_LIRC')
                env.AppendUnique(OPTLIB_LDFLAGS = '-llirc_client')

        # optional libraries
        if env.has_key('OPTLIB_CCFLAGS'):
            env.AppendUnique(CCFLAGS = env['OPTLIB_CCFLAGS'] )
            env.AppendUnique(CXXFLAGS = env['OPTLIB_CCFLAGS'] )
        if env.has_key('OPTLIB_LDFLAGS'):
            env.AppendUnique(LINKFLAGS = env['OPTLIB_LDFLAGS'] )






