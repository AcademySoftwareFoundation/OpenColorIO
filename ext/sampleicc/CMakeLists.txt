# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

# Sample ICC (modified)
# http://sampleicc.sourceforge.net
add_library(sampleicc::sampleicc INTERFACE IMPORTED GLOBAL)
set_target_properties(sampleicc::sampleicc PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/src/include"
)
