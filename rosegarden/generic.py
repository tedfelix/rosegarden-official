## Thomas Nagy, 2005

BOLD   ="\033[1m"
RED    ="\033[91m"
GREEN  ="\033[92m"
YELLOW ="\033[93m"
CYAN   ="\033[96m"
NORMAL ="\033[0m"

def exists(env):
	return true

def generate(env):
	"""
	Detect and store the most common options
	* debug  : anything (-g) or 'full' (-g3, slower)
	* prefix : the installation path
	* extraincludes : a list of paths separated by ':'

	ie: scons configure debug=full prefix=/usr/local extraincludes=/tmp/include:/usr/local
	"""

	# load the options
	from SCons.Options import Options, PathOption
	opts = Options('generic.cache.py')
	opts.AddOptions(
		( 'DEBUGLEVEL', 'debug level for the project : full or just anything' ),
		( 'PREFIX', 'prefix for installation' ),
		( 'EXTRAINCLUDES', 'extra include paths for the project' ),
	)
	opts.Update(env)
	
	# use this to avoid an error message 'how to make target configure ?'
	env.Alias('configure', None)

	# configure the environment if needed
	if 'configure' in env['TARGS']:
		# need debugging ?
		if env['ARGS'].get('debug', None):
			debuglevel = env['ARGS'].get('debug', None)
			print CYAN+'** Enabling debug for the project **' + NORMAL
			if (debuglevel == "full"):
				env['DEBUGLEVEL'] = '-DDEBUG:-g3'
			else:
				env['DEBUGLEVEL'] = '-DDEBUG:-g'
		elif env.has_key('DEBUGLEVEL'):
			env.__delitem__('DEBUGLEVEL')

		# user-specified prefix
		if env['ARGS'].get('prefix', None):
			env['PREFIX'] = env['ARGS'].get('prefix', None)
			print CYAN+'** set the installation prefix for the project : ' + env['PREFIX'] +' **'+ NORMAL
		elif env.has_key('PREFIX'):
			env.__delitem__('PREFIX')

		# user-specified include paths
		env['EXTRAINCLUDES'] = env['ARGS'].get('extraincludes', None)
		if env['ARGS'].get('extraincludes', None):
			print CYAN+'** set extra include paths for the project : ' + env['EXTRAINCLUDES'] +' **'+ NORMAL
		elif env.has_key('EXTRAINCLUDES'):
			env.__delitem__('EXTRAINCLUDES')

		# and finally save the options in a cache
		opts.Save('generic.cache.py', env)

	if env.has_key('DEBUGLEVEL'):
		env.AppendUnique(CPPFLAGS = [str(env['DEBUGLEVEL']).split(':')])

	if env.has_key('EXTRAINCLUDES'):
		incpaths = []
		for dir in str(env['EXTRAINCLUDES']).split(':'):
			incpaths.append( dir )
		env.Append(CPPPATH = incpaths)

