#
# General compile options
#

include(CheckLinkerFlag)
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-ffast-math -finline-limit=80 -finline-functions -finline-functions-called-once)
if(WANT_SSE)
    add_compile_options(-msse -mfpmath=sse)
endif()
if(MSVC)
    add_compile_definitions(_USE_MATH_DEFINES)
endif()

#
# libcalf.a
#
if(MSVC)
    add_library(${PROJECT_NAME} STATIC audio_fx.cpp analyzer.cpp lv2wrap.cpp metadata.cpp modules_tools.cpp modules_delay.cpp modules_comp.cpp modules_limit.cpp modules_dist.cpp modules_filter.cpp modules_mod.cpp modules_pitch.cpp fluidsynth.cpp giface.cpp monosynth.cpp organ.cpp osctl.cpp plugin.cpp preset.cpp synth.cpp utils.cpp wavetable.cpp modmatrix.cpp pffft.c shaping_clipper.cpp)
else()
    add_library(${PROJECT_NAME} audio_fx.cpp analyzer.cpp lv2wrap.cpp metadata.cpp modules_tools.cpp modules_delay.cpp modules_comp.cpp modules_limit.cpp modules_dist.cpp modules_filter.cpp modules_mod.cpp modules_pitch.cpp fluidsynth.cpp giface.cpp monosynth.cpp organ.cpp osctl.cpp plugin.cpp preset.cpp synth.cpp utils.cpp wavetable.cpp modmatrix.cpp pffft.c shaping_clipper.cpp)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(${PROJECT_NAME} PRIVATE ${EXPAT_INCLUDE_DIRS} ${FluidSynth_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads ${EXPAT_LIBRARIES} fluidsynth)

if(USE_GUI)
    target_include_directories(${PROJECT_NAME} PRIVATE ${GTK_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads ${GTK_LIBRARIES})
endif()

if(USE_JACK)
    target_include_directories(${PROJECT_NAME} PRIVATE ${JACK_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads ${JACK_LIBRARIES})
endif()

if(USE_LASH)
    target_include_directories(${PROJECT_NAME} PRIVATE ${LASH_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads ${LASH_LIBRARIES})
endif()

if(USE_LV2)
    target_include_directories(${PROJECT_NAME} PRIVATE ${LV2_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads ${LV2_LIBRARIES})
    set(LV2DIR_PROJ "${LV2DIR}/${PROJECT_NAME}.lv2")
endif()

#
# UIs
#

if(USE_GUI)
	add_library(${PROJECT_NAME}gui STATIC ctl_curve.cpp ctl_keyboard.cpp ctl_knob.cpp ctl_led.cpp ctl_tube.cpp ctl_vumeter.cpp ctl_frame.cpp ctl_fader.cpp ctl_buttons.cpp ctl_notebook.cpp ctl_meterscale.cpp ctl_combobox.cpp ctl_tuner.cpp ctl_phasegraph.cpp ctl_pattern.cpp gui.cpp gui_config.cpp gui_controls.cpp osctl.cpp plugin_gui_window.cpp preset_gui.cpp utils.cpp ctl_linegraph.cpp drawingutils.cpp)
    target_include_directories(${PROJECT_NAME}gui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${PROJECT_NAME}gui PRIVATE Threads::Threads ${GTK_LIBRARIES} fluidsynth)
endif()


if(USE_LV2 AND USE_GUI)
    add_library(${PROJECT_NAME}lv2gui gui.cpp gui_config.cpp gui_controls.cpp ctl_curve.cpp ctl_keyboard.cpp ctl_knob.cpp ctl_led.cpp ctl_tube.cpp ctl_vumeter.cpp ctl_frame.cpp ctl_fader.cpp ctl_buttons.cpp ctl_notebook.cpp ctl_meterscale.cpp ctl_combobox.cpp ctl_tuner.cpp ctl_phasegraph.cpp ctl_pattern.cpp metadata.cpp giface.cpp plugin_gui_window.cpp preset.cpp preset_gui.cpp lv2gui.cpp osctl.cpp utils.cpp ctl_linegraph.cpp drawingutils.cpp)
    target_include_directories(${PROJECT_NAME}lv2gui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    check_linker_flag(CXX -Wl,-z,nodelete LINKER_SUPPORTS_NODELETE)
    if (LINKER_SUPPORTS_NODELETE)
        target_link_options(${PROJECT_NAME}lv2gui PRIVATE -Wl,-z,nodelete)
    endif()
    target_link_libraries(${PROJECT_NAME}lv2gui PRIVATE Threads::Threads ${GTK_LIBRARIES} ${EXPAT_LIBRARIES} fluidsynth)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INSTALL_RPATH "${LV2DIR_PROJ}"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
endif()

#
# calfjackhost
#
if(USE_GUI AND USE_JACK)
    add_executable(${PROJECT_NAME}jackhost gtk_session_env.cpp host_session.cpp jack_client.cpp jackhost.cpp gtk_main_win.cpp connector.cpp session_mgr.cpp)
    
    target_include_directories(${PROJECT_NAME}jackhost PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${PROJECT_NAME}jackhost PRIVATE Threads::Threads ${PROJECT_NAME}gui ${PROJECT_NAME} ${JACK_LIBRARIES} ${GTK_LIBRARIES} fluidsynth)
    
    set_target_properties(${PROJECT_NAME}jackhost PROPERTIES
        INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
endif()

#
# calfmakerdf
#

if(MSVC)
    add_executable(${PROJECT_NAME}makerdf makerdf.cpp getopt_windows.cpp)
else()
    add_executable(${PROJECT_NAME}makerdf makerdf.cpp)
endif()
target_include_directories(${PROJECT_NAME}makerdf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${EXPAT_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME}makerdf PRIVATE ${PROJECT_NAME} Threads::Threads ${EXPAT_LIBRARIES} fluidsynth)

#
# install
#

install(FILES ${CMAKE_SOURCE_DIR}/presets.xml
        DESTINATION share/${PROJECT_NAME}/)

if(USE_GUI)
    install(FILES ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}-gui.xml
            DESTINATION share/${PROJECT_NAME}/)
    if(USE_JACK)
        install(TARGETS ${PROJECT_NAME}jackhost DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif()
endif()

# Install libs
install(TARGETS ${PROJECT_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME})
if(USE_GUI)
    install(TARGETS ${PROJECT_NAME}lv2gui DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME})
endif()

if(BUILD_SHARED_LIBS)
    set(LIB_PREFIX "${CMAKE_SHARED_LIBRARY_PREFIX}")
    set(LIB_SUFFIX "${CMAKE_SHARED_LIBRARY_SUFFIX}")
else()
    set(LIB_PREFIX "${CMAKE_STATIC_LIBRARY_PREFIX}")
    set(LIB_SUFFIX "${CMAKE_STATIC_LIBRARY_SUFFIX}")
endif()

if(USE_LV2)
    if(MSVC)
        # symlinks require special privilege on Windows
        set(COPY_ACTION "copy")
    else()
        set(COPY_ACTION "create_symlink")
    endif()

    # Create Lv2 install dir
    install(CODE "execute_process(COMMAND \"${CMAKE_COMMAND}\" -E make_directory \"\$ENV{DESTDIR}${LV2DIR_PROJ}\")"
            RESULT_VARIABLE calfResult)
    if(NOT ${calfResult} EQUAL 0)
        message(FATAL_ERROR "Could not create ${LV2DIR_PROJ}/")
    endif()
    # Create symlinks from Lv2 install dir to libs
    install(CODE "execute_process( \
        COMMAND \"${CMAKE_COMMAND}\" -E ${COPY_ACTION} \
	\"${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}/${LIB_PREFIX}${PROJECT_NAME}${LIB_SUFFIX}\" \
	\"\$ENV{DESTDIR}${LV2DIR_PROJ}/${PROJECT_NAME}${LIB_SUFFIX}\" \
        )"
        RESULT_VARIABLE calfResult
    )
    if(NOT ${calfResult} EQUAL 0)
        message(FATAL_ERROR "Could not create ${LV2DIR_PROJ}/${PROJECT_NAME}${LIB_SUFFIX}")
    endif()
    if(USE_GUI)
        install(CODE "execute_process( \
            COMMAND \"${CMAKE_COMMAND}\" -E ${COPY_ACTION} \
            \"${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}/${LIB_PREFIX}${PROJECT_NAME}lv2gui${LIB_SUFFIX}\" \
            \"\$ENV{DESTDIR}${LV2DIR_PROJ}/${PROJECT_NAME}lv2gui${LIB_SUFFIX}\" \
            )"
            RESULT_VARIABLE calfResult
        )
        if(NOT ${calfResult} EQUAL 0)
            message(FATAL_ERROR "Could not create ${LV2DIR_PROJ}/${PROJECT_NAME}lv2gui${LIB_SUFFIX}")
        endif()
    endif()

    # Remove previous .ttl files
    file(GLOB ttl_files "${LV2DIR_PROJ}/*.ttl")
    foreach(ttl_file ${ttl_files})
        file(REMOVE "${ttl_file}")
    endforeach()

    # Install new .ttl files
    install(CODE "execute_process(COMMAND \"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}makerdf\" -m ttl -p \"\$ENV{DESTDIR}${LV2DIR_PROJ}\" -d \"\$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/\")")

    # Check files with "sordi"
    if(USE_SORDI)
        file(GLOB ttl_files "${LV2DIR_PROJ}/*.ttl")
        foreach(ttl_file ${ttl_files})
            install(
                CODE "execute_process(COMMAND \"${SORDI}\" -o turtle \"${ttl_file}\")"
                RESULT_VARIABLE result
                OUTPUT_VARIABLE nothing
                )
            if(NOT ${result} EQUAL 0)
                message(FATAL_ERROR "Sordi failed for ${ttl_file}")
            endif()
        endforeach()
    endif()
endif()

