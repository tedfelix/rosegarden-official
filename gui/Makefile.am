KDE_CXXFLAGS = $(USE_EXCEPTIONS) @KDE_CXXFLAGS@

bin_PROGRAMS = rosegarden

rosegarden_SOURCES = \
        audiomanagerdialog.cpp \
        barbuttons.cpp \
	basiccommand.cpp \
	chordnameruler.cpp \
	colours.cpp \
	csoundio.cpp \
	dialogs.cpp \
	editcommands.cpp \
	edittool.cpp \
	editviewbase.cpp \
	editview.cpp \
        eventview.cpp \
        instrumentlabel.cpp \
        instrumentparameterbox.cpp \
	kstartuplogo.cpp \
	ktmpstatusmsg.cpp \
        lilypondio.cpp \
        loopruler.cpp \
	main.cpp \
	matrixcanvasview.cpp \
	matrixelement.cpp \
	matrixhlayout.cpp \
        matrixparameterbox.cpp \
	matrixstaff.cpp \
	matrixtool.cpp \
	matrixview.cpp \
	matrixvlayout.cpp \
	multiviewcommandhistory.cpp \
	notationcanvasview.cpp \
	notationcommands.cpp \
	notationelement.cpp \
	notationhlayout.cpp \
	notationproperties.cpp \
	notationsets.cpp \
	notationstaff.cpp \
	notationtool.cpp \
	notationview.cpp \
	notationviewslots.cpp \
	notationvlayout.cpp \
	notecharname.cpp \
	notefont.cpp \
	notepixmapfactory.cpp \
	notestyle.cpp \
	pianokeyboard.cpp \
	qcanvasgroupableitem.cpp \
	qcanvassimplesprite.cpp \
	qdeferscrollview.cpp \
	rg21io.cpp \
	rosedebug.cpp \
        rosegardenconfiguredialog.cpp \
	rosegardencanvasview.cpp \
	rosegardengui.cpp \
	rosegardenguidoc.cpp \
	rosegardenguiiface.skel \
	rosegardenguiiface.cpp \
	rosegardenguiview.cpp \
        rosegardentransport.ui \
        rosegardentransportdialog.cpp \
	rosestrings.cpp \
	rosexmlhandler.cpp \
	segmentcanvas.cpp \
        segmentcommands.cpp \
        segmentparameterbox.cpp \
        sequencemanager.cpp \
	spline.cpp \
	staffline.cpp \
	temporuler.cpp \
	textruler.cpp \
        trackbuttons.cpp \
	trackeditor.cpp \
        trackheader.cpp \
        tracklabel.cpp \
	trackseditoriface.skel \
        trackvumeter.cpp \
        velocitycolour.cpp \
        vumeter.cpp \
        widgets.cpp \
	xmlstorableevent.cpp

rosegarden_LDADD   = ../sound/libRosegardenSound.la ../base/libbase.la -lkdeui -lkdecore -lkdeprint $(LIB_QT) $(LIBSOCKET) $(LIB_KFILE)

SUBDIRS = docs

EXTRA_DIST = rosegardenguiiface.kidl trackseditoriface.kidl \
	styles/*.xml \
	pixmaps/rg21/mapping.xml \
	pixmaps/rg21/4/*.xpm \
	pixmaps/rg21/8/*.xpm \
	pixmaps/feta/mapping.xml \
	pixmaps/feta/4/*.xpm \
	pixmaps/feta/6/*.xpm \
	pixmaps/feta/8/*.xpm \
	pixmaps/feta/10/*.xpm \
	pixmaps/feta/12/*.xpm \
	pixmaps/toolbar/*.xpm \
	pixmaps/transport/*.xpm \
	pixmaps/splash.png \
	testfiles/*.rg \
        testfiles/audio/*.wav \
	testfiles/*.rose

# Where pixmaps for notes, clefs, etc... will be installed
data_dirname = rosegarden

install-data-local:
	$(mkinstalldirs) $(kde_appsdir)/Applications/
	$(INSTALL_DATA) $(srcdir)/rosegarden.desktop $(kde_appsdir)/Applications/rosegarden.desktop
	$(mkinstalldirs) $(kde_icondir)/medium/locolor/apps/
	$(mkinstalldirs) $(kde_icondir)/medium/hicolor/apps/
	- $(INSTALL_DATA) $(srcdir)/rosegarden.xpm $(kde_icondir)/locolor/32x32/apps/rosegarden.xpm
	- $(INSTALL_DATA) $(srcdir)/rosegarden.xpm $(kde_icondir)/hicolor/32x32/apps/rosegarden.xpm
	$(mkinstalldirs) $(kde_icondir)/small/locolor/apps/
	$(mkinstalldirs) $(kde_icondir)/small/hicolor/apps/
	- $(INSTALL_DATA) $(srcdir)/mini-rosegarden.xpm $(kde_icondir)/locolor/16x16/apps/rosegarden.xpm
	- $(INSTALL_DATA) $(srcdir)/mini-rosegarden.xpm $(kde_icondir)/hicolor/16x16/apps/rosegarden.xpm
	$(mkinstalldirs) $(kde_datadir)/$(data_dirname)/styles
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
	$(INSTALL_DATA) $(srcdir)/styles/*.xml $(kde_datadir)/$(data_dirname)/styles
	$(INSTALL_DATA) $(srcdir)/pixmaps/rg21/mapping.xml $(kde_datadir)/$(data_dirname)/pixmaps/rg21
	$(INSTALL_DATA) $(srcdir)/pixmaps/rg21/4/*.xpm     $(kde_datadir)/$(data_dirname)/pixmaps/rg21/4
	$(INSTALL_DATA) $(srcdir)/pixmaps/rg21/8/*.xpm     $(kde_datadir)/$(data_dirname)/pixmaps/rg21/8
	$(INSTALL_DATA) $(srcdir)/pixmaps/feta/mapping.xml $(kde_datadir)/$(data_dirname)/pixmaps/feta
	$(INSTALL_DATA) $(srcdir)/pixmaps/feta/4/*.xpm     $(kde_datadir)/$(data_dirname)/pixmaps/feta/4
	$(INSTALL_DATA) $(srcdir)/pixmaps/feta/6/*.xpm     $(kde_datadir)/$(data_dirname)/pixmaps/feta/6
	$(INSTALL_DATA) $(srcdir)/pixmaps/feta/8/*.xpm     $(kde_datadir)/$(data_dirname)/pixmaps/feta/8
	$(INSTALL_DATA) $(srcdir)/pixmaps/feta/10/*.xpm    $(kde_datadir)/$(data_dirname)/pixmaps/feta/10
	$(INSTALL_DATA) $(srcdir)/pixmaps/feta/12/*.xpm    $(kde_datadir)/$(data_dirname)/pixmaps/feta/12
	$(INSTALL_DATA) $(srcdir)/pixmaps/toolbar/*.xpm    $(kde_datadir)/$(data_dirname)/pixmaps/toolbar
	$(INSTALL_DATA) $(srcdir)/pixmaps/transport/*.xpm  $(kde_datadir)/$(data_dirname)/pixmaps/transport
	$(INSTALL_DATA) $(srcdir)/pixmaps/splash.png       $(kde_datadir)/$(data_dirname)/pixmaps
	$(INSTALL_DATA) $(srcdir)/testfiles/autoload.rg    $(kde_datadir)/$(data_dirname)
	grep VERSION $(top_builddir)/config.h | sed 's/^[^"]*"//' | sed 's/"[^"]*//' |head -1 > $(kde_datadir)/$(data_dirname)/version.txt



uninstall-local:
	-rm -f $(kde_datadir)/$(data_dirname)/version.txt
	-rm -f $(kde_appsdir)/Applications/rosegarden.desktop
	-rm -f $(kde_icondir)/locolor/32x32/apps/rosegarden.xpm
	-rm -f $(kde_icondir)/hicolor/32x32/apps/rosegarden.xpm
	-rm -f $(kde_icondir)/locolor/16x16/apps/rosegarden.xpm
	-rm -f $(kde_icondir)/hicolor/16x16/apps/rosegarden.xpm
	-rm -rf $(kde_datadir)/$(data_dirname)/styles
	-rm -rf $(kde_datadir)/$(data_dirname)/pixmaps
	-rm -f $(kde_datadir)/$(data_dirname)/autoload.rg

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
INCLUDES= $(all_includes) -I$(top_srcdir)/base -I$(top_srcdir)/sound $(KDE_CXXFLAGS)
# claim, which subdirectories you want to install
# you can add here more. This one gets installed 

rosegarden_METASOURCES = AUTO

# the library search path. 
rosegarden_LDFLAGS = $(all_libraries) $(KDE_RPATH)

# them while "make clean", use CLEANFILES
DISTCLEANFILES = $(rosegarden_METASOURCES)

rcdir = $(kde_datadir)/rosegarden
rc_DATA = rosegardenui.rc notation.rc matrix.rc \
	  notationeraser.rc  noteinserter.rc

# make messages.po. Move this one to ../po/ and "make merge" in po
# the -x is for skipping messages already translated in kdelibs
messages:
	$(XGETTEXT) -C -ki18n -x $(KDEDIR)/include/kde.pot `find . -name \*.h -o -name \*.cpp -o -name \*.cc` -o ../po/rosegarden.pot 

