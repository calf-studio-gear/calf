set(SIZES 16x16 22x22 24x24 32x32 48x48 64x64 128x128 256x256)

foreach(size ${SIZES})
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}_${size}.png"
        DESTINATION "share/icons/hicolor/${size}/apps/"
        RENAME "${PROJECT_NAME}.png")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}_plugin_${size}.png"
        DESTINATION "share/icons/hicolor/${size}/apps/"
        RENAME "${PROJECT_NAME}_plugin.png")
endforeach()

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}_scalable.svg"
        DESTINATION "share/icons/hicolor/scalable/apps/"
        RENAME "${PROJECT_NAME}.svg")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}_plugin_scalable.svg"
        DESTINATION "share/icons/hicolor/scalable/apps/"
        RENAME "${PROJECT_NAME}_plugin.svg")

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    install(CODE "execute_process(COMMAND gtk-update-icon-cache -f -t \"\$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/icons/hicolor\")")
endif()
