# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

# from https://stackoverflow.com/a/52136398
# necessary until cmake 3.25 https://cmake.org/cmake/help/latest/prop_tgt/SYSTEM.html#prop_tgt:SYSTEM

function(acts_target_link_libraries_system target)
    set(options PRIVATE PUBLIC INTERFACE)
    cmake_parse_arguments(TLLS "${options}" "" "" ${ARGN})
    foreach(op ${options})
        if(TLLS_${op})
            set(scope ${op})
        endif()
    endforeach(op)
    set(libs ${TLLS_UNPARSED_ARGUMENTS})

    foreach(lib ${libs})
        get_target_property(
            lib_include_dirs
            ${lib}
            INTERFACE_INCLUDE_DIRECTORIES
        )
        if(lib_include_dirs)
            if(scope)
                target_include_directories(
                    ${target}
                    SYSTEM
                    ${scope}
                    ${lib_include_dirs}
                )
            else()
                target_include_directories(
                    ${target}
                    SYSTEM
                    PRIVATE ${lib_include_dirs}
                )
            endif()
        else()
            message(
                "Warning: ${lib} doesn't set INTERFACE_INCLUDE_DIRECTORIES. No include_directories set."
            )
        endif()
        if(scope)
            target_link_libraries(${target} ${scope} ${lib})
        else()
            target_link_libraries(${target} ${lib})
        endif()
    endforeach()
endfunction(acts_target_link_libraries_system)
