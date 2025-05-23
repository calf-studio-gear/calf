add_subdirectory(icons/LV2)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        DESTINATION share/${PROJECT_NAME} FILES_MATCHING PATTERN "*.xml")
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        DESTINATION share/${PROJECT_NAME} FILES_MATCHING PATTERN "*.xml")

install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}\")")
install(CODE "execute_process(COMMAND \"${CMAKE_BINARY_DIR}/src/${PROJECT_NAME}makerdf\" -m gui -p \"\$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}\")")

file(GLOB STYLES ${CMAKE_CURRENT_SOURCE_DIR}/styles/*)
foreach(f ${STYLES})
    get_filename_component(sn "${f}" NAME)

    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/styles/${sn}/"
        DESTINATION "share/${PROJECT_NAME}/styles/${sn}" FILES_MATCHING PATTERN "*.png")

    # Process and install gtk.rc
    set(guidatadir "${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}/styles/${sn}")
    configure_file("${f}/gtk.rc.in" "${CMAKE_CURRENT_BINARY_DIR}/styles/${sn}/gtk.rc" @ONLY)
    
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/styles/${sn}/gtk.rc"
        DESTINATION "share/${PROJECT_NAME}/styles/${sn}")
endforeach()

