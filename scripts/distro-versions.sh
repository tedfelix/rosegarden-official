#!/bin/bash


echo '<table cellspacing=0 border=0 cellpadding=5>'

echo '<tr><td>&nbsp;&nbsp;</td><td>&nbsp;<b>Distribution release</b>&nbsp;</td><td>&nbsp;<b>Rosegarden version</b>&nbsp;</td><td>&nbsp;<b>Available from</b>&nbsp;</td></tr>'

maxrelease=5


# Ubuntu

ubuntu_package_url='http://people.ubuntu.com/~ubuntu-archive/madison.cgi?package=rosegarden&text=on'

ubuntu_release_url='http://releases.ubuntu.com/'

ubuntu_release_description=`wget -O- "$ubuntu_release_url" 2>/dev/null | grep '<li>'`

echo '<tr><td>&nbsp;</td></tr>'
echo '<tr>'
echo '<td class="a" align="center" rowspan="'$maxrelease'"><a href="http://www.ubuntu.com/"><img src="http://www.ubuntu.com/themes/ubuntu07/images/ubuntulogo.png" alt="Ubuntu" border="0"></a></td>'

wget -O- "$ubuntu_package_url" 2>/dev/null | \
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

wget -O- "$debian_package_url" 2>/dev/null | \
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






echo '</table>'
