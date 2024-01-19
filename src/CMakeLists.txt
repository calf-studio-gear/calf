#
# General compile options
#

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-ffast-math)
if(MSVC)
    add_compile_definitions(_USE_MATH_DEFINES)
endif()

#
# libcalf.a
#
if(MSVC)
    add_library(calf STATIC audio_fx.cpp analyzer.cpp lv2wrap.cpp metadata.cpp modules_tools.cpp modules_delay.cpp modules_comp.cpp modules_limit.cpp modules_dist.cpp modules_filter.cpp modules_mod.cpp modules_pitch.cpp fluidsynth.cpp giface.cpp monosynth.cpp organ.cpp osctl.cpp plugin.cpp preset.cpp synth.cpp utils.cpp wavetable.cpp modmatrix.cpp pffft.c shaping_clipper.cpp)
else()
    add_library(calf SHARED audio_fx.cpp analyzer.cpp lv2wrap.cpp metadata.cpp modules_tools.cpp modules_delay.cpp modules_comp.cpp modules_limit.cpp modules_dist.cpp modules_filter.cpp modules_mod.cpp modules_pitch.cpp fluidsynth.cpp giface.cpp monosynth.cpp organ.cpp osctl.cpp plugin.cpp preset.cpp synth.cpp utils.cpp wavetable.cpp modmatrix.cpp pffft.c shaping_clipper.cpp)
endif()

target_include_directories(calf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(calf PRIVATE ${EXPAT_INCLUDE_DIRS} ${FluidSynth_INCLUDE_DIRS})
target_link_libraries(calf PUBLIC ${EXPAT_LIBRARIES} fluidsynth)

if(USE_GUI)
    target_include_directories(calf PRIVATE ${GTK_INCLUDE_DIRS})
    target_link_libraries(calf PRIVATE ${GTK_LIBRARIES})
endif()

if(USE_JACK)
    target_include_directories(calf PRIVATE ${JACK_INCLUDE_DIRS})
    target_link_libraries(calf PRIVATE ${JACK_LIBRARIES})
endif()

if(USE_LASH)
    target_include_directories(calf PRIVATE ${LASH_INCLUDE_DIRS})
    target_link_libraries(calf PRIVATE ${LASH_LIBRARIES})
endif()

if(USE_LV2)
    target_include_directories(calf PRIVATE ${LV2_INCLUDE_DIRS})
    target_link_libraries(calf PRIVATE ${LV2_LIBRARIES})
endif()

#
# UIs
#

if(USE_GUI)
    add_library(calfgui ctl_curve.cpp ctl_keyboard.cpp ctl_knob.cpp ctl_led.cpp ctl_tube.cpp ctl_vumeter.cpp ctl_frame.cpp ctl_fader.cpp ctl_buttons.cpp ctl_notebook.cpp ctl_meterscale.cpp ctl_combobox.cpp ctl_tuner.cpp ctl_phasegraph.cpp ctl_pattern.cpp gui.cpp gui_config.cpp gui_controls.cpp osctl.cpp plugin_gui_window.cpp preset_gui.cpp utils.cpp ctl_linegraph.cpp drawingutils.cpp)
    target_include_directories(calfgui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(calfgui PRIVATE ${GTK_LIBRARIES} fluidsynth)
endif()


if(USE_LV2 AND USE_GUI)
    set(LV2DIR "${CMAKE_INSTALL_PREFIX}/lib/lv2/calf.lv2")
    add_library(calflv2gui SHARED gui.cpp gui_config.cpp gui_controls.cpp ctl_curve.cpp ctl_keyboard.cpp ctl_knob.cpp ctl_led.cpp ctl_tube.cpp ctl_vumeter.cpp ctl_frame.cpp ctl_fader.cpp ctl_buttons.cpp ctl_notebook.cpp ctl_meterscale.cpp ctl_combobox.cpp ctl_tuner.cpp ctl_phasegraph.cpp ctl_pattern.cpp metadata.cpp giface.cpp plugin_gui_window.cpp preset.cpp preset_gui.cpp lv2gui.cpp osctl.cpp utils.cpp ctl_linegraph.cpp drawingutils.cpp)
    target_include_directories(calflv2gui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_options(calflv2gui PRIVATE -Wl,-z,nodelete)
    target_link_libraries(calflv2gui PRIVATE ${GTK_LIBRARIES} fluidsynth)
    set_target_properties(calf PROPERTIES
        INSTALL_RPATH "${LV2DIR}"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
endif()

#
# calfjackhost
#
if(USE_GUI AND USE_JACK)
    add_executable(calfjackhost gtk_session_env.cpp host_session.cpp jack_client.cpp jackhost.cpp gtk_main_win.cpp connector.cpp session_mgr.cpp)
    
    target_include_directories(calfjackhost PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(calfjackhost PRIVATE calfgui calf ${JACK_LIBRARIES} ${GTK_LIBRARIES} fluidsynth)
    
    set_target_properties(calfjackhost PROPERTIES
        INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib/${PROJECT_NAME}"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
endif()

#
# calfmakerdf
#

if(MSVC)
    add_executable(calfmakerdf makerdf.cpp getopt_windows.cpp)
else()
    add_executable(calfmakerdf makerdf.cpp)
endif()
target_include_directories(calfmakerdf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${EXPAT_INCLUDE_DIRS})
target_link_libraries(calfmakerdf PRIVATE calf ${EXPAT_LIBRARIES} fluidsynth)

#
# install
#

install(FILES ${CMAKE_SOURCE_DIR}/presets.xml
        DESTINATION share/${PROJECT_NAME}/)

if(USE_GUI)
    install(FILES ${CMAKE_SOURCE_DIR}/calf-gui.xml
            DESTINATION share/${PROJECT_NAME}/)
    if(USE_JACK)
        install(TARGETS calfjackhost DESTINATION bin)
    endif()
endif()

if(USE_LV2)
    install(TARGETS calf LIBRARY DESTINATION lib/${PROJECT_NAME})
    install(
        CODE "file(COPY $<TARGET_FILE:calf>/ DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/lv2/${PROJECT_NAME}.lv2/calf.so)"
        RESULT_VARIABLE calfResult
        OUTPUT_VARIABLE nothing
    )
    if(NOT ${calfResult} EQUAL 0)
        message(FATAL_ERROR "Could not create ${CMAKE_INSTALL_PREFIX}/lib/lv2/${PROJECT_NAME}.lv2/calf.so")
    endif()
    if(USE_GUI)
        install(
            CODE "file(COPY $<TARGET_FILE:calflv2gui>/ DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/lv2/${PROJECT_NAME}.lv2/calflv2gui.so)"
            RESULT_VARIABLE calfResult
            OUTPUT_VARIABLE nothing
        )
        if(NOT ${calfResult} EQUAL 0)
            message(FATAL_ERROR "Could not create ${CMAKE_INSTALL_PREFIX}/lib/lv2/${PROJECT_NAME}.lv2/calflv2gui.so")
        endif()
        install(TARGETS calflv2gui DESTINATION lib/${PROJECT_NAME})
    endif()

    # Remove previous .ttl files
    file(GLOB ttl_files "${CMAKE_INSTALL_PREFIX}/lib/lv2/*.ttl")
    foreach(ttl_file ${ttl_files})
        file(REMOVE "${ttl_file}")
    endforeach()

    install(CODE "execute_process(COMMAND ${CMAKE_CURRENT_BINARY_DIR}/calfmakerdf -m ttl -p ${CMAKE_INSTALL_PREFIX}/lib/lv2/calf.lv2 -d ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/)")

    if(USE_SORDI)
        file(GLOB ttl_files "${CMAKE_INSTALL_PREFIX}/lib/lv2/${PROJECT_NAME}.lv2/*.ttl")
        foreach(ttl_file ${ttl_files})
            install(
                CODE "execute_process(COMMAND ${SORDI} -o turtle ${ttl_file})"
                RESULT_VARIABLE result
                OUTPUT_VARIABLE nothing
                )
            if(NOT ${result} EQUAL 0)
                message(FATAL_ERROR "Sordi failed for ${ttl_file}")
            endif()
        endforeach()
    endif()
endif()

