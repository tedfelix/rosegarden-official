MESSAGE("\n"
"Installation Summary\n"
"--------------------\n"
"\n"
"Install Directory             : ${CMAKE_INSTALL_PREFIX}\n"
"Build type                    : ${CMAKE_BUILD_TYPE}\n"
"\n"
"Xft notation font support     : ${HAVE_XFT}")
IF(WANT_LIRC)
MESSAGE(
"LIRC infrared remote support  : ${HAVE_LIRC}")
ENDIF(WANT_LIRC)
MESSAGE("")
IF(NOT WANT_SOUND)
MESSAGE("No sound support configured.")
ELSE(NOT WANT_SOUND)
MESSAGE(
"ALSA MIDI support             : ${HAVE_ALSA}\n"
"JACK audio support            : ${HAVE_JACK}\n"
"LADSPA plugin support         : ${HAVE_LADSPA}\n"
"DSSI synth plugin support     : ${HAVE_DSSI}\n"
"Custom OSC plugin GUI support : ${HAVE_LIBLO}\n"
"LRDF plugin metadata support  : ${HAVE_LRDF}")
ENDIF(NOT WANT_SOUND)

IF(NOT HAVE_XFT)
MESSAGE("\n* Score rendering quality and performance may be\n"
"improved if Xft 2.1.0 and Freetype 2 are available, to permit\n"
"Rosegarden to override the Qt font selection mechanism.  It\n"
"may not be worth trying to install them if they aren't already\n"
"present in your distribution though.")
ENDIF(NOT HAVE_XFT)

IF(NOT HAVE_ALSA)
MESSAGE("\n* Rosegarden requires the ALSA (Advanced Linux Sound Architecture) drivers\n"
"for MIDI, and the JACK audio framework for audio sequencing.\n"
"Please see the documentation at http://www.rosegardenmusic.com/getting/\n"
"for more information about these dependencies.")
ENDIF(NOT HAVE_ALSA)
		
IF(NOT HAVE_JACK)
MESSAGE("\n* Rosegarden uses the JACK audio server for audio recording and\n"
"sequencing.  See http://jackit.sf.net/ for more information about\n"
"getting and installing JACK.  If you want to use Rosegarden only\n"
"for MIDI, then you do not need JACK.")
ENDIF(NOT HAVE_JACK)

IF(NOT HAVE_LADSPA)
MESSAGE("\n* Rosegarden supports LADSPA audio plugins if available.  See\n"
"http://www.ladspa.org/ for more information about LADSPA.  To\n"
"build LADSPA support into Rosegarden, you need to make sure\n"
"you have ladspa.h available on your system.")
ENDIF(NOT HAVE_LADSPA)

IF(NOT HAVE_DSSI)
MESSAGE("\n* Rosegarden supports DSSI audio plugins if available.  See\n"
"http://dssi.sf.net/ for more information about DSSI.  To\n"
"build DSSI support into Rosegarden, you need to make sure\n"
"you have dssi.h available on your system.")
ENDIF(NOT HAVE_DSSI)

IF(NOT HAVE_LIBLO)
MESSAGE("\n* Rosegarden supports custom GUIs for DSSI (and LADSPA) plugins using\n"
"the Open Sound Control protocol, if the Lite OSC library liblo is\n"
"available.  Go to http://www.plugin.org.uk/liblo/ to obtain liblo\n"
"and http://dssi.sf.net/ for more information about DSSI GUIs.")
ENDIF(NOT HAVE_LIBLO)

IF(NOT HAVE_LRDF)
MESSAGE("\n* Rosegarden supports the LRDF metadata format for classification\n"
"of LADSPA and DSSI plugins.  This will improve the usability of\n"
"plugin selection dialogs.  You can obtain LRDF from\n"
"http://www.plugin.org.uk/lrdf/.")
ENDIF(NOT HAVE_LRDF)
MESSAGE("")
