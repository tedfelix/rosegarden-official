#!/bin/sh

rm -rf reorganise_tmp
cp -Rp pristine reorganise_tmp || exit 1
cd reorganise_tmp || exit 1

startdate=`date`
echo Starting at $startdate... 1>&2    

#mkdir data
#mv chords data/chords
#mv gui/testfiles data/examples
#mv gui/fonts data/fonts
#mv gui/library data/library
#mv gui/pixmaps data/pixmaps
#mv presets data/presets
#mv gui/styles data/styles
#mkdir data/desktop
#mv gui/*.desktop data/desktop/
#mv gui/docs .

#mkdir src
#mv base src/base
#rm -rf src/base/old
#mv sound src/sound
#mv sequencer src/sequencer

#for x in document document/io commands gui helpers; do
#    mkdir -p src/$x
#done
#for x in audio edit event matrix notation segment studio; do
#    mkdir -p src/commands/$x
#done
#for x in application configuration dialogs editors general kdeext rulers seqmanager studio ui widgets; do
#    mkdir -p src/gui/$x
#done
#for x in eventlist matrix notation parameters segment guitar markers tempo; do
#    mkdir -p src/gui/editors/$x
#done

#mkdir src/misc

cd gui
s=../src
g=$s/gui
#mv *.rc *.ui $g/ui/
#mv kde*.{cpp,h} ktmp*.{cpp,h} kstart*.{cpp,h} rgled.cpp qcanvas*.{cpp,h} $g/kdeext/
#mv rosegarden-lilypondview rosegarden-project-package $s/helpers/

mv kstartuplogo.cpp $g/kdeext/KStartupLogo.cpp
mv kstartuplogo.h $g/kdeext/KStartupLogo.h
mv ktmpstatusmsg.cpp $g/kdeext/KTmpStatusMsg.cpp
mv ktmpstatusmsg.h $g/kdeext/KTmpStatusMsg.h
mv rgled.cpp $g/kdeext/RGLed.cpp
mv qcanvasgroupableitem.cpp $g/kdeext/QCanvasGroupableItem.cpp
mv qcanvasgroupableitem.h $g/kdeext/QCanvasGroupableItem.h
mv qcanvassimplesprite.cpp $g/kdeext/QCanvasSimpleSprite.cpp
mv qcanvassimplesprite.h $g/kdeext/QCanvasSimpleSprite.h

mv clefindex.h $g/general/ClefIndex.h
mv constants.h $s/document/ConfigGroups.h
mv notecharname.h $g/editors/notation/NoteCharacterNames.h
mv notecharname.cpp $g/editors/notation/NoteCharacterNames.cpp
mv rosegardendcop.h $g/application/RosegardenDCOP.h

#cd gui
#mkdir -p ui qt-kde-ext
#mkdir -p widgets dialogs commands io rulers configuration editors/notation editors/matrix editors/eventlist editors/parameters editors/tempoview application studio editors/segment seqmanager general
#mv *.rc *.ui ui/
#mv kde*.{cpp,h} ktmp*.{cpp,h} kstart*.{cpp,h} rgled.cpp qcanvas*.{cpp,h} qt-kde-ext/

QTINCDIR=/usr/include/qt3
KDEINCDIR=/usr/include/kde

extract_class_from_header() {
    _seek=$1; shift
    _file=$1; shift
    _sym=`echo "$_seek" | tr '[a-z]' '[A-Z]'`
    echo "//replacement_candidate!"
    echo "//rubric!"
    echo "#ifndef _RG_${_sym}_H_"
    echo "#define _RG_${_sym}_H_"
    echo
    echo "//inc!"
    echo
    echo "//dec!"
    echo
    echo "namespace Rosegarden"
    echo "{"
    echo
    echo "//ndec!"
    echo
    grep -v '^namespace .*{' $_file | perl -e '
	$seek="'$_seek'";
	@classdata=();
	$ifnest=0;
	$havetemplate=0;
	while (<>) { 
	    if (/^#if/ and $inclass) {
		$ifnest++;
	    }
	    if (/^} *$/ and $havetemplate == 0) {
		next;
	    }
	    if (/^\/\*\*/ or (/^#endif/ and $ifnest == 0) or (/^template / and $havetemplate == 0) or /^(class|struct) [^;]*$/ or /^\/\/\//) { 
		$ifnest=0;
		if ($rightclass) { 
		    map { print } @classdata;
		};
		if (/^template /) {
		    $havetemplate=1;
		}
		$inclass=1;
		if (/^(class|struct) /) {
		    if ($classname ne "") {
			@classdata=();
	            }
		    $classname=$_;
		    $classname =~ s/^(class|struct)  *([^ :]*).*$/$2/;
		    chomp $classname;
		    if ($classname eq $seek){
			$rightclass=1;
		    } else {
			$rightclass=0;
		    }
		} else {
		    $classname="";
		    $rightclass=0;
		    @classdata=();
		}
	    }
	    if (/^#endif/ and $ifnest > 0) {
		$ifnest--;
            }
	    if ($inclass) {
		push @classdata, $_;
	    }
	} 
	if ($rightclass) {
	    map { print } @classdata;
	}'
    echo
    echo "}"
    echo
    echo "#endif"
}

extract_class_from_cpp() {
    _seek=$1; shift
    _file=$1; shift
    echo "//rubric!"
    echo
    echo "#include \"${_seek}.h\""
    echo
    echo "//inc!"
    echo
    echo "namespace Rosegarden"
    echo "{"
    perl -e '
	$seek="'$_seek'";
	@classdata=();
	@provisional=();
	$rightclass=0;
	$inclass=0;
	while (<>) { 
	    if (/^([a-zA-Z:]+ )?$seek\:\:/) { 
		push @classdata, "\n";
		push @classdata, @provisional;
		$inclass=1;
		$rightclass=1;
	    } elsif (/^([a-zA-Z:]+ )?[a-zA-Z]+\:\:/ and !/^Rosegarden::/) { 
		$inclass=1;
		@provisional=();
	    } elsif (/^}/) {
		if ($rightclass) { push @classdata, $_; }
		$rightclass=0;
		$inclass=0;
		@provisional=();
	    } elsif (/^[a-zA-Z][^;]*$/ and !$inclass) {
		push @provisional, $_;
	    }
	    if ($rightclass) { push @classdata, $_; }
        }
	map { print } @classdata;
    ' $_file
    echo 
    echo "}"
}

class_header_cache=/tmp/$$_chc

get_header_for_class() {
    _seek=$1; shift
    _cwd=$1
    [ "$_seek" = "Qt" ] && return
    [ "$_seek" = "Rosegarden" ] && return

#    echo "Looking for $_seek..." 1>&2

    _cached=`grep "^$_seek:" $class_header_cache`
    if [ -n "$_cached" ]; then
	_cached="${_cached##$_seek:}"
	_cached="${_cached##$_cwd/}"
	_cached="${_cached##../}"
	echo "$_cached"
#        echo "Found in cache" 1>&2
	return
    fi

#    echo "Not cached" 1>&2

    for _x in */$_seek.h */*/$_seek.h */*/*/$_seek.h; do
	if [ -f "$_x" ]; then
	    echo "$_seek:$_x" >> $class_header_cache
	    _result="${_x##$_cwd/}"
	    echo $_result
	    return
	fi
    done
#    for _x in ../*/$_seek.h; do
#	if [ -f "$_x" ]; then
#	    echo "$_seek:$_x" >> $class_header_cache
#	    _result="${_x##../}"
#	    echo $_result
#	    return
#	fi
#    done
    _lc=`echo "$_seek" | tr '[A-Z]' '[a-z]'`
    for _d in $KDEINCDIR $QTINCDIR `pwd`/qt-kde-ext; do
	if [ -f "$_d/${_lc}.h" ]; then
	    _result="$_d/${_lc}.h"
	    echo "$_seek:$_result" >> $class_header_cache
	    echo $_result
	    return
        fi
    done
##    for _x in ../*/*.h; do
##	if [ -f "$_x" ]; then
##	    if egrep "class .*$_seek[^;]*$" $_x | grep -vq "class .*$_seek[a-zA-Z]" ; then
##		_result="${_x#../}"
##		echo "$_seek:$_result" >> $class_header_cache
##		echo $_result
##		return
##	    fi
##	fi
##    done

#    echo Done 1>&2
    echo "$_seek:" >> $class_header_cache
}

add_includes() {
    _dest=$1; shift
    _includes="$@"
    _bd=`basename $_dest`
    _includes="`echo $_includes | fmt -1 | grep -v $_bd | sort`"
    for _i in $_includes; do
	if [ "${_i##/}" = "$_i" ]; then
	    _line="#include \"$_i\""
	else
	    _line="#include <`basename $_i`>"
	fi
	if ! grep -q "$_line" "$_dest"; then
	    perl -i -p -e "s,^//inc!,$_line\n//inc!," $_dest
	fi
    done
}

add_declarations() {
    _dest=$1; shift
    _decs="$@"
    for _i in $_decs; do
	_line="class $_i;"
	perl -i -p -e "s,^//dec!,$_line\n//dec!," $_dest
    done
}

add_declarations_in_namespace() {
    _dest=$1; shift
    _decs="$@"
    for _i in $_decs; do
	_line="class $_i;"
	perl -i -p -e "s,^//ndec!,$_line\n//ndec!," $_dest
    done
}

add_include_if_tag() {
    _file=$1; shift
    _include=$1; shift
    _tags="$@"
    for _tag in $_tags; do
	if grep -q "$_tag" "$_file"; then
	    add_includes "$_file" "$_include"
	    return 0
	fi
    done
    return 1
}

add_include_if_tag_both() {
    __hfile=$1; shift
    __cfile=$1; shift
    __include=$1; shift
    __tags="$@"
    if add_include_if_tag "$__hfile" "$__include" $__tags; then :;
    elif [ -f "$__cfile" ]; then add_include_if_tag "$__cfile" "$__include" $__tags; fi
}

add_rubric() {
    _dest=$1
    perl -i -p -e "s/^\/\/rubric!/\/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: *\/\n\n\/*\n    Rosegarden\n    A MIDI and audio sequencer and musical notation editor.\n\n    This program is Copyright 2000-2006\n        Guillaume Laurent   <glaurent\@telegraph-road.org>,\n        Chris Cannam        <cannam\@all-day-breakfast.com>,\n        Richard Bown        <richard.bown\@ferventsoftware.com>\n\n    The moral rights of Guillaume Laurent, Chris Cannam, and Richard\n    Bown to claim authorship of this work have been asserted.\n\n    Other copyrights also apply to some parts of this work.  Please\n    see the AUTHORS file and individual file headers for details.\n\n    This program is free software; you can redistribute it and\/or\n    modify it under the terms of the GNU General Public License as\n    published by the Free Software Foundation; either version 2 of the\n    License, or (at your option) any later version.  See the file\n    COPYING included with this distribution for more information.\n*\/\n/" $_dest
}

replacements=""

echo commands: `date`... 1>&2

commandfiles="*commands.h"

for file in $commandfiles; do 
    classes=`egrep '^(class|struct)' $file | grep -v ';' | sed -e 's/^class //' -e 's/^struct //' | sed 's/:.*$//'`
    target=`echo $file | sed 's/command.*$//'`
#    if [ "$target" = "basic" ]; then target="base"; fi
    mkdir -p "$s/commands/$target"
    for class in $classes; do
	targetfile=$class
	if echo $class | grep -q '^[A-Za-z][A-Za-z]*Menu[A-Za-z]'; then
	    targetfile=`echo $class | sed 's/^[A-Za-z]*Menu//'`
	    replacements="$replacements $class,$targetfile"
	fi
	extract_class_from_header "$class" "$file" > "$s/commands/$target/$targetfile.h"
	extract_class_from_cpp "$class" "`basename $file .h`.cpp" > "$s/commands/$target/$targetfile.cpp"
    done
    rm $file "`basename $file .h`.cpp"
done

echo widgets: `date`... 1>&2

widgetfiles="*widget*.h scrollbox.h vumeter.h zoomslider.h qdeferscrollview.h eventfilter.h collapsingframe.h " 

for file in $widgetfiles; do 
    classes=`egrep '^(class|struct)' $file | grep -v ';' | sed -e 's/^class //' -e 's/^struct //' | sed 's/:.*$//'`
    for class in $classes; do
	targetfile=$class
	if echo $class | grep -q '^Rosegarden[A-Za-z]'; then
	    targetfile=`echo $class | sed 's/^Rosegarden//'`
	    replacements="$replacements $class,$targetfile"
	fi
	extract_class_from_header "$class" "$file" > "$g/widgets/$targetfile.h"
	extract_class_from_cpp "$class" "`basename $file .h`.cpp" > "$g/widgets/$targetfile.cpp"
    done
    rm $file "`basename $file .h`.cpp"
done

echo dialogs: `date`... 1>&2

dialogfiles="*dialog*.h " 

for file in $dialogfiles; do 
    classes=`egrep '^(class|struct)' $file | grep -v ';' | sed -e 's/^class //' -e 's/^struct //' | sed 's/:.*$//'`
    for class in $classes; do
	targetfile=$class
	if echo $class | grep -q '^Rosegarden[A-Za-z]'; then
	    targetfile=`echo $class | sed 's/^Rosegarden//'`
	    replacements="$replacements $class,$targetfile"
	fi
	extract_class_from_header "$class" "$file" > "$g/dialogs/$targetfile.h"
	extract_class_from_cpp "$class" "`basename $file .h`.cpp" > "$g/dialogs/$targetfile.cpp"
    done
    rm $file "`basename $file .h`.cpp"
done

echo io: `date`... 1>&2

iofiles="*io.h"

for file in $iofiles; do 
    classes=`egrep '^(class|struct)' $file | grep -v ';' | sed -e 's/^class //' -e 's/^struct //' | sed 's/:.*$//'`
    for class in $classes; do
	targetfile=$class
	extract_class_from_header "$class" "$file" > "$s/document/io/$targetfile.h"
	extract_class_from_cpp "$class" "`basename $file .h`.cpp" > "$s/document/io/$targetfile.cpp"
    done
    rm $file "`basename $file .h`.cpp"
done

echo ruler: `date`... 1>&2

rulerfiles="*ruler.h tempocolour.h velocitycolour.h"

for file in $rulerfiles; do 
    classes=`egrep '^(class|struct)' $file | grep -v ';' | sed -e 's/^class //' -e 's/^struct //' | sed 's/:.*$//'`
    for class in $classes; do
	targetfile=$class
	extract_class_from_header "$class" "$file" > "$g/rulers/$targetfile.h"
	extract_class_from_cpp "$class" "`basename $file .h`.cpp" > "$g/rulers/$targetfile.cpp"
    done
    rm $file "`basename $file .h`.cpp"
done

echo notation: `date`... 1>&2

notationfiles="not*.h"

for file in $notationfiles; do 
    classes=`egrep '^(class|struct)' $file | grep -v ';' | sed -e 's/^class //' -e 's/^struct //' | sed 's/:.*$//'`
    for class in $classes; do
	targetfile=$class
	extract_class_from_header "$class" "$file" > "$g/editors/notation/$targetfile.h"
	extract_class_from_cpp "$class" "`basename $file .h`.cpp" > "$g/editors/notation/$targetfile.cpp"
	if [ "$file" = "notationview.h" ]; then
	    extract_class_from_cpp "$class" "notationviewslots.cpp" >> "$g/editors/notation/$targetfile.cpp"
	fi
    done
    rm $file "`basename $file .h`.cpp"
done
rm notationviewslots.cpp

echo matrix: `date`... 1>&2

matrixfiles="matrix*.h pianokeyboard.h"

for file in $matrixfiles; do 
    classes=`egrep '^(class|struct)' $file | grep -v ';' | sed -e 's/^class //' -e 's/^struct //' | sed 's/:.*$//'`
    for class in $classes; do
	targetfile=$class
	extract_class_from_header "$class" "$file" > "$g/editors/matrix/$targetfile.h"
	extract_class_from_cpp "$class" "`basename $file .h`.cpp" > "$g/editors/matrix/$targetfile.cpp"
    done
    rm $file "`basename $file .h`.cpp"
done

echo preferences: `date`... 1>&2

preferencesfiles="*config*.h"

for file in $preferencesfiles; do 
    classes=`egrep '^(class|struct)' $file | grep -v ';' | sed -e 's/^class //' -e 's/^struct //' | sed 's/:.*$//'`
    for class in $classes; do
	targetfile=$class
	if echo $class | grep -q '^Rosegarden[A-Za-z]'; then
	    targetfile=`echo $class | sed 's/^Rosegarden//'`
	    replacements="$replacements $class,$targetfile"
	fi
	extract_class_from_header "$class" "$file" > "$g/configuration/$targetfile.h"
	extract_class_from_cpp "$class" "`basename $file .h`.cpp" > "$g/configuration/$targetfile.cpp"
    done
    rm $file "`basename $file .h`.cpp"
done

echo event: `date`... 1>&2

eventfiles="event*.h"

for file in $eventfiles; do 
    classes=`egrep '^(class|struct)' $file | grep -v ';' | sed -e 's/^class //' -e 's/^struct //' | sed 's/:.*$//'`
    for class in $classes; do
	targetfile=$class
	extract_class_from_header "$class" "$file" > "$g/editors/eventlist/$targetfile.h"
	extract_class_from_cpp "$class" "`basename $file .h`.cpp" > "$g/editors/eventlist/$targetfile.cpp"
    done
    rm $file "`basename $file .h`.cpp"
done

echo tempo: `date`... 1>&2

tempofiles="tempoview.h"

for file in $tempofiles; do 
    classes=`egrep '^(class|struct)' $file | grep -v ';' | sed -e 's/^class //' -e 's/^struct //' | sed 's/:.*$//'`
    for class in $classes; do
	targetfile=$class
	extract_class_from_header "$class" "$file" > "$g/editors/tempo/$targetfile.h"
	extract_class_from_cpp "$class" "`basename $file .h`.cpp" > "$g/editors/tempo/$targetfile.cpp"
    done
    rm $file "`basename $file .h`.cpp"
done

echo parameter: `date`... 1>&2

parameterfiles="segmentparameter*.h trackparameterbox.h instrumentparameterbox.h rosegardenparameterarea.h rosegardenparameterbox.h"

for file in $parameterfiles; do 
    classes=`egrep '^(class|struct)' $file | grep -v ';' | sed -e 's/^class //' -e 's/^struct //' | sed 's/:.*$//' | sed 's/{/ /g'`
    for class in $classes; do
	targetfile=$class
	extract_class_from_header "$class" "$file" > "$g/editors/parameters/$targetfile.h"
	extract_class_from_cpp "$class" "`basename $file .h`.cpp" > "$g/editors/parameters/$targetfile.cpp"
    done
    rm $file "`basename $file .h`.cpp"
done

echo segment: `date`... 1>&2

segmentfiles="track*.h segment[^c]*.h audiopr*.h composition*.h barbuttons.h instrumentparameterbox.h markereditor.h playlist.h triggermanager.h controleditor.h"

for file in $segmentfiles; do 
    classes=`egrep '^(class|struct)' $file | grep -v ';' | sed -e 's/^class //' -e 's/^struct //' | sed 's/:.*$//' | sed 's/{/ /g'`
    for class in $classes; do
	targetfile=$class
	extract_class_from_header "$class" "$file" > "$g/editors/segment/$targetfile.h"
	extract_class_from_cpp "$class" "`basename $file .h`.cpp" > "$g/editors/segment/$targetfile.cpp"
    done
    rm $file "`basename $file .h`.cpp"
done

echo main: `date`... 1>&2

docfiles="rosegardenguidoc.h rosexmlhandler.h xmlstorableevent.h multivi*.h basiccommand* "

for file in $docfiles; do 
    classes=`egrep '^(class|struct)' $file | grep -v ';' | sed -e 's/^class //' -e 's/^struct //' | sed 's/:.*$//'`
    for class in $classes; do
	targetfile=$class
	extract_class_from_header "$class" "$file" > "$s/document/$targetfile.h"
	extract_class_from_cpp "$class" "`basename $file .h`.cpp" > "$s/document/$targetfile.cpp"
    done
    rm $file "`basename $file .h`.cpp"
done

mainfiles="rgapplication.h rosegardenguidoc.h rosegardengui.h rosegardenguiiface.h rosegardenguiview.h lirc*.h rosexmlhandler.h xmlstorableevent.h rosegardendcop.h startuptester.h"

for file in $mainfiles; do 
    classes=`egrep '^(class|struct)' $file | grep -v ';' | sed -e 's/^class //' -e 's/^struct //' | sed 's/:.*$//'`
    for class in $classes; do
	targetfile=$class
	extract_class_from_header "$class" "$file" > "$g/application/$targetfile.h"
	extract_class_from_cpp "$class" "`basename $file .h`.cpp" > "$g/application/$targetfile.cpp"
    done
    rm $file "`basename $file .h`.cpp"
done

mv main.cpp $g/application/

echo studio: `date`... 1>&2

studiofiles="audio[^cp]*.h audiopl*.h device*.h bank*.h *mixer*.h studio*.h"

for file in $studiofiles; do 
    classes=`egrep '^(class|struct)' $file | grep -v ';' | sed -e 's/^class //' -e 's/^struct //' | sed 's/:.*$//'`
    for class in $classes; do
	targetfile=$class
	extract_class_from_header "$class" "$file" > "$g/studio/$targetfile.h"
	extract_class_from_cpp "$class" "`basename $file .h`.cpp" > "$g/studio/$targetfile.cpp"
    done
    rm $file "`basename $file .h`.cpp"
done

echo seqmanager: `date`... 1>&2

seqmanagerfiles="sequence*.h mmap*.h midif*.h"

for file in $seqmanagerfiles; do 
    classes=`egrep '^(class|struct)' $file | grep -v ';' | sed -e 's/^class //' -e 's/^struct //' | sed 's/:.*$//'`
    for class in $classes; do
	targetfile=$class
	extract_class_from_header "$class" "$file" > "$g/seqmanager/$targetfile.h"
	extract_class_from_cpp "$class" "`basename $file .h`.cpp" > "$g/seqmanager/$targetfile.cpp"
    done
    rm $file "`basename $file .h`.cpp"
done

echo general: `date`... 1>&2

generalfiles="hzoomable.h colours.h constants.h edit*.h lined*.h pixmapf*.h progres*.h qcanvas*.h rosegardenscrollview.h spline.h staffline.h rosegardencanvasview.h clefindex.h midipitchlabel.h presethandler.h"

for file in $generalfiles; do 
    classes=`egrep '^(class|struct)' $file | grep -v ';' | sed -e 's/^class //' -e 's/^struct //' | sed 's/:.*$//'`
    for class in $classes; do
	targetfile=$class
	extract_class_from_header "$class" "$file" > "$g/general/$targetfile.h"
	extract_class_from_cpp "$class" "`basename $file .h`.cpp" > "$g/general/$targetfile.cpp"
    done
    rm $file "`basename $file .h`.cpp"
done

echo file: `date`... 1>&2

for file in guitar/*.h ; do
    class=`grep 'class .*[^;]$' $file | head -1 | sed -e 's/^.*class //' -e 's/ :.*//'`
    if [ -n "$class" ]; then
	mv $file $g/editors/guitar/$class.h
	if [ -f guitar/"`basename $file .h`.cpp" ]; then
	   mv guitar/"`basename $file .h`.cpp" $g/editors/guitar/$class.cpp
	fi
    fi
done
mv guitar/*.{cpp,h} $g/editors/guitar/

cd ../src

echo Doing replacements at `date`... 1>&2    

candidate_h=`grep -l '//replacement_candidate!' */*.h */*/*.h */*/*/*.h`
candidate_cpp=`grep -l '//replacement_candidate!' */*.cpp */*/*.cpp */*/*/*.cpp`

perlcommand="s/Rosegarden:://g"
for replacement in $replacements; do
    before=${replacement%%,*}
    after=${replacement##*,}
    perlcommand="s/$before/$after/g ; $perlcommand"
done

for file in */*.{cpp,h} */*/*.{cpp,h} */*/*/*.{cpp,h}; do
    perl -i -p -e "$perlcommand" $file
done

echo Adding headers at `date`... 1>&2    

for x in {base,sound,sequencer}/*.h ; do
    egrep "^class [A-Za-z_0-9][^:;]*" $x | \
	grep -v ';' | \
	sed -e 's/^class //' -e 's/[:;{</].*//' | \
	sort | uniq | \
	sed "s, *\$,:$x,"
    egrep '^typedef ([A-Za-z0-9_]+ +)+[A-Za-z][A-Za-z0-9_]* *; *$' $x | \
	sed 's/^.* \([A-Za-z0-9_]*\) *; *$/\1/' | \
	sed "s, *\$,:$x,"
done > $class_header_cache

for hfile in $candidate_h ; do 

    dir=`dirname $hfile`
    inc=""
    dec=""
    ndec=""
    classes=""

    for stdclass in `\
	cat $hfile | tr '\t' ' ' | egrep -v '^ */[/\*]' | \
	egrep ' std::[a-z_]+[ <\*]' | \
	sed 's/^ *//' | \
	fmt -1 | \
	grep '^std::' | \
	sed 's/^std::\([a-z_][a-z_]*\).*/\1/' | \
	sed 's/pair/utility/' | \
	sed 's/^multi//' | \
	sort | uniq`; do
	inc="/$stdclass $inc"
	classes=" $stdclass $classes "
    done

    for baseclass in `\
	cat $hfile | tr '\t' ' ' | egrep -v '^ */[/\*]' | \
	egrep '[^\/\*]+(public|protected|private) [A-Za-z:]+,?' | \
	sed -e 's/^[^:]*://' -e 's/[^A-Za-z: ]//g' -e 's/virtual\|public\|protected\|private//g'`; do
	[ "$baseclass" = "KDockMainWindow" ] && baseclass=KDockWidget
	[ "$baseclass" = "KNamedCommand" ] && baseclass=KCommand
	[ "$baseclass" = "KProgressDialog" ] && baseclass=KProgress
	header="`get_header_for_class $baseclass $dir`"
	inc="$header $inc"
	classes=" $baseclass $classes "
    done

    for castclass in `\
	cat $hfile | grep '_cast<[A-Za-z0-9:_]* *\*\**>' | \
	sed 's/^.*_cast<\([A-Za-z0-9:_]*\) *\*\**>.*$/\1/'`; do
	header="`get_header_for_class $castclass $dir`"
	inc="$header $inc"
	classes=" $castclass $classes "
    done

    for containerclass in `\
	cat $hfile | grep '[^A-Za-z0-9_][A-Z][A-Za-z_0-9]*<' | \
	sed 's/^.*[^A-Za-z_0-9]\([A-Z][A-Za-z_0-9]*\)<.*$/\1/'`; do
	header="`get_header_for_class $containerclass $dir`"
	inc="$header $inc"
	classes=" $containerclass $classes "
    done

    for potentialclass in `\
	cpp $hfile 2>/dev/null | \
	egrep '[A-Z][a-z]' | \
	grep -v 'case ' | \
	grep -v 'class ' | \
	egrep -v '(public|protected|private) ' | \
	sed 's/::[A-Z][a-zA-Z]*/ /g' | \
	sed 's/\([:(]\)/ \1 /g' | \
	sed 's/  *\([&*]\)/\1/g' | \
	fmt -1 | \
	grep -v '[&*]' | \
	sed 's/^ *//' | \
	egrep '^([A-Z]+[a-z]+)+$' | sort | uniq`; do
	if ! echo " $classes " | grep -q " $potentialclass " ; then
	    header="`get_header_for_class $potentialclass $dir`"
	    if [ -n "$header" ]; then inc="$header $inc"; fi
	    classes=" $potentialclass $classes "
	fi
    done

    for declclass in `\
	cat $hfile | tr '\t' ' ' | egrep -v '^ */[/\*]' | \
	egrep '[A-Za-z_] *[\*&]' | \
	sed 's/  *\([\*&]\)/\1 /g' | \
	sed 's/^ *//' | \
	sed 's/[^A-Za-z_\*&]/ /g' | \
	fmt -1 | \
	egrep '[A-Z].*[\*&]$' | sed 's/[\*&]$//' | sort | uniq`; do
	[ "$declclass" = "timeT" ] && continue
	[ "$declclass" = "MappedObjectProperty" ] && continue
	if ! echo " $classes " | grep -q " $declclass " ; then
	    header="`get_header_for_class $declclass $dir`"
	    if [ -n "$header" ]; then
		if [ "${header##/}" = "$header" ]; then
		    ndec="$declclass $ndec"
		else
		    dec="$declclass $dec"
		fi
	    else
		dec="$declclass $dec"
	    fi
	    classes=" $declclass $classes "
        fi
    done

    echo "Includes for $hfile: $inc"
    add_includes $hfile $inc

    echo "Declarations for $hfile: $dec"
    add_declarations $hfile $dec

    echo "Declarations in namespace for $hfile: $ndec"
    add_declarations_in_namespace $hfile $ndec

    add_rubric $hfile

    cfile=`dirname $hfile`/`basename $hfile .h`.cpp

    add_include_if_tag_both $hfile $cfile "base/Event.h" timeT
    add_include_if_tag_both $hfile $cfile "/qxml.h" QXml
    add_include_if_tag_both $hfile $cfile "sound/Midi.h" MIDI_
    add_include_if_tag_both $hfile $cfile "gui/editors/segment/TrackEditor.h" getTrackEditor
    add_include_if_tag_both $hfile $cfile "gui/editors/segment/TrackButtons.h" getTrackButtons
    add_include_if_tag_both $hfile $cfile "base/BaseProperties.h" BaseProperties
    add_include_if_tag_both $hfile $cfile "/klocale.h" i18n
    add_include_if_tag_both $hfile $cfile "/kstddirs.h" KGlobal::dirs
    add_include_if_tag_both $hfile $cfile "misc/Debug.h" RG_DEBUG
    add_include_if_tag_both $hfile $cfile "misc/Strings.h" qstrtostr strtoqstr
    add_include_if_tag_both $hfile $cfile "gui/general/ClefIndex.h" TrebleClef BassClef CrotalesClef XylophoneClef GuitarClef ContrabassClef CelestaClef OldCelestaClef SopranoClef AltoClef TenorClef TwoBarClef   
    add_include_if_tag_both $hfile $cfile "document/ConfigGroups.h" ConfigGroup
    add_include_if_tag_both $hfile $cfile "gui/editors/notation/NoteCharacterNames.h" NoteCharacterNames CharName
    add_include_if_tag_both $hfile $cfile "gui/application/RosegardenDCOP.h" APP_NAME IFACE_NAME TransportStatus STOPPED PLAYING RECORDING STOPPING STARTING

    if [ -f "$cfile" ]; then

	add_rubric $cfile

	qkclasses=`cat $cfile | \
	    egrep '[QK][A-Za-z0-9]' | \
	    sed 's/\([^A-Za-z0-9_]\)/ \1 /g' | \
	    fmt -1 | \
	    sed 's/^ *//' | \
	    egrep '^[QK][A-Za-z0-9]+$' | sort | uniq`

	potentialclasses=`cpp $cfile 2>/dev/null | \
	    egrep '[A-Z][a-z]' | \
	    grep -v 'case ' | \
	    sed 's/::[A-Z][a-zA-Z]*/ /g' | \
	    sed 's/\([:(]\)/ \1 /g' | \
	    fmt -1 | \
	    sed 's/^ *//' | \
	    egrep '^([A-Z]+[a-z]+)+$' | sort | uniq`

	inc=""

	for class in $ndec $dec $qkclasses $potentialclasses; do
	    header=`get_header_for_class "$class" "$dir"`
	    if [ -n "$header" ]; then
		inc="$header $inc"
	    fi
	done

	echo "Includes for $cfile: $inc"
	add_includes $cfile $inc
    fi
done

mv ../gui/rosedebug.h misc/Debug.h
cat ../gui/rosedebug.cpp | sed -e 's/#include "\([A-Z]\)/#include "base\/\1/' -e 's/rosestrings.h/Strings.h/' -e 's/rosedebug.h/Debug.h/' > misc/Debug.cpp

mv ../gui/rosestrings.h misc/Strings.h
cat ../gui/rosestrings.cpp | sed -e 's/#include "\([A-Z]\)/#include "base\/\1/' -e 's/rosestrings.h/Strings.h/' -e 's/rosedebug.h/Debug.h/' > misc/Strings.cpp

add_includes document/MultiViewCommandHistory.cpp /kpopupmenu.h
add_includes gui/application/RosegardenGUIApp.h sound/AudioFile.h
add_includes gui/studio/AudioPluginManager.h AudioPlugin.h
add_includes document/RosegardenGUIDoc.cpp gui/widgets/ProgressBar.h

echo Formatting at `date`... 1>&2    

for file in */*.{cpp,h} */*/*.{cpp,h} */*/*/*.{cpp,h} ; do 
    perl -i -p -e 's,^//(inc|dec|ndec|replacement_candidate)!,,' $file
done

for file in */*.cpp */*/*.cpp */*/*/*.cpp ; do 
    astyle --style=kr --brackets=linux --pad=oper $file
    rm $file.orig
done

echo Done at `date`, having started at $startdate 1>&2    
