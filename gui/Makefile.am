KDE_CXXFLAGS = $(USE_EXCEPTIONS)

bin_PROGRAMS = rosegarden

rosegarden_SOURCES = \
	ktmpstatusmsg.cpp \
	main.cpp \
	notationcanvasview.cpp \
	notationelement.cpp \
	notationhlayout.cpp \
	notationproperties.cpp \
	notationsets.cpp \
	notationstaff.cpp \
	notationview.cpp \
	notationvlayout.cpp \
	notefont.cpp \
	notepixmapfactory.cpp \
	qcanvasgroupableitem.cpp \
	qcanvassimplesprite.cpp \
	qcanvasspline.cpp \
	rg21io.cpp \
	rosedebug.cpp \
	rosegardengui.cpp \
	rosegardenguidoc.cpp \
	rosegardenguiiface.skel \
	rosegardenguiiface.cpp \
	rosegardenguiview.cpp \
	rosexmlhandler.cpp \
	staffline.cpp \
	staffruler.cpp \
	trackscanvas.cpp \
	trackseditor.cpp \
	trackseditoriface.skel \
	xmlstorableevent.cpp \
        rosegardentransport.ui \
        rosegardentransportdialog.cpp \
        trackheader.cpp

rosegarden_LDADD   = ../sound/libRosegardenSound.la ../base/libbase.la -lqt -lkfile -lkdeui -lkdecore -lXext -lX11 $(LIBSOCKET)

SUBDIRS = docs

EXTRA_DIST = rosegardenguiiface.kidl trackseditoriface.kidl \
	pixmaps/rg21/mapping.xml \
	pixmaps/rg21/4/*.xpm \
	pixmaps/rg21/8/*.xpm \
	pixmaps/feta/mapping.xml \
	pixmaps/feta/4/*.xpm \
	pixmaps/feta/6/*.xpm \
	pixmaps/feta/8/*.xpm \
	pixmaps/feta/10/*.xpm \
	pixmaps/feta/12/*.xpm \
	pixmaps/toolbar/*.xpm

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
	$(mkinstalldirs) $(kde_datadir)/$(data_dirname)/pixmaps/rg21
	$(mkinstalldirs) $(kde_datadir)/$(data_dirname)/pixmaps/rg21/4
	$(mkinstalldirs) $(kde_datadir)/$(data_dirname)/pixmaps/rg21/8
	$(mkinstalldirs) $(kde_datadir)/$(data_dirname)/pixmaps/feta
	$(mkinstalldirs) $(kde_datadir)/$(data_dirname)/pixmaps/feta/4
	$(mkinstalldirs) $(kde_datadir)/$(data_dirname)/pixmaps/feta/6
	$(mkinstalldirs) $(kde_datadir)/$(data_dirname)/pixmaps/feta/8
	$(mkinstalldirs) $(kde_datadir)/$(data_dirname)/pixmaps/feta/10
	$(mkinstalldirs) $(kde_datadir)/$(data_dirname)/pixmaps/feta/12
	$(mkinstalldirs) $(kde_datadir)/$(data_dirname)/pixmaps/toolbar
	$(mkinstalldirs) $(kde_datadir)/$(data_dirname)/pixmaps/transport
	$(INSTALL_DATA) $(srcdir)/pixmaps/rg21/mapping.xml $(kde_datadir)/$(data_dirname)/pixmaps/rg21
	$(INSTALL_DATA) $(srcdir)/pixmaps/rg21/4/*.xpm $(kde_datadir)/$(data_dirname)/pixmaps/rg21/4
	$(INSTALL_DATA) $(srcdir)/pixmaps/rg21/8/*.xpm $(kde_datadir)/$(data_dirname)/pixmaps/rg21/8
	$(INSTALL_DATA) $(srcdir)/pixmaps/feta/mapping.xml $(kde_datadir)/$(data_dirname)/pixmaps/feta
	$(INSTALL_DATA) $(srcdir)/pixmaps/feta/4/*.xpm $(kde_datadir)/$(data_dirname)/pixmaps/feta/4
	$(INSTALL_DATA) $(srcdir)/pixmaps/feta/6/*.xpm $(kde_datadir)/$(data_dirname)/pixmaps/feta/6
	$(INSTALL_DATA) $(srcdir)/pixmaps/feta/8/*.xpm $(kde_datadir)/$(data_dirname)/pixmaps/feta/8
	$(INSTALL_DATA) $(srcdir)/pixmaps/feta/10/*.xpm $(kde_datadir)/$(data_dirname)/pixmaps/feta/10
	$(INSTALL_DATA) $(srcdir)/pixmaps/feta/12/*.xpm $(kde_datadir)/$(data_dirname)/pixmaps/feta/12
	$(INSTALL_DATA) $(srcdir)/pixmaps/toolbar/*.xpm $(kde_datadir)/$(data_dirname)/pixmaps/toolbar
	$(INSTALL_DATA) $(srcdir)/pixmaps/transport/*.xpm $(kde_datadir)/$(data_dirname)/pixmaps/transport

uninstall-local:
	-rm -f $(kde_appsdir)/Applications/rosegarden.desktop
	-rm -f $(kde_icondir)/medium/locolor/apps/rosegarden.xpm
	-rm -f $(kde_icondir)/medium/hicolor/apps/rosegarden.xpm
	-rm -f $(kde_icondir)/small/locolor/apps/rosegarden.xpm
	-rm -f $(kde_icondir)/small/hicolor/apps/rosegarden.xpm
	-rm -rf $(kde_datadir)/rosegarden/pixmaps

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
INCLUDES= $(all_includes) -I../base -I../sound
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

