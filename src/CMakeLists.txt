#
# libcalf.a
#

add_library(libcalf.la audio_fx.cpp analyzer.cpp lv2wrap.cpp metadata.cpp modules_tools.cpp modules_delay.cpp modules_comp.cpp modules_limit.cpp modules_dist.cpp modules_filter.cpp modules_mod.cpp modules_pitch.cpp fluidsynth.cpp giface.cpp monosynth.cpp organ.cpp osctl.cpp plugin.cpp preset.cpp synth.cpp utils.cpp wavetable.cpp modmatrix.cpp pffft.c shaping_clipper.cpp)


target_include_directories(libcalf.la PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(libcalf.la PRIVATE ${EXPAT_INCLUDE_DIRS})
target_link_libraries(libcalf.la PRIVATE ${EXPAT_LIBRARIES})

IF(USE_GUI)
    target_include_directories(libcalf.la PRIVATE ${GTK_INCLUDE_DIRS})
    target_link_libraries(libcalf.la PRIVATE ${GTK_LIBRARIES} FluidSynth::libfluidsynth)
ENDIF()

IF(USE_JACK)
    target_include_directories(libcalf.la PRIVATE ${JACK_INCLUDE_DIRS})
    target_link_libraries(libcalf.la PRIVATE ${JACK_LIBRARIES})
ENDIF()

IF(USE_LASH)
    target_include_directories(libcalf.la PRIVATE ${LASH_INCLUDE_DIRS})
    target_link_libraries(libcalf.la PRIVATE ${LASH_LIBRARIES})
ENDIF()

IF(USE_LV2)
    target_include_directories(libcalf.la PRIVATE ${LV2_INCLUDE_DIRS})
    target_link_libraries(libcalf.la PRIVATE ${LV2_LIBRARIES})
ENDIF()

SET(PKGLIBDIR "${CMAKE_INSTALL_PREFIX}/lib/calf")
target_link_options(libcalf.la PRIVATE -rpath "${PKGLIBDIR}" -avoid-version -module -disable-static)

#
# UIs
#

IF(USE_GUI)
    add_library(libcalfgui.la ctl_curve.cpp ctl_keyboard.cpp ctl_knob.cpp ctl_led.cpp ctl_tube.cpp ctl_vumeter.cpp ctl_frame.cpp ctl_fader.cpp ctl_buttons.cpp ctl_notebook.cpp ctl_meterscale.cpp ctl_combobox.cpp ctl_tuner.cpp ctl_phasegraph.cpp ctl_pattern.cpp gui.cpp gui_config.cpp gui_controls.cpp osctl.cpp plugin_gui_window.cpp preset_gui.cpp utils.cpp ctl_linegraph.cpp drawingutils.cpp)
    target_include_directories(libcalfgui.la PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(libcalfgui.la PRIVATE FluidSynth::libfluidsynth)
    target_link_options(libcalfgui.la PRIVATE -static -disable-shared)
ENDIF()


IF(USE_LV2 AND USE_GUI)
    SET(LV2DIR "${CMAKE_INSTALL_PREFIX}/lib/lv2/calf.lv2")
    add_library(libcalflv2gui.la gui.cpp gui_config.cpp gui_controls.cpp ctl_curve.cpp ctl_keyboard.cpp ctl_knob.cpp ctl_led.cpp ctl_tube.cpp ctl_vumeter.cpp ctl_frame.cpp ctl_fader.cpp ctl_buttons.cpp ctl_notebook.cpp ctl_meterscale.cpp ctl_combobox.cpp ctl_tuner.cpp ctl_phasegraph.cpp ctl_pattern.cpp metadata.cpp giface.cpp plugin_gui_window.cpp preset.cpp preset_gui.cpp lv2gui.cpp osctl.cpp utils.cpp ctl_linegraph.cpp drawingutils.cpp)
    target_include_directories(libcalflv2gui.la PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_options(libcalflv2gui.la PRIVATE -rpath ${LV2DIR} -avoid-version -module -disable-static -Wl,-z,nodelete)
ENDIF()


#
# calfjackhost
#

IF(USE_GUI AND USE_JACK)

    message(STATUS "FLUID: ${FluidSynth_FOUND}")
    add_executable(calfjackhost gtk_session_env.cpp host_session.cpp jack_client.cpp jackhost.cpp gtk_main_win.cpp connector.cpp session_mgr.cpp)
    
    target_include_directories(calfjackhost PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(calfjackhost PRIVATE libcalfgui.la libcalf.la ${JACK_LIBRARIES} ${GTK_LIBRARIES} FluidSynth::libfluidsynth)


ENDIF()


#
# calfmakerdf
#

add_executable(calfmakerdf makerdf.cpp)
target_include_directories(calfmakerdf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${EXPAT_INCLUDE_DIRS})
target_link_libraries(calfmakerdf PRIVATE libcalf.la ${EXPAT_LIBRARIES})
