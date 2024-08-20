# Provide helper macros to handle option interdependencies.
#
# Ensure an option is set if a condition is met. Can be used to encode
# dependencies between different options, e.g. if OptionA is on, OptionB has
# to be on as well.
#
#     set_option_if(OptionB OptionA)
#
# The macro can take arbitrary conditions as the second parameter, e.g.
#
#     set_option_if(OptionB OptionA AND ConditionB OR ConditionC)
#

macro(set_option_if option)
    if(${ARGN})
        # create a regular (directory-scope) variable that should take precedence
        # over a cache entry of the same name. that means that automatically
        # activated options are not stored in the cache.
        set(${option} ON)
    endif()
endmacro()
