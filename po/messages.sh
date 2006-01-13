#!/bin/sh

# Inspired by Makefile.common from coolo
# this script is used to update the .po files

# To update the translations, you will need a specific gettext 
# patched for kde and a lot of patience, tenacity, luck, time ..


# I guess one should only update the .po files when all .cpp files 
# are generated (after a make or scons)

# If you have a better way to do this, do not keep that info 
# for yourself and help me to improve this script, thanks
# (tnagyemail-mail tat yahoo d0tt fr)

if [ -z "$KDE_GETTEXT_BASE" ]; then
    if [ -d /opt/gettext-kde ]; then
	KDE_GETTEXT_BASE=/opt/gettext-kde
    fi
fi
if [ ! -d "$KDE_GETTEXT_BASE" ]; then
    echo 1>&2
    echo "WARNING: Environment variable KDE_GETTEXT_BASE must be set" 1>&2
    echo "such that the KDE patched version of gettext is found in " 1>&2
    echo "KDE_GETTEXT_BASE/bin/." 1>&2
    echo 1>&2
    echo "Falling back to default gettext, but plural translations " 1>&2
    echo "will probably be wrong." 1>&2
    echo 1>&2
    echo "See ftp://ftp.kde.org/devel/gettext-kde/ for the patched gettext." 1>&2
    echo 1>&2
else
    KDE_GETTEXT_PATH=${KDE_GETTEXT_BASE}/bin/
fi

SRCDIR=../gui # srcdir is the directory containing the source code
TIPSDIR=$SRCDIR/docs/en # tipsdir is the directory containing the tips

KDEDIR=`kde-config --prefix`
EXTRACTRC=extractrc
KDEPOT=$KDEDIR/include/kde.pot
if [ ! -f "$KDEPOT" ] && [ -f /usr/include/kde/kde.pot ]; then
    KDEPOT=/usr/include/kde/kde.pot
fi
XGETTEXT="${KDE_GETTEXT_PATH}xgettext -C -ki18n -ktr2i18n -kI18N_NOOP -ktranslate -kaliasLocale -x $KDEPOT "

## check that kde.pot is available
if ! test -e $KDEPOT; then
	echo "$KDEPOT does not exist, there is something wrong with your installation!"
	XGETTEXT="${KDE_GETTEXT_PATH}xgettext -C -ki18n -ktr2i18n -kI18N_NOOP -ktranslate -kaliasLocale "
fi

> rc.cpp

## extract the strings
echo "extracting the strings"

# process the .ui and .rc files
$EXTRACTRC `find $SRCDIR -iname *.rc` >> rc.cpp
$EXTRACTRC `find $SRCDIR -iname *.ui` >> rc.cpp
echo -e 'i18n("_: NAME OF TRANSLATORS\\n"\n"Your names")\ni18n("_: EMAIL OF TRANSLATORS\\n"\n"Your emails")' > $SRCDIR/_translatorinfo.cpp

# process the tips - $SRCDIR is supposed to be where the tips are living
pushd $TIPSDIR; preparetips >tips.cpp; popd

#$XGETTEXT `find $SRCDIR -name "*.cpp"` -o tmp.pot
$XGETTEXT `find $SRCDIR -name "*.cpp" -o -name "*.h"` rc.cpp $TIPSDIR/tips.cpp -o tmp.pot

# remove the intermediate files
rm -f $TIPSDIR/tips.cpp
rm -f rc.cpp
rm -f $SRCDIR/_translatorinfo.cpp

## now merge the .po files ..
echo "merging the .po files"

for i in `ls *.po`; do
    echo $i
    msgmerge $i tmp.pot -o $i || exit 1
done

## finished
echo "Done"

