bin_PROGRAMS = rosegardensequencer
rosegardensequencer_SOURCES = rosegardensequencer.cpp \
			main.cpp \
			rosegardensequenceriface.skel

rosegardensequencer_LDADD   = -lqt -lkfile -lkdeui -lkdecore -lXext -lX11 $(LIBSOCKET) -lRosegardenSound -lRosegardenSequencer -lcore

EXTRA_DIST = main.cpp rosegardensequencer.cpp rosegardensequencer.h rosegardensequenceriface.skel

# this 10 paths are KDE specific. Use them:
# kde_htmldir       Where your docs should go to. (contains lang subdirs)
# kde_appsdir       Where your application file (.kdelnk) should go to. 
# kde_icondir       Where your icon should go to.
# kde_minidir       Where your mini icon should go to.
# kde_datadir       Where you install application data. (Use a subdir)
# kde_locale        Where translation files should go to.(contains lang subdirs)
# kde_cgidir        Where cgi-bin executables should go to.
# kde_confdir       Where config files should go to.
# kde_mimedir       Where mimetypes should go to.
# kde_toolbardir    Where general toolbar icons should go to.
# kde_wallpaperdir  Where general wallpapers should go to.

# set the include path for X, qt and KDE
INCLUDES= $(all_includes) -I../core -I../sound
# claim, which subdirectories you want to install
# you can add here more. This one gets installed 
bin_PROGRAMS = rosegardensequencer

rosegardensequencer_METASOURCES = AUTO

# the library search path. 
rosegardensequencer_LDFLAGS = $(all_libraries) $(KDE_RPATH) -L../core -L../sound

# them while "make clean", use CLEANFILES
DISTCLEANFILES = $(rosegardensequencer_METASOURCES)

# make messages.po. Move this one to ../po/ and "make merge" in po
# the -x is for skipping messages already translated in kdelibs
messages:
	$(XGETTEXT) -C -ki18n -x $(KDEDIR)/include/kde.pot `find . -name \*.h -o -name \*.cpp -o -name \*.cc` -o ../po/rosegardensequencer.pot 
