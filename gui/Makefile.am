bin_PROGRAMS = rosegardengui
rosegardengui_SOURCES = trackscanvas.cpp \
			trackseditor.cpp \
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
			rosegardenguiiface.skel \
			trackseditoriface.skel

rosegardengui_LDADD   = -lcore -lqt -lkfile -lkdeui -lkdecore -lXext -lX11 $(LIBSOCKET) -lRosegardenSound

SUBDIRS = docs

EXTRA_DIST = main.cpp rosegardengui.cpp rosegardengui.h rosegardenguidoc.cpp rosegardenguidoc.h rosegardenguiview.cpp rosegardenguiview.h resource.h rosegardengui.desktop rosegardengui.xpm mini-rosegardengui.xpm notepixmapfactory.cpp notepixmapfactory.h qcanvasgroupableitem.cpp qcanvasgroupableitem.h qcanvasitemgroup.cpp qcanvasitemgroup.h qcanvaslinegroupable.cpp qcanvaslinegroupable.h qcanvasspritegroupable.cpp qcanvasspritegroupable.h staff.cpp staff.h qcanvassimplesprite.cpp qcanvassimplesprite.h layoutengine.cpp layoutengine.h notationhlayout.cpp notationhlayout.h notationvlayout.cpp notationvlayout.h xmlstorableevent.cpp xmlstorableevent.h rosexmlhandler.cpp rosexmlhandler.h rosedebug.h notationelement.cpp notationelement.h notationsets.cpp notationsets.h viewelementsmanager.cpp viewelementsmanager.h notationview.cpp notationview.h notationcanvasview.cpp notationcanvasview.h staffline.cpp staffline.h rosedebug.cpp trackseditor.cpp trackseditor.h trackscanvas.cpp trackscanvas.h rosegardenguiiface.h rosegardenguiiface.cpp trackseditoriface.cpp qcanvasspline.cpp qcanvasspline.h

install-data-local:
	$(mkinstalldirs) $(kde_appsdir)/Applications/
	$(INSTALL_DATA) $(srcdir)/rosegardengui.desktop $(kde_appsdir)/Applications/rosegardengui.desktop
	$(mkinstalldirs) $(kde_icondir)/medium/locolor/apps/
	$(INSTALL_DATA) $(srcdir)/rosegardengui.xpm $(kde_icondir)/medium/locolor/apps/rosegardengui.xpm
	$(mkinstalldirs) $(kde_icondir)/small/locolor/apps/
	$(INSTALL_DATA) $(srcdir)/mini-rosegardengui.xpm $(kde_icondir)/small/locolor/apps/rosegardengui.xpm

uninstall-local:
	-rm -f $(kde_appsdir)/Applications/rosegardengui.desktop
	-rm -f $(kde_icondir)/medium/locolor/apps/rosegardengui.xpm
	-rm -f $(kde_icondir)/small/locolor/apps/rosegardengui.xpm

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
bin_PROGRAMS = rosegardengui 

rosegardengui_METASOURCES = AUTO

# the library search path. 
rosegardengui_LDFLAGS = $(all_libraries) $(KDE_RPATH) -L../core -L../sound

# them while "make clean", use CLEANFILES
DISTCLEANFILES = $(rosegardengui_METASOURCES)

# make messages.po. Move this one to ../po/ and "make merge" in po
# the -x is for skipping messages already translated in kdelibs
messages:
	$(XGETTEXT) -C -ki18n -x $(KDEDIR)/include/kde.pot `find . -name \*.h -o -name \*.cpp -o -name \*.cc` -o ../po/rosegardengui.pot 

