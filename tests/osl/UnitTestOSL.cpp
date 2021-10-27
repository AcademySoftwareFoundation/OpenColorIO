// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "UnitTestOSL.h"

#include <OSL/oslexec.h>
#include <OSL/rendererservices.h>


namespace
{

// Define a userdata structure that holds any varying per-point values that
// might be retrieved by shader.
struct MyUserData
{
    OSL::Vec3 inColor_rgb{0.0f, 0.0f, 0.0f};
    float     inColor_a{1.0f};

    MyUserData() = delete;
    MyUserData(const Vec4 & c)
        : inColor_rgb(c[0], c[1], c[2])
        , inColor_a(c[3]) 
    {
    }

    // Make a retrieve-by-name function.
    bool retrieve(OSL::ustring name, OSL::TypeDesc type, void * val, bool /*derivatives*/)
    {
        static const OSL::ustring ucolor_rgb("inColor.rgb");
        if (name == ucolor_rgb && type == OIIO::TypeColor)
        {
            ((float*)val)[0] = inColor_rgb[0];
            ((float*)val)[1] = inColor_rgb[1];
            ((float*)val)[2] = inColor_rgb[2];

            return true;
        }

        static const OSL::ustring ucolor_a("inColor.a");
        if (name == ucolor_a && type == OIIO::TypeFloat)
        {
            ((float*)val)[0] = inColor_a;

            return true;
        }

        // Not a named value we know about, so just return false.
        return false;
    }
};


// RendererServices is the interface through which OSL requests things back
// from the app (called a "renderer", but it doesn't have to literally be
// one). The important feature we are concerned about here is that this is
// how "userdata" is retrieved. We set up a subclass that overloads
// get_userdata() to retrieve it from a per-point MyUserData that whose
// pointer is stored in shaderglobals.renderstate.
class MyRendererServices final : public OSL::RendererServices
{
public:
    virtual bool get_userdata(bool derivatives, OSL::ustring name,
                              OSL::TypeDesc type, OSL::ShaderGlobals * sg,
                              void * val)
    {
        // In this case, our implementation of get_userdata just requests
        // it from the MyUserData, which we have arranged is pointed to
        // by shaderglobals.renderstate.
        MyUserData * userdata = (MyUserData *)sg->renderstate;
        return userdata ? userdata->retrieve(name, type, val, derivatives)
                        : false;
    }
};

inline bool AbsoluteDifference(float x1, float x2, float diff)
{
    const float thisDiff = fabs(x2 - x1);
    if (thisDiff > diff)
    {
        diff = thisDiff;
        return true;
    }
    return false;
}

inline bool RelativeDifference(float x1, float x2, float min_x1, float diff)
{
    const float absx1 = fabs(x1);
    float div = std::max(absx1, min_x1);
    const float thisDiff = fabs(x1 - x2) / div;
    if (thisDiff > diff)
    {
        diff = thisDiff;
        return true;
    }
    return false;
}

// Return true if diff has been updated.
inline bool ComputeDiff(float x1, float x2, bool rel, float min_x1, float diff)
{
    if (rel)
    {
        return RelativeDifference(x1, x2, min_x1, diff);
    }
    else
    {
        return AbsoluteDifference(x1, x2, diff);
    }

}

} // anon.


void ExecuteOSLShader(const std::string & shaderName,
                      const Image & inValues,
                      const Image & outValues,
                      float threshold,
                      float minValue,
                      bool relativeComparison)
{
    MyRendererServices renderer;
    ErrorRecorder msg;
    std::unique_ptr<OSL::ShadingSystem> shadsys(new OSL::ShadingSystem(&renderer, nullptr, &msg));

    OSL::ShaderGroupRef mygroup = shadsys->ShaderGroupBegin("my_color_mgt");
    shadsys->Parameter(*mygroup, "inColor.rgb", 1.0f, /*lockgeom=*/false);
    shadsys->Parameter(*mygroup, "inColor.a", 1.0f, /*lockgeom=*/false);
    shadsys->Shader(*mygroup, "shader", shaderName, "layer1");
    shadsys->ShaderGroupEnd(*mygroup);

    const std::vector<OSL::ustring> output_names{ OSL::ustring("outColor.rgb"), OSL::ustring("outColor.a") };
    shadsys->attribute (mygroup.get(), "renderer_outputs",
                        OSL::TypeDesc(OSL::TypeDesc::STRING, output_names.size()),
                        output_names.data());

    // Now we want to create a context in which we can execute the shader.
    // We need one context per thread.
    OSL::PerThreadInfo * perthread = shadsys->create_thread_info();
    OSL::ShadingContext * ctx      = shadsys->get_context(perthread);

    // The group must already be optimized before we call find_symbol,
    // so we force that to happen now.
    shadsys->optimize_group(mygroup.get(), ctx);

    // Get a ShaderSymbol* handle to the final output we care about. This
    // will greatly speed up retrieving the value later, rather than by
    // looking it up by name on every shade.
    const OSL::ShaderSymbol * outsymRGB
        = shadsys->find_symbol(*mygroup.get(), OSL::ustring("layer1"), OSL::ustring("outColor.rgb"));
    OSL_ASSERT(outsymRGB);

    const OSL::ShaderSymbol * outsymA
        = shadsys->find_symbol(*mygroup.get(), OSL::ustring("layer1"), OSL::ustring("outColor.a"));
    OSL_ASSERT(outsymA);

    for (size_t idx = 0; idx < inValues.size(); ++idx)
    {
        OSL::ShaderGlobals shaderglobals;

        // Make a userdata record. Make sure the shaderglobals points to it.
        MyUserData userdata(inValues[idx]);
        shaderglobals.renderstate = &userdata;

        // Run the shader (will automagically optimize and JIT the first time it executes).
        if (!shadsys->execute(*ctx, *mygroup.get(), shaderglobals))
        {
            std::string errormessage;

            if (msg.haserror())
            {
                errormessage = msg.geterror();
            }
            else
            {
                errormessage = "OSL: Could not compile the shader";
            }

            throw std::runtime_error(errormessage);
        }

        const OSL::Vec3 outRGB = *(OSL::Vec3 *)shadsys->symbol_address(*ctx, outsymRGB);
        const float outA = *(float *)shadsys->symbol_address(*ctx, outsymA);

        // Check the result.

        if (   ComputeDiff(outValues[idx][0], outRGB[0], relativeComparison, minValue, threshold)
            || ComputeDiff(outValues[idx][1], outRGB[1], relativeComparison, minValue, threshold)
            || ComputeDiff(outValues[idx][2], outRGB[2], relativeComparison, minValue, threshold)
            || ComputeDiff(outValues[idx][3], outA,      relativeComparison, minValue, threshold))
        {
            std::stringstream str;
            str << "Values from [" 
                << inValues[idx][0] << ", " << inValues[idx][1] << ", " 
                << inValues[idx][2] << ", " << inValues[idx][3]
                << "] to [" 
                << outValues[idx][0] << ", " << outValues[idx][1] << ", "
                << outValues[idx][2] << ", " << outValues[idx][3]
                << "], but OSL computed values are [" 
                << outRGB[0] << ", " << outRGB[1] << ", " << outRGB[2] << ", " << outA << "].";

            throw std::runtime_error(str.str());
        }
    }

    // All done. Release the contexts and threadinfo for each thread:
    shadsys->release_context(ctx);
    shadsys->destroy_thread_info(perthread);

}
