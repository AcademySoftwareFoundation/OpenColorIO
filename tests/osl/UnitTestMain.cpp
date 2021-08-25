// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OSL/oslcomp.h>
#include <OSL/oslexec.h>
#include <OSL/rendererservices.h>

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include "UnitTestMain.h"

#include <memory>
#include <vector>


namespace
{

class MyOSLShaderCreator : public OCIO::GpuShaderCreator
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

    // TODO: OSL: Throw when unexpected methods are used.

    void setTextureMaxWidth(unsigned /*maxWidth*/) override {}
    unsigned getTextureMaxWidth() const noexcept override { return 0; }

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

class ErrorRecorder final : public OIIO::ErrorHandler
{
public:
    ErrorRecorder() : ErrorHandler() {}
 
    virtual void operator()(int errcode, const std::string& msg)
    {
        if (errcode >= EH_ERROR) {
            if (m_errormessage.size()
                && m_errormessage[m_errormessage.length() - 1] != '\n')
                m_errormessage += '\n';
            m_errormessage += msg;
        }
    }

    bool haserror() const { return m_errormessage.size(); }
 
    std::string geterror(bool erase = true)
    {
        std::string s;
        if (erase)
            std::swap(s, m_errormessage);
        else
            s = m_errormessage;
        return s;
    }

private:
    std::string m_errormessage;
} g_errhandler;



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

} //anon namespace


OSLDataRcPtr OSLData::Create()
{ 
    OSLDataRcPtr ptr = std::make_shared<OSLData>(); 
    AddUnitTest(ptr);
    return ptr;
}

int main(int, const char **)
{
    const std::string shaders{OCIO::GetEnvVariable("SHADERS_DIR")};
    const std::string optionShaders{"-I" + shaders};
    const std::vector<std::string> options{ optionShaders, "-Werror" };

    size_t failures = 0;
    size_t noTest   = 0;

    std::cerr << "\n OpenColorIO_Core_OSL_Unit_Tests\n\n";

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

        OSL::RendererServices * renderer = new OSL::RendererServices();
        OIIO::TextureSystem * texturesys = OIIO::TextureSystem::create();

        OSL::ShadingSystem * shadingsys = new OSL::ShadingSystem(renderer, texturesys);
        OSL::ShaderGroupRef shadergroup = shadingsys->ShaderGroupBegin();
        
        // TODO: OSL: Create the OSL group!

        try
        {
            OCIO::ConstConfigRcPtr config = OCIO::Config::CreateRaw();
            OCIO::ConstProcessorRcPtr processor = config->getProcessor(test->m_transform);
            OCIO::ConstGPUProcessorRcPtr gpu = processor->getOptimizedGPUProcessor(OCIO::OPTIMIZATION_NONE);

            OCIO::GpuShaderCreatorRcPtr shaderDesc = MyOSLShaderCreator::Create(test);
            gpu->extractGpuShaderInfo(shaderDesc);

            OSL::OSLCompiler compiler(&g_errhandler);
            const std::string oslBuffer
                = dynamic_cast<MyOSLShaderCreator*>(shaderDesc.get())->getShaderCode();
            std::string osoBuffer;

            if (!compiler.compile_buffer(oslBuffer, osoBuffer, options))
            {
                failures++;
                std::cerr << "FAILED\n";

                std::string errormessage;

                if (g_errhandler.haserror())
                {
                    errormessage = g_errhandler.geterror();
                }
                else
                {
                    errormessage = "OSL: Could not compile the shader";
                }

                std::cerr << errormessage << std::endl;

                continue;
            }

            if (!shadingsys->LoadMemoryCompiledShader("shadername", osoBuffer))
            {
                failures++;
                std::cerr << "FAILED\n";

                std::string errormessage;

                if (g_errhandler.haserror())
                {
                    errormessage = g_errhandler.geterror();
                }
                else
                {
                    errormessage = "OSL: Could not load the shader";
                }

                std::cerr << errormessage << std::endl;

                continue;
            }


            // TODO: OSL: Add the CPU execution of the shader to validate the processing.


            std::cerr << "PASSED\n";
        }
        catch(std::exception & ex)
        {
            failures++;
            std::cerr << "FAILED\n";
            std::cerr << ex.what() << std::endl;
        }
        catch(...)
        {
            failures++;
            std::cerr << "FAILED\n";
            std::cerr << "Unexpected exception!" << std::endl;
        }

        shadergroup.reset();

        delete shadingsys;
        delete renderer;

        noTest++;
    }

    std::cerr << std::endl << failures << " tests failed" << std::endl << std::endl;
    return failures;
}
