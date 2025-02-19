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
SharedCompressionParameters init_SharedCompressionParams(float peakLuminance, const JMhParams &inputJMhParams, const JMhParams &reachParams);
ResolvedSharedCompressionParameters resolve_CompressionParams(float hue, const SharedCompressionParameters &p);
ChromaCompressParams init_ChromaCompressParams(float peakLuminance, const ToneScaleParams &tsParams);
GamutCompressParams init_GamutCompressParams(float peakLuminance, const JMhParams &inputJMhParams, const JMhParams &limitJMhParams,
                                             const ToneScaleParams &tsParams, const SharedCompressionParameters &shParams, const JMhParams &reachParams);

f3 RGB_to_Aab(const f3 &RGB, const JMhParams &p);
f3 Aab_to_JMh(const f3 &Aab, const JMhParams &p);
f3 RGB_to_JMh(const f3 &RGB, const JMhParams &p);
f3 JMh_to_Aab(const f3 &JMh, const JMhParams &p);
f3 JMh_to_Aab(const f3 &JMh, const float &cos_hr, const float &sin_hr, const JMhParams &p);
f3 Aab_to_RGB(const f3 &Aab, const JMhParams &p);
f3 JMh_to_RGB(const f3 &JMh, const JMhParams &p);

float tonescale_fwd(const float J, const JMhParams &p, const ToneScaleParams &pt);
float tonescale_inv(const float J, const JMhParams &p, const ToneScaleParams &pt);
float tonescale_A_to_J_fwd(const float A, const JMhParams &p, const ToneScaleParams &pt);

float chroma_compress_norm(float cos_hr, float sin_hr, float chroma_compress_scale);

f3 chroma_compress_fwd(const f3 &JMh, const float J_ts, const float Mnorm, const ResolvedSharedCompressionParameters &ps, const ChromaCompressParams &pc);
f3 chroma_compress_inv(const f3 &JMh, const float J, const float Mnorm, const ResolvedSharedCompressionParameters &ps, const ChromaCompressParams &pc);

f3 gamut_compress_fwd(const f3 &JMh, const ResolvedSharedCompressionParameters &ps, const GamutCompressParams &p);
f3 gamut_compress_inv(const f3 &JMh, const ResolvedSharedCompressionParameters &ps, const GamutCompressParams &p);


} // namespace ACES2

} // OCIO namespace

#endif
