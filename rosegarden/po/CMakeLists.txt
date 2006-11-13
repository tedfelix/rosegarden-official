
# Automatic discovery of sources
FILE(GLOB PO_FILES *.po)

# DISABLED: explicit set of source files
# SET(PO_FILES
#    ca.po
#    cs.po
#    cy.po
#    de.po
#    en_GB.po
#    en.po
#    es.po
#    et.po
#    fi.po
#    fr.po
#    it.po
#    ja.po
#    nl.po
#    ru.po
#    sv.po
#    zh_CN.po
# )

ADD_TRANSLATIONS(rosegarden ${PO_FILES})
