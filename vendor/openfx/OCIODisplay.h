// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OFX_OCIODISPLAY_H
#define INCLUDED_OFX_OCIODISPLAY_H

#include <map>
#include <string>

#include "ofxsImageEffect.h"

class OCIODisplay : public OFX::ImageEffect
{
protected:
    // Do not need to delete these. The ImageEffect is managing them for us.
    OFX::Clip *dstClip_;
    OFX::Clip *srcClip_;

    OFX::ChoiceParam * srcCsNameParam_;
    OFX::ChoiceParam * displayParam_;
    OFX::ChoiceParam * viewParam_;
    OFX::BooleanParam * inverseParam_;

    std::map<std::string, OFX::StringParam *> contextParams_;

public:
    OCIODisplay(OfxImageEffectHandle handle);

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

mDeclarePluginFactory(OCIODisplayFactory, {}, {});

#endif // INCLUDED_OFX_OCIODISPLAY_H
