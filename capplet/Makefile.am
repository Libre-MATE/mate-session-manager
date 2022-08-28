bin_PROGRAMS = mate-session-properties

AM_CPPFLAGS =						\
	$(SESSION_PROPERTIES_CFLAGS)			\
	-I$(top_srcdir)/mate-session			\
	-DLOCALE_DIR=\""$(datadir)/locale"\"		\
	-DGTKBUILDER_DIR=\""$(pkgdatadir)"\"		\
	$(DISABLE_DEPRECATED_CFLAGS)

AM_CFLAGS = $(WARN_CFLAGS)

BUILT_SOURCES = mate-session-properties-resources.h mate-session-properties-resources.c
nodist_mate_session_properties_SOURCES = $(BUILT_SOURCES)
mate_session_properties_SOURCES =			\
	main.c						\
	gsm-properties-dialog.h				\
	gsm-properties-dialog.c				\
	gsm-app-dialog.h				\
	gsm-app-dialog.c				\
	gsp-app.h					\
	gsp-app.c					\
	gsp-app-manager.h				\
	gsp-app-manager.c				\
	gsp-keyfile.h					\
	gsp-keyfile.c

mate-session-properties-resources.h mate-session-properties-resources.c: $(srcdir)/../data/org.mate.session.gresource.xml Makefile $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies --sourcedir $(srcdir)/../data $(srcdir)/../data/org.mate.session.gresource.xml)
	$(AM_V_GEN) XMLLINT=$(XMLLINT) $(GLIB_COMPILE_RESOURCES) --target $@ --sourcedir $(srcdir)/../data --generate --c-name session_properties $<

mate_session_properties_LDADD =				\
	$(SESSION_PROPERTIES_LIBS)			\
	$(top_builddir)/mate-session/libgsmutil.la

CLEANFILES = \
	$(BUILT_SOURCES)

-include $(top_srcdir)/git.mk
