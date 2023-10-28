CMAKE_MINIMUM_REQUIRED(VERSION 3.15)

PROJECT(calf)
STRING(TOUPPER "${CMAKE_PROJECT_NAME}" PROJECT_NAME_UCASE)

SET(PROJECT_YEAR 2023)
SET(PROJECT_AUTHOR      "CALF Developers")
SET(PROJECT_URL         "https://calf-studio-gear.org/")
SET(PROJECT_DESCRIPTION "${CMAKE_PROJECT_NAME} Studio Gear")
SET(PROJECT_COPYRIGHT   "2007-${PROJECT_YEAR} ${PROJECT_AUTHOR}")
SET(VERSION_MAJOR       "0")
SET(VERSION_MINOR       "90")
SET(VERSION_RELEASE     "4")
SET(VERSION_STAGE       "alpha")
SET(VERSION_BUILD       "")
SET(VERSION             "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_RELEASE}")
# IF(VERSION_STAGE)
# 	SET(VERSION     "${VERSION}-${VERSION_STAGE}")
# ENDIF()
# IF(VERSION_BUILD)
# 	SET(VERSION     "${VERSION}-${VERSION_BUILD}")
# ENDIF()


#
# Options
#

SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules" ${CMAKE_MODULE_PATH})  # TODO: unused?
OPTION(WANT_GUI		"Include Graphical User Interface" ON)
OPTION(WANT_JACK	"Include JACK (Jack Audio Connection Kit) support" ON)
OPTION(WANT_LASH	"Include LASH (LASH Audio Session Handler) support" ON)
OPTION(WANT_DEBUG	"Create debug build" OFF)
OPTION(WANT_LV2	    "Build Lv2 wrapper" ON)
OPTION(WANT_LV2_GUI	"GUI executable will be used for LV2" ON)
OPTION(WANT_SORDI	"Use the sordi tool to check Lv2 files" ON)
OPTION(WANT_EXPERIMENTAL	"Enable experimental effects" OFF)


#
# Configuration
#


find_package(PkgConfig REQUIRED)
find_package(EXPAT REQUIRED)
# PKG_CHECK_MODULES(GLIB glib-2.0>=2.0.0 gthread-2.0>=2.0.0)

# TODO: always check PKG_CONFIG_FOUND
IF(WANT_GUI)
    PKG_CHECK_MODULES(GTK gtk+-2.0>=2.12.0 cairo>=1.2.0)
    FIND_PACKAGE(FluidSynth REQUIRED)
    if(GTK_FOUND AND FluidSynth_FOUND)
        include_directories(${GTK_INCLUDE_DIRS})
        SET(USE_GUI TRUE)
        SET(STATUS_GUI "OK")
    ELSEIF(NOT GTK_FOUND)
        SET(STATUS_GUI "not found - (GTK+ 2.12 and cairo 1.2 or newer required)")
    ELSEIF(NOT FluidSynth_FOUND)
        SET(STATUS_GUI "not found - (FLUIDSYNTH required)")
    ENDIF()
ELSE()
    SET(STATUS_GUI "not built as requested")
ENDIF()

IF(WANT_JACK)
    PKG_CHECK_MODULES(JACK jack>=0.77)
    IF(JACK_FOUND)
        SET(USE_JACK TRUE)
        # SET(CMAKE_REQUIRED_LIBRARIES_BACKUP "${CMAKE_REQUIRED_LIBRARIES}")
        # SET(CMAKE_REQUIRED_LIBRARIES "${JACK_LIBRARIES}")
        # CHECK_LIBRARY_EXISTS(jack jack_port_rename "" JACK_HAS_RENAME)
        # SET(CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES_BACKUP}")
        # UNSET(CMAKE_REQUIRED_LIBRARIES_BACKUP)
        if (JACK_VERSION VERSION_GREATER_EQUAL "0.105.0")
            SET (OLD_JACK FALSE)
        else ()
            SET (OLD_JACK TRUE)
        endif ()
        if (JACK_VERSION VERSION_GREATER_EQUAL "0.124.0")
            SET (JACK_HAS_RENAME TRUE)
        else ()
            SET (JACK_HAS_RENAME FALSE)
        endif ()
        SET(STATUS_JACK "OK")
    ELSE()
        SET(STATUS_JACK "not found, install it or set PKG_CONFIG_PATH appropriately")
    ENDIF()
ELSE()
    SET(STATUS_JACK "not built as requested")
ENDIF()

IF(WANT_LASH)
    pkg_search_module(LASH lash-1.0)
    if(LASH_FOUND)
        include_directories(${LASH_INCLUDE_DIRS})
        add_definitions(-DLASH=1)
        list(APPEND AUDIO_LIBRARIES ${LASH_LIBRARIES})
        list(APPEND AUDIO_LIBRARY_DIRS ${LASH_LIBRARY_DIRS})
        
        if (LASH_VERSION VERSION_GREATER_EQUAL "0.6")
            SET (USE_LASH_0_6 TRUE)
        else ()
            SET (USE_LASH_0_6 FALSE)
        endif ()
        SET(STATUS_LASH "OK")
    else()
        SET(STATUS_LASH "not found, install it or set PKG_CONFIG_PATH appropriately")
    endif()
    # mark_as_advanced(LASH_LIBRARIES)
ELSE()
    SET(STATUS_LASH "not built as requested")
ENDIF()

# IF(WANT_DEBUG)
#     SET(USE_DEBUG TRUE)
# ENDIF()
IF(WANT_LV2)
    IF(PKG_CONFIG_FOUND)
        PKG_CHECK_MODULES(LV2 lv2)
    ENDIF()
    IF(NOT LV2_FOUND)
        UNSET(LV2_FOUND CACHE)
        FIND_PACKAGE(LV2 CONFIG)
    ENDIF()
    IF(LV2_FOUND)
        SET(USE_LV2 TRUE)
        SET(STATUS_LV2 "OK")
    ELSE()
        SET(STATUS_LV2 "not found, install it or set PKG_CONFIG_PATH appropriately")
    ENDIF()
ELSE()
    SET(STATUS_LV2 "not built, as requested")
ENDIF()

IF(WANT_LV2_GUI)
    IF(USE_LV2 AND USE_GUI)
        SET(USE_LV2_GUI TRUE)
        SET(STATUS_LV2_GUI "OK")
    ELSEIF(USE_LV2 AND NOT USE_GUI)
        SET(STATUS_LV2_GUI "not build, because no GUI support")
    ELSEIF(NOT USE_LV2 AND USE_GUI)
        SET(STATUS_LV2_GUI "not build, because no Lv2 support")
    ELSE(NOT USE_LV2 AND USE_GUI)
        SET(STATUS_LV2_GUI "not build, because no GUI and no Lv2 support")
    ENDIF()
ELSE()
    SET(STATUS_LV2_GUI "not built, as requested")
ENDIF()

IF(WANT_SORDI)
    FIND_PROGRAM(SORDI NAMES sordi)
    IF(SORDI)
        SET(USE_SORDI TRUE)
        SET(STATUS_SORDI "OK")
    ELSE()
        SET(STATUS_SORDI "not found")
    ENDIF()
ELSE()
    SET(STATUS_LV2_SORDI "not run, as requested")
ENDIF()

IF(WANT_EXPERIMENTAL)
    SET(STATUS_EXPERIMENTAL "OK")
    SET(ENABLE_EXPERIMENTAL TRUE)
ELSE()
    SET(STATUS_EXPERIMENTAL "not built, as requested")
ENDIF()

configure_file(config.h.cmake.in config.h)
include_directories(${CMAKE_BINARY_DIR})



#
# Flags, Compiling
#

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-ffast-math)

add_subdirectory(src)

#
# Status
#

MESSAGE("\n"
"Summary\n"
"-------\n"
"* Build product           : ${PROJECT_NAME} ${VERSION}\n"
"* Build type              : ${CMAKE_BUILD_TYPE}\n"
"* Install Prefix          : ${CMAKE_INSTALL_PREFIX}\n"
"* Jack                    : ${STATUS_JACK}\n"
"* LASH                    : ${STATUS_LASH}\n"
"* GUI                     : ${STATUS_GUI}\n"
"* LV2                     : ${STATUS_LV2}\n"
"* LV2 GUI                 : ${STATUS_LV2_GUI}\n"
"* Sordi sanity checks     : ${STATUS_SORDI}\n"
"* Experimental plugins    : ${STATUS_EXPERIMENTAL}\n"
)



