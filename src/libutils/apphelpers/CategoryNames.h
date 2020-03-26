// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_CATEGORY_NAMES_H
#define INCLUDED_OCIO_CATEGORY_NAMES_H


#include <OpenColorIO/OpenColorIO.h>


namespace OCIO_NAMESPACE
{

//
// The following namespace contains expected categories.
//

namespace ColorSpaceCategoryNames
{

constexpr char Input[] = "input";

constexpr char SceneLinearWorkingSpace[] = "scene_linear_working_space";
constexpr char LogWorkingSpace[] = "log_working_space";
constexpr char VideoWorkingSpace[] = "video_working_space";

constexpr char LutInputSpace[] = "lut_input_space";

};


} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_CATEGORY_NAMES_H
