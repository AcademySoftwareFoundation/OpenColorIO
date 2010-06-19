#include <OpenColorSpace/OpenColorSpace.h>
namespace OCS = OCS_NAMESPACE;

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
void createConfig();
void loadConfig();

int main(int argc, const char* argv[])
{
    (void)argc;
    (void)argv;
    std::cout << std::endl;
    
    try
    {
        loadConfigFromEnv();
        
        //testCoordinateTransform();
        //createConfig();
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

void loadConfigFromEnv()
{
    OCS::ConstConfigRcPtr config = OCS::GetCurrentConfig();
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
    
    OCS::PackedImageDesc img(&imageVec[0], width, height, numChannels);
    std::cout << "img " << img << std::endl;
    
    OCS::ConstColorSpaceRcPtr srcColorSpace = config->getColorSpaceForRole(OCS::ROLE_COMPOSITING_LOG);
    OCS::ConstColorSpaceRcPtr dstColorSpace = config->getColorSpaceForRole(OCS::ROLE_SCENE_LINEAR);
    
    /*
    OCS::ConstColorSpaceRcPtr srcColorSpace = config->getColorSpaceByName("lnf");
    OCS::ConstColorSpaceRcPtr dstColorSpace = config->getColorSpaceByName("qt8");
    */
    
    imageVec[0] = 445.0f/1023.0f;
    std::cout << srcColorSpace->getName() << " ";
    PrintColor(std::cout, &imageVec[0], "input");
    std::cout << std::endl;
    
    std::cout << dstColorSpace->getName() << " ";
    config->applyTransform(img, srcColorSpace, dstColorSpace);
    PrintColor(std::cout, &imageVec[0], "transformed");
    std::cout << std::endl;
    
    std::cout << srcColorSpace->getName() << " ";
    config->applyTransform(img, dstColorSpace, srcColorSpace);
    PrintColor(std::cout, &imageVec[0], "round trip");
    std::cout << std::endl;
}


void createConfig()
{
    OCS::ConfigRcPtr config = OCS::Config::Create();
    
    config->setResourcePath("/net/soft_scratch/users/jeremys/git/Color/configs/spivfx/luts");
    
    // Add a colorspace
    {
        OCS::ColorSpaceRcPtr cs = OCS::ColorSpace::Create();
        cs->setName("lnh");
        cs->setFamily("ln");
        cs->setBitDepth(OCS::BIT_DEPTH_F16);
        cs->setIsData(false);
        cs->setHWAllocation(OCS::HW_ALLOCATION_LG2);
        cs->setHWMin(-16.0);
        cs->setHWMax(6.0);
        config->addColorSpace(cs);
        
        config->setColorSpaceForRole( OCS::ROLE_SCENE_LINEAR, cs->getName() );
    }
    
    // Add a colorspace
    {
        OCS::ColorSpaceRcPtr cs = OCS::ColorSpace::Create();
        cs->setName("lgh");
        cs->setFamily("lg");
        cs->setBitDepth(OCS::BIT_DEPTH_F16);
        cs->setIsData(false);
        cs->setHWAllocation(OCS::HW_ALLOCATION_UNIFORM);
        cs->setHWMin(-0.2f);
        cs->setHWMax(1.615f);
        
        OCS::FileTransformRcPtr transform = OCS::FileTransform::Create();
        transform->setSrc("lgf.spi1d");
        transform->setInterpolation(OCS::INTERP_LINEAR);
        
        OCS::GroupTransformRcPtr groupTransform = OCS::GroupTransform::Create();
        groupTransform->push_back(transform);
        cs->setTransform( groupTransform, OCS::COLORSPACE_DIR_TO_REFERENCE);
        
        config->addColorSpace(cs);
        
        config->setColorSpaceForRole( OCS::ROLE_COMPOSITING_LOG, cs->getName() );
    }
    
    {
        std::string outputname = "/mcp/test.ocs";
        std::cout << "Writing " << outputname << std::endl;
        std::ofstream outfile(outputname.c_str());
        config->writeXML(outfile);
        outfile.close();
        
        //config.loadFromFile(outputname.c_str());
    }
    /*
    {
        std::string outputname = "/mcp/test2.ocs";
        std::cout << "Writing " << outputname << std::endl;
        std::ofstream outfile(outputname.c_str());
        config.writeXML(outfile);
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
    
    OCS::PackedImageDesc img1(&imageVec1[0], width, height, numChannels);
    std::cout << "img1 " << img1 << std::endl;
    
    timeval t;
    gettimeofday(&t, 0);
    double aaa = (double) t.tv_sec + (double) t.tv_usec / 1000000.0;
    /*
    OCS::ApplyTransform(img1,
                        OCS::COORDINATE_SPACE_RGB,
                        OCS::COORDINATE_SPACE_HSV);
    */
    gettimeofday(&t, 0);
    double bbb = (double) t.tv_sec + (double) t.tv_usec / 1000000.0;
    
    double timeS = bbb-aaa;
    
    std::cout << std::endl;
    
    printf("time %0.1f ms  - %0.1f fps\n", timeS*1000.0, 1.0/timeS);
}
