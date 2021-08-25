// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OFX_OCIOCOLORSPACE_H
#define INCLUDED_OFX_OCIOCOLORSPACE_H

#include "OCIOUtils.h"

#include "ofxsImageEffect.h"

class OCIOColorSpace : public OFX::ImageEffect
{
protected:
    // Do not need to delete these. The ImageEffect is managing them for us.
    OFX::Clip *dstClip_;
    OFX::Clip *srcClip_;

    OFX::ChoiceParam * srcCsNameParam_;
    OFX::ChoiceParam * dstCsNameParam_;
    OFX::BooleanParam * inverseParam_;
    OFX::PushButtonParam * swapSrcDstParam_;

    ParamMap contextParams_;

public:
    OCIOColorSpace(OfxImageEffectHandle handle);

    /* Override the render */
    void render(const OFX::RenderArguments & args) override;

    /* Override identity (~no-op) check */
    bool isIdentity(const OFX::IsIdentityArguments & args, 
                    OFX::Clip *& identityClip, 
                    double & identityTime) override;

    /* Override changedParam */
    void changedParam(const OFX::InstanceChangedArgs & args, 
                      const std::string & paramName) override;
};

mDeclarePluginFactory(OCIOColorSpaceFactory, {}, {});

#endif // INCLUDED_OFX_OCIOCOLORSPACE_H
