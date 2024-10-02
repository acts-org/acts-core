# Acts compiler flags
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE
        RelWithDebInfo
        CACHE STRING
        "Build type configuration"
        FORCE
    )
    message(STATUS "Setting default build type: ${CMAKE_BUILD_TYPE}")
endif()

set(cxx_flags
    "-Wall -Wextra -Wpedantic -Wshadow -Wzero-as-null-pointer-constant -Wold-style-cast"
)

# This adds some useful conversion checks like float-to-bool, float-to-int, etc.
# However, at the moment this is only added to clang builds, since GCC's -Wfloat-conversion
# is much more aggressive and also triggers on e.g., double-to-float
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(cxx_flags "${cxx_flags} -Wfloat-conversion")
endif()

# -Wnull-dereference gets applied to -isystem includes in GCC13,
# which causes lots of warnings in code we have no control over
if(
    CMAKE_CXX_COMPILER_ID MATCHES "Clang"
    OR (
        CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS_EQUAL 12
    )
)
    set(cxx_flags "${cxx_flags} -Wnull-dereference")
endif()

set(ACTS_CXX_STANDARD 20)
set(ACTS_CXX_STANDARD_FEATURE cxx_std_20)
if(DEFINED CMAKE_CXX_STANDARD)
    if(${CMAKE_CXX_STANDARD} GREATER_EQUAL 20)
        set(ACTS_CXX_STANDARD ${CMAKE_CXX_STANDARD})
        set(ACTS_CXX_STANDARD_FEATURE "cxx_std_${CMAKE_CXX_STANDARD}")
    else()
        message(
            SEND_ERROR
            "CMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}, but ACTS requires C++ >=20"
        )
    endif()
endif()

if(ACTS_ENABLE_CPU_PROFILING OR ACTS_ENABLE_MEMORY_PROFILING)
    message(STATUS "Added debug symbol compile flag")
    set(cxx_flags "${cxx_flags} ${CMAKE_CXX_FLAGS_DEBUG_INIT}")
endif()

# assign to global CXX flags
set(CMAKE_CXX_FLAGS "${cxx_flags} ${CMAKE_CXX_FLAGS}")
message(STATUS "Using compiler flags: ${CMAKE_CXX_FLAGS}")

# silence warning about missing RPATH on Mac OSX
set(CMAKE_MACOSX_RPATH 1)

# bake where we found external dependencies, if they were not in the default library directories
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
# set relative library path for ACTS libraries
set(CMAKE_INSTALL_RPATH "\$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
