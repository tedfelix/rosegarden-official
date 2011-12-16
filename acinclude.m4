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
	case "$QTDIR" in *3*)
	     AC_MSG_WARN([
The QTDIR environment variable is set to "$QTDIR".
This looks suspiciously like the location of a Qt3 installation
instead of the Qt4 installation we require.  If configure fails,
please ensure QTDIR is either set correctly or not set at all.
])
		;;
	esac
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
		AC_CHECK_PROG(MOC, moc.exe, $QTDIR/bin/moc.exe,,$QTDIR/bin/)
		if test x$MOC = x ; then
        		AC_MSG_ERROR([
Failed to find required moc-qt4 or moc program.
Please ensure you have the Qt4 development files installed,
and if necessary set QTDIR to the location of your Qt4 installation.
])
		fi
	fi
fi

# Check that uic is in path
AC_CHECK_PROG(UIC, uic-qt4, $QTDIR/bin/uic-qt4,,$QTDIR/bin/)
if test x$UIC = x ; then
	AC_CHECK_PROG(UIC, uic, $QTDIR/bin/uic,,$QTDIR/bin/)
	if test x$UIC = x ; then
		AC_CHECK_PROG(UIC, uic.exe, $QTDIR/bin/uic.exe,,$QTDIR/bin/)
		if test x$UIC = x ; then
        		AC_MSG_ERROR([
Failed to find required uic-qt4 or uic program.
Please ensure you have the Qt4 development files installed,
and if necessary set QTDIR to the location of your Qt4 installation.
])
		fi
	fi
fi

# Check that rcc is in path
AC_CHECK_PROG(RCC, rcc-qt4, $QTDIR/bin/rcc-qt4,,$QTDIR/bin/)
if test x$RCC = x ; then
	AC_CHECK_PROG(RCC, rcc, $QTDIR/bin/rcc,,$QTDIR/bin/)
	if test x$RCC = x ; then
		AC_CHECK_PROG(RCC, rcc.exe, $QTDIR/bin/rcc.exe,,$QTDIR/bin/)
		if test x$RCC = x ; then
        	   	AC_MSG_ERROR([
Failed to find required rcc-qt4 or rcc program.
Please ensure you have the Qt4 development files installed,
and if necessary set QTDIR to the location of your Qt4 installation.
])
		fi
	fi
fi

# lupdate is the Qt translation-update utility.
AC_CHECK_PROG(LUPDATE, lupdate-qt4, $QTDIR/bin/lupdate-qt4,,$QTDIR/bin/)
if test x$LUPDATE = x ; then
	AC_CHECK_PROG(LUPDATE, lupdate, $QTDIR/bin/lupdate,,$QTDIR/bin/)
	if test x$LUPDATE = x ; then
		AC_CHECK_PROG(LUPDATE, lupdate.exe, $QTDIR/bin/lupdate.exe,,$QTDIR/bin/)
		if test x$LUPDATE = x ; then
        	   	AC_MSG_WARN([
Failed to find lupdate-qt4 or lupdate program.
This program is not needed for a simple build,
but it should be part of a Qt4 development installation
and its absence is troubling.
])
		fi
	fi
fi

# lrelease is the Qt translation-release utility.
AC_CHECK_PROG(LRELEASE, lrelease-qt4, $QTDIR/bin/lrelease-qt4,,$QTDIR/bin/)
if test x$LRELEASE = x ; then
	AC_CHECK_PROG(LRELEASE, lrelease, $QTDIR/bin/lrelease,,$QTDIR/bin/)
	if test x$LRELEASE = x ; then
		AC_CHECK_PROG(LRELEASE, lrelease.exe, $QTDIR/bin/lrelease.exe,,$QTDIR/bin/)
		if test x$LRELEASE = x ; then
        	   	AC_MSG_WARN([
Failed to find lrelease-qt4 or lrelease program.
This program is not needed for a simple build,
but it should be part of a Qt4 development installation
and its absence is troubling.
])
		fi
	fi
fi

QT_CXXFLAGS="-I$QT_INCLUDES/QtGui -I$QT_INCLUDES/QtXml -I$QT_INCLUDES/QtNetwork -I$QT_INCLUDES/QtCore -I$QT_INCLUDES"

AC_MSG_CHECKING([QTLIBDIR])
AC_ARG_WITH([qtlibdir], [  --with-qtlibdir=DIR     Qt library directory [default=$QTLIBDIR]], QTLIBDIR=$withval)
if test x"$QTLIBDIR" = x ; then
   	# bin is included because that's where Qt DLLs hide on Windows
    # On Mandriva Qt libraries are in /usr/lib or /usr/lib64 although
    # QTDIR is /usr/lib/qt4

        QTLIB_SEARCH="$QTDIR/lib $QTDIR/lib64 $QTDIR/lib32 $QTDIR/bin /usr/lib /usr/lib/${build_alias} /usr/lib64"
else
	case "$QTLIBDIR" in *3*)
	     AC_MSG_WARN([
The QTLIBDIR environment variable is set to "$QTLIBDIR".
This looks suspiciously like the location for Qt3 libraries
instead of the Qt4 libraries we require.  If configure fails,
please ensure QTLIBDIR is either set correctly or not set at all.
])
		;;
	esac
	QTLIB_SEARCH="$QTLIBDIR"
	QTDIR=""
fi
QTLIB_EXTS=".so .a .dylib 4.dll"
QTLIB_NEED_4=""
for i in $QTLIB_SEARCH ; do
    for j in $QTLIB_EXTS ; do
	if test -f $i/libQtGui$j && test x$QTLIBDIR = x ; then
	   	QTLIBDIR=$i
	elif test -f $i/QtGui$j && test x$QTLIBDIR = x ; then
	   	QTLIBDIR=$i
		if test x$j = x4.dll ; then
		   	QTLIB_NEED_4=1
		fi
	fi
    done
done
if test x"$QTLIBDIR" = x ; then
	AC_MSG_ERROR([
Failed to find required Qt4 GUI link entry point (libQtGui.so or equivalent).
Define QTLIBDIR or use --with-qtlibdir to specify the library location.
])
fi
AC_MSG_RESULT([$QTLIBDIR])

if test x$QTLIB_NEED_4 = x ; then
	QT_LIBS="-L$QTLIBDIR -lQtGui -lQtXml -lQtNetwork -lQtCore"
else
	QT_LIBS="-L$QTLIBDIR -lQtGui4 -lQtXml4 -lQtNetwork4 -lQtCore4"
fi

AC_MSG_CHECKING([QT_CXXFLAGS])
AC_MSG_RESULT([$QT_CXXFLAGS])
AC_MSG_CHECKING([QT_LIBS])
AC_MSG_RESULT([$QT_LIBS])

AC_SUBST(QT_CXXFLAGS)
AC_SUBST(QT_LIBS)

])

