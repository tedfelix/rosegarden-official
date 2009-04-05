#!/bin/bash

maxrelease=5


# Ubuntu

ubuntu_package_url='http://people.ubuntu.com/~ubuntu-archive/madison.cgi?package=rosegarden&text=on'

ubuntu_release_url='http://releases.ubuntu.com/'

ubuntu_release_description=`wget -O- "$ubuntu_release_url" 2>/dev/null | grep '<li>'`

echo '<tr><td>&nbsp;</td></tr>'
echo '<tr>'
echo '<td class="a" align="center" rowspan="'$maxrelease'"><a href="http://www.ubuntu.com/"><img src="http://www.ubuntu.com/themes/ubuntu07/images/ubuntulogo.png" alt="Ubuntu" border="0"></a></td>'

wget -O- "$ubuntu_package_url" 2>/dev/null | egrep '(all|i386)' | \
    awk '{ gsub(".*:","",$3); gsub("-[^-]*$","",$3); gsub("/"," ",$5); print $5 " " $3}' | \
    sort --key=3 -rn | fgrep -v 2.1 | \
    while read uv ur rv; do
    uvn=`echo "$ubuntu_release_description" | grep '"'"$uv"'/"' | sed 's/^.*Ubuntu //' | sed 's/<.*$//' | grep -v '<'`
    urn=""
    case "$ur" in
	main) urn="Official Ubuntu Packages";;
	universe) urn="Community-supported Packages";;
    esac
    if [ -n "$uvn" -a -n "$rv" ]; then
	echo "</tr><tr>"
	echo "<td class=a>&nbsp;$uvn&nbsp;</td>"
	echo "<td class=a>&nbsp;Rosegarden v$rv&nbsp;</td>"
	echo "<td class=a>&nbsp;$urn&nbsp;</td>"
    fi
    done

echo '</tr>'

for x in `seq 1 $maxrelease` ; do echo '<tr></tr>'; done


# Debian

debian_package_url='http://qa.debian.org/madison.php?package=rosegarden&text=on'

echo '<tr><td>&nbsp;</td></tr>'
echo '<tr>'
echo '<td class="b" align="center" rowspan="'$maxrelease'"><a href="http://www.debian.org/"><img src="http://www.debian.org/logos/openlogo-50.png" alt="Debian" border="0"></a></td>'

wget -O- "$debian_package_url" 2>/dev/null | egrep '(all|i386)' | \
    awk '{ gsub(".*:","",$3); gsub("-[^-]*$","",$3); print $5 " " $3}' | \
    sort --key=2 -rn | fgrep -v 2.1 | fgrep -v m68k | \
    while read dv rv; do
    if [ -n "$rv" ]; then
	dvn=`echo $dv | cut -c1 | tr '[a-z]' '[A-Z]'``echo $dv | cut -c2-`
	echo "</tr><tr>"
	echo "<td class=b>&nbsp;Debian $dvn&nbsp;</td>"
	echo "<td class=b>&nbsp;Rosegarden v$rv&nbsp;</td>"
	echo "<td class=b></td>"
    fi
    done

echo '</tr>'

for x in `seq 1 $maxrelease` ; do echo '<tr></tr>'; done


# OpenSUSE Packman

packman_package_url='http://packman.links2linux.org/package/rosegarden4'

echo '<tr><td>&nbsp;</td></tr>'
echo '<tr>'
echo '<td class="b" align="center" rowspan="'$maxrelease'"><a href="http://en.opensuse.org/"><img src="http://en.opensuse.org/skins/opensuse/images/common/geeko.jpg" alt="OpenSUSE" border="0"></a></td>'

packman_version=`wget -O- "$packman_package_url" 2>/dev/null | fgrep src.rpm | sed -e 's/^.*rosegarden4*-//' -e 's/-[0-9]\.pm.*//'`

case "$packman_version" in
    [0-9]*)
    	echo '</tr><tr>'
	echo "<td class=a>&nbsp;OpenSUSE&nbsp;</td>"
	echo "<td class=a>&nbsp;Rosegarden v$packman_version&nbsp;</td>"
	echo "<td class=a>Packman repository</td>"
	;;
esac
echo "</tr>"
for x in `seq 1 $maxrelease` ; do echo '<tr></tr>'; done


# Arch Linux


arch_package_url='http://www.archlinux.org/packages/6128/'

echo '<tr><td>&nbsp;</td></tr>'
echo '<tr>'
echo '<td class="b" align="center" rowspan="'$maxrelease'"><a href="http://www.archlinux.org/"><img src="http://www.archlinux.org/media/titlelogo.png" alt="Arch Linux" width="175" height="51" border="0"></a></td>'

arch_version=`wget -O- "$arch_package_url" 2>/dev/null |  grep 'osegarden 4\.[0-9]\.' | head -1 | sed -e 's/^.*rosegarden 4\.\([0-9\.]*\).*$/\1/'`

case "$arch_version" in
    [0-9]*)
        echo '</tr><tr>'
        echo "<td class=a>&nbsp;Arch Linux&nbsp;</td>"
        echo "<td class=a>&nbsp;Rosegarden v$arch_version&nbsp;</td>"
        echo "<td class=a>Extra repository</td>"
        ;;
esac
echo "</tr>"
for x in `seq 1 $maxrelease` ; do echo '<tr></tr>'; done


# Gentoo

gentoo_package_url='http://gentoo-portage.com/media-sound/rosegarden/'

echo '<tr><td>&nbsp;</td></tr>'
echo '<tr>'
echo '<td class="b" align="center" rowspan="'$maxrelease'"><a href="http://www.gentoo.org/"><img src="http://www.gentoo.org/images/gentoo-new.gif" alt="Gentoo" border="0"></a></td>'

gentoo_version=`wget -O- "$gentoo_package_url" 2>/dev/null |  grep 'rosegarden-[0-9]\.' | head -1 | sed -e 's/^.*rosegarden-\([0-9\.]*\).*$/\1/'`

case "$gentoo_version" in
    [0-9]*)
        echo '</tr><tr>'
        echo "<td class=a>&nbsp;Gentoo&nbsp;</td>"
        echo "<td class=a>&nbsp;Rosegarden v$gentoo_version&nbsp;</td>"
        echo "<td class=a>Portage</td>"
        ;;
esac
echo "</tr>"
for x in `seq 1 $maxrelease` ; do echo '<tr></tr>'; done


