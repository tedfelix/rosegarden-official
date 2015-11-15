# Magic for getting the SVN revision at build time
# http://stackoverflow.com/questions/3780667/use-cmake-to-get-build-time-svn-revision
# combined with
# http://stackoverflow.com/questions/17451226/cmake-how-to-get-subversion-revision-but-not-to-fail-if-we-made-an-export?s=1%7C0.1607

# the FindSubversion.cmake module is part of the standard distribution
include(FindSubversion)

if(Subversion_FOUND)
    if(EXISTS "${SOURCE_DIR}/../.svn")
        # extract working copy information for SOURCE_DIR into MY_XXX variables
        Subversion_WC_INFO(${SOURCE_DIR} MY)
        SET(SVN_REVISION "${MY_WC_REVISION}")
    else ()
        SET(SVN_REVISION "-1")
    endif()
else()
    set(SVN_REVISION "-1")
endif()

# write a file with the define
file(WRITE svnversion.h.txt "#define BUILDKEY ${SVN_REVISION}\n")

# copy the file to the final header only if the version changes
# reduces needless rebuilds
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different svnversion.h.txt svnversion.h)

