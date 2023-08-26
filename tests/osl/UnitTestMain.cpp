// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <fstream>
#include <memory>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include "UnitTestMain.h"
#include "UnitTestOSL.h"

#include <OSL/oslcomp.h>


namespace
{

class MyOSLShaderCreator final : public OCIO::GpuShaderCreator
{
public:
    static OCIO::GpuShaderCreatorRcPtr Create(const OSLDataRcPtr & test)
    {
        OCIO::GpuShaderCreatorRcPtr ptr(new MyOSLShaderCreator(test));
        ptr->setLanguage(OCIO::LANGUAGE_OSL_1);
        return ptr;
    }

    const std::string & getShaderCode() const noexcept { return m_shaderCode; }

    OCIO::GpuShaderCreatorRcPtr clone() const override { return OCIO::GpuShaderCreatorRcPtr(); } 

    void setTextureMaxWidth(unsigned /*maxWidth*/) override {}
    unsigned getTextureMaxWidth() const noexcept override { return 0; }

    void setAllowTexture1D(bool /*allowed*/) override {}
    bool getAllowTexture1D() const noexcept override { return true; }

    bool addUniform(const char * /*name*/, const DoubleGetter & /*getDouble*/) override
    {
        throw OCIO::Exception("Unsupported by OSL unit tests.");
        return false; 
    }
    bool addUniform(const char * /*name*/, const BoolGetter &   /*getBool*/)   override
    {
        throw OCIO::Exception("Unsupported by OSL unit tests.");
        return false; 
    }
    bool addUniform(const char * /*name*/, const Float3Getter & /*getFloat3*/) override
    {
        throw OCIO::Exception("Unsupported by OSL unit tests.");
        return false; 
    }

    bool addUniform(const char * /*name*/, 
                    const SizeGetter & /*getSize*/,
                    const VectorFloatGetter & /*getVectorFloat*/) override
    {
        throw OCIO::Exception("Unsupported by OSL unit tests.");
        return false; 
    }

    bool addUniform(const char * /*name*/,
                    const SizeGetter & /*getSize*/,
                    const VectorIntGetter & /*getVectorInt*/) override
    {
        throw OCIO::Exception("Unsupported by OSL unit tests.");
        return false; 
    }

    void addTexture(const char * /*textureName*/,
                    const char * /*samplerName*/,
                    unsigned /*width*/, 
                    unsigned /*height*/,
                    TextureType /*channel*/,
                    TextureDimensions /*dimensions*/,
                    OCIO::Interpolation /*interpolation*/,
                    const float * /*values*/) override
    {
        throw OCIO::Exception("Unsupported by OSL unit tests.");
    }

    void add3DTexture(const char * /*textureName*/,
                      const char * /*samplerName*/,
                      unsigned /*edgelen*/,
                      OCIO::Interpolation /*interpolation*/,
                      const float * /*values*/) override
    {
        throw OCIO::Exception("Unsupported by OSL unit tests.");
    }

    void createShaderText(const char * shaderDeclarations,
                          const char * shaderHelperMethods,
                          const char * shaderFunctionHeader,
                          const char * shaderFunctionBody,
                          const char * shaderFunctionFooter) override
    {
        m_shaderCode += shaderDeclarations;
        m_shaderCode += shaderHelperMethods;
        m_shaderCode += shaderFunctionHeader;
        m_shaderCode += shaderFunctionBody;
        m_shaderCode += shaderFunctionFooter;
    }

protected:
    MyOSLShaderCreator(const OSLDataRcPtr & test) : OCIO::GpuShaderCreator(), m_test(test) {}

private:
    const OSLDataRcPtr m_test;
    std::string m_shaderCode;
};



typedef std::vector<OSLDataRcPtr> UnitTests;

UnitTests & GetUnitTests()
{
    static UnitTests ocio_osl_unit_tests;
    return ocio_osl_unit_tests;
}

void AddUnitTest(const OSLDataRcPtr & test)
{
    GetUnitTests().push_back(test);
}

void logFunction(const char *)
{
    // Mute all messages, warnings, etc.
}

} // anon. namespace


OSLDataRcPtr OSLData::Create()
{ 
    OSLDataRcPtr ptr = std::make_shared<OSLData>(); 
    AddUnitTest(ptr);
    return ptr;
}

int main(int, const char **)
{
    const std::string oslShaderPath{OCIO::GetEnvVariable("OSL_SHADERS_DIR")};
    const std::string newShaderPath{OCIO::GetEnvVariable("TMP_SHADERS_DIR")};
    const std::string shaderName{"OSL_OCIOMain"};

    const std::string oslShaderOption{"-I" + oslShaderPath};
    const std::vector<std::string> options{ oslShaderOption, "-Werror" };

    size_t failures = 0;
    size_t noTest   = 0;

    std::cerr << "\n OpenColorIO_Core_OSL_Unit_Tests\n\n";

    OCIO::SetLoggingFunction(logFunction);

    for (const auto & test : GetUnitTests())
    {
        constexpr size_t maxCharToDisplay = 59;
        if (test->m_name.size() > maxCharToDisplay)
        {
            test->m_name.resize(maxCharToDisplay);
        }

        std::cerr << "["
                  << std::right << std::setw(3)
                  << (noTest+1) << "/" << GetUnitTests().size() << "] ["
                  << std::left << std::setw(maxCharToDisplay+1)
                  << test->m_name 
                  << "] - ";

        std::string oslShaderString;

        try
        {
            // Create the OSL shader.

            OCIO::ConstProcessorRcPtr processor
                = test->m_config->getProcessor(test->m_transform);
            OCIO::ConstCPUProcessorRcPtr cpu
                = processor->getOptimizedCPUProcessor(OCIO::OPTIMIZATION_NONE);

            OCIO::ConstGPUProcessorRcPtr gpu
                = processor->getOptimizedGPUProcessor(OCIO::OPTIMIZATION_NONE);

            OCIO::GpuShaderCreatorRcPtr shaderDesc = MyOSLShaderCreator::Create(test);
            gpu->extractGpuShaderInfo(shaderDesc);

            oslShaderString = dynamic_cast<MyOSLShaderCreator*>(shaderDesc.get())->getShaderCode();

            // Compute the expected output values for the following input values.

            static const Image g_inValues{
                { 0.00f,  0.02f,  0.04f,  0.06f},
                { 0.08f,  0.10f,  0.12f,  0.14f},
                { 0.16f,  0.18f,  0.20f,  0.22f},
                { 0.24f,  0.26f,  0.28f,  0.30f},
                { 0.32f,  0.34f,  0.36f,  0.38f},
                { 0.40f,  0.42f,  0.44f,  0.46f},
                { 0.48f,  0.50f,  0.52f,  0.54f},
                { 0.56f,  0.58f,  0.60f,  0.62f},
                { 0.64f,  0.66f,  0.68f,  0.70f},
                { 0.72f,  0.74f,  0.76f,  0.78f},
                { 0.80f,  0.82f,  0.84f,  0.86f},
                { 0.90f,  0.92f,  0.94f,  0.96f},

                { 0.00f,  0.00f,  0.00f,  0.00f},
                { 1.10f,  1.10f,  1.10f,  1.00f},
                { 0.50f,  0.40f,  0.30f,  0.00f},
                { 1.00f,  0.00f,  0.00f,  1.00f},
                { 0.90f,  0.10f,  0.20f,  0.00f},
                {-0.10f, -0.10f, -0.10f,  0.00f},
                {-0.10f,  0.40f,  1.10f,  1.00f}
            };

            // Used the custom input values if any.
            const Image inValues = test->m_inputValues.empty() ? g_inValues : test->m_inputValues;
            Image outValues      = inValues;
            
            for (size_t idx = 0; idx < inValues.size(); ++idx)
            {
                cpu->applyRGBA((float *)&outValues[idx]);
            }

            // Compile the OSL shader i.e. no need to execute if the compilation fails.

            ErrorRecorder msg;
            OSL::OSLCompiler compiler(&msg);
            std::string osoShaderBuffer;

            if (!compiler.compile_buffer(oslShaderString, osoShaderBuffer, options))
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

            // Save the OSL shader (use the compiled version as it's now available).

            {
                std::string filepath{newShaderPath};
                filepath += "/";
                filepath += shaderName;
                filepath += ".oso";
                std::ofstream out(filepath);
                out << osoShaderBuffer;
            }

            // Excecute in-memory the OSL shader.

            ExecuteOSLShader(shaderName,
                             inValues,
                             outValues, 
                             test->m_threshold,
                             test->m_expectedMinimalValue,
                             test->m_relativeComparison);

            std::cerr << "PASSED" << std::endl;
        }
        catch(std::exception & ex)
        {
            failures++;

            std::cerr << "FAILED" << std::endl;
            std::cerr << ex.what() << std::endl;

            std::cerr << "\n***********\n";
            std::cerr << oslShaderString;
            std::cerr << "\n***********\n";
        }
        catch(...)
        {
            failures++;

            std::cerr << "FAILED" << std::endl;
            std::cerr << "Unexpected exception!" << std::endl;

            std::cerr << "\n***********\n";
            std::cerr << oslShaderString;
            std::cerr << "\n***********\n";
        }

        noTest++;
    }

    std::cerr << std::endl << failures << " tests failed" << std::endl << std::endl;
    return failures;
}
