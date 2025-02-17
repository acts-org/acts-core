# This function adds a helper target called ${target}_HEADERS which has
# generated source files that include each hedaer that was given one-by-one.
# The generated target links against the one given in `target` and therefore
# has access to all includes.
# The generated target is not included in the default build, and is only meant
# to help tools like clangd discover compiler flags.
function(acts_compile_headers target)
    set(options "")
    set(oneValueArgs GLOB)
    set(multiValueArgs "")
    cmake_parse_arguments(
        PARSE_ARGV
        0
        ARGS
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
    )

    if(NOT ACTS_COMPILE_HEADERS)
        return()
    endif()

    if(NOT "${ARGS_GLOB}" STREQUAL "")
        if(NOT "${_headers}" STREQUAL "")
            message(SEND_ERROR "Cannot use HEADERS and GLOB at the same time")
            return()
        endif()

        file(
            GLOB_RECURSE _headers
            RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
            ${ARGS_GLOB}
        )
    endif()

    if("${_headers}" STREQUAL "")
        message(SEND_ERROR "No headers specified")
        return()
    endif()

    set(_sources "")

    foreach(_file ${_headers})
        if(IS_ABSOLUTE "${_file}")
            message(SEND_ERROR "Absolute paths are not allowed: ${_file}")
            continue()
        endif()

        set(_header_file "${CMAKE_CURRENT_SOURCE_DIR}/${_file}")

        if(NOT EXISTS "${_header_file}")
            message(SEND_ERROR "File not found: ${_header_file}")
        endif()
        if(IS_DIRECTORY "${_header_file}")
            message(SEND_ERROR "Path is a directory: ${_header_file}")
        endif()

        get_filename_component(_header_file_name "${_file}" NAME_WLE)
        get_filename_component(_header_directory "${_file}" DIRECTORY)

        set(_temporary_dir "${CMAKE_CURRENT_BINARY_DIR}/${_header_directory}")

        file(MAKE_DIRECTORY "${_temporary_dir}")

        set(_temporary_path "${_temporary_dir}/${_header_file_name}.cpp")

        file(WRITE "${_temporary_path}" "#include \"${_header_file}\"")

        list(APPEND _sources "${_temporary_path}")
    endforeach()

    add_library(${target}_HEADERS SHARED EXCLUDE_FROM_ALL ${_sources})
    target_link_libraries(${target}_HEADERS PRIVATE ${target})
endfunction()
