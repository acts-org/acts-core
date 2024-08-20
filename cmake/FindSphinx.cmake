# Find the Sphinx documentation generator.
#
# This module defines the following variables:
#   Sphinx_FOUND - whether Sphinx was found or not
#   Sphinx_EXECUTABLE - Sphinx executable if if was found
#

set(_hints)

# use python installation to find potential install locations for sphinx
find_package(Python3 QUIET)
if(Python3_EXECUTABLE)
    execute_process(
        COMMAND ${Python3_EXECUTABLE} -m site --user-base
        OUTPUT_VARIABLE Python3_USER_BASE
    )
    execute_process(
        COMMAND ${Python3_EXECUTABLE} -c "import sys; print(sys.exec_prefix)"
        OUTPUT_VARIABLE Python3_EXEC_PREFIX
    )
    # strip newlines
    string(REPLACE "\n" "" Python3_USER_BASE ${Python3_USER_BASE})
    string(REPLACE "\n" "" Python3_EXEC_PREFIX ${Python3_EXEC_PREFIX})
    # add derived binary dirs as hints
    list(APPEND _hints "${Python3_USER_BASE}/bin" "${Python3_EXEC_PREFIX}/bin")
endif()

# find sphinx executable
if(_hints)
    message(STATUS "Using additional Sphinx search hints:")
    foreach(_hint IN LISTS _hints)
        message(STATUS " ${_hint}")
    endforeach()
endif()
find_program(
    Sphinx_EXECUTABLE
    NAMES sphinx-build sphinx-build-3
    HINTS ${_hints}
    DOC "Sphinx documentation generator"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Sphinx REQUIRED_VARS Sphinx_EXECUTABLE)

unset(_hints)
