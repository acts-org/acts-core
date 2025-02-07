# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

if(ACTS_RUN_CLANG_TIDY)
    find_program(CLANG_TIDY_COMMAND NAMES clang-tidy)
    if(NOT CLANG_TIDY_COMMAND)
        message(
            WARNING
            "ACTS_RUN_CLANG_TIDY is ON but clang-tidy is not found!"
        )
        set(CMAKE_CXX_CLANG_TIDY "" CACHE STRING "" FORCE)
    else()
        message(STATUS "Setting up clang-tidy run")

        set(CLANG_TIDY_HEADER_FILTER ".*")

        set(CMAKE_CXX_CLANG_TIDY
            "${CLANG_TIDY_COMMAND};-header-filter=${CLANG_TIDY_HEADER_FILTER};-config-file=${CMAKE_SOURCE_DIR}/.clang-tidy"
        )
    endif()
endif()
