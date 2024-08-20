# Provide helper functions to simplify the handling of (optional) components
#
# Components must always be placed in a separate directory which can than
# be added using either of the two provided functions
#
#     add_component(<SUBDIR> <NAME>)
#     add_component_if(<SUBDIR> <NAME> ...) # for optional components
#
# Both functions are wrappers around `add_subdirectory` that also register a
# component under the given name. All additional arguments to the second
# function are treated as a boolean expression that determines whether the
# subdirectory should be added and the component registered. The list of
# components is stored in the global `_components` variable. If
# components are added from a subdirectory, this variable must be propagated to
# the global scope using the following helper macro:
#
#     propagate_components_to_parent()
#
# For cases, where a subdirectory needs to be optionally included but does not
# need to be registered as a component, the following helper function can be
# used
#
#     add_subdirectory_if(<SUBDIR> ...)
#
# where all additional arguments are again treated as a boolean expression that
# determines whether the subdirectory should be added.

set(_components)

# add an optional directory and register its name as a component
function(add_component_if path name)
    file(
        RELATIVE_PATH
        _rel
        ${PROJECT_SOURCE_DIR}
        "${CMAKE_CURRENT_SOURCE_DIR}/${path}"
    )
    if(${ARGN})
        list(APPEND _components "${name}")
        # propagate variable outside function scope
        set(_components "${_components}" PARENT_SCOPE)
        message(STATUS "Enable component '${name}' in '${_rel}'")
        add_subdirectory(${path})
    else()
        message(STATUS "Ignore component '${name}' in '${_rel}'")
    endif()
endfunction()

# add a directory and register its name as a component
macro(add_component path name)
    add_component_if(${path} ${name} TRUE)
endmacro()

# propagate the list of components to the parent scope
macro(propagate_components_to_parent)
    set(_components "${_components}" PARENT_SCOPE)
endmacro()

# add an optional subdirectory that is **not** registered as a component
function(add_subdirectory_if path)
    file(
        RELATIVE_PATH
        _rel
        ${PROJECT_SOURCE_DIR}
        "${CMAKE_CURRENT_SOURCE_DIR}/${path}"
    )
    if(${ARGN})
        message(STATUS "Enable subdirectory '${_rel}'")
        add_subdirectory(${path})
    else()
        message(STATUS "Ignore subdirectory '${_rel}'")
    endif()
endfunction()
