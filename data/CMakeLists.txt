# Install icons
set(ICON_INSTALL_DIR "share/icons")
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

foreach(_size 32;64;128)
  set(_dir "${ICON_INSTALL_DIR}/hicolor/${_size}x${_size}/apps")
  install(FILES ${ICON_SRC_DIR}/rg-rwb-rose3-${_size}x${_size}.png DESTINATION ${_dir}
          RENAME rosegarden.png)
endforeach()

# Install mimetype
install(FILES mime/rosegarden.xml DESTINATION "share/mime/packages")

# Install desktop file
install(FILES desktop/rosegarden.desktop DESTINATION "share/applications")

# Install appdata file
install(FILES appdata/rosegarden.appdata.xml DESTINATION "share/appdata")
