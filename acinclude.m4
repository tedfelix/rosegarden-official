# Qt location code adapted from iTALC (italc.sourceforge.net) -- with thanks!

# Check for Qt compiler flags, linker flags, and binary packages
AC_DEFUN([RG_CHECK_QT],
[
AC_REQUIRE([AC_PROG_CXX])
AC_REQUIRE([AC_PATH_X])

AC_MSG_CHECKING([QTDIR])
AC_ARG_WITH([qtdir], [  --with-qtdir=DIR        Qt installation directory [default=$QTDIR]], QTDIR=$withval)
# Check that QTDIR is defined or that --with-qtdir given
if test x"$QTDIR" = x ; then
	# some usual Qt locations
	QT_SEARCH="/usr /opt /usr/lib/qt"
else
	QT_SEARCH=$QTDIR
	QTDIR=""
fi
for i in $QT_SEARCH ; do
	QT_INCLUDE_SEARCH="include/qt4 include"
	for j in $QT_INCLUDE_SEARCH ; do
	        if test -f $i/$j/Qt/qglobal.h && test x$QTDIR = x ; then
			QTDIR=$i
			QT_INCLUDES=$i/$j
		fi
	done
done
if test x"$QTDIR" = x ; then
	AC_MSG_ERROR([*** Failed to find Qt4 installation. QTDIR must be defined, or --with-qtdir option given])
fi
AC_MSG_RESULT([$QTDIR])

# Change backslashes in QTDIR to forward slashes to prevent escaping
# problems later on in the build process, mainly for Cygwin build
# environment using MSVC as the compiler
QTDIR=`echo $QTDIR | sed 's/\\\\/\\//g'`

AC_MSG_CHECKING([Qt includes])
# Check where includes are located
if test x"$QT_INCLUDES" = x ; then
	AC_MSG_ERROR([
Failed to find required Qt4 headers.
Please ensure you have the Qt4 development files installed,
and if necessary set QTDIR to the location of your Qt4 installation.
])
fi
AC_MSG_RESULT([$QT_INCLUDES])

# Check that moc is in path
AC_CHECK_PROG(MOC, moc-qt4, $QTDIR/bin/moc-qt4,,$QTDIR/bin/)
if test x$MOC = x ; then
	AC_CHECK_PROG(MOC, moc, $QTDIR/bin/moc,,$QTDIR/bin/)
	if test x$MOC = x ; then
        	AC_MSG_ERROR([
Failed to find required moc-qt4 or moc program.
Please ensure you have the Qt4 development files installed,
and if necessary set QTDIR to the location of your Qt4 installation.
])
	fi
fi

# Check that uic is in path
AC_CHECK_PROG(UIC, uic-qt4, $QTDIR/bin/uic-qt4,,$QTDIR/bin/)
if test x$UIC = x ; then
	AC_CHECK_PROG(UIC, uic, $QTDIR/bin/uic,,$QTDIR/bin/)
	if test x$UIC = x ; then
        	AC_MSG_ERROR([
Failed to find required uic-qt4 or uic program.
Please ensure you have the Qt4 development files installed,
and if necessary set QTDIR to the location of your Qt4 installation.
])
	fi
fi

# Check that rcc is in path
AC_CHECK_PROG(RCC, rcc-qt4, $QTDIR/bin/rcc-qt4,,$QTDIR/bin/)
if test x$RCC = x ; then
	AC_CHECK_PROG(RCC, rcc, $QTDIR/bin/rcc,,$QTDIR/bin/)
	if test x$RCC = x ; then
        	AC_MSG_ERROR([
Failed to find required rcc-qt4 or rcc program.
Please ensure you have the Qt4 development files installed,
and if necessary set QTDIR to the location of your Qt4 installation.
])
	fi
fi

# lupdate is the Qt translation-update utility.
AC_CHECK_PROG(LUPDATE, lupdate-qt4, $QTDIR/bin/lupdate-qt4,,$QTDIR/bin/)
if test x$LUPDATE = x ; then
	AC_CHECK_PROG(LUPDATE, lupdate, $QTDIR/bin/lupdate,,$QTDIR/bin/)
	if test x$MOC = x ; then
        	AC_MSG_WARN([
Failed to find lupdate-qt4 or lupdate program.
This program is not needed for a simple build,
but it should be part of a Qt4 development installation
and its absence is troubling.
])
	fi
fi

# lrelease is the Qt translation-release utility.
AC_CHECK_PROG(LRELEASE, lrelease-qt4, $QTDIR/bin/lrelease-qt4,,$QTDIR/bin/)
if test x$LRELEASE = x ; then
	AC_CHECK_PROG(LRELEASE, lrelease, $QTDIR/bin/lrelease,,$QTDIR/bin/)
	if test x$MOC = x ; then
        	AC_MSG_WARN([
Failed to find lrelease-qt4 or lrelease program.
This program is not needed for a simple build,
but it should be part of a Qt4 development installation
and its absence is troubling.
])
	fi
fi

QT_CXXFLAGS="-I$QT_INCLUDES/Qt3Support -I$QT_INCLUDES/QtGui -I$QT_INCLUDES/QtXml -I$QT_INCLUDES/QtNetwork -I$QT_INCLUDES/QtCore -I$QT_INCLUDES -DQT3_SUPPORT"

AC_MSG_CHECKING([QTLIBDIR])
AC_ARG_WITH([qtlibdir], [  --with-qtlibdir=DIR     Qt library directory [default=$QTLIBDIR]], QTLIBDIR=$withval)
if test x"$QTLIBDIR" = x ; then
	QTLIB_SEARCH="$QTDIR/lib $QTDIR/lib64 $QTDIR/lib32"
else
	QTLIB_SEARCH="$QTLIBDIR"
	QTDIR=""
fi
QTLIB_EXTS="so a dylib dll"
for i in $QTLIB_SEARCH ; do
    for j in $QTLIB_EXTS ; do
	if test -f $i/libQtCore.$j && test x$QTLIBDIR = x ; then
	   	QTLIBDIR=$i
	fi
    done
done
if test x"$QTLIBDIR" = x ; then
	AC_MSG_ERROR([
Failed to find required Qt4 GUI link entry point (libQtGui.so).
Define QTLIBDIR or use --with-qtlibdir to specify the library location.
])
fi
QT3SUPPORT_PATH=""
for j in $QTLIB_EXTS ; do
    if test -f $QTLIBDIR/libQt3Support.$j ; then
        QT3SUPPORT_PATH=$QTLIBDIR
	break
    fi
done
if [ x"$QT3SUPPORT_PATH" = x ]; then
	AC_MSG_ERROR([
Failed to find required Qt3 support library (libQt3Support) in
the Qt4 library directory $QTLIBDIR.
Please ensure you have the Qt3 compatibility library installed,
and if necessary set QTDIR to the location of your Qt4 installation.
])
fi
AC_MSG_RESULT([$QTLIBDIR])

QT_LIBS="-L$QTLIBDIR -lQt3Support -lQtGui -lQtXml -lQtNetwork -lQtCore"

AC_MSG_CHECKING([QT_CXXFLAGS])
AC_MSG_RESULT([$QT_CXXFLAGS])
AC_MSG_CHECKING([QT_LIBS])
AC_MSG_RESULT([$QT_LIBS])

AC_SUBST(QT_CXXFLAGS)
AC_SUBST(QT_LIBS)

])

