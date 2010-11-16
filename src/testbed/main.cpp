#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/time.h>
#include <vector>

void loadConfigFromEnv();
void testCoordinateTransform();
void testASCTransform();
void testFilmlooks();
void createConfig();
void loadConfig();

int main(int argc, const char* argv[])
{
    (void)argc;
    (void)argv;
    std::cout << std::endl;
    
    try
    {
        //testFilmlooks();
        //loadConfigFromEnv();
        //testCoordinateTransform();
        createConfig();
        //testASCTransform();
        
    }
    catch(std::exception& e)
    {
        std::cout << "Exception: " << e.what() << std::endl;
    }
    
    std::cout << "Done." << std::endl;
    return 0;
}

namespace
{
    void PrintColor(std::ostream & os,
                    const float * c,
                    const std::string & name)
    {
        os << name << " : " << c[0] << " " << c[1] << " " << c[2];
    }
}

void testASCTransform()
{
    float slope[3] = { 1.2f, 0.8f, 0.9f };
    
    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    cdl->setSat(2.0);
    cdl->setSlope(slope);
    cdl->setID("bs020");
    cdl->setDescription("taco");
    
    std::cerr << "xml " << cdl->getXML() << std::endl;
    
}

void testFilmlooks()
{
    OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
    
    int numDisplayDevices = config->getNumDisplayDeviceNames();
    std::cout << "numDisplayDevices " << numDisplayDevices << std::endl;
    std::cout << "Default device: " << config->getDefaultDisplayDeviceName() << std::endl;
    std::cout << std::endl;
    
    for(int i=0; i<numDisplayDevices; ++i)
    {
        const char * device = config->getDisplayDeviceName(i);
        std::cout << " Device: " << device << std::endl;
        std::cout << " Default transform " << config->getDefaultDisplayTransformName(device) << std::endl;
        
        int numTransforms = config->getNumDisplayTransformNames(device);
        for(int j=0; j<numTransforms; ++j)
        {
            const char * displayTransformName = config->getDisplayTransformName(device, j);
            std::cout << "     ";
            std::cout << displayTransformName << " = ";
            std::cout << config->getDisplayColorSpaceName(device, displayTransformName);
            std::cout << "\n";
        }
    }
}


void loadConfigFromEnv()
{
    OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
    // std::cout << *config << std::endl;
    
    
    std::vector<float> imageVec;
    
    int width = 123;
    int height = 456;
    int numChannels = 4;
    
    imageVec.resize(width*height*numChannels);
    memset(&imageVec[0], 0, imageVec.size()*sizeof(float));
    
    srand48(0);
    
    for(int i=0; i<width*height*numChannels; ++i)
    {
        imageVec[i] = (float)drand48();
    }
    
    OCIO::PackedImageDesc img(&imageVec[0], width, height, numChannels);
    std::cout << "img " << img << std::endl;
    
    //OCIO::ConstColorSpaceRcPtr csSrc = config->getColorSpace(OCIO::ROLE_COMPOSITING_LOG);
    //OCIO::ConstColorSpaceRcPtr csDst = config->getColorSpace(OCIO::ROLE_SCENE_LINEAR);
    
    OCIO::ConstColorSpaceRcPtr csSrc = config->getColorSpace("dt8");
    OCIO::ConstColorSpaceRcPtr csDst = config->getColorSpace("lnh");
    
    imageVec[0] = 445.0f/1023.0f;
    imageVec[1] = 1023.0/1023.0f;
    imageVec[2] = 0.0/1023.0f;
    std::cout << csSrc->getName() << " ";
    PrintColor(std::cout, &imageVec[0], "input");
    std::cout << std::endl;
    
    std::cout << csDst->getName() << " ";
    OCIO::ConstProcessorRcPtr p1 = config->getProcessor(csSrc, csDst);
    p1->apply(img);
    PrintColor(std::cout, &imageVec[0], "transformed");
    std::cout << std::endl;
    
    std::cout << csSrc->getName() << " ";
    OCIO::ConstProcessorRcPtr p2 = config->getProcessor(csDst, csSrc);
    p2->apply(img);
    
    PrintColor(std::cout, &imageVec[0], "round trip");
    std::cout << std::endl;
    
    /*
    OCIO::GpuShaderDesc shaderDesc;
    shaderDesc.setLut3DEdgeLen(32);
    shaderDesc.setFunctionName("ocio_color_convert");
    shaderDesc.setLanguage(OCIO::GPU_LANGUAGE_CG);
    std::cout << p2->getGPUShaderText(shaderDesc) << std::endl;
    */
    
    {
        std::string outputname = "/tmp/test1.ocio";
        std::cout << "Writing " << outputname << std::endl;
        std::ofstream outfile(outputname.c_str());
        config->serialize(outfile);
        outfile.close();
    }
}


void createConfig()
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    
    config->setResourcePath("luts");
    
    // Add a colorspace
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("lnh");
        cs->setFamily("ln");
        cs->setBitDepth(OCIO::BIT_DEPTH_F16);
        cs->setIsData(false);
        cs->setAllocation(OCIO::ALLOCATION_LG2);
        float vars[] = {-16.0f, 6.0f};
        cs->setAllocationVars(2, vars);
        
        config->addColorSpace(cs);
        
        config->setRole( OCIO::ROLE_SCENE_LINEAR, cs->getName() );
    }
    
    // Add a colorspace
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("lgh");
        cs->setFamily("lg");
        cs->setBitDepth(OCIO::BIT_DEPTH_F16);
        cs->setIsData(false);
        cs->setAllocation(OCIO::ALLOCATION_UNIFORM);
        float vars[] = {-0.2f, 1.6f};
        cs->setAllocationVars(2, vars);
        
        OCIO::FileTransformRcPtr transform = OCIO::FileTransform::Create();
        transform->setSrc("lgf.spi1d");
        transform->setInterpolation(OCIO::INTERP_LINEAR);
        
        OCIO::GroupTransformRcPtr groupTransform = OCIO::GroupTransform::Create();
        groupTransform->push_back(transform);
        cs->setTransform( groupTransform, OCIO::COLORSPACE_DIR_TO_REFERENCE);
        
        config->addColorSpace(cs);
        
        config->setRole( OCIO::ROLE_COMPOSITING_LOG, cs->getName() );
    }
    
    {
        std::string outputname = "/tmp/test2.ocio";
        std::cout << "Writing " << outputname << std::endl;
        std::ofstream outfile(outputname.c_str());
        config->serialize(outfile);
        outfile.close();
        
        //config.loadFromFile(outputname.c_str());
    }
    /*
    {
        std::string outputname = "/mcp/test2.ocio";
        std::cout << "Writing " << outputname << std::endl;
        std::ofstream outfile(outputname.c_str());
        config.serialize(outfile);
        outfile.close();
    }
    */
}


void testCoordinateTransform()
{
    std::cout << "Test 1" << std::endl;
    std::vector<float> imageVec1, imageVec2;
    int width = 2048;
    int height = 1556;
    int numChannels = 4;
    
    imageVec1.resize(width*height*numChannels);
    memset(&imageVec1[0], 0, imageVec1.size()*sizeof(float));
    
    srand48(0);
    
    for(int i=0; i<width*height*numChannels; ++i)
    {
        imageVec1[i] = (float)drand48();
    }
    
    OCIO::PackedImageDesc img1(&imageVec1[0], width, height, numChannels);
    std::cout << "img1 " << img1 << std::endl;
    
    timeval t;
    gettimeofday(&t, 0);
    double aaa = (double) t.tv_sec + (double) t.tv_usec / 1000000.0;
    /*
    OCIO::ApplyTransform(img1,
                        OCIO::COORDINATE_SPACE_RGB,
                        OCIO::COORDINATE_SPACE_HSV);
    */
    gettimeofday(&t, 0);
    double bbb = (double) t.tv_sec + (double) t.tv_usec / 1000000.0;
    
    double timeS = bbb-aaa;
    
    std::cout << std::endl;
    
    printf("time %0.1f ms  - %0.1f fps\n", timeS*1000.0, 1.0/timeS);
}
