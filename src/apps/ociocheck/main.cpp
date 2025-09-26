// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <set>
#include <vector>
#include <algorithm>

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include "apputils/argparse.h"
#include "apputils/logGuard.h"
#include "utils/StringUtils.h"


const char * DESC_STRING = "\n\n"
"Ociocheck is useful to validate that the specified OCIO configuration\n"
"is valid, and that all the color transforms are defined and loadable.\n"
"For example, it is possible that the configuration may reference\n"
"lookup tables that do not exist and ociocheck will find these cases.\n"
"Unlike the config validate method, ociocheck parses all required LUTs.\n"
"All display/view pairs, color spaces, and named transforms are checked,\n"
"regardless of whether they are active or inactive.\n\n"
"Ociocheck can also be used to clean up formatting on an existing profile\n"
"that has been manually edited, using the '-o' option.\n";


// returns true if the interopID is valid
bool isValidInteropID(const std::string& id)
{
    // See https://github.com/AcademySoftwareFoundation/ColorInterop for the details.

    static const std::set<std::string> cifTextureIDs = {
        "lin_ap1_scene",
        "lin_ap0_scene",
        "lin_rec709_scene",
        "lin_p3d65_scene",
        "lin_rec2020_scene",
        "lin_adobergb_scene",
        "lin_ciexyzd65_scene",
        "srgb_rec709_scene",
        "g22_rec709_scene",
        "g18_rec709_scene",
        "srgb_ap1_scene",
        "g22_ap1_scene",
        "srgb_p3d65_scene",
        "g22_adobergb_scene",
        "data",
        "unknown"
    };

    static const std::set<std::string> cifDisplayIDs = {
        "srgb_rec709_display",
        "g24_rec709_display",
        "srgb_p3d65_display",
        "srgbx_p3d65_display",
        "pq_p3d65_display",
        "pq_rec2020_display",
        "hlg_rec2020_display",
        "g22_rec709_display",
        "g22_adobergb_display",
        "g26_p3d65_display",
        "g26_xyzd65_display",
        "pq_xyzd65_display",
    };

    if (id.empty()) 
        return true;

    // Check if has a namespace.
    size_t pos = id.find(':');
    if (pos == std::string::npos) 
    {
        // No namespace, so id must be in the Color Interop Forum ID list.
        if (cifTextureIDs.count(id) == 0 && cifDisplayIDs.count(id)==0)
        {
            std::cout << "ERROR: InteropID '" << id << "' is not valid. "
                "It should either be one of Color Interop Forum standard IDs or "
                "it must contain a namespace followed by ':', e.g. 'mycompany:mycolorspace'." << 
                std::endl;
            return false;
        }
    }
    else
    {
        // Namespace found, split into namespace and id.
        std::string ns = id.substr(0, pos);
        std::string cs = id.substr(pos+1);

        // Id should not be in the Color Interop Forum ID list.
        if (cifTextureIDs.count(cs) > 0 || cifDisplayIDs.count(cs)> 0) 
        {
            std::cout << "ERROR: InteropID '" << id << "' is not valid. "
                "The ID part must not be one of the Color Interop Forum standard IDs when a namespace is used." << 
                std::endl;
            return false;
        }
    }

    // all clear.
    return true;
}

int main(int argc, const char **argv)
{
    bool help = false;
    int errorcount = 0;
    std::string inputconfig;
    std::string outputconfig;

    ArgParse ap;
    ap.options("ociocheck -- validate an OpenColorIO configuration\n\n"
               "usage:  ociocheck [options]\n",
               "--help", &help, "Print help message",
               "--iconfig %s", &inputconfig, "Input .ocio configuration file (default: $OCIO)",
               "--oconfig %s", &outputconfig, "Output .ocio file",
               NULL);

    if (ap.parse(argc, argv) < 0)
    {
        std::cout << ap.geterror() << std::endl;
        ap.usage();
        std::cout << DESC_STRING;
        return 1;
    }

    if (help)
    {
        ap.usage();
        std::cout << DESC_STRING;
        return 1;
    }

    // Set the logging level to INFO.
    OCIO::SetLoggingLevel(OCIO::LOGGING_LEVEL_INFO);

    try
    {
        OCIO::ConstConfigRcPtr srcConfig;

        std::cout << std::endl;
        std::cout << "OpenColorIO Library Version: " << OCIO::GetVersion() << std::endl;
        std::cout << "OpenColorIO Library VersionHex: " << OCIO::GetVersionHex() << std::endl;

        if(!inputconfig.empty())
        {
            std::cout << std::endl;
            std::cout << "Loading " << inputconfig << std::endl;
            srcConfig = OCIO::Config::CreateFromFile(inputconfig.c_str());
        }
        else if(OCIO::GetEnvVariable("OCIO"))
        {
            std::cout << std::endl;
            std::cout << "Loading $OCIO " << OCIO::GetEnvVariable("OCIO") << std::endl;
            srcConfig = OCIO::Config::CreateFromEnv();
        }
        else
        {
            std::cout << std::endl;
            std::cout << "ERROR: You must specify an input OCIO configuration ";
            std::cout << "(either with --iconfig or $OCIO).\n";
            ap.usage ();
            std::cout << DESC_STRING;
            return 1;
        }

        // This program calls getProcessor for every color space and display/view in the config,
        // so turn off the Processor cache.
        OCIO::ConfigRcPtr config = srcConfig->createEditableCopy();
        config->setProcessorCacheFlags(OCIO::PROCESSOR_CACHE_OFF);

        std::cout << std::endl;
        std::cout << "** General **" << std::endl;

        if (config->getNumEnvironmentVars() > 0)
        {
            std::cout << "Environment:" << std::endl;
            for (int idx = 0; idx < config->getNumEnvironmentVars(); ++idx)
            {
                const char * name = config->getEnvironmentVarNameByIndex(idx);
                std::cout << "  " << name
                          << ": " << config->getEnvironmentVarDefault(name)
                          << std::endl;
            }
        }
        else
        {
            if (config->getEnvironmentMode() == OCIO::ENV_ENVIRONMENT_LOAD_PREDEFINED)
            {
                std::cout << "Environment: {}" << std::endl;
            }
            else
            {
                std::cout << "Environment: <missing>" << std::endl;
            }
        }

        std::cout << "Search Path: " << config->getSearchPath() << std::endl;
        std::cout << "Working Dir: " << config->getWorkingDir() << std::endl;

        if (config->getNumDisplays() == 0)
        {
            std::cout << std::endl;
            std::cout << "ERROR: At least one (display, view) pair must be defined." << std::endl;
            errorcount += 1;
        }
        else
        {
            std::cout << std::endl;
            std::cout << "Default Display: " << config->getDefaultDisplay() << std::endl;
            std::cout << "Default View: " << config->getDefaultView(config->getDefaultDisplay()) << std::endl;

            // It's important that the getProcessor call below always loads the transforms
            // involved in each display/view pair.  However, if the src color space is a
            // data space or if the view's colorspace happens to be the same as the src
            // color space, it will effectively bypass the loading of the transform.
            // The work-around used here is to create a copy of the config and add a unique
            // src color space that will definitely allow the Processor to be created.

            OCIO::ConfigRcPtr displayTestConfig = config->createEditableCopy();
            auto cs = OCIO::ColorSpace::Create(OCIO::REFERENCE_SPACE_SCENE);
            const std::string srcColorSpace = "ocioCheckTotallyUniqueColorSpaceName";
            cs->setName(srcColorSpace.c_str());
            auto ff = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_GLOW_10);
            cs->setTransform(ff, OCIO::COLORSPACE_DIR_TO_REFERENCE);
            displayTestConfig->addColorSpace(cs);

            if (config->getNumColorSpaces() > 0)
            {
                std::cout << std::endl;
                std::cout << "** (Display, View) pairs **" << std::endl;

                // Iterate over all displays & views (active & inactive).

                for (int idxDisp = 0; idxDisp < config->getNumDisplaysAll(); ++idxDisp)
                {
                    const char * displayName = config->getDisplayAll(idxDisp);

                    // Iterate over shared views.
                    int numViews = config->getNumViews(OCIO::VIEW_SHARED, displayName);
                    for (int idxView = 0; idxView < numViews; ++idxView)
                    {
                        const char * viewName = config->getView(OCIO::VIEW_SHARED, 
                                                                displayName, 
                                                                idxView);
                        try
                        {
                            OCIO::ConstProcessorRcPtr process 
                                = displayTestConfig->getProcessor(srcColorSpace.c_str(), 
                                                                  displayName,
                                                                  viewName,
                                                                  OCIO::TRANSFORM_DIR_FORWARD);

                            std::cout << "(" << displayName << ", " << viewName << ")"
                                      << std::endl;
                        }
                        catch(OCIO::Exception & exception)
                        {
                            std::cout << "ERROR: " << exception.what() << std::endl;
                            errorcount += 1;
                        }
                    }

                    // Iterate over display-defined views.
                    numViews = config->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, displayName);
                    for (int idxView = 0; idxView < numViews; ++idxView)
                    {
                        const char * viewName = config->getView(OCIO::VIEW_DISPLAY_DEFINED, 
                                                                displayName, idxView);
                        try
                        {
                            OCIO::ConstProcessorRcPtr process 
                                = displayTestConfig->getProcessor(srcColorSpace.c_str(), 
                                                                  displayName,
                                                                  viewName,
                                                                  OCIO::TRANSFORM_DIR_FORWARD);

                            std::cout << "(" << displayName << ", " << viewName << ")"
                                      << std::endl;
                        }
                        catch(OCIO::Exception & exception)
                        {
                            std::cout << "ERROR: " << exception.what() << std::endl;
                            errorcount += 1;
                        }
                    }
                }
            }
        }

        {
            std::cout << std::endl;
            std::cout << "** Roles **" << std::endl;

            // All roles defined in OpenColorTypes.h.
            std::set<std::string> allRolesSet = { 
                                                    OCIO::ROLE_DEFAULT, 
                                                    OCIO::ROLE_SCENE_LINEAR,
                                                    OCIO::ROLE_DATA, 
                                                    OCIO::ROLE_REFERENCE,
                                                    OCIO::ROLE_COMPOSITING_LOG, 
                                                    OCIO::ROLE_COLOR_TIMING,
                                                    OCIO::ROLE_COLOR_PICKING,
                                                    OCIO::ROLE_TEXTURE_PAINT, 
                                                    OCIO::ROLE_MATTE_PAINT,
                                                    OCIO::ROLE_RENDERING,
                                                    OCIO::ROLE_INTERCHANGE_SCENE,
                                                    OCIO::ROLE_INTERCHANGE_DISPLAY
                                                };

            // Print a list of the config's roles, appending ": user" if they are not one
            // of the "standard" roles defined in the library.
            for(int i=0; i<config->getNumRoles(); ++i)
            {
                const char * role = config->getRoleName(i);
                OCIO::ConstColorSpaceRcPtr cs = config->getColorSpace(role);
                if(cs)
                {
                    if(allRolesSet.find(role) != allRolesSet.end())
                    {
                        std::cout << cs->getName() << " (" << role << ")" << std::endl;
                    }
                    else
                    {
                        std::cout << cs->getName() << " (" << role << ": user)" << std::endl;
                    }
                }
                else
                {
                    // Note: The config->validate check below will also fail due to this.
                    std::cout << "ERROR: SPACE MISSING (" << role << ")" << std::endl;
                    errorcount += 1;
                }
            }

            // Roles that are actually used by the library or important tools/plug-ins.
            //   Config::validate ensures these are present for 2.2 and later configs,
            //   but want to warn if they are missing in earlier configs.  No warnings
            //   are given for other roles since most are not widely used anymore.
            std::vector<std::string> essentialRoles = {
                OCIO::ROLE_SCENE_LINEAR,        // LegacyViewingPipeline
                OCIO::ROLE_COLOR_TIMING,        // LegacyViewingPipeline
                OCIO::ROLE_COMPOSITING_LOG,     // LogConvert plug-in
                OCIO::ROLE_INTERCHANGE_SCENE,   // Used by the library
                OCIO::ROLE_INTERCHANGE_DISPLAY, // Used by the library
                };

            // Print a warning for any essential roles that are missing.
            // (Subsequent sections may raise an error.)
            for(size_t i=0;i<essentialRoles.size(); ++i)
            {
                const std::string role = essentialRoles[i];
                OCIO::ConstColorSpaceRcPtr cs = config->getColorSpace(role.c_str());
                if(!cs)
                {
                    std::cout << "WARNING: NOT DEFINED (" << role << ")" << std::endl;
                }
            }
        }

        {
            std::cout << std::endl;
            std::cout << "** ColorSpaces **" << std::endl;

            const int numCS = config->getNumColorSpaces(
                OCIO::SEARCH_REFERENCE_SPACE_ALL,   // Iterate over scene & display color spaces.
                OCIO::COLORSPACE_ALL);              // Iterate over active & inactive color spaces.

            for(int i=0; i<numCS; ++i)
            {
                OCIO::ConstColorSpaceRcPtr cs = config->getColorSpace(config->getColorSpaceNameByIndex(
                    OCIO::SEARCH_REFERENCE_SPACE_ALL,
                    OCIO::COLORSPACE_ALL,
                    i));

                std::string interopID = cs->getInteropID();
                if (!interopID.empty())
                {
                    if (!isValidInteropID(interopID))
                    {
                        errorcount += 1;
                    }
                }

                // Try to load the transform for the to_ref direction -- this will load any LUTs.
                bool toRefOK = true;
                std::string toRefErrorText;
                try
                {
                    OCIO::ConstTransformRcPtr t = cs->getTransform(OCIO::COLORSPACE_DIR_TO_REFERENCE);
                    if(t)
                    {
                        OCIO::ConstProcessorRcPtr p = config->getProcessor(t);
                    }
                }
                catch(OCIO::Exception & exception)
                {
                    toRefOK = false;
                    toRefErrorText = exception.what();
                }

                // Try to load the transform for the from_ref direction -- this will load any LUTs.
                bool fromRefOK = true;
                std::string fromRefErrorText;
                try
                {
                    OCIO::ConstTransformRcPtr t = cs->getTransform(OCIO::COLORSPACE_DIR_FROM_REFERENCE);
                    if(t)
                    {
                        OCIO::ConstProcessorRcPtr p = config->getProcessor(t);
                    }
                }
                catch(OCIO::Exception & exception)
                {
                    fromRefOK = false;
                    fromRefErrorText = exception.what();
                }

                if(!toRefOK || !fromRefOK)
                {
                    // There was a problem with one of the color space's transforms.
                    std::cout << cs->getName();
                    std::cout << " -- error" << std::endl;
                    if(!toRefOK)
                    {
                        std::cout << "\t" << toRefErrorText << std::endl;
                    }
                    if(!fromRefOK)
                    {
                        std::cout << "\t" << fromRefErrorText << std::endl;
                    }
                    errorcount += 1;
                }
                else
                {
                    // The color space's transforms load ok.
                    std::cout << cs->getName() << std::endl;
                }
            }
        }

        {
            std::cout << std::endl;
            std::cout << "** Named Transforms **" << std::endl;

            // Iterate over active & inactive named transforms.
            const int numNT = config->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL);
            if(numNT==0)
            {
                std::cout << "no named transforms defined" << std::endl;
            }

            for(int i = 0; i<numNT; ++i)
            {
                OCIO::ConstNamedTransformRcPtr nt = config->getNamedTransform(
                    config->getNamedTransformNameByIndex(OCIO::NAMEDTRANSFORM_ALL, i));

                // Try to load the transform -- this will load any LUTs.
                bool fwdOK = true;
                std::string fwdErrorText;
                try
                {
                    OCIO::ConstTransformRcPtr t = nt->getTransform(OCIO::TRANSFORM_DIR_FORWARD);
                    if(t)
                    {
                        OCIO::ConstProcessorRcPtr p = config->getProcessor(t);
                    }
                }
                catch(OCIO::Exception & exception)
                {
                    fwdOK = false;
                    fwdErrorText = exception.what();
                }

                // Try to load the inverse_transform -- this will load any LUTs.
                bool invOK = true;
                std::string invErrorText;
                try
                {
                    OCIO::ConstTransformRcPtr t = nt->getTransform(OCIO::TRANSFORM_DIR_INVERSE);
                    if(t)
                    {
                        OCIO::ConstProcessorRcPtr p = config->getProcessor(t);
                    }
                }
                catch(OCIO::Exception & exception)
                {
                    invOK = false;
                    invErrorText = exception.what();
                }

                if(!fwdOK || !invOK)
                {
                    // There was a problem with one of the named transform's transforms.
                    std::cout << nt->getName();
                    std::cout << " -- error" << std::endl;
                    if(!fwdOK)
                    {
                        std::cout << "\t" << fwdErrorText << std::endl;
                    }
                    if(!invOK)
                    {
                        std::cout << "\t" << invErrorText << std::endl;
                    }
                    errorcount += 1;
                }
                else
                {
                    // The named transform's transforms load ok.
                    std::cout << nt->getName() << std::endl;
                }
            }
        }

        {
            std::cout << std::endl;
            std::cout << "** Looks **" << std::endl;

            const int numL = config->getNumLooks();
            if(numL==0)
            {
                std::cout << "no looks defined" << std::endl;
            }

            for(int i=0; i<numL; ++i)
            {
                OCIO::ConstLookRcPtr look = config->getLook(config->getLookNameByIndex(i)); 

                // Try to load the transform -- this will load any LUTs.
                bool fwdOK = true;
                std::string fwdErrorText;
                try
                {
                    OCIO::ConstTransformRcPtr t = look->getTransform();
                    if(t)
                    {
                        OCIO::ConstProcessorRcPtr p = config->getProcessor(t);
                    }
                }
                catch(OCIO::Exception & exception)
                {
                    fwdOK = false;
                    fwdErrorText = exception.what();
                }

                // Try to load the inverse transform -- this will load any LUTs.
                bool invOK = true;
                std::string invErrorText;
                try
                {
                    OCIO::ConstTransformRcPtr t = look->getInverseTransform();
                    if(t)
                    {
                        OCIO::ConstProcessorRcPtr p = config->getProcessor(t);
                    }
                }
                catch(OCIO::Exception & exception)
                {
                    invOK = false;
                    invErrorText = exception.what();
                }

                if(!fwdOK || !invOK)
                {
                    // There was a problem with one of the look transform's transforms.
                    std::cout << look->getName();
                    std::cout << " -- error" << std::endl;
                    if(!fwdOK)
                    {
                        std::cout << "\t" << fwdErrorText << std::endl;
                    }
                    if(!invOK)
                    {
                        std::cout << "\t" << invErrorText << std::endl;
                    }
                    errorcount += 1;
                }
                else
                {
                    // The look transform's transforms load ok.
                    std::cout << look->getName() << std::endl;
                }
            }
        }

        std::cout << std::endl;
        std::cout << "** Validation **" << std::endl;

        std::string cacheID;
        bool isArchivable = false;
        try
        {
            LogGuard logGuard;

            config->validate();
            std::cout << logGuard.output();
            
            cacheID = config->getCacheID();
            isArchivable = config->isArchivable();

            // Passed if there are no Error level logs.
            StringUtils::StringVec svec = StringUtils::SplitByLines(logGuard.output());
            if (!StringUtils::Contain(svec, "[OpenColorIO Error]"))
            {
                std::cout << "passed" << std::endl;
            }
            else
            {
                std::cout << "failed" << std::endl;
                errorcount += 1;
            }
        }
        catch(OCIO::Exception & exception)
        {
            std::cout << "ERROR:" << std::endl;
            errorcount += 1;
            std::cout << exception.what() << std::endl;
            std::cout << "failed" << std::endl;
        }

        std::cout << std::endl;
        std::cout << "** Miscellaneous **" << std::endl;
        std::cout << "CacheID: " << cacheID << std::endl;
        std::cout << "Archivable: " << (isArchivable ? "yes" : "no") << std::endl;

        if(!outputconfig.empty())
        {
            std::ofstream output;
            output.open(outputconfig.c_str());

            if(!output.is_open())
            {
                std::cout << "Error opening " << outputconfig << " for writing." << std::endl;
            }
            else
            {
                config->serialize(output);
                output.close();
                std::cout << "Wrote " << outputconfig << std::endl;
            }
        }
    }
    catch(OCIO::Exception & exception)
    {
        std::cout << "ERROR: " << exception.what() << std::endl;
        return 1;
    } catch (std::exception& exception) {
        std::cout << "ERROR: " << exception.what() << "\n";
        return 1;
    }
    catch(...)
    {
        std::cout << "Unknown error encountered." << std::endl;
        return 1;
    }

    std::cout << std::endl;
    if(errorcount == 0)
    {
        std::cout << "Tests complete." << std::endl << std::endl;
        return 0;
    }
    else
    {
        std::cout << errorcount << " tests failed." << std::endl << std::endl;
        return 1;
    }
}