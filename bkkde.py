import sys
import os
import bksys
import re
import string

class bkkdeModule(bksys.bksysModule):
    def __init__(self):
        self.name = "bksys Qt/KDE detection module"
        self.author = "Julien Antille"
        self.version  = "0.1"
        self.options = []
        self.options.append(bksys.bksysOption('prefix', 'Set the installation prefix'))  
        self.options.append(bksys.bksysOption('exec_prefix', 'Executable data path'))
        self.options.append(bksys.bksysOption('datadir', 'Application data path'))
        self.options.append(bksys.bksysOption('libdir', 'Plugin path'))
        self.options.append(bksys.bksysOption('kdeincludes', 'Path to the kde headers'))
        self.options.append(bksys.bksysOption('kdelibs', 'Path to the kde libs'))
        self.options.append(bksys.bksysOption('qtincludes', 'Path to the Qt headers'))
        self.options.append(bksys.bksysOption('qtlibs', 'Path to the Qt libs'))
        self.options.append(bksys.bksysOption('extraincludes', "additional include paths separated by a ':'"))
        self.options.append(bksys.bksysOption('enable-debug', 'Enable debug output'))
        #additional description
        self.options.append(bksys.bksysOption(descr = '\nQt and KDE may not be installed as expected (in QTDIR and KDEDIR)\n\
Until kde-config is able to give that information, you might\n\
have to give those paths.\n\
Here is an example :\n\
   ./configure --qtincludes=/usr/include/qt/ --kdeinclude=/usr/include/kde/\n\
To install in some particular location\n\
   ./configure --prefix=~/tmp'))
        
    def configure(self, args):
        config_py_content = []
                    
        #add the data
        #config_py_content.append("def set_kde_env(env):")
        
        prefix=None
        execprefix=None
        datadir=None
        libdir=None
        kdeincludes=None
        kdelibs=None
        qtincludes=None
        qtlibs=None
        extraincludes=None
        enable_debug=0
        
        if args.has_key('prefix'):
            prefix = args['prefix']
        if args.has_key('exec_prefix'):
            exec_prefix = args['exec_prefix']
        if args.has_key('datadir'):
            datadir = args['datadir']
        if args.has_key('libdir'):
            libdir = args['libdir']        
        if args.has_key('kdeincludes'):
            kdeincludes = args['kdeincludes']
        if args.has_key('kdelibs'):
            kdelibs = args['kdelibs']  
        if args.has_key('qtincludes'):
            qtincludes = args['qtincludes']
        if args.has_key('qtlibs'):
            qtlibs = args['qtlibs']        
        if args.has_key('extraincludes'):
            extraincludes = args['extraincludes']
        if args.has_key('enable-debug'):
            enable_debug = 1
        
        #################################################
        ## check for the operating system
        print "Checking for the operating system : ",
        operating_system = os.popen("uname 2>/dev/null").read().strip()
        
        if len(operating_system):
            if operating_system.find("Linux") > -1 :
                print bksys.GREEN + "You are using Linux, that's good (tm)" + bksys.NORMAL
            else:
                print bksys.GREEN + "Not using Linux (yet) ? - make sure you are using 'gmake'" + bksys.NORMAL
        else:
            print bksys.RED + "Your system is broken, the test programs cannot be built" + bksys.NORMAL
	    sys.exit(1)
        
                
        #################################################
        ## check for scons
        print "Checking for scons                : ",
        scons = os.popen("which scons 2>/dev/null").read().strip()
        
        #scons not found in path ? try the local version
        if not len(scons):
            if os.path.isfile("scons.py"):
                print bksys.YELLOW + "scons.py found here will be used" + bksys.NORMAL
                makefile_config_descr = open("Makefile.config", 'w')
                makefile_config_descr.write("SCONS=python scons.py")
                makefile_config_descr.close()
            else:
                print bksys.RED + "scons was not found, run ./unpack_local_scons.sh and reconfigure" + bksys.NORMAL
                sys.exit(1)
        else:
            print bksys.GREEN + "scons was found" + bksys.NORMAL
            makefile_config_descr = open("Makefile.config", 'w')
            makefile_config_descr.write("SCONS=scons")
            makefile_config_descr.close()
        
        
        #################################################
        ## check for kde-config
        print "Checking for kde-config           : ",
        kde_config = os.popen("which kde-config 2>/dev/null").read().strip()
        if len(kde_config):
            print bksys.GREEN + "kde-config was found" + bksys.NORMAL
        else:
            print bksys.RED + "kde-config was NOT found in your PATH"+ bksys.NORMAL
            print "Make sure kde is installed properly"
            print "(missing package kdebase-devel?)"
            sys.exit(1)
        
        # add kdedir to the config
        config_py_content.append("env['KDEDIR']   ='" + os.popen('kde-config -prefix').read().strip() + "'")
        
        #################################################
        ## check for kde version
        print "Checking for kde version          : ",
        kde_version = os.popen("kde-config --version|grep KDE").read().strip().split()[1]
        if int(kde_version[0]) != 3 or int(kde_version[2]) < 2:
            print bksys.RED + kde_version
            print bksys.RED + "Your kde version can be too old" + bksys.NORMAL
            print bksys.RED + "Please make sure kde is at least 3.2" + bksys.NORMAL
        else:
            print bksys.GREEN + kde_version + bksys.NORMAL
        
        
        #################################################
        ## the qt library
        print "Checking for the qt library       : ",
        qtdir = os.getenv("QTDIR")
        if qtdir:
            print bksys.GREEN + "qt is in " + qtdir + bksys.NORMAL
        else:
            m = re.search('(.*)/lib/libqt.*', os.popen('ldd `kde-config --expandvars --install lib`' + '/libkdeui.so | grep libqt').read().strip().split()[2])
            if m:
                qtdir = m.group(1)
                print bksys.YELLOW + "qt was found as " + m.group(1) + bksys.NORMAL
            else:
                print bksys.RED + "qt was not found" + bksys.NORMAL
                print bksys.RED + "Please set QTDIR first (/usr/lib/qt3?)" + bksys.NORMAL
                sys.exit(1)
        
        config_py_content.append("env['QTDIR']    ='" + qtdir.strip() + "'")
        
        #################################################
        ## find the necessary programs uic and moc
        print "Checking for uic                  : ",
        uic = qtdir + "/bin/uic"
        if os.path.isfile(uic):
            print bksys.GREEN + "uic was found as " + uic + bksys.NORMAL
        else:
            uic = os.popen("which uic 2>/dev/null").read().strip()
            if len(uic):
                print bksys.YELLOW + "uic was found as " + uic + bksys.NORMAL
            else:
                print bksys.RED + "uic was not found - put it in your PATH ?" + bksys.NORMAL
                sys.exit(1)
                
        print "Checking for moc                  : ",
        moc = qtdir + "/bin/moc"
        if os.path.isfile(moc):
            print bksys.GREEN + "moc was found as " + moc + bksys.NORMAL
        else:
            moc = os.popen("which moc 2>/dev/null").read().strip()
            if len(moc):
                print bksys.YELLOW + "moc was found as " + moc + bksys.NORMAL
            else:
                print bksys.RED + "moc was not found - put it in your PATH ?" + bksys.NORMAL
                sys.exit(1)
                
        # add to the config
        config_py_content.append("env['QT_UIC']   ='" + uic + "'")
        config_py_content.append("env['QT_MOC']   ='" + moc + "'")
        
        
        #################################################
        ## check for the qt and kde includes
        print "Checking for the qt includes      : ",
        if qtincludes and os.path.isfile(qtincludes + "/qlayout.h"):
            # the user told where to look for and it looks valid
            print bksys.GREEN + "ok " + qtincludes + bksys.NORMAL
        else:
            if os.path.isfile(qtdir + "/include/qlayout.h"):
                # Automatic detection
                print bksys.GREEN + "ok " + qtdir + "/include/ " + bksys.NORMAL
                qtincludes = qtdir + "/include/"
            elif os.path.isfile("/usr/include/qt3/qlayout.h"):
                # Debian probably
                print bksys.YELLOW + "the qt headers were found in /usr/include/qt3/ " + bksys.NORMAL
                qtincludes = "/usr/include/qt3"
            else:
                print bksys.RED + "the qt headers were not found, use ./configure --qtincludes=/path/to/" + bksys.NORMAL
                sys.exit(1)
        
        print "Checking for the kde includes     : ",
        kdeprefix = os.popen("kde-config --prefix").read().strip()
        if not kdeincludes:
            kdeincludes = kdeprefix+"/include/"
        if os.path.isfile(kdeincludes + "/klineedit.h"):
            # the user told where to look for and it looks valid
            print bksys.GREEN + "ok " + kdeincludes + bksys.NORMAL
        else:
            if os.path.isfile(kdeprefix+"/include/kde/klineedit.h"):
                # Debian, Fedora
                print bksys.YELLOW + "the kde headers were found in " + kdeprefix + "/include/kde/" + bksys.NORMAL
                kdeincludes = kdeprefix + "/include/kde/"
            else:
                # the headers could not be found, try with locate
                kdeheaders = os.popen("locate klineedit.h 2>/dev/null|grep include|head -n1").read().strip().replace('klineedit.h','')
                if len(kdeheaders) and os.path.isdir(kdeheaders):
                    print bksys.YELLOW + "the kde headers were found in " + kdeheaders + bksys.NORMAL
                    kdeincludes = kdeheaders
                else:
                    print bksys.RED + "The kde includes were NOT found" + bksys.NORMAL
                    print
                    print bksys.BOLD + "WARNING : !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" + bksys.NORMAL
                    print bksys.BOLD + "WARNING : Set the path to the kde includes with --kdeincludes  " + bksys.NORMAL
                    print bksys.BOLD + "WARNING : !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" + bksys.NORMAL
		    sys.exit(1)
        
        
        #################################################
        ## set debugging mode or on off
        if enable_debug:
            config_py_content.append("env['CXXFLAGS'].append('-g')")
            print bksys.CYAN + "Enabling debug for the project" + bksys.NORMAL
        
            
        if prefix:
            ## the user has given a prefix, use it
            destdir = prefix
            if not execprefix:
                execprefix = prefix
            if not datadir:
                datadir = prefix  + "/share"
            if not libdir:
                libdir = execprefix + "/lib"
            
            subst_vars = lambda x: x.replace('${exec_prefix}', execprefix).replace('${datadir}', datadir).replace('${libdir}', libdir)
            
            config_py_content.append("env['PREFIX']   ='" + destdir + "'")
            config_py_content.append("env['KDEBIN']   ='" + subst_vars(os.popen('kde-config --install exe').read().strip()) + "'")
            config_py_content.append("env['KDEAPPS']  ='" + subst_vars(os.popen('kde-config --install apps').read().strip()) + "'")
            config_py_content.append("env['KDEDATA']  ='" + subst_vars(os.popen('kde-config --install data').read().strip()) + "'")
            config_py_content.append("env['KDEMODULE']='" + subst_vars(os.popen('kde-config --install module').read().strip()) + "'")
            config_py_content.append("env['KDELOCALE']='" + subst_vars(os.popen('kde-config --install locale').read().strip()) + "'")
            config_py_content.append("env['KDEDOC']   ='" + subst_vars(os.popen('kde-config --install html').read().strip()) + "'")
            config_py_content.append("env['KDEKCFG']  ='" + subst_vars(os.popen('kde-config --install kcfg').read().strip()) + "'")
            config_py_content.append("env['KDEXDG']   ='" + subst_vars(os.popen('kde-config --install xdgdata-apps').read().strip()) + "'")
            config_py_content.append("env['KDEMENU']  ='" + subst_vars(os.popen('kde-config --install apps').read().strip()) + "'")
            config_py_content.append("env['KDEMIME']  ='" + subst_vars(os.popen('kde-config --install mime').read().strip()) + "'")
            config_py_content.append("env['KDEICONS'] ='" + subst_vars(os.popen('kde-config --install icon').read().strip()) + "'")
            config_py_content.append("env['KDESERV']  ='" + subst_vars(os.popen('kde-config --install services').read().strip()) + "'")
            
        else:
            # the user has given no prefix, install as a normal kde app
            destdir = os.popen('kde-config --prefix').read().strip()
            config_py_content.append("env['PREFIX']   ='" + destdir + "'")
            config_py_content.append("env['KDEBIN']   ='" + os.popen('kde-config --expandvars --install exe').read().strip() + "'")
            config_py_content.append("env['KDEAPPS']  ='" + os.popen('kde-config --expandvars --install apps').read().strip() + "'")
            config_py_content.append("env['KDEDATA']  ='" + os.popen('kde-config --expandvars --install data').read().strip() + "'")
            config_py_content.append("env['KDEMODULE']='" + os.popen('kde-config --expandvars --install module').read().strip() + "'")
            config_py_content.append("env['KDELOCALE']='" + os.popen('kde-config --expandvars --install locale').read().strip() + "'")
            config_py_content.append("env['KDEDOC']   ='" + os.popen('kde-config --expandvars --install html').read().strip() + "'")
            config_py_content.append("env['KDEKCFG']  ='" + os.popen('kde-config --expandvars --install kcfg').read().strip() + "'")
            config_py_content.append("env['KDEXDG']   ='" + os.popen('kde-config --expandvars --install xdgdata-apps').read().strip() + "'")
            config_py_content.append("env['KDEMENU']  ='" + os.popen('kde-config --expandvars --install apps').read().strip() + "'")
            config_py_content.append("env['KDEMIME']  ='" + os.popen('kde-config --expandvars --install mime').read().strip() + "'")
            config_py_content.append("env['KDEICONS'] ='" + os.popen('kde-config --expandvars --install icon').read().strip() + "'")
            config_py_content.append("env['KDESERV']  ='" + os.popen('kde-config --expandvars --install services').read().strip() + "'")
        
        config_py_content.append("env['QTPLUGINS']='" + os.popen('kde-config --expandvars --install qtplugins').read().strip() + "'")
        config_py_content.append("")
        config_py_content.append("## includes and libraries")
        
        # kde libs and includes
        config_py_content.append("env.Append(CXXFLAGS = ['-I" + kdeincludes + "'])")
        if kdelibs:
            config_py_content.append("env.Append(LIBPATH = ['" + kdelibs + "'])")
        else:
            config_py_content.append("env.Append(LIBPATH = ['" + os.popen('kde-config --expandvars --install lib').read().strip() + "'])")
        
        # qt libs and includes
        config_py_content.append("env.Append(CXXFLAGS = ['-I" + qtincludes + "'])")
        if qtlibs:
            config_py_content.append("env.Append(LIBPATH = ['" + qtlibs + "'])")
        else:
            config_py_content.append("env.Append(LIBPATH = ['" + qtdir + "/lib'])")
        
        # others ?
        if extraincludes:
            for dir in extraincludes.split(':'):
                config_py_content.append("env.Append(CXXFLAGS = ['-I" + dir + "'])")
        
        
        print "The project will be installed in  :  " + bksys.BOLD + destdir + bksys.NORMAL        
        return config_py_content
