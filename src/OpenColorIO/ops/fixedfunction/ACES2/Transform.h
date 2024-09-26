// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_ACES2_TRANSFORM_H
#define INCLUDED_OCIO_ACES2_TRANSFORM_H

#include "Common.h"

namespace OCIO_NAMESPACE
{

namespace ACES2
{

JMhParams init_JMhParams(const Primaries &P);
ToneScaleParams init_ToneScaleParams(float peakLuminance);
ChromaCompressParams init_ChromaCompressParams(float peakLuminance);
GamutCompressParams init_GamutCompressParams(float peakLuminance, const Primaries &P);

f3 RGB_to_JMh(const f3 &RGB, const JMhParams &p);
f3 JMh_to_RGB(const f3 &JMh, const JMhParams &p);

f3 tonescale_chroma_compress_fwd(const f3 &JMh, const JMhParams &p, const ToneScaleParams &pt, const ChromaCompressParams &pc);
f3 tonescale_chroma_compress_inv(const f3 &JMh, const JMhParams &p, const ToneScaleParams &pt, const ChromaCompressParams &pc);

f3 gamut_compress_fwd(const f3 &JMh, const GamutCompressParams &p);
f3 gamut_compress_inv(const f3 &JMh, const GamutCompressParams &p);


} // namespace ACES2

} // OCIO namespace

#endif
