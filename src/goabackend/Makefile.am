
NULL =
CLEANFILES =
EXTRA_DIST =

INCLUDES = 							\
	-I$(top_builddir)/src -I$(top_srcdir)/src		\
	-I$(top_builddir)/src -I$(top_srcdir)/src		\
	-DPACKAGE_LIBEXEC_DIR=\""$(libexecdir)"\" 		\
	-DPACKAGE_SYSCONF_DIR=\""$(sysconfdir)"\" 		\
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" 			\
	-DPACKAGE_BIN_DIR=\""$(bindir)"\" 			\
	-DPACKAGE_LOCALSTATE_DIR=\""$(localstatedir)"\" 	\
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" 		\
	-DPACKAGE_LIB_DIR=\""$(libdir)"\" 			\
	-D_POSIX_PTHREAD_SEMANTICS -D_REENTRANT			\
	$(WARN_CFLAGS)						\
	$(NULL)

# ----------------------------------------------------------------------------------------------------

goabackendenumtypes.h: goabackendenums.h goabackendenumtypes.h.template
	( top_builddir=`cd $(top_builddir) && pwd`; 							\
	 cd $(srcdir) && glib-mkenums --template goabackendenumtypes.h.template goabackendenums.h ) > 	\
	   goabackendenumtypes.h.tmp && mv goabackendenumtypes.h.tmp goabackendenumtypes.h

goabackendenumtypes.c: goabackendenums.h goabackendenumtypes.c.template goabackendenumtypes.h
	( top_builddir=`cd $(top_builddir) && pwd`; 							\
	 cd $(srcdir) && glib-mkenums --template goabackendenumtypes.c.template goabackendenums.h ) > 	\
	   goabackendenumtypes.c.tmp && mv goabackendenumtypes.c.tmp goabackendenumtypes.c

enum_built_sources =							\
	goabackendenumtypes.h		goabackendenumtypes.c		\
	$(NULL)

EXTRA_DIST += goabackendenumtypes.h.template goabackendenumtypes.c.template

# ----------------------------------------------------------------------------------------------------

lib_LTLIBRARIES = libgoa-backend-1.0.la

libgoa_backend_1_0_ladir = $(includedir)/goa-1.0/goabackend

libgoa_backend_1_0_la_HEADERS =						\
	goabackend.h							\
	goabackendtypes.h						\
	goabackendenums.h						\
	goabackendenumtypes.h						\
	goalogging.h							\
	goaeditablelabel.h						\
	goaprovider.h							\
	goaoauthprovider.h						\
	goaoauth2provider.h						\
	goagoogleprovider.h						\
	goafacebookprovider.h						\
	goayahooprovider.h						\
	goatwitterprovider.h						\
	goaflickrprovider.h						\
	goawindowsliveprovider.h					\
	$(NULL)

libgoa_backend_1_0_la_SOURCES =						\
	goabackend.h							\
	goabackendtypes.h						\
	goabackendenums.h						\
	goabackendenumtypes.h		goabackendenumtypes.c		\
	goaeditablelabel.h		goaeditablelabel.c		\
	goaprovider.h			goaprovider.c			\
	goalogging.h			goalogging.c			\
	goaoauthprovider.h		goaoauthprovider.c		\
	goaoauth2provider.h		goaoauth2provider.c		\
	goagoogleprovider.h		goagoogleprovider.c		\
	goafacebookprovider.h		goafacebookprovider.c		\
	goayahooprovider.h		goayahooprovider.c		\
	goatwitterprovider.h		goatwitterprovider.c		\
	goaflickrprovider.h		goaflickrprovider.c		\
	goawindowsliveprovider.h	goawindowsliveprovider.c	\
	$(NULL)

libgoa_backend_1_0_la_CPPFLAGS =				\
	-DG_LOG_DOMAIN=\"GoaBackend\"				\
	$(NULL)

libgoa_backend_1_0_la_CFLAGS =					\
	-DGOA_BACKEND_COMPILATION				\
	-DGOA_API_IS_SUBJECT_TO_CHANGE				\
	$(WEBKIT_GTK_CFLAGS)					\
	$(JSON_GLIB_CFLAGS)					\
	$(GLIB_CFLAGS) 						\
	$(GTK_CFLAGS) 						\
	$(GNOME_KEYRING_CFLAGS)					\
	$(REST_CFLAGS)						\
	$(LIBSOUP_CFLAGS)					\
	$(NULL)

libgoa_backend_1_0_la_LIBADD = 					\
	$(WEBKIT_GTK_LIBS)					\
	$(JSON_GLIB_LIBS)					\
	$(GLIB_LIBS) 						\
	$(GTK_LIBS) 						\
	$(GNOME_KEYRING_LIBS)					\
	$(REST_LIBS)						\
	$(LIBSOUP_LIBS)						\
	$(NULL)

# ----------------------------------------------------------------------------------------------------

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = goa-backend-1.0.pc

# ----------------------------------------------------------------------------------------------------

clean-local :
	rm -f *~ $(enum_built_sources)

-include $(top_srcdir)/git.mk
