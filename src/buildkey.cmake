# Compute build key based on file contents

set(RG_BUILDKEY "")
find_program(SHA1SUM sha1sum)
if (SHA1SUM)
   file(READ source_files_list rg_SOURCES)
   execute_process(COMMAND sh -c "cat ${rg_SOURCES} | ${SHA1SUM}"
      OUTPUT_VARIABLE RG_SHA1SUM
      OUTPUT_STRIP_TRAILING_WHITESPACE)

   string(SUBSTRING "${RG_SHA1SUM}" 0 10 RG_BUILDKEY)
endif()

if (NOT RG_BUILDKEY)
   set(RG_BUILDKEY "Error")
endif()

#[[

# Old svn stuff.

# Magic for getting the SVN revision at build time
# http://stackoverflow.com/questions/3780667/use-cmake-to-get-build-time-svn-revision
# combined with
# http://stackoverflow.com/questions/17451226/cmake-how-to-get-subversion-revision-but-not-to-fail-if-we-made-an-export?s=1%7C0.1607

# the FindSubversion.cmake module is part of the standard distribution
include(FindSubversion)

SET(MY_WC_REVISION "")
if(Subversion_FOUND)
   if(EXISTS "${SOURCE_DIR}/../.svn")
      # extract working copy information for SOURCE_DIR into MY_XXX variables, especially MY_WC_REVISION
      Subversion_WC_INFO(${SOURCE_DIR} MY)
      message(STATUS "SVN revision ${MY_WC_REVISION}")
   elseif(EXISTS "${SOURCE_DIR}/../.git")
      # Assume this is git-svn
      find_program(GIT_EXE git)
      if (GIT_EXE)
         execute_process(COMMAND ${GIT_EXE} svn info
            OUTPUT_VARIABLE MY_WC_INFO
            ERROR_VARIABLE git_svn_info_error
            RESULT_VARIABLE git_svn_info_result
            OUTPUT_STRIP_TRAILING_WHITESPACE)

         string(REGEX REPLACE "^(.*\n)?Revision: ([^\n]+).*" "\\2" MY_WC_REVISION "${MY_WC_INFO}")
         #message(STATUS "SVN revision ${MY_WC_REVISION}")
         SET(RG_BUILDKEY "${RG_BUILDKEY} (SVN rev. ${MY_WC_REVISION})")
      endif()
   endif()
endif()

]]

find_program(GIT_EXE git)
if (GIT_EXE)
   # Get the git commit hash.
   execute_process(COMMAND ${GIT_EXE} rev-parse --short HEAD
      OUTPUT_VARIABLE MY_WC_REVISION
      ERROR_VARIABLE git_rev_parse_error
      RESULT_VARIABLE git_rev_parse_result
      OUTPUT_STRIP_TRAILING_WHITESPACE)

   # git status --porcelain
   execute_process(COMMAND ${GIT_EXE} status --porcelain
      OUTPUT_VARIABLE MY_WC_STATUS
      ERROR_VARIABLE git_status_error
      RESULT_VARIABLE git_status_result
      OUTPUT_STRIP_TRAILING_WHITESPACE)

   # Working copy not clean?  Add a "*" to the revision.
   if (NOT ${MY_WC_STATUS} STREQUAL "")
      SET(MY_WC_REVISION "${MY_WC_REVISION}*")
   endif()

   #message(STATUS "git revision ${MY_WC_REVISION}")
   SET(RG_BUILDKEY "${RG_BUILDKEY} (git: ${MY_WC_REVISION})")
endif()

message(STATUS "Build key ${RG_BUILDKEY}")

# write a file with the define
file(WRITE svnversion.h.txt "#define BUILDKEY \"${RG_BUILDKEY}\"\n")

# copy the file to the final header only if the version changes
# reduces needless rebuilds
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different svnversion.h.txt svnversion.h)

