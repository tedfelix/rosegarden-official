KDE_CXXFLAGS = $(USE_EXCEPTIONS)

bin_PROGRAMS = rosegarden
rosegarden_SOURCES = trackscanvas.cpp \
			trackseditor.cpp \
			notationproperties.cpp \
			rosedebug.cpp \
			staffline.cpp \
			notationcanvasview.cpp \
			notationview.cpp \
			viewelementsmanager.cpp \
			notationelement.cpp \
			notationsets.cpp \
			rosexmlhandler.cpp \
			xmlstorableevent.cpp \
			notationvlayout.cpp \
			notationhlayout.cpp \
			layoutengine.cpp \
			qcanvassimplesprite.cpp \
			staff.cpp \
			qcanvasspline.cpp \
			qcanvasspritegroupable.cpp \
			qcanvaslinegroupable.cpp \
			qcanvasitemgroup.cpp \
			qcanvasgroupableitem.cpp \
			notepixmapfactory.cpp \
			rosegardenguiview.cpp \
			rosegardenguidoc.cpp \
			rosegardengui.cpp \
			main.cpp \
                        rg21io.cpp \
			ktmpstatusmsg.cpp \
			rosegardenguiiface.skel \
			trackseditoriface.skel

rosegarden_LDADD   = ../sound/libRosegardenSound.la ../core/libcore.la -lqt -lkfile -lkdeui -lkdecore -lXext -lX11 $(LIBSOCKET)

SUBDIRS = docs

EXTRA_DIST = rosegardenguiiface.kidl trackseditoriface.kidl

# Where pixmaps for notes, clefs, etc... will be installed
data_dirname = rosegarden

install-data-local:
	$(mkinstalldirs) $(kde_appsdir)/Applications/
	$(INSTALL_DATA) $(srcdir)/rosegarden.desktop $(kde_appsdir)/Applications/rosegarden.desktop
	$(mkinstalldirs) $(kde_icondir)/medium/locolor/apps/
	$(mkinstalldirs) $(kde_icondir)/medium/hicolor/apps/
	$(INSTALL_DATA) $(srcdir)/rosegarden.xpm $(kde_icondir)/medium/locolor/apps/rosegarden.xpm
	$(INSTALL_DATA) $(srcdir)/rosegarden.xpm $(kde_icondir)/medium/hicolor/apps/rosegarden.xpm
	$(mkinstalldirs) $(kde_icondir)/small/locolor/apps/
	$(mkinstalldirs) $(kde_icondir)/small/hicolor/apps/
	$(INSTALL_DATA) $(srcdir)/mini-rosegarden.xpm $(kde_icondir)/small/locolor/apps/rosegarden.xpm
	$(INSTALL_DATA) $(srcdir)/mini-rosegarden.xpm $(kde_icondir)/small/hicolor/apps/rosegarden.xpm
	$(mkinstalldirs) $(kde_datadir)/$(data_dirname)/pixmaps
	$(mkinstalldirs) $(kde_datadir)/$(data_dirname)/pixmaps/5
	$(mkinstalldirs) $(kde_datadir)/$(data_dirname)/pixmaps/9
	$(INSTALL_DATA) $(srcdir)/pixmaps/5/*.xpm $(kde_datadir)/$(data_dirname)/pixmaps/5
	$(INSTALL_DATA) $(srcdir)/pixmaps/9/*.xpm $(kde_datadir)/$(data_dirname)/pixmaps/9

uninstall-local:
	-rm -f $(kde_appsdir)/Applications/rosegarden.desktop
	-rm -f $(kde_icondir)/medium/locolor/apps/rosegarden.xpm
	-rm -f $(kde_icondir)/medium/hicolor/apps/rosegarden.xpm
	-rm -f $(kde_icondir)/small/locolor/apps/rosegarden.xpm
	-rm -f $(kde_icondir)/small/hicolor/apps/rosegarden.xpm
	-rm -f $(kde_datadir)/rosegarden/pixmaps

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

rosegarden_METASOURCES = AUTO

# the library search path. 
rosegarden_LDFLAGS = $(all_libraries) $(KDE_RPATH)

# them while "make clean", use CLEANFILES
DISTCLEANFILES = $(rosegarden_METASOURCES)

rcdir = $(kde_datadir)/rosegarden
rc_DATA = rosegardenui.rc notation.rc

# make messages.po. Move this one to ../po/ and "make merge" in po
# the -x is for skipping messages already translated in kdelibs
messages:
	$(XGETTEXT) -C -ki18n -x $(KDEDIR)/include/kde.pot `find . -name \*.h -o -name \*.cpp -o -name \*.cc` -o ../po/rosegarden.pot 

