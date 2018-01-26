#!/bin/bash

maxrelease=6

# Ubuntu

ubuntu_package_base='http://packages.ubuntu.com/'
ubuntu_package_path='/sound/rosegarden'

ubuntu_release_url='http://releases.ubuntu.com/'

ubuntu_release_description=`wget -O- "$ubuntu_release_url" 2>/dev/null | grep '<li>' | grep '[0-9] [A-Z]* *('`
ubuntu_release_shortnames=`echo "$ubuntu_release_description" | sed 's/^.*(//' | sed 's/ .*//'`

rowspan=$(($maxrelease+1))

echo '<tr><td>&nbsp;</td></tr>'
echo '<tr>'
echo '<td class="a" align="center" rowspan="'$rowspan'"><a href="http://www.ubuntu.com/"><img src="https://assets.ubuntu.com/v1/29985a98-ubuntu-logo32.png" style="width: 3em; height: 3em" alt="Ubuntu" border="0"></a></td>'

for shortname in $ubuntu_release_shortnames ; do
    rv=`wget -O- "$ubuntu_package_base$shortname$ubuntu_package_path" 2>/dev/null | \
	grep 'Package: rosegarden' | \
	sed 's/^.*1://' | \
	sed 's/[^0-9\.].*//' | \
	head -1`
    [ -z "$rv" ] && continue
    longname=`echo "$ubuntu_release_description" | grep "$shortname" | sed 's/^.*Ubuntu //' | sed 's/<.*//'`
    echo "</tr><tr><td class=a>&nbsp;Ubuntu $longname&nbsp;</td>"
    echo "<td class=a>&nbsp;Rosegarden v$rv&nbsp;</td>"
    echo "<td class=a>&nbsp;Community-supported Packages</td>"
done

echo "</tr>"

for x in `seq 1 $maxrelease` ; do echo '<tr></tr>'; done


# Debian

debian_package_url='http://qa.debian.org/madison.php?package=rosegarden&text=on'

echo '<tr><td>&nbsp;</td></tr>'
echo '<tr>'
echo '<td class="b" align="center" rowspan="'$rowspan'"><a href="http://www.debian.org/"><img src="http://www.debian.org/logos/openlogo-50.png" alt="Debian" border="0"></a></td>'

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


# OpenSUSE

opensuse_repo_url='http://download.opensuse.org/repositories/multimedia:/apps/'

echo '<tr><td>&nbsp;</td></tr>'
echo '<tr>'
echo '<td class="b" align="center" rowspan="'$rowspan'"><a href="http://en.opensuse.org/"><img src="https://en.opensuse.org/images/d/d0/Icon-distribution.png" style="width: 3em; height: 3em" alt="OpenSUSE" border="0"></a></td>'

opensuse_versions=`wget -O- "$opensuse_repo_url" 2>/dev/null | grep 'openSUSE_[A-Z0-9]' | sed 's/^.*openSUSE_//' | sed 's/[ <"\/].*//' | sort -rn`

for version in $opensuse_versions; do
    rv=`wget -O- "$opensuse_repo_url""openSUSE_$version/src" 2>/dev/null | grep rosegarden | sed 's/^.*rosegarden[0-9]*-//' | sed 's/-.*//' | head -1`
    [ -z "$rv" ] && continue
    echo "</tr><tr>"
    echo "<td class=b>&nbsp;openSUSE $version&nbsp;</td>"
    echo "<td class=b>&nbsp;Rosegarden v$rv&nbsp;</td>"
    echo "<td class=b>&nbsp;multimedia/apps</td>"
done

echo '</tr>'

for x in `seq 1 $maxrelease` ; do echo '<tr></tr>'; done



# Arch Linux


arch_package_url='http://www.archlinux.org/packages/extra/x86_64/rosegarden/'

echo '<tr><td>&nbsp;</td></tr>'
echo '<tr>'
echo '<td class="b" align="center" rowspan="'$rowspan'"><a
href="http://www.archlinux.org/"><img src="https://www.archlinux.org/static/logos/archlinux-logo-dark-scalable.518881f04ca9.svg" style="height: 2.5em; object-fit: scale-down" alt="Arch Linux" width="180" height="60" border="0"></a></td>'

arch_version=`wget -O- "$arch_package_url" 2>/dev/null |  grep 'osegarden [0-9]' | head -1 | sed -e 's/^.*rosegarden \([0-9\.]*\).*$/\1/'`

case "$arch_version" in
    [0-9]*)
        echo '</tr><tr>'
        echo "<td class=a>&nbsp;Arch Linux&nbsp;</td>"
        echo "<td class=a>&nbsp;Rosegarden v$arch_version&nbsp;</td>"
        echo "<td class=a>&nbsp;Extra repository</td>"
        ;;
esac
echo "</tr>"
for x in `seq 1 $maxrelease` ; do echo '<tr></tr>'; done


# Gentoo

gentoo_package_url='http://packages.gentoo.org/package/media-sound/rosegarden/'

echo '<tr><td>&nbsp;</td></tr>'
echo '<tr>'
echo '<td class="b" align="center" rowspan="'$rowspan'"><a href="http://www.gentoo.org/"><img src="https://assets.gentoo.org/tyrian/site-logo.svg" style="height: 2.5em; object-fit: scale-down" alt="Gentoo" border="0"></a></td>'

gentoo_version=`wget -O- "$gentoo_package_url" 2>/dev/null |  grep 'rosegarden-[0-9]' | head -1 | sed -e 's/^[^(]*rosegarden-\([0-9\.]*[0-9]\).*$/\1/'`

case "$gentoo_version" in
    [0-9]*)
        echo '</tr><tr>'
        echo "<td class=a>&nbsp;Gentoo&nbsp;</td>"
        echo "<td class=a>&nbsp;Rosegarden v$gentoo_version&nbsp;</td>"
        echo "<td class=a>&nbsp;Portage</td>"
        ;;
esac
echo "</tr>"
for x in `seq 1 $maxrelease` ; do echo '<tr></tr>'; done

