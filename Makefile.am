EXTRA_DIST = AUTHORS COPYING ChangeLog INSTALL README TODO TRANSLATORS rosegarden.lsm 

#COMPILE_FIRST = base sound
#COMPILE_LAST = gui
#SUBDIRS = $(COMPILE_FIRST) $(TOPSUBDIRS) $(COMPILE_LAST)
SUBDIRS = base sound sequencer gui po

$(top_srcdir)/configure.in: configure.in.in $(top_srcdir)/subdirs
	cd $(top_srcdir) && $(MAKE) -f admin/Makefile.common configure.in ;

$(top_srcdir)/subdirs:
	cd $(top_srcdir) && $(MAKE) -f admin/Makefile.common subdirs

$(top_srcdir)/acinclude.m4: $(top_srcdir)/admin/acinclude.m4.in $(top_srcdir)/admin/libtool.m4.in
	@cd $(top_srcdir) && cat admin/acinclude.m4.in admin/libtool.m4.in > acinclude.m4

MAINTAINERCLEANFILES = subdirs configure.files 

package-messages:
	$(MAKE) -f admin/Makefile.common package-messages
	$(MAKE) -C po merge

# omit docs/web from dist for space reasons, and omit docs/CVS just because
EXTRA_DIST += admin docs/code docs/data_struct docs/design docs/discussion docs/howtos docs/old docs/RAISONDETRE docs/task_lists docs/test

dist-hook:
	cd $(top_distdir) && perl admin/am_edit -padmin
	cd $(top_distdir) && $(MAKE) -f admin/Makefile.common subdirs
