// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_ACES2INIT_H
#define INCLUDED_OCIO_ACES2INIT_H

#include "Common.h"

namespace OCIO_NAMESPACE
{

namespace ACES2
{

JMhParams init_JMhParams(const Primaries &P);

ToneScaleParams init_ToneScaleParams(float peakLuminance);

ChromaCompressParams init_ChromaCompressParams(float peakLuminance);

GamutCompressParams init_GamutCompressParams(float peakLuminance, const Primaries &P);

} // namespace ACES2

} // OCIO namespace

#endif
