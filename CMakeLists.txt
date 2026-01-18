cmake_minimum_required(VERSION 3.15)

project(calf)
string(TOUPPER "${CMAKE_PROJECT_NAME}" PROJECT_NAME_UCASE)

set(PROJECT_YEAR 2023)
set(PROJECT_AUTHOR      "${PROJECT_NAME_UCASE} Developers")
set(PROJECT_URL         "https://calf-studio-gear.org/")
set(PROJECT_DESCRIPTION "${PROJECT_NAME_UCASE} Studio Gear")
set(PROJECT_COPYRIGHT   "2007-${PROJECT_YEAR} ${PROJECT_AUTHOR}")
set(VERSION_MAJOR       "0")
set(VERSION_MINOR       "90")
set(VERSION_RELEASE     "9")
set(VERSION_STAGE       "alpha")
set(VERSION_BUILD       "")
set(VERSION             "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_RELEASE}")

configure_file("${CMAKE_SOURCE_DIR}/COPYING" "${CMAKE_BINARY_DIR}/COPYING.txt" COPYONLY)

#
# Options
#

include(GNUInstallDirs)

option(WANT_GUI         "Include Graphical User Interface" ON)
option(WANT_JACK        "Include JACK (Jack Audio Connection Kit) support" ON)
option(WANT_LASH        "Include LASH (LASH Audio Session Handler) support" ON)
option(WANT_DEBUG       "Create debug build" OFF)
option(WANT_SSE         "Compile with SSE extensions" OFF)
option(WANT_LV2         "Build Lv2 wrapper" ON)
option(WANT_LV2_GUI     "GUI executable will be used for LV2" ON)
option(WANT_SORDI       "Use the sordi tool to check Lv2 files" ON)
option(WANT_EXPERIMENTAL "Enable experimental effects" OFF)
option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

set(LV2DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/lv2"
    CACHE PATH "Install path (absolute) for the LV2 plugin bundle")

#
# Configuration
#

if(CMAKE_BUILD_TYPE STREQUAL "")
    SET(CMAKE_BUILD_TYPE "Release")
endif()

SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(EXPAT REQUIRED)

if(WANT_GUI)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(GTK gtk+-2.0>=2.12.0 cairo>=1.2.0)
    endif()
    find_package(FluidSynth REQUIRED)
    if(GTK_FOUND AND FluidSynth_FOUND)
        include_directories(${GTK_INCLUDE_DIRS})
        set(USE_GUI TRUE)
        set(STATUS_GUI "OK")
    elseif(NOT GTK_FOUND)
        set(STATUS_GUI "not found - (GTK+ 2.12 and cairo 1.2 or newer required)")
    elseif(NOT FluidSynth_FOUND)
        set(STATUS_GUI "not found - (FLUIDSYNTH required)")
    endif()
else()
    set(STATUS_GUI "not built as requested")
endif()

if(WANT_JACK)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(JACK jack>=0.77)
    endif()
    if(JACK_FOUND)
        set(USE_JACK TRUE)
        if(JACK_VERSION VERSION_GREATER_EQUAL "0.105.0")
            set(OLD_JACK FALSE)
        else()
            set(OLD_JACK TRUE)
        endif()
        if(JACK_VERSION VERSION_GREATER_EQUAL "0.124.0")
            set(JACK_HAS_RENAME TRUE)
        else()
            set(JACK_HAS_RENAME FALSE)
        endif()
        set(STATUS_JACK "OK")
    else()
        set(STATUS_JACK "not found, install it or set PKG_CONFIG_PATH appropriately")
    endif()
else()
    set(STATUS_JACK "not built as requested")
endif()

if(WANT_LASH)
    if(PKG_CONFIG_FOUND)
        pkg_search_module(LASH lash-1.0)
    endif()
    if(LASH_FOUND)
        include_directories(${LASH_INCLUDE_DIRS})
        add_definitions(-DLASH=1)
        list(APPEND AUDIO_LIBRARIES ${LASH_LIBRARIES})
        list(APPEND AUDIO_LIBRARY_DIRS ${LASH_LIBRARY_DIRS})

        if(LASH_VERSION VERSION_GREATER_EQUAL "0.6")
            set(USE_LASH_0_6 TRUE)
        else()
            set(USE_LASH_0_6 FALSE)
        endif()
        set(STATUS_LASH "OK")
    else()
        set(STATUS_LASH "not found, install it or set PKG_CONFIG_PATH appropriately")
    endif()
    # mark_as_advanced(LASH_LIBRARIES)
else()
    set(STATUS_LASH "not built as requested")
endif()

if(WANT_LV2)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(LV2 lv2)
    endif()
    if(NOT LV2_FOUND)
        unset(LV2_FOUND CACHE)
        find_package(LV2 CONFIG)
    endif()
    if(LV2_FOUND)
        set(USE_LV2 TRUE)
        set(STATUS_LV2 "OK")
    else()
        set(STATUS_LV2 "not found, install it or set PKG_CONFIG_PATH appropriately")
    endif()
else()
    set(STATUS_LV2 "not built, as requested")
endif()

if(WANT_LV2_GUI)
    if(USE_LV2 AND USE_GUI)
        set(USE_LV2_GUI TRUE)
        set(STATUS_LV2_GUI "OK")
    elseif(USE_LV2 AND NOT USE_GUI)
        set(STATUS_LV2_GUI "not build, because no GUI support")
    elseif(NOT USE_LV2 AND USE_GUI)
        set(STATUS_LV2_GUI "not build, because no Lv2 support")
    else(NOT USE_LV2 AND USE_GUI)
        set(STATUS_LV2_GUI "not build, because no GUI and no Lv2 support")
    endif()
else()
    set(STATUS_LV2_GUI "not built, as requested")
endif()

if(WANT_SORDI)
    find_program(SORDI NAMES sordi)
    if(SORDI)
        set(USE_SORDI TRUE)
        set(STATUS_SORDI "OK")
    else()
        set(STATUS_SORDI "not found")
    endif()
else()
    set(STATUS_LV2_SORDI "not run, as requested")
endif()

if(WANT_EXPERIMENTAL)
    set(STATUS_EXPERIMENTAL "OK")
    set(ENABLE_EXPERIMENTAL TRUE)
else()
    set(STATUS_EXPERIMENTAL "not built, as requested")
endif()

configure_file(config.h.cmake.in config.h)
include_directories(${CMAKE_BINARY_DIR})

#
# Compile & Install
#

add_subdirectory(src)
add_subdirectory(gui)
add_subdirectory(icons)
add_subdirectory(licenses)

#
# Install targets (except src)
#

install(FILES ${CMAKE_SOURCE_DIR}/README.md
        DESTINATION ${CMAKE_INSTALL_DOCDIR})
install(DIRECTORY ${CMAKE_SOURCE_DIR}/licenses
        DESTINATION ${CMAKE_INSTALL_DOCDIR} FILES_MATCHING PATTERN "*.txt")
install(FILES ${CMAKE_BINARY_DIR}/licenses/ALL_LICENSES.txt
        DESTINATION ${CMAKE_INSTALL_DOCDIR}/licenses/)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/doc/manuals/
        DESTINATION ${CMAKE_INSTALL_DOCDIR} FILES_MATCHING PATTERN "*.html")
install(DIRECTORY ${CMAKE_SOURCE_DIR}/doc/manuals/images/
        DESTINATION ${CMAKE_INSTALL_DOCDIR}/images FILES_MATCHING PATTERN "*.png")
install(DIRECTORY ${CMAKE_SOURCE_DIR}/doc/manuals/images/
        DESTINATION ${CMAKE_INSTALL_DOCDIR}/images FILES_MATCHING PATTERN "*.jpg")
install(DIRECTORY ${CMAKE_SOURCE_DIR}/doc/manuals/images/prettyPhoto/dark_rounded/
        DESTINATION ${CMAKE_INSTALL_DOCDIR}/images/prettyPhoto/dark_rounded)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/doc/manuals/images/icons/
        DESTINATION ${CMAKE_INSTALL_DOCDIR}/images/icons)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/doc/manuals/scripts
        DESTINATION ${CMAKE_INSTALL_DOCDIR} FILES_MATCHING PATTERN "*.css")
install(DIRECTORY ${CMAKE_SOURCE_DIR}/doc/manuals/scripts/
        DESTINATION ${CMAKE_INSTALL_DOCDIR}/scripts FILES_MATCHING PATTERN "*.js")
install(DIRECTORY ${CMAKE_SOURCE_DIR}/sf2/
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/sf2 FILES_MATCHING PATTERN "*.sf2")
if(WANT_GUI)
    install(FILES ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.desktop
            DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
endif()
install(FILES ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}jackhost.1
        DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
install(FILES ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.7
        DESTINATION ${CMAKE_INSTALL_MANDIR}/man7)
install(FILES ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}
        DESTINATION ${CMAKE_INSTALL_DATADIR}/bash-completion/completions)

#
# CPack
#

SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY           "${PROJECT_DESCRIPTION}")
SET(CPACK_PACKAGE_VENDOR                        "${PROJECT_AUTHOR}")
SET(CPACK_PACKAGE_DESCRIPTION_FILE              "${CMAKE_SOURCE_DIR}/README.md")
SET(CPACK_RESOURCE_FILE_LICENSE                 "${CMAKE_BINARY_DIR}/licenses/ALL_LICENSES.txt")
SET(CPACK_PACKAGE_VERSION_MAJOR                 "${VERSION_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR                 "${VERSION_MINOR}")
SET(CPACK_PACKAGE_VERSION_PATCH                 "${VERSION_RELEASE}")
IF(VERSION_STAGE)
        SET(CPACK_PACKAGE_VERSION_PATCH         "${CPACK_PACKAGE_VERSION_PATCH}-${VERSION_STAGE}")
ENDIF()
IF(VERSION_BUILD)
        SET(CPACK_PACKAGE_VERSION_PATCH         "${CPACK_PACKAGE_VERSION_PATCH}-${VERSION_BUILD}")
ENDIF()
SET(CPACK_SOURCE_GENERATOR                      "TBZ2")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME              "${CMAKE_PROJECT_NAME}-${VERSION}")
set(CPACK_SOURCE_IGNORE_FILES                   "/build;/\.github;/\.git/;/\.gitignore;/\.svnignore;/autogen\.sh;/configure\.ac;/Makefile\.am;${CPACK_SOURCE_IGNORE_FILES}")
SET(CPACK_PACKAGE_EXECUTABLES           "${CMAKE_PROJECT_NAME}jackhost" "${PROJECT_NAME_UCASE} binary")
# Disable strip for Debug|RelWithDebInfo
if(CMAKE_BUILD_TYPE MATCHES "Deb")
	unset(CPACK_STRIP_FILES)
else()
	set(CPACK_STRIP_FILES TRUE)
endif()

INCLUDE(CPack)

#
# Status
#

message(
    "\n"
    "Summary\n"
    "-------\n"
    "* Build product           : ${PROJECT_NAME} ${VERSION}\n")
if(CMAKE_BUILD_TYPE)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(BUILD_TYPE_HINT " (WARNING: VERY SLOW!)")
    endif()
    message("* Build type              : ${CMAKE_BUILD_TYPE}${BUILD_TYPE_HINT}\n")
endif()
message(
    "* Install Prefix          : ${CMAKE_INSTALL_PREFIX}\n"
    "* Jack                    : ${STATUS_JACK}\n"
    "* LASH                    : ${STATUS_LASH}\n"
    "* GUI                     : ${STATUS_GUI}\n"
    "* LV2                     : ${STATUS_LV2}\n"
    "* LV2 GUI                 : ${STATUS_LV2_GUI}\n"
    "* Sordi sanity checks     : ${STATUS_SORDI}\n"
    "* Experimental plugins    : ${STATUS_EXPERIMENTAL}\n"
)



