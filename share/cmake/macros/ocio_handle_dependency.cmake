# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

include(ocio_install_dependency)

###################################################################################################
# ocio_print_versions_error is a wrapper for messages when the dependency is not found.
#
# Arguments:
#   dep_name is the name of the dependency (package). Please note that dep_name is case sensitive.
#   message_color is the color of the message.
#
# Note that this macro is used in ocio_handle_dependency and should be there only.
###################################################################################################
macro (ocio_print_versions_error dep_name message_color)
    if(NOT ocio_dep_MIN_VERSION AND NOT ocio_dep_MAX_VERSION AND NOT ocio_dep_RECOMMENDED_VERSION)
        message(STATUS "${message_color}Could NOT find ${dep_name} (no version specified)${ColorReset}")
    elseif(NOT ocio_dep_MIN_VERSION AND NOT ocio_dep_RECOMMENDED_VERSION)
        message(STATUS "${message_color}Could NOT find ${dep_name} (maximum version: \"${ocio_dep_MAX_VERSION)\"${ColorReset}")
    elseif(NOT ocio_dep_MIN_VERSION AND NOT ocio_dep_MAX_VERSION)
        message(STATUS "${message_color}Could NOT find ${dep_name} (recommended version: \"${ocio_dep_RECOMMENDED_VERSION}\")\"${ColorReset}")
    elseif(NOT ocio_dep_RECOMMENDED_VERSION AND NOT ocio_dep_MAX_VERSION)
        message(STATUS "${message_color}Could NOT find ${dep_name} (minimum version: \"${ocio_dep_MIN_VERSION})\"${ColorReset}")
    elseif(NOT ocio_dep_MIN_VERSION)
        message(STATUS "${message_color}Could NOT find ${dep_name} (recommended version: \"${ocio_dep_RECOMMENDED_VERSION}\", maximum version: \"${ocio_dep_MAX_VERSION}\")${ColorReset}")
    elseif(NOT ocio_dep_RECOMMENDED_VERSION)
        message(STATUS "${message_color}Could NOT find ${dep_name} (minimum version: \"${ocio_dep_MIN_VERSION}\",  maximum version: \"${ocio_dep_MAX_VERSION}\")${ColorReset}")
    elseif(NOT ocio_dep_MAX_VERSION)
        message(STATUS "${message_color}Could NOT find ${dep_name} (minimum version: \"${ocio_dep_MIN_VERSION}\", recommended version: \"${ocio_dep_RECOMMENDED_VERSION}\")${ColorReset}")
    else()
        message(STATUS "${message_color}Could NOT find ${dep_name} (minimum version: \"${ocio_dep_MIN_VERSION}\", recommended version: \"${ocio_dep_RECOMMENDED_VERSION}\", max: \"${ocio_dep_MAX_VERSION}\")${ColorReset}")
    endif()
endmacro()

# Using a macro because OCIO wants the scope to be the same as the caller scope.
# Find modules will set variables via the PARENT_SCOPE option and OCIO needs those 
# variables in the caller scope.

###################################################################################################
# ocio_handle_dependency is a wrapper for find_package with extra options (features).
#
# Argument:
#   dep_name is the name of the dependency (package). Please note that dep_name is case sensitive.
#
# Options (no value): 
#   REQUIRED                        - Whether the dependency is required or not. Fails if missing.
#   PREFER_CONFIG                   - Call find_package in CONFIG mode internally. 
#                                     Note that it tries to find <dep_name>Config.cmake or 
#                                     <dep_name>-Config.cmake directly.
#   ALLOW_INSTALL                   - Try to install the dependency if not found. Note that a 
#                                     corresponding Install<dep_name>.cmake file must exist.
#   VERBOSE                         - Enable extra logging.
#
# Options (one value):
#   MIN_VERSION                     - Minimum version for the dependency.
#   MAX_VERSION                     - Maximum version for the dependency.
#   RECOMMENDED_VERSION         - Recommended version for the dependency.
#   RECOMMENDED_VERSION_REASON  - Reason for the recommended version.
#
# Options (multiple values):
#   VERSION_VARS                    - List of version variables. Default to <dep_name>_VERSION.
#   COMPONENTS                      - List of components to find. Just like find_package option.
#   PROMOTE_TARGET                  - List of targets that needs to be globally visible by setting 
#                                     the targets property IMPORTED_GLOBAL to true. Note that the
#                                     targets must be created by the dependency.
#
# This is a macro because OCIO wants the scope to be the same as the caller scope.
# Find modules will set variables via the PARENT_SCOPE option and OCIO needs those 
# variables in the caller scope.
#
###################################################################################################

macro (ocio_handle_dependency dep_name)
    cmake_parse_arguments(
        # prefix
        ocio_dep
        # options
        "REQUIRED;PREFER_CONFIG;ALLOW_INSTALL;VERBOSE"
        # one value keywords
        "MIN_VERSION;MAX_VERSION;RECOMMENDED_VERSION;RECOMMENDED_VERSION_REASON;PROMOTE_TARGET"
        # multi value keywords
        "VERSION_VARS;COMPONENTS"
        # args
        ${ARGN})

    set(ocio_dep_FORCE_INSTALLATION OFF)
    if(ocio_dep_ALLOW_INSTALL AND OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
        set(ocio_dep_FORCE_INSTALLATION ON)
    endif()
    
    if(NOT ocio_dep_FORCE_INSTALLATION)
        set(ocio_dep_QUIET_string "")
        # Do not set to QUIET when OCIO_VERBOSE is ON.
        if(NOT ocio_dep_VERBOSE AND NOT OCIO_VERBOSE)
            set(${dep_name}_FIND_QUIETLY true)
            set(ocio_dep_QUIET_string "QUIET")
        endif()

        set(ocio_dep_CONFIG_string "")
        if(ocio_dep_PREFER_CONFIG)
            set(ocio_dep_CONFIG_string "CONFIG")
        endif()

        set(ocio_dep_COMPONENTS_string "")
        if(ocio_dep_COMPONENTS)
            set(ocio_dep_COMPONENTS_string "COMPONENTS")
        endif()

        if (${dep_name}_FOUND)
            # Nothing to do. Already found.
        else()
            # Try to find the recommended, or higher, version.
            # Note that the recommended version should always be specified.
            find_package(${dep_name}
                        ${ocio_dep_RECOMMENDED_VERSION}
                        ${ocio_dep_CONFIG_string}
                        ${ocio_dep_COMPONENTS_string} 
                        ${ocio_dep_COMPONENTS}
                        ${ocio_dep_QUIET_string}
                        ${ocio_dep_UNPARSED_ARGUMENTS})

            if (NOT ${dep_name}_FOUND AND ocio_dep_PREFER_CONFIG)
                # Try find_package in module mode instead of config mode.
                find_package(${dep_name}
                    ${ocio_dep_RECOMMENDED_VERSION}
                    ${ocio_dep_COMPONENTS_string} 
                    ${ocio_dep_COMPONENTS}
                    ${ocio_dep_QUIET_string}
                    ${ocio_dep_UNPARSED_ARGUMENTS})
            endif()
            
            if(NOT ${dep_name}_FOUND AND ocio_dep_MIN_VERSION
            AND NOT ocio_dep_MIN_VERSION VERSION_EQUAL ocio_dep_MAX_VERSION
            AND NOT ocio_dep_MIN_VERSION VERSION_EQUAL ocio_dep_RECOMMENDED_VERSION)

                # if the recommended, or higher, version is not found, try to find dependency with 
                # the minimum version.
                find_package(${dep_name} 
                            ${ocio_dep_MIN_VERSION} 
                            ${ocio_dep_CONFIG_string}
                            ${ocio_dep_COMPONENTS_string} 
                            ${ocio_dep_COMPONENTS}
                            ${ocio_dep_QUIET_string}
                            ${ocio_dep_UNPARSED_ARGUMENTS})
                if (NOT ${dep_name}_FOUND AND ocio_dep_PREFER_CONFIG)
                    # Try find_package in module mode instead of config mode.
                    find_package(${dep_name} 
                        ${ocio_dep_MIN_VERSION}
                        ${ocio_dep_COMPONENTS_string} 
                        ${ocio_dep_COMPONENTS}
                        ${ocio_dep_QUIET_string}
                        ${ocio_dep_UNPARSED_ARGUMENTS})
                endif()
            endif()
        endif()
        
        # Check which VERSION_VARS was set by find_package.
        set(_VERSION_VAR "${dep_name}_VERSION")
        foreach (_vervar ${ocio_dep_VERSION_VARS})
            if(${_vervar})
                set(_VERSION_VAR ${_vervar})
                break()
            endif()
        endforeach()

        if(_VERSION_VAR)
            set(ocio_dep_VERSION ${${_VERSION_VAR}})
        endif()

        # Expecting that the minimum and recommended version are always provided.
        # Make sure that the version is within the valid range.
        if(${dep_name}_FOUND)
            if (ocio_dep_VERSION)
                # Make sure that the version found is not greater than the maximum version.
                if(DEFINED ocio_dep_MAX_VERSION)
                    if(ocio_dep_VERSION VERSION_GREATER ocio_dep_MAX_VERSION)
                        # Display it as an error, but do not abort right now.
                        message(SEND_ERROR "${ColorError}Found ${dep_name} ${ocio_dep_VERSION}, but it is over the maximum version \"${ocio_dep_MAX_VERSION}\" ${ColorReset}")
                        set(_${dep_name}_found_displayed true)
                    endif()
                endif()
                
                if(DEFINED ocio_dep_RECOMMENDED_VERSION)
                    if (ocio_dep_VERSION VERSION_LESS ocio_dep_RECOMMENDED_VERSION)
                        message(STATUS "${ColorSuccess}Found ${dep_name} (version \"${ocio_dep_VERSION}\") (recommended version: \"${ocio_dep_RECOMMENDED_VERSION}\")${ColorReset}")
                        if (ocio_dep_RECOMMENDED_VERSION_REASON)
                            message(STATUS "   Reason: ${ocio_dep_RECOMMENDED_VERSION_REASON}")
                        endif()
                        set(_${dep_name}_found_displayed true)
                    endif()
                endif()
                
                if(NOT _${dep_name}_found_displayed)
                    message(STATUS "${ColorSuccess}Found ${dep_name} (version \"${ocio_dep_VERSION}\")${ColorReset}")
                endif()
            else()
                message(STATUS "${ColorSuccess}Found ${dep_name} (no version information)${ColorReset}")
            endif()
        else()
            if(ocio_dep_REQUIRED AND NOT ocio_dep_ALLOW_INSTALL)
                set(message_color "${ColorError}")
            else()
                set(message_color "${ColorReset}")
            endif()

            ocio_print_versions_error(${dep_name} ${message_color})

            if(${dep_name}_ROOT)
                message(STATUS "${message_color}   ${dep_name}_ROOT was: ${${dep_name}_ROOT} ${ColorReset}")
            elseif($ENV{${dep_name}_ROOT})
                message(STATUS "${message_color}   ENV ${dep_name}_ROOT was: ${${dep_name}_ROOT} ${ColorReset}")
            endif()

            if(ocio_dep_ALLOW_INSTALL)
                ocio_install_dependency(${dep_name} VERSION ${ocio_dep_RECOMMENDED_VERSION})
            endif()

            if(ocio_dep_REQUIRED)
                if(NOT ${dep_name}_FOUND)
                    message(SEND_ERROR "${ColorError}${dep_name} is required, will abort at the end.${ColorReset}")
                endif()
            endif()
        endif()
    elseif(ocio_dep_FORCE_INSTALLATION)
        # Skip the search and install dependency right away.
        ocio_install_dependency(${dep_name} VERSION ${ocio_dep_RECOMMENDED_VERSION})
    endif()

    if(${dep_name}_FOUND AND ocio_dep_PROMOTE_TARGET)
        foreach (_target_to_be_promoted_ ${ocio_dep_PROMOTE_TARGET})
            set_target_properties(${_target_to_be_promoted_} PROPERTIES IMPORTED_GLOBAL TRUE)
        endforeach()
    endif()
endmacro()