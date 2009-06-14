# Yoichi Kawasaki <yokawasa@gmail.com>
pkgplugindir=@MYSQL_PLUGINDIR@
INCLUDES= -I$(top_builddir)/include -I$(top_srcdir)/include @MYSQL_SRC_INCLUDES@
#noinst_LTLIBRARIES= mypluglib.la
pkgplugin_LTLIBRARIES= is_locks.la
is_locks_la_SOURCES= is_locks.cc
is_locks_la_LDFLAGS= -module -rpath $(pkgplugindir)
is_locks_la_CXXFLAGS= $(AM_CFLAGS) -DMYSQL_DYNAMIC_PLUGIN
is_locks_la_CFLAGS= $(AM_CFLAGS) -DMYSQL_DYNAMIC_PLUGIN

