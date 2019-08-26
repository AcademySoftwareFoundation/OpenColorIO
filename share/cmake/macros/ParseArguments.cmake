# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.
#
# The PARSE_ARGUMENTS macro will take the arguments of another macro and define
# several variables. The first argument to PARSE_ARGUMENTS is a prefix to put
# on all variables it creates. The second argument is a list of names, and the
# third argument is a list of options. Both of these lists should be quoted.
# The rest of the PARSE_ARGUMENTS args are arguments from another macro to be
# parsed.
#
#     PARSE_ARGUMENTS(prefix arg_names options arg1 arg2...)
#
# For each item in options, PARSE_ARGUMENTS will create a variable with that
# name, prefixed with prefix_. So, for example, if prefix is MY_MACRO and
# options is OPTION1;OPTION2, then PARSE_ARGUMENTS will create the variables
# MY_MACRO_OPTION1 and MY_MACRO_OPTION2. These variables will be set to true if
# the option exists in the command line or false otherwise.
#
# For each item in arg_names, PARSE_ARGUMENTS will create a variable with that
# name, prefixed with prefix_. Each variable will be filled with the arguments
# that occur after the given arg_name is encountered up to the next arg_name or
# the end of the arguments. All options are removed from these lists.
# PARSE_ARGUMENTS also creates a prefix_DEFAULT_ARGS variable containing the
# list of all arguments up to the first arg_name encountered.
#
# Downloaded from: http://www.itk.org/Wiki/CMakeMacroParseArguments
#

cmake_minimum_required(VERSION 2.4.7)

MACRO(PARSE_ARGUMENTS prefix arg_names option_names)
    SET(DEFAULT_ARGS)
    FOREACH(arg_name ${arg_names})    
        SET(${prefix}_${arg_name})
    ENDFOREACH(arg_name)
    FOREACH(option ${option_names})
        SET(${prefix}_${option} FALSE)
    ENDFOREACH(option)
    SET(current_arg_name DEFAULT_ARGS)
    SET(current_arg_list)
    FOREACH(arg ${ARGN})            
        SET(larg_names ${arg_names})    
        LIST(FIND larg_names "${arg}" is_arg_name)                   
        IF (is_arg_name GREATER -1)
            SET(${prefix}_${current_arg_name} ${current_arg_list})
            SET(current_arg_name ${arg})
            SET(current_arg_list)
        ELSE (is_arg_name GREATER -1)
            SET(loption_names ${option_names})    
            LIST(FIND loption_names "${arg}" is_option)
            IF (is_option GREATER -1)
                SET(${prefix}_${arg} TRUE)
            ELSE (is_option GREATER -1)
                SET(current_arg_list ${current_arg_list} ${arg})
            ENDIF (is_option GREATER -1)
        ENDIF (is_arg_name GREATER -1)
    ENDFOREACH(arg)
    SET(${prefix}_${current_arg_name} ${current_arg_list})
ENDMACRO(PARSE_ARGUMENTS)
