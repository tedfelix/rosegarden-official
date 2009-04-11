# Data directory

SET(DATADIR ${KDE4DATADIR}/rosegarden)
SET(ICONDIR ${KDE4ICONDIR})
SET(MIMEDIR ${KDE4MIMEDIR}/audio)
SET(MENUDIR ${KDE4MENUDIR})

# Guitar chord files
FILE(GLOB CHORD_FILES chords/*.xml)
INSTALL(FILES ${CHORD_FILES} DESTINATION ${DATADIR}/chords)

# Desktop file
INSTALL(FILES desktop/rosegarden.desktop DESTINATION ${MENUDIR})

# MIME files
SET(MIME_FILES
        desktop/x-rosegarden21.desktop
        desktop/x-rosegarden.desktop
        desktop/x-rosegarden-device.desktop
        desktop/x-soundfont.desktop)
INSTALL(FILES ${MIME_FILES} DESTINATION ${MIMEDIR})

# Icons
INSTALL(FILES pixmaps/icons/cc-hi16-rosegarden.xpm      DESTINATION ${ICONDIR}/locolor/16x16/apps   RENAME rosegarden.xpm)
INSTALL(FILES pixmaps/icons/rg-rwb-rose3-16x16.png      DESTINATION ${ICONDIR}/hicolor/16x16/apps   RENAME rosegarden.png)
INSTALL(FILES pixmaps/icons/cc-hi32-rosegarden.xpm      DESTINATION ${ICONDIR}/locolor/32x32/apps   RENAME rosegarden.xpm)
INSTALL(FILES pixmaps/icons/rg-rwb-rose3-32x32.png      DESTINATION ${ICONDIR}/hicolor/32x32/apps   RENAME rosegarden.png)
INSTALL(FILES pixmaps/icons/rg-rwb-rose3-48x48.png      DESTINATION ${ICONDIR}/hicolor/48x48/apps   RENAME rosegarden.png)
INSTALL(FILES pixmaps/icons/rg-rwb-rose3-64x64.png      DESTINATION ${ICONDIR}/hicolor/64x64/apps   RENAME rosegarden.png)
INSTALL(FILES pixmaps/icons/rg-rwb-rose3-128x128.png    DESTINATION ${ICONDIR}/hicolor/128x128/apps RENAME rosegarden.png)

INSTALL(FILES pixmaps/icons/mm-mime-hi16-rosegarden.png DESTINATION ${ICONDIR}/hicolor/16x16/mimetypes RENAME x-rosegarden.png)
INSTALL(FILES pixmaps/icons/mm-mime-hi16-rosegarden.png DESTINATION ${ICONDIR}/locolor/16x16/mimetypes RENAME x-rosegarden.png)
INSTALL(FILES pixmaps/icons/mm-mime-hi32-rosegarden.png DESTINATION ${ICONDIR}/hicolor/32x32/mimetypes RENAME x-rosegarden.png)
INSTALL(FILES pixmaps/icons/mm-mime-hi32-rosegarden.png DESTINATION ${ICONDIR}/locolor/32x32/mimetypes RENAME x-rosegarden.png)

# Examples
SET(EXAMPLE_FILES
    examples/aveverum.rg
    examples/aylindaamiga.rg
    examples/bogus-surf-jam.rg
    examples/bwv-1060-trumpet-duet-excerpt.rg
    examples/children.rg
    examples/Djer-Fire.rg
    examples/doodle-q.rg
    examples/exercise_notation.rg
    examples/glazunov.rg
    examples/glazunov-for-solo-and-piano-with-cue.rg
    examples/headers-and-unicode-lyrics.rg
    examples/himno_de_riego.rg
    examples/interpretation-example.rg
    examples/let-all-mortal-flesh.rg
    examples/lilypond-alternative-endings.rg
    examples/lilypond-directives.rg
    examples/lilypond-staff-groupings.rg
    examples/mandolin-sonatina.rg
    examples/mozart-quartet.rg
    examples/notation-for-string-orchestra-in-D-minor.rg
    examples/perfect-moment.rg
    examples/ravel-pc-gmaj-adagio.rg
    examples/sonataC.rose
    examples/stormy-riders.rg
    examples/test_tuplets.rg
    examples/the-rose-garden.rg
    examples/vivaldi-cs3mv2.rg
    examples/vivaldi_op44_11_1.rg )

INSTALL(FILES ${EXAMPLE_FILES} DESTINATION ${DATADIR}/examples) 
INSTALL(FILES examples/autoload.rg DESTINATION ${DATADIR})

# Library 
FILE(GLOB RGD_FILES library/*.rgd)
INSTALL(FILES ${RGD_FILES} DESTINATION ${DATADIR}/library)

# KDE LIRC Profile
#INSTALL(FILES profile/rosegarden.profile.xml DESTINATION ${KDE4DATADIR}/profiles)
