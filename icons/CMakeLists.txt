set(SIZES 16x16 22x22 24x24 32x32 48x48 64x64 128x128 256x256)

foreach(size ${SIZES})
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/calf_${size}.png"
        DESTINATION "share/icons/hicolor/${size}/apps/"
        RENAME "calf.png")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/calf_plugin_${size}.png"
        DESTINATION "share/icons/hicolor/${size}/apps/"
        RENAME "calf_plugin.png")
endforeach()

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/calf_scalable.svg"
        DESTINATION "share/icons/hicolor/scalable/apps/"
        RENAME "calf.svg")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/calf_plugin_scalable.svg"
        DESTINATION "share/icons/hicolor/scalable/apps/"
        RENAME "calf_plugin.svg")

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    install(CODE "execute_process(COMMAND gtk-update-icon-cache -f -t \"${CMAKE_INSTALL_PREFIX}/share/icons/hicolor\"")
endif()
