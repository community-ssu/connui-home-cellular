AC_PREREQ[(2.58)]
AC_INIT([operator-name-cbs-home-item], [0.1])
AC_CONFIG_SRCDIR([sbpluginsrc/operator-name-cbs-home-item.c])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADER([config.h])

AC_PROG_CC
AM_PROG_CC_C_O
AC_HEADER_STDC
AM_PATH_GLIB_2_0([2.16.0])
AC_PROG_INSTALL
AM_PROG_LIBTOOL

PKG_CHECK_MODULES([OPERATORNAMECBSHOMEITEM], [glib-2.0 gobject-2.0 cairo dbus-glib-1 libosso gconf-2.0 gtk+-2.0 hildon-1 libhildondesktop-1 libconnui_cell])
AC_SUBST([OPERATORNAMECBSHOMEITEM_CFLAGS])
AC_SUBST([OPERATORNAMECBSHOMEITEM_LIBS])

PKG_CHECK_MODULES([OPERATORNAMECBSCPA], [glib-2.0 libosso gconf-2.0 gtk+-2.0 hildon-1 hildon-control-panel])
AC_SUBST([OPERATORNAMECBSCPA_CFLAGS])
AC_SUBST([OPERATORNAMECBSCPA_LIBS])

PKG_CHECK_MODULES([CBSPATCH], [glib-2.0])
AC_SUBST([CBSPATCH_CFLAGS])
AC_SUBST([CBSPATCH_LIBS])

HILDON_DESKTOP_LIB_DIR=`pkg-config libhildondesktop-1 --variable=hildondesktoplibdir`
AC_SUBST([HILDON_DESKTOP_LIB_DIR])

CONTROL_PANEL_LIB_DIR=`pkg-config hildon-control-panel --variable=pluginlibdir`
AC_SUBST([CONTROL_PANEL_LIB_DIR])

CONTROL_PANEL_DESKTOP_DIR=`pkg-config hildon-control-panel --variable=plugindesktopentrydir`
AC_SUBST([CONTROL_PANEL_DESKTOP_DIR])

AC_OUTPUT([Makefile data/Makefile sbpluginsrc/Makefile cppluginsrc/Makefile cbspatch/Makefile])
