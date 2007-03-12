# Data directory

SET(DATADIR share/apps/rosegarden)
SET(ICONDIR share/icons)
SET(MIMEDIR share/mimelnk/audio)
SET(LINKDIR share/applications)

# Guitar chord files
FILE(GLOB CHORD_FILES chords/*.xml)
INSTALL(FILES ${CHORD_FILES}
                DESTINATION ${DATADIR}/chords)

# Desktop file
INSTALL(FILES desktop/rosegarden.desktop
                DESTINATION ${LINKDIR})

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

# styles
FILE(GLOB STYLES_FILES styles/*.xml)
INSTALL(FILES ${STYLES_FILES} DESTINATION ${DATADIR}/styles)

# presets
FILE(GLOB PRESETS_FILES presets/*.xml)
INSTALL(FILES ${PRESETS_FILES} DESTINATION ${DATADIR}/presets)

# fonts
FILE(GLOB PFA_FILES fonts/*.pfa)
INSTALL(FILES ${PFA_FILES} DESTINATION ${DATADIR}/fonts)

FILE(GLOB XML_FILES fonts/mappings/*.xml)
INSTALL(FILES ${XML_FILES} DESTINATION ${DATADIR}/fonts/mappings)

FILE(GLOB XPM_FILES pixmaps/rg21/4/*.xpm)
INSTALL(FILES ${XPM_FILES} DESTINATION ${DATADIR}/fonts/rg21/4)

FILE(GLOB XPM_FILES pixmaps/rg21/8/*.xpm)
INSTALL(FILES ${XPM_FILES} DESTINATION ${DATADIR}/fonts/rg21/8)

FILE(GLOB XPM_FILES pixmaps/feta/4/*.xpm)
INSTALL(FILES ${XPM_FILES} DESTINATION ${DATADIR}/fonts/feta/4)

FILE(GLOB XPM_FILES pixmaps/feta/6/*.xpm)
INSTALL(FILES ${XPM_FILES} DESTINATION ${DATADIR}/fonts/feta/6)

FILE(GLOB XPM_FILES pixmaps/feta/8/*.xpm)
INSTALL(FILES ${XPM_FILES} DESTINATION ${DATADIR}/fonts/feta/8)

FILE(GLOB XPM_FILES pixmaps/feta/10/*.xpm)
INSTALL(FILES ${XPM_FILES} DESTINATION ${DATADIR}/fonts/feta/10)

FILE(GLOB XPM_FILES pixmaps/feta/12/*.xpm)
INSTALL(FILES ${XPM_FILES} DESTINATION ${DATADIR}/fonts/feta/12)

# pixmaps
FILE(GLOB PIXMAP_FILES pixmaps/misc/*.xpm pixmaps/misc/*.png)
INSTALL(FILES ${PIXMAP_FILES} DESTINATION ${DATADIR}/pixmaps/misc)

FILE(GLOB PIXMAP_FILES pixmaps/toolbar/*.xpm pixmaps/toolbar/*.png)
INSTALL(FILES ${PIXMAP_FILES} DESTINATION ${DATADIR}/pixmaps/toolbar)

FILE(GLOB PIXMAP_FILES pixmaps/transport/*.xpm pixmaps/transport/*.png)
INSTALL(FILES ${PIXMAP_FILES} DESTINATION ${DATADIR}/pixmaps/transport)

# Splash 
INSTALL(FILES pixmaps/splash.png DESTINATION ${DATADIR}/pixmaps)

# Examples
SET(EXAMPLE_FILES
    examples/bogus-surf-jam.rg
    examples/bwv-1060-trumpet-duet-excerpt.rg
    examples/children.rg
    examples/Djer-Fire.rg
    examples/doodle-q.rg
    examples/exercise_notation.rg
    examples/glazunov.rg
    examples/interpretation-example.rg
    examples/let-all-mortal-flesh.rg
    examples/lilypond-alternative-endings.rg
    examples/lilypond-directives.rg
    examples/mandolin-sonatina.rg
    examples/mozart-quartet.rg
    examples/notation-for-string-orchestra-in-D-minor.rg
    examples/perfect-moment.rg
    examples/ravel-pc-gmaj-adagio.rg
    examples/sonataC.rose
    examples/stormy-riders.rg
    examples/test_tuplets.rg
    examples/the-rose-garden.rg
    examples/vivaldi-cs3mv2.rg )

INSTALL(FILES ${EXAMPLE_FILES} DESTINATION ${DATADIR}/examples) 
INSTALL(FILES examples/autoload.rg DESTINATION ${DATADIR})

# Library 
FILE(GLOB RGD_FILES library/*.rgd)
INSTALL(FILES ${RGD_FILES} DESTINATION ${DATADIR}/library)
