# Install icons
set(ICON_INSTALL_DIR "${CMAKE_INSTALL_DATAROOTDIR}/icons")
set(ICON_SRC_DIR "pixmaps/icons")
foreach(_size 16;32;64)
  set(_dir "${ICON_INSTALL_DIR}/hicolor/${_size}x${_size}/mimetypes")
  install(FILES ${ICON_SRC_DIR}/mm-mime-hi${_size}-rosegarden.png DESTINATION ${_dir}
          RENAME application-x-rosegarden-composition.png)
  install(FILES ${ICON_SRC_DIR}/mm-mime-hi${_size}-rosegarden-rgd.png DESTINATION ${_dir}
          RENAME application-x-rosegarden-device.png)
  install(FILES ${ICON_SRC_DIR}/mm-mime-hi${_size}-rosegarden-rgp.png DESTINATION ${_dir}
          RENAME application-x-rosegarden-project.png)
  install(FILES ${ICON_SRC_DIR}/mm-mime-hi${_size}-rosegarden-rgt.png DESTINATION ${_dir}
          RENAME application-x-rosegarden-template.png)
endforeach()

foreach(_size 16;32;64;128)
  set(_dir "${ICON_INSTALL_DIR}/hicolor/${_size}x${_size}/apps")
  install(FILES ${ICON_SRC_DIR}/rg-rwb-rose3-${_size}x${_size}.png DESTINATION ${_dir}
          RENAME rosegarden.png)
endforeach()

# Install mimetype
set(RG_INSTALL_MIMEDIR "${CMAKE_INSTALL_DATAROOTDIR}/mime/packages")
install(FILES mime/rosegarden.xml DESTINATION "${RG_INSTALL_MIMEDIR}")

find_package(SharedMimeInfo 0.50)
set_package_properties(SharedMimeInfo PROPERTIES
                       TYPE OPTIONAL
                       PURPOSE "Allows KDE applications to determine file types"
                      )

# update XDG mime-types if shared mime info is around
if(SharedMimeInfo_FOUND)
    update_xdg_mimetypes(${RG_INSTALL_MIMEDIR})
endif()


# Install desktop file
install(FILES desktop/rosegarden.desktop DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications")

# Install appdata file
install(FILES appdata/rosegarden.appdata.xml DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/appdata")
