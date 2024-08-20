# Find the Pythia8 includes and libraries.
#
# This module defines the `Pythia8` imported target that encodes all
# necessary information in its target properties.

find_library(
    Pythia8_LIBRARY
    NAMES Pythia8 pythia8
    HINTS
    ENV PYTHIA8_DIR
    PYTHIA8
    PATHS /opt/pythia8 /usr/local
    DOC "The Pythia8 library"
)
find_path(
    Pythia8_INCLUDE_DIR
    NAMES Pythia8/Pythia.h
    HINTS
    ENV PYTHIA8_DIR
    PYTHIA8
    PATHS /opt/pythia8 /usr/local
    DOC "The Pythia8 include directory"
)

file(READ "${Pythia8_INCLUDE_DIR}/Pythia8/Pythia.h" Pythia8_VERSION_FILE)
string(
    REGEX MATCH
    "#define PYTHIA_VERSION (8\.[0-9]+)"
    _
    ${Pythia8_VERSION_FILE}
)
set(Pythia8_VERSION ${CMAKE_MATCH_1})

find_package_handle_standard_args(
    Pythia8
    REQUIRED_VARS Pythia8_LIBRARY Pythia8_INCLUDE_DIR
    VERSION_VAR Pythia8_VERSION
)

add_library(Pythia8 SHARED IMPORTED)
set_property(TARGET Pythia8 PROPERTY IMPORTED_LOCATION ${Pythia8_LIBRARY})
set_property(
    TARGET Pythia8
    PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${Pythia8_INCLUDE_DIR}
)

mark_as_advanced(Pythia8_FOUND Pythia8_INCLUDE_DIR Pythia8_LIBRARY)
