# Made from scons qt.py and (heavily) modified into kde.py
# Thomas Nagy, 2004, 2005 <tnagy2^8@yahoo.fr>

"""
Run scons -h to display the associated help, or look below ..
"""
from SCons.Script.SConscript import SConsEnvironment

def exists(env):
	return True;

def generate(env):
	def UnitTest(env, testprog, **kwargs):
		if type(testprog)==type([]):
			testprog = testprog[0]
		outfile = str(testprog) + '.test_output'
    		return env.Command(outfile, testprog, '$SOURCE $TESTARGS > $TARGET', **kwargs)

	SConsEnvironment.UnitTest = UnitTest
