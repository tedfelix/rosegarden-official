# - Try to find LIRC
# Once done this will define:
#
#  LIRC_FOUND - system has LIRC
#  LIRC_LIBRARY - LIRC library
#  LIRC_LIBS - LIRC link flags
#  LIRC_INC_DIR - LIRC include path

IF(LIRC_LIBRARY)
    SET(LIRC_FOUND TRUE)
ELSE(LIRC_LIBRARY)
    INCLUDE (CheckIncludeFile)
    INCLUDE (CheckLibraryExists)
    SET(LIRC_FOUND FALSE)
    SET(LIRC_LIBRARY)
    SET(LIRC_LIBS)
    SET(LIRC_INC_DIR)
    CHECK_INCLUDE_FILE("lirc/lirc_client.h" HAVE_LIRC_H)
    IF(HAVE_LIRC_H)
        CHECK_LIBRARY_EXISTS(lirc_client lirc_init "" HAVE_LIRC_INIT)
        IF(HAVE_LIRC_INIT)
            SET(LIRC_LIBS "lirc_client")
            FIND_PATH(LIRC_INC_DIR "lirc/lirc_client.h")
            FIND_LIBRARY(LIRC_LIBRARY NAMES lirc_client)
            IF(LIRC_INC_DIR AND LIRC_LIBRARY)
                SET(LIRC_FOUND TRUE)
            ENDIF(LIRC_INC_DIR AND LIRC_LIBRARY)
        ENDIF(HAVE_LIRC_INIT)
    ENDIF(HAVE_LIRC_H)
ENDIF(LIRC_LIBRARY)

MARK_AS_ADVANCED(LIRC_LIBRARY LIRC_LIBS)
