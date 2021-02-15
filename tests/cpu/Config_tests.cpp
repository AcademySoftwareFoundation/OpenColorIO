// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <sys/stat.h>

#include "Config.cpp"
#include "utils/StringUtils.h"

#include <pystring/pystring.h>
#include "testutils/UnitTest.h"
#include "UnitTestLogUtils.h"
#include "UnitTestUtils.h"
#include "utils/StringUtils.h"


namespace OCIO = OCIO_NAMESPACE;


#if 0
OCIO_ADD_TEST(Config, test_searchpath_filesystem)
{

    OCIO::EnvMap env = OCIO::GetEnvMap();
    std::string OCIO_TEST_AREA("$OCIO_TEST_AREA");
    EnvExpand(&OCIO_TEST_AREA, &env);

    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    // basic get/set/expand
    config->setSearchPath("."
                          ":$OCIO_TEST1"
                          ":/$OCIO_JOB/${OCIO_SEQ}/$OCIO_SHOT/ocio");

    OCIO_CHECK_ASSERT(strcmp(config->getSearchPath(),
        ".:$OCIO_TEST1:/$OCIO_JOB/${OCIO_SEQ}/$OCIO_SHOT/ocio") == 0);
    OCIO_CHECK_ASSERT(strcmp(config->getSearchPath(true),
        ".:foobar:/meatballs/cheesecake/mb-cc-001/ocio") == 0);

    // find some files
    config->setSearchPath(".."
                          ":$OCIO_TEST1"
                          ":${OCIO_TEST_AREA}/test_search/one"
                          ":$OCIO_TEST_AREA/test_search/two");

    // setup for search test
    std::string base_dir("$OCIO_TEST_AREA/test_search/");
    EnvExpand(&base_dir, &env);
    mkdir(base_dir.c_str(), 0777);

    std::string one_dir("$OCIO_TEST_AREA/test_search/one/");
    EnvExpand(&one_dir, &env);
    mkdir(one_dir.c_str(), 0777);

    std::string two_dir("$OCIO_TEST_AREA/test_search/two/");
    EnvExpand(&two_dir, &env);
    mkdir(two_dir.c_str(), 0777);

    std::string lut1(one_dir+"somelut1.lut");
    std::ofstream somelut1(lut1.c_str());
    somelut1.close();

    std::string lut2(two_dir+"somelut2.lut");
    std::ofstream somelut2(lut2.c_str());
    somelut2.close();

    std::string lut3(two_dir+"somelut3.lut");
    std::ofstream somelut3(lut3.c_str());
    somelut3.close();

    std::string lutdotdot(OCIO_TEST_AREA+"/lutdotdot.lut");
    std::ofstream somelutdotdot(lutdotdot.c_str());
    somelutdotdot.close();

    // basic search test
    OCIO_CHECK_ASSERT(strcmp(config->findFile("somelut1.lut"),
        lut1.c_str()) == 0);
    OCIO_CHECK_ASSERT(strcmp(config->findFile("somelut2.lut"),
        lut2.c_str()) == 0);
    OCIO_CHECK_ASSERT(strcmp(config->findFile("somelut3.lut"),
        lut3.c_str()) == 0);
    OCIO_CHECK_ASSERT(strcmp(config->findFile("lutdotdot.lut"),
        lutdotdot.c_str()) == 0);

}
#endif

OCIO_ADD_TEST(Config, internal_raw_profile)
{
    std::istringstream is;
    is.str(OCIO::INTERNAL_RAW_PROFILE);

    OCIO_CHECK_NO_THROW(OCIO::Config::CreateFromStream(is));
}

OCIO_ADD_TEST(Config, create_raw_config)
{
    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateRaw());
    OCIO_CHECK_NO_THROW(config->validate());
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 1);
    OCIO_CHECK_EQUAL(std::string(config->getColorSpaceNameByIndex(0)), std::string("raw"));

    OCIO::ConstProcessorRcPtr proc;
    OCIO_CHECK_NO_THROW(proc = config->getProcessor("raw", "raw"));
    OCIO_CHECK_NO_THROW(proc->getDefaultCPUProcessor());

    OCIO_CHECK_THROW_WHAT(config->getProcessor("not_found", "raw"), OCIO::Exception,
                          "Color space 'not_found' could not be found");
    OCIO_CHECK_THROW_WHAT(config->getProcessor("raw", "not_found"), OCIO::Exception,
                          "Color space 'not_found' could not be found");
}

OCIO_ADD_TEST(Config, simple_config)
{

    constexpr char SIMPLE_PROFILE[] =
        "ocio_profile_version: 1\n"
        "resource_path: luts\n"
        "strictparsing: false\n"
        "luma: [0.2126, 0.7152, 0.0722]\n"
        "roles:\n"
        "  default: raw\n"
        "  scene_linear: lnh\n"
        "displays:\n"
        "  sRGB:\n"
        "  - !<View> {name: Film1D, colorspace: loads_of_transforms}\n"
        "  - !<View> {name: Ln, colorspace: lnh}\n"
        "  - !<View> {name: Raw, colorspace: raw}\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "      name: raw\n"
        "      family: raw\n"
        "      equalitygroup: \n"
        "      bitdepth: 32f\n"
        "      description: |\n"
        "        A raw color space. Conversions to and from this space are no-ops.\n"
        "      isdata: true\n"
        "      allocation: uniform\n"
        "  - !<ColorSpace>\n"
        "      name: lnh\n"
        "      family: ln\n"
        "      equalitygroup: \n"
        "      bitdepth: 16f\n"
        "      description: |\n"
        "        The show reference space. This is a sensor referred linear\n"
        "        representation of the scene with primaries that correspond to\n"
        "        scanned film. 0.18 in this space corresponds to a properly\n"
        "        exposed 18% grey card.\n"
        "      isdata: false\n"
        "      allocation: lg2\n"
        "  - !<ColorSpace>\n"
        "      name: loads_of_transforms\n"
        "      family: vd8\n"
        "      equalitygroup: \n"
        "      bitdepth: 8ui\n"
        "      description: 'how many transforms can we use?'\n"
        "      isdata: false\n"
        "      allocation: uniform\n"
        "      to_reference: !<GroupTransform>\n"
        "        direction: forward\n"
        "        children:\n"
        "          - !<FileTransform>\n"
        "            src: diffusemult.spimtx\n"
        "            interpolation: unknown\n"
        "          - !<ColorSpaceTransform>\n"
        "            src: raw\n"
        "            dst: lnh\n"
        "          - !<ExponentTransform>\n"
        "            value: [2.2, 2.2, 2.2, 1]\n"
        "          - !<MatrixTransform>\n"
        "            matrix: [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]\n"
        "            offset: [0, 0, 0, 0]\n"
        "          - !<CDLTransform>\n"
        "            slope: [1, 1, 1]\n"
        "            offset: [0, 0, 0]\n"
        "            power: [1, 1, 1]\n"
        "            saturation: 1\n"
        "\n";

    std::istringstream is;
    is.str(SIMPLE_PROFILE);
    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());
}

OCIO_ADD_TEST(Config, colorspace_duplicate)
{

    constexpr char SIMPLE_PROFILE[] =
        "ocio_profile_version: 2\n"
        "search_path: luts\n"
        "roles:\n"
        "  default: raw\n"
        "file_rules:\n"
        "  - !<Rule> {name: Default, colorspace: default}\n"
        "displays:\n"
        "  Disp1:\n"
        "    - !<View> {name: View1, colorspace: raw}\n"
        "active_displays: []\n"
        "active_views: []\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: raw_duplicated\n"
        "    name: raw\n"
        "\n";

    std::istringstream is;
    is.str(SIMPLE_PROFILE);
    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                          "Key-value pair with key 'name' specified more than once. ");
}

OCIO_ADD_TEST(Config, cdltransform_duplicate)
{

    constexpr char SIMPLE_PROFILE[] =
        "ocio_profile_version: 2\n"
        "search_path: luts\n"
        "roles:\n"
        "  default: raw\n"
        "file_rules:\n"
        "  - !<Rule> {name: Default, colorspace: default}\n"
        "displays:\n"
        "  Disp1:\n"
        "    - !<View> {name: View1, colorspace: raw}\n"
        "active_displays: []\n"
        "active_views: []\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: raw\n"
        "    to_scene_reference: !<CDLTransform> {slope: [1, 2, 1], slope: [1, 2, 1]}\n"
        "\n";

    std::istringstream is;
    is.str(SIMPLE_PROFILE);
    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                          "Key-value pair with key 'slope' specified more than once. ");
}

OCIO_ADD_TEST(Config, searchpath_duplicate)
{

    constexpr char SIMPLE_PROFILE[] =
        "ocio_profile_version: 2\n"
        "search_path: luts\n"
        "search_path: luts-dir\n"
        "roles:\n"
        "  default: raw\n"
        "file_rules:\n"
        "  - !<Rule> {name: Default, colorspace: default}\n"
        "displays:\n"
        "  Disp1:\n"
        "    - !<View> {name: View1, colorspace: raw}\n"
        "active_displays: []\n"
        "active_views: []\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: raw\n"
        "\n";

    std::istringstream is;
    is.str(SIMPLE_PROFILE);
    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                          "Key-value pair with key 'search_path' specified more than once. ");
}

OCIO_ADD_TEST(Config, roles)
{

    std::string SIMPLE_PROFILE =
    "ocio_profile_version: 1\n"
    "strictparsing: false\n"
    "roles:\n"
    "  compositing_log: lgh\n"
    "  default: raw\n"
    "  scene_linear: lnh\n"
    "colorspaces:\n"
    "  - !<ColorSpace>\n"
    "      name: raw\n"
    "  - !<ColorSpace>\n"
    "      name: lnh\n"
    "  - !<ColorSpace>\n"
    "      name: lgh\n"
    "\n";

    std::istringstream is;
    is.str(SIMPLE_PROFILE);
    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));

    OCIO_CHECK_EQUAL(config->getNumRoles(), 3);

    OCIO_CHECK_ASSERT(config->hasRole("compositing_log"));
    OCIO_CHECK_ASSERT(!config->hasRole("cheese"));
    OCIO_CHECK_ASSERT(!config->hasRole(""));

    OCIO_CHECK_EQUAL(std::string(config->getRoleName(2)), "scene_linear");
    OCIO_CHECK_EQUAL(std::string(config->getRoleColorSpace(2)), "lnh");

    OCIO_CHECK_EQUAL(std::string(config->getRoleName(0)), "compositing_log");
    OCIO_CHECK_EQUAL(std::string(config->getRoleColorSpace(0)), "lgh");

    OCIO_CHECK_EQUAL(std::string(config->getRoleName(1)), "default");

    OCIO_CHECK_EQUAL(std::string(config->getRoleName(10)), "");
    OCIO_CHECK_EQUAL(std::string(config->getRoleColorSpace(10)), "");

    OCIO_CHECK_EQUAL(std::string(config->getRoleName(-4)), "");
    OCIO_CHECK_EQUAL(std::string(config->getRoleColorSpace(-4)), "");
}

OCIO_ADD_TEST(Config, serialize_group_transform)
{
    // The unit test validates that a group transform is correctly serialized.

    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("testing");
        cs->setFamily("test");
        OCIO::GroupTransformRcPtr groupTransform = OCIO::GroupTransform::Create();
        // Default and unknown interpolation are not saved.
        OCIO::FileTransformRcPtr transform1 = OCIO::FileTransform::Create();
        groupTransform->appendTransform(transform1);
        OCIO::FileTransformRcPtr transform2 = OCIO::FileTransform::Create();
        transform2->setInterpolation(OCIO::INTERP_UNKNOWN);
        groupTransform->appendTransform(transform2);
        OCIO::FileTransformRcPtr transform3 = OCIO::FileTransform::Create();
        transform3->setInterpolation(OCIO::INTERP_BEST);
        groupTransform->appendTransform(transform3);
        OCIO::FileTransformRcPtr transform4 = OCIO::FileTransform::Create();
        transform4->setInterpolation(OCIO::INTERP_NEAREST);
        groupTransform->appendTransform(transform4);
        OCIO::FileTransformRcPtr transform5 = OCIO::FileTransform::Create();
        transform5->setInterpolation(OCIO::INTERP_CUBIC);
        groupTransform->appendTransform(transform5);
        OCIO_CHECK_NO_THROW(cs->setTransform(groupTransform, OCIO::COLORSPACE_DIR_FROM_REFERENCE));
        config->addColorSpace(cs);
        config->setRole( OCIO::ROLE_DEFAULT, cs->getName() );
        config->setRole(OCIO::ROLE_COMPOSITING_LOG, cs->getName());
    }
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("testing2");
        cs->setFamily("test");
        OCIO::ExponentTransformRcPtr transform1 = OCIO::ExponentTransform::Create();
        OCIO::GroupTransformRcPtr groupTransform = OCIO::GroupTransform::Create();
        groupTransform->appendTransform(transform1);
        OCIO_CHECK_NO_THROW(cs->setTransform(groupTransform, OCIO::COLORSPACE_DIR_TO_REFERENCE));
        config->addColorSpace(cs);
        // Replace the role.
        config->setRole( OCIO::ROLE_COMPOSITING_LOG, cs->getName() );
    }

    std::ostringstream os;
    config->serialize(os);

    std::string PROFILE_OUT =
    "ocio_profile_version: 2\n"
    "\n"
    "environment:\n"
    "  {}\n"
    "search_path: \"\"\n"
    "strictparsing: true\n"
    "luma: [0.2126, 0.7152, 0.0722]\n"
    "\n"
    "roles:\n"
    "  compositing_log: testing2\n"
    "  default: testing\n"
    "\n"
    "file_rules:\n"
    "  - !<Rule> {name: Default, colorspace: default}\n"
    "\n"
    "displays:\n"
    "  {}\n"
    "\n"
    "active_displays: []\n"
    "active_views: []\n"
    "\n"
    "colorspaces:\n"
    "  - !<ColorSpace>\n"
    "    name: testing\n"
    "    family: test\n"
    "    equalitygroup: \"\"\n"
    "    bitdepth: unknown\n"
    "    isdata: false\n"
    "    allocation: uniform\n"
    "    from_scene_reference: !<GroupTransform>\n"
    "      children:\n"
    "        - !<FileTransform> {src: \"\"}\n"
    "        - !<FileTransform> {src: \"\", interpolation: unknown}\n"
    "        - !<FileTransform> {src: \"\", interpolation: best}\n"
    "        - !<FileTransform> {src: \"\", interpolation: nearest}\n"
    "        - !<FileTransform> {src: \"\", interpolation: cubic}\n"
    "\n"
    "  - !<ColorSpace>\n"
    "    name: testing2\n"
    "    family: test\n"
    "    equalitygroup: \"\"\n"
    "    bitdepth: unknown\n"
    "    isdata: false\n"
    "    allocation: uniform\n"
    "    to_scene_reference: !<GroupTransform>\n"
    "      children:\n"
    "        - !<ExponentTransform> {value: 1}\n";

    const StringUtils::StringVec osvec          = StringUtils::SplitByLines(os.str());
    const StringUtils::StringVec PROFILE_OUTvec = StringUtils::SplitByLines(PROFILE_OUT);

    OCIO_CHECK_EQUAL(osvec.size(), PROFILE_OUTvec.size());
    for (unsigned int i = 0; i < PROFILE_OUTvec.size(); ++i)
    {
        OCIO_CHECK_EQUAL(osvec[i], PROFILE_OUTvec[i]);
    }
}

OCIO_ADD_TEST(Config, serialize_searchpath)
{
    {
        OCIO::ConfigRcPtr config = OCIO::Config::Create();
        {
            OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
            cs->setName("default");
            cs->setIsData(true);
            config->addColorSpace(cs);
        }

        std::ostringstream os;
        config->serialize(os);

        std::string PROFILE_OUT =
            "ocio_profile_version: 2\n"
            "\n"
            "environment:\n"
            "  {}\n"
            "search_path: \"\"\n"
            "strictparsing: true\n"
            "luma: [0.2126, 0.7152, 0.0722]\n"
            "\n"
            "roles:\n"
            "  {}\n"
            "\n"
            "file_rules:\n"
            "  - !<Rule> {name: Default, colorspace: default}\n"
            "\n"
            "displays:\n"
            "  {}\n"
            "\n"
            "active_displays: []\n"
            "active_views: []\n"
            "\n"
            "colorspaces:\n"
            "  - !<ColorSpace>\n"
            "    name: default\n"
            "    family: \"\"\n"
            "    equalitygroup: \"\"\n"
            "    bitdepth: unknown\n"
            "    isdata: true\n"
            "    allocation: uniform\n";

        const StringUtils::StringVec osvec          = StringUtils::SplitByLines(os.str());
        const StringUtils::StringVec PROFILE_OUTvec = StringUtils::SplitByLines(PROFILE_OUT);

        OCIO_CHECK_EQUAL(osvec.size(), PROFILE_OUTvec.size());
        for (unsigned int i = 0; i < PROFILE_OUTvec.size(); ++i)
            OCIO_CHECK_EQUAL(osvec[i], PROFILE_OUTvec[i]);
    }

    {
        OCIO::ConfigRcPtr config = OCIO::Config::Create();
        config->setMajorVersion(OCIO::FirstSupportedMajorVersion);
        config->setMinorVersion(0);

        std::string searchPath("a:b:c");
        config->setSearchPath(searchPath.c_str());

        std::ostringstream os;
        config->serialize(os);

        StringUtils::StringVec osvec = StringUtils::SplitByLines(os.str());

        // V1 saves search_path as a single string.
        const std::string expected1{ "search_path: a:b:c" };
        OCIO_CHECK_EQUAL(osvec[2], expected1);

        // V2 saves search_path as separate strings.
        config->setMajorVersion(2);
        os.clear();
        os.str("");
        config->serialize(os);

        osvec = StringUtils::SplitByLines(os.str());

        const std::string expected2[] = { "search_path:", "  - a", "  - b", "  - c" };
        OCIO_CHECK_EQUAL(osvec[4], expected2[0]);
        OCIO_CHECK_EQUAL(osvec[5], expected2[1]);
        OCIO_CHECK_EQUAL(osvec[6], expected2[2]);
        OCIO_CHECK_EQUAL(osvec[7], expected2[3]);

        std::istringstream is;
        is.str(os.str());
        OCIO::ConstConfigRcPtr configRead;
        OCIO_CHECK_NO_THROW(configRead = OCIO::Config::CreateFromStream(is));

        OCIO_CHECK_EQUAL(configRead->getNumSearchPaths(), 3);
        OCIO_CHECK_EQUAL(std::string(configRead->getSearchPath()), searchPath);
        OCIO_CHECK_EQUAL(std::string(configRead->getSearchPath(0)), std::string("a"));
        OCIO_CHECK_EQUAL(std::string(configRead->getSearchPath(1)), std::string("b"));
        OCIO_CHECK_EQUAL(std::string(configRead->getSearchPath(2)), std::string("c"));

        os.clear();
        os.str("");
        config->clearSearchPaths();
        const std::string sp0{ "a path with a - in it/" };
        const std::string sp1{ "/absolute/linux/path" };
        const std::string sp2{ "C:\\absolute\\windows\\path" };
        const std::string sp3{ "!<path> using /yaml/symbols" };
        config->addSearchPath(sp0.c_str());
        config->addSearchPath(sp1.c_str());
        config->addSearchPath(sp2.c_str());
        config->addSearchPath(sp3.c_str());
        config->serialize(os);

        osvec = StringUtils::SplitByLines(os.str());

        const std::string expected3[] = { "search_path:",
                                          "  - a path with a - in it/",
                                          "  - /absolute/linux/path",
                                          "  - C:\\absolute\\windows\\path",
                                          "  - \"!<path> using /yaml/symbols\"" };
        OCIO_CHECK_EQUAL(osvec[4], expected3[0]);
        OCIO_CHECK_EQUAL(osvec[5], expected3[1]);
        OCIO_CHECK_EQUAL(osvec[6], expected3[2]);
        OCIO_CHECK_EQUAL(osvec[7], expected3[3]);
        OCIO_CHECK_EQUAL(osvec[8], expected3[4]);

        is.clear();
        is.str(os.str());
        OCIO_CHECK_NO_THROW(configRead = OCIO::Config::CreateFromStream(is));

        OCIO_CHECK_EQUAL(configRead->getNumSearchPaths(), 4);
        OCIO_CHECK_EQUAL(std::string(configRead->getSearchPath(0)), sp0);
        OCIO_CHECK_EQUAL(std::string(configRead->getSearchPath(1)), sp1);
        OCIO_CHECK_EQUAL(std::string(configRead->getSearchPath(2)), sp2);
        OCIO_CHECK_EQUAL(std::string(configRead->getSearchPath(3)), sp3);
    }
}

OCIO_ADD_TEST(Config, validation)
{
    {
    std::string SIMPLE_PROFILE =
    "ocio_profile_version: 1\n"
    "colorspaces:\n"
    "  - !<ColorSpace>\n"
    "      name: raw\n"
    "  - !<ColorSpace>\n"
    "      name: raw\n"
    "strictparsing: false\n"
    "roles:\n"
    "  default: raw\n"
    "displays:\n"
    "  sRGB:\n"
    "  - !<View> {name: Raw, colorspace: raw}\n"
    "\n";

    std::istringstream is;
    is.str(SIMPLE_PROFILE);
    OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                          "Colorspace with name 'raw' already defined");
    }

    {
    std::string SIMPLE_PROFILE =
    "ocio_profile_version: 1\n"
    "colorspaces:\n"
    "  - !<ColorSpace>\n"
    "      name: raw\n"
    "strictparsing: false\n"
    "roles:\n"
    "  default: raw\n"
    "displays:\n"
    "  sRGB:\n"
    "  - !<View> {name: Raw, colorspace: raw}\n"
    "\n";

    std::istringstream is;
    is.str(SIMPLE_PROFILE);
    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));

    OCIO_CHECK_NO_THROW(config->validate());
    }
}


OCIO_ADD_TEST(Config, context_variable_v1)
{
    std::string SIMPLE_PROFILE =
    "ocio_profile_version: 1\n"
    "environment:\n"
    "  SHOW: super\n"
    "  SHOT: test\n"
    "  SEQ: foo\n"
    "  test: bar${cheese}\n"
    "  cheese: chedder\n"
    "colorspaces:\n"
    "  - !<ColorSpace>\n"
    "      name: raw\n"
    "strictparsing: false\n"
    "roles:\n"
    "  default: raw\n"
    "displays:\n"
    "  sRGB:\n"
    "  - !<View> {name: Raw, colorspace: raw}\n"
    "\n";

    std::string SIMPLE_PROFILE2 =
    "ocio_profile_version: 1\n"
    "colorspaces:\n"
    "  - !<ColorSpace>\n"
    "      name: raw\n"
    "strictparsing: false\n"
    "roles:\n"
    "  default: raw\n"
    "displays:\n"
    "  sRGB:\n"
    "  - !<View> {name: Raw, colorspace: raw}\n"
    "\n";

    struct Guard
    {
        Guard()
        {
            OCIO::Platform::Setenv("SHOW", "bar");
            OCIO::Platform::Setenv("TASK", "lighting");
        }
        ~Guard()
        {
            OCIO::Platform::Unsetenv("SHOW");
            OCIO::Platform::Unsetenv("TASK");
        }
    } guard;

    std::istringstream is;
    is.str(SIMPLE_PROFILE);
    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());
    OCIO_CHECK_EQUAL(config->getNumEnvironmentVars(), 5);

    OCIO::ContextRcPtr usedContextVars = OCIO::Context::Create();

    // Test context variable resolution.

    OCIO_CHECK_EQUAL(0, strcmp(config->getCurrentContext()->resolveStringVar("test${test}", 
                                                                             usedContextVars),
                               "testbarchedder"));
    OCIO_CHECK_EQUAL(2, usedContextVars->getNumStringVars());
    OCIO_CHECK_EQUAL(0, strcmp(usedContextVars->getStringVarNameByIndex(0), "cheese"));
    OCIO_CHECK_EQUAL(0, strcmp(usedContextVars->getStringVarByIndex(0), "chedder"));
    OCIO_CHECK_EQUAL(0, strcmp(usedContextVars->getStringVarNameByIndex(1), "test"));
    OCIO_CHECK_EQUAL(0, strcmp(usedContextVars->getStringVarByIndex(1), "bar${cheese}"));

    usedContextVars->clearStringVars();
    OCIO_CHECK_EQUAL(0, strcmp(config->getCurrentContext()->resolveStringVar("${SHOW}", 
                                                                             usedContextVars),
                               "bar"));
    OCIO_CHECK_EQUAL(1, usedContextVars->getNumStringVars());
    OCIO_CHECK_EQUAL(0, strcmp(usedContextVars->getStringVarNameByIndex(0), "SHOW"));
    OCIO_CHECK_EQUAL(0, strcmp(usedContextVars->getStringVarByIndex(0), "bar"));
    // Even if an environment variable overrides $SHOW, its default value is still "super".
    OCIO_CHECK_ASSERT(strcmp(config->getEnvironmentVarDefault("SHOW"), "super") == 0);

    // Test default context variables.

    OCIO::ConfigRcPtr edit = config->createEditableCopy();
    OCIO_CHECK_EQUAL(edit->getNumEnvironmentVars(), 5);
    edit->clearEnvironmentVars();
    OCIO_CHECK_EQUAL(edit->getNumEnvironmentVars(), 0);

    edit->addEnvironmentVar("testing", "dupvar");
    OCIO_CHECK_EQUAL(edit->getNumEnvironmentVars(), 1);
    edit->addEnvironmentVar("testing", "dupvar"); // No duplications.
    OCIO_CHECK_EQUAL(edit->getNumEnvironmentVars(), 1);
    edit->addEnvironmentVar("foobar", "testing");
    OCIO_CHECK_EQUAL(edit->getNumEnvironmentVars(), 2);
    edit->addEnvironmentVar("blank", "");
    OCIO_CHECK_EQUAL(edit->getNumEnvironmentVars(), 3);
    edit->addEnvironmentVar("dontadd", nullptr);
    OCIO_CHECK_EQUAL(edit->getNumEnvironmentVars(), 3);
    edit->addEnvironmentVar("foobar", nullptr); // Remove an entry.
    OCIO_CHECK_EQUAL(edit->getNumEnvironmentVars(), 2);
    edit->clearEnvironmentVars();
    OCIO_CHECK_EQUAL(edit->getNumEnvironmentVars(), 0);

    OCIO_CHECK_EQUAL(edit->getEnvironmentMode(), OCIO::ENV_ENVIRONMENT_LOAD_PREDEFINED);
    OCIO_CHECK_NO_THROW(edit->setEnvironmentMode(OCIO::ENV_ENVIRONMENT_LOAD_ALL));
    OCIO_CHECK_EQUAL(edit->getEnvironmentMode(), OCIO::ENV_ENVIRONMENT_LOAD_ALL);

    // Test the second config i.e. not in predefined mode.

    // As a debug message is expected, trap & check its content.
    OCIO::LogGuard log;

    is.str(SIMPLE_PROFILE2);
    OCIO::ConstConfigRcPtr noenv;
    OCIO_CHECK_NO_THROW(noenv = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(noenv->validate());
    OCIO_CHECK_EQUAL(noenv->getEnvironmentMode(), OCIO::ENV_ENVIRONMENT_LOAD_ALL);
    // In all mode, use all system env. variables as potential context variables.
    OCIO_CHECK_ASSERT(strcmp(noenv->getCurrentContext()->resolveStringVar("${TASK}"),
        "lighting") == 0);

    OCIO_CHECK_EQUAL(log.output(), 
                     "[OpenColorIO Debug]: This .ocio config has no environment section defined."
                     " The default behaviour is to load all environment variables (0), which reduces"
                     " the efficiency of OCIO's caching. Consider predefining the environment"
                     " variables used.\n");
}

OCIO_ADD_TEST(Config, context_variable_faulty_cases)
{
    // Check that all transforms using color space names correctly support the context variable
    // validation.

    static constexpr char CONFIG[] = 
        "ocio_profile_version: 2\n"
        "\n"
        "search_path: luts\n"
        "\n"
        "environment:\n"
        "  DST1: cs2\n"
        "  DST2: cs2\n"
        "  DST3: cs2\n"
        "\n"
        "roles:\n"
        "  default: cs1\n"
        "\n"
        "view_transforms:\n"
        "  - !<ViewTransform>\n"
        "    name: vt1\n"
        "    from_scene_reference: !<ColorSpaceTransform> {src: cs1, dst: $DST3}\n"
        "\n"
        "displays:\n"
        "  disp1:\n"
        "    - !<View> {name: view1, view_transform: vt1, display_colorspace: dcs1}\n"
        "    - !<View> {name: view2, colorspace: cs3, looks: look1}\n"
        "\n"
        "looks:\n"
        "  - !<Look>\n"
        "    name: look1\n"
        "    process_space: cs2\n"
        "    transform: !<ColorSpaceTransform> {src: cs1, dst: $DST1}\n"
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: cs1\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: cs2\n"
        "    from_scene_reference: !<MatrixTransform> {offset: [0.11, 0.12, 0.13, 0]}\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: cs3\n"
        "    from_scene_reference: !<ColorSpaceTransform> {src: cs1, dst: $DST2}\n"
        "\n"
        "display_colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: dcs1\n"
        "    allocation: uniform\n"
        "    from_display_reference: !<CDLTransform> {slope: [1, 2, 1]}\n";


    std::istringstream iss;
    iss.str(CONFIG);

    OCIO::ConfigRcPtr cfg;
    OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateFromStream(iss)->createEditableCopy());
    OCIO_CHECK_NO_THROW(cfg->validate());
    OCIO_CHECK_NO_THROW(cfg->getProcessor("cs1", "disp1", "view1", OCIO::TRANSFORM_DIR_FORWARD));

    {
        // Remove environment variable DST3.

        OCIO_CHECK_NO_THROW(cfg->addEnvironmentVar("DST3", nullptr)); 
        OCIO_CHECK_EQUAL(cfg->getNumEnvironmentVars(), 2);

        OCIO_CHECK_THROW_WHAT(cfg->validate(),
                              OCIO::Exception,
                              "references a color space '$DST3' using an unknown context variable");

        OCIO_CHECK_THROW_WHAT(cfg->getProcessor("cs1", "disp1", "view1", OCIO::TRANSFORM_DIR_FORWARD),
                              OCIO::Exception,
                              "Color space '$DST3' could not be found");
    }

    {
        OCIO_CHECK_NO_THROW(cfg->addEnvironmentVar("DST2", nullptr)); 
        OCIO_CHECK_EQUAL(cfg->getNumEnvironmentVars(), 1);

        OCIO_CHECK_THROW_WHAT(cfg->validate(),
                              OCIO::Exception,
                              "references a color space '$DST2' using an unknown context variable");

        OCIO_CHECK_THROW_WHAT(cfg->getProcessor("cs1", "disp1", "view2", OCIO::TRANSFORM_DIR_FORWARD),
                              OCIO::Exception,
                              "Color space '$DST2' could not be found");
    }

    {
        OCIO_CHECK_NO_THROW(cfg->addEnvironmentVar("DST2", "cs1")); 
        OCIO_CHECK_NO_THROW(cfg->addEnvironmentVar("DST1", nullptr)); 
        OCIO_CHECK_EQUAL(cfg->getNumEnvironmentVars(), 1);

        OCIO_CHECK_THROW_WHAT(cfg->validate(),
                              OCIO::Exception,
                              "references a color space '$DST1' using an unknown context variable");

        OCIO_CHECK_THROW_WHAT(cfg->getProcessor("cs1", "disp1", "view2", OCIO::TRANSFORM_DIR_FORWARD),
                              OCIO::Exception,
                              "Color space '$DST1' could not be found");
    }
}

OCIO_ADD_TEST(Config, context_variable)
{
    // Test the context "predefined" mode (this is where the config contains the "environment"
    // section).

    static constexpr char CONFIG[] = 
        "ocio_profile_version: 2\n"
        "\n"
        "environment:\n"
        "  VAR1: $VAR1\n"     // No default value so the env. variable must exist.
        "  VAR2: var2\n"      // Default value if env. variable does not exist.
        "  VAR3: env3\n"      // Same as above.
        "  VAR4: env4$VAR1\n" // Same as above and built from another envvar.
        "  VAR5: env5$VAR2\n" // Same as above and built from another envvar.
        "  VAR6: env6$VAR3\n" // Same as above and built from another envvar.
        "search_path: luts\n"
        "strictparsing: true\n"
        "luma: [0.2126, 0.7152, 0.0722]\n"
        "\n"
        "roles:\n"
        "  default: cs1\n"
        "\n"
        "file_rules:\n"
        "  - !<Rule> {name: Default, colorspace: default}\n"
        "\n"
        "displays:\n"
        "  disp1:\n"
        "    - !<View> {name: view1, colorspace: cs1}\n"
        "\n"
        "active_displays: []\n"
        "active_views: []\n"
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: cs1\n"
        "    family: \"\"\n"
        "    equalitygroup: \"\"\n"
        "    bitdepth: unknown\n"
        "    isdata: false\n"
        "    allocation: uniform\n";

    std::istringstream iss;
    iss.str(CONFIG);
    
    struct Guard
    {
        Guard()
        {
            OCIO::Platform::Setenv("VAR1", "env1");
            OCIO::Platform::Setenv("VAR2", "env2");
        }
        ~Guard()
        {
            OCIO::Platform::Unsetenv("VAR1");
            OCIO::Platform::Unsetenv("VAR2");
        }
    } guard;

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(iss));
    OCIO_CHECK_NO_THROW(config->validate());
    OCIO_CHECK_EQUAL(config->getEnvironmentMode(), OCIO::ENV_ENVIRONMENT_LOAD_PREDEFINED);
 
    OCIO_CHECK_EQUAL(std::string("env1"), config->getCurrentContext()->resolveStringVar("$VAR1"));
    OCIO_CHECK_EQUAL(std::string("env2"), config->getCurrentContext()->resolveStringVar("$VAR2"));
    OCIO_CHECK_EQUAL(std::string("env3"), config->getCurrentContext()->resolveStringVar("$VAR3"));

    OCIO_CHECK_EQUAL(std::string("env4env1"), config->getCurrentContext()->resolveStringVar("$VAR4"));
    OCIO_CHECK_EQUAL(std::string("env5env2"), config->getCurrentContext()->resolveStringVar("$VAR5"));
    OCIO_CHECK_EQUAL(std::string("env6env3"), config->getCurrentContext()->resolveStringVar("$VAR6"));

    std::ostringstream oss;
    OCIO_CHECK_NO_THROW(oss << *config.get());
    OCIO_CHECK_EQUAL(oss.str(), iss.str());

    // VAR2 reverts to its default value.

    OCIO::Platform::Unsetenv("VAR2");
    iss.str(CONFIG);
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(iss));
    OCIO_CHECK_NO_THROW(config->validate());

    // Test a faulty case i.e. the env. variable VAR1 is now missing.

    OCIO::Platform::Unsetenv("VAR1");
    iss.str(CONFIG);
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(iss));
    OCIO_CHECK_THROW_WHAT(config->validate(),
                          OCIO::Exception,
                          "Unresolved context variable 'VAR1 = $VAR1'.");
}

OCIO_ADD_TEST(Config, context_variable_with_sanity_check)
{
    // Add some extra tests for the environment section. If declared, the context is then
    // in the predefined mode so it must be self-contained i.e. contains all needed context
    // variables. It also means that sanity check must throw if at least one context variable
    // used in the config, is missing.

    static constexpr char CONFIG[] = 
        "ocio_profile_version: 2\n"
        "\n"
        "search_path: luts\n"
        "\n"
        "environment: {CS2: lut1d_green.ctf}\n"
        "\n"
        "roles:\n"
        "  default: cs1\n"
        "\n"
        "displays:\n"
        "  disp1:\n"
        "    - !<View> {name: view1, colorspace: cs2}\n"
        "\n"
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: cs1\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: cs2\n"
        "    from_scene_reference: !<FileTransform> {src: $CS2}\n";

    std::istringstream iss;
    iss.str(CONFIG);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(iss));
    OCIO_CHECK_NO_THROW(config->validate());

    // Set the right search_path. Note that the ctf files used below already exist on that path.
    OCIO::ConfigRcPtr cfg = config->createEditableCopy();
    OCIO_CHECK_NO_THROW(cfg->clearSearchPaths());
    OCIO_CHECK_NO_THROW(cfg->addSearchPath(OCIO::GetTestFilesDir().c_str()));

    OCIO_CHECK_NO_THROW(cfg->getProcessor("cs1", "disp1", "view1", OCIO::TRANSFORM_DIR_FORWARD));

    // Having an 'environment' section in a config means to only keep the listed context
    // variables. The context is then in the predefined mode i.e. ENV_ENVIRONMENT_LOAD_PREDEFINED.

    OCIO_CHECK_EQUAL(cfg->getNumEnvironmentVars(), 1);
    OCIO_CHECK_EQUAL(cfg->getCurrentContext()->getNumStringVars(), 1);
    OCIO_CHECK_EQUAL(cfg->getCurrentContext()->getEnvironmentMode(),
                     OCIO::ENV_ENVIRONMENT_LOAD_PREDEFINED);

    {
        OCIO_CHECK_NO_THROW(cfg->addEnvironmentVar("CS2", "lut1d_green.ctf")); 
        OCIO_CHECK_EQUAL(cfg->getNumEnvironmentVars(), 1);
        OCIO_CHECK_NO_THROW(cfg->validate());
    }

    {
        OCIO_CHECK_NO_THROW(cfg->addEnvironmentVar("CS2", "exposure_contrast_log.ctf")); 
        OCIO_CHECK_EQUAL(cfg->getNumEnvironmentVars(), 1);
        OCIO_CHECK_NO_THROW(cfg->validate());
    }

    {
        // $TOTO is added but not used.
        // Even if that's useless it does not break anything.

        OCIO_CHECK_NO_THROW(cfg->addEnvironmentVar("TOTO", "exposure_contrast_log.ctf")); 
        OCIO_CHECK_EQUAL(cfg->getNumEnvironmentVars(), 2);
        OCIO_CHECK_NO_THROW(cfg->validate());
    }

    {
        // Update $CS2 to use $TOTO. That's still a self-contained context because
        // $TOTO exists. 
        OCIO_CHECK_NO_THROW(cfg->addEnvironmentVar("CS2", "$TOTO")); 
        OCIO_CHECK_EQUAL(cfg->getNumEnvironmentVars(), 2);
        OCIO_CHECK_NO_THROW(cfg->validate());

        // Note that the default value of the context variable is unresolved.
        OCIO_CHECK_EQUAL(std::string(cfg->getEnvironmentVarDefault("CS2")), std::string("$TOTO"));
    }

    {
        // Remove $TOTO from the context. That's a faulty case because $CS2 is still used
        // but resolved using $TOTO so, the environment is not self-contained. Sanity check
        // must throw in that case.
        OCIO_CHECK_NO_THROW(cfg->addEnvironmentVar("TOTO", nullptr)); 
        OCIO_CHECK_EQUAL(cfg->getNumEnvironmentVars(), 1);

        OCIO_CHECK_THROW_WHAT(cfg->validate(),
                              OCIO::Exception,
                              "Unresolved context variable 'CS2 = $TOTO'.");
        OCIO_CHECK_THROW_WHAT(cfg->getProcessor("cs1", "disp1", "view1", OCIO::TRANSFORM_DIR_FORWARD),
                              OCIO::Exception,
                              "The specified file reference '$CS2' could not be located");
    }

    {
        // Remove $CS2 from the context. That's a faulty case because $CS2 is used so,
        // the environment is not self-contained.
        OCIO_CHECK_NO_THROW(cfg->addEnvironmentVar("CS2", nullptr)); 
        OCIO_CHECK_EQUAL(cfg->getNumEnvironmentVars(), 0);

        OCIO_CHECK_THROW_WHAT(cfg->validate(),
                              OCIO::Exception,
                              "The file Transform source cannot be resolved: '$CS2'.");
        OCIO_CHECK_THROW_WHAT(cfg->getProcessor("cs1", "disp1", "view1", OCIO::TRANSFORM_DIR_FORWARD),
                              OCIO::Exception,
                              "The specified file reference '$CS2' could not be located");
    }
    
    {
        OCIO_CHECK_NO_THROW(cfg->addEnvironmentVar("CS2", "lut1d_green.ctf")); 

        // Several faulty cases for the 'search_path'.

        OCIO_CHECK_NO_THROW(cfg->clearSearchPaths());
        OCIO_CHECK_NO_THROW(cfg->setSearchPath(nullptr));
        OCIO_CHECK_THROW_WHAT(cfg->validate(),
                              OCIO::Exception,
                              "The search_path is empty");

        OCIO_CHECK_NO_THROW(cfg->clearSearchPaths());
        OCIO_CHECK_NO_THROW(cfg->setSearchPath(""));
        OCIO_CHECK_THROW_WHAT(cfg->validate(),
                              OCIO::Exception,
                              "The search_path is empty");

        OCIO_CHECK_NO_THROW(cfg->clearSearchPaths());
        OCIO_CHECK_NO_THROW(cfg->setSearchPath("$MYPATH"));
        OCIO_CHECK_THROW_WHAT(cfg->validate(),
                              OCIO::Exception,
                              "The search_path '$MYPATH' cannot be resolved.");

        // Note that search_path is mandatory only when at least one file transform is present
        // in the config.

        OCIO_CHECK_NO_THROW(cfg->clearSearchPaths());
        OCIO_CHECK_NO_THROW(cfg->setSearchPath(nullptr));
        OCIO_CHECK_NO_THROW(cfg->addDisplayView("disp1", "view1", "cs1", "")); 
        OCIO_CHECK_NO_THROW(cfg->removeColorSpace("cs2")); 
        OCIO_CHECK_NO_THROW(cfg->validate());
    }
}

OCIO_ADD_TEST(Config, colorspacename_with_reserved_token)
{
    // Using context variable tokens (i.e. $ and %) in color space names is forbidden.

    auto cfg = OCIO::Config::CreateRaw()->createEditableCopy();
    auto cs = OCIO::ColorSpace::Create();
    cs->setName("cs1$VAR");
    OCIO_CHECK_THROW_WHAT(cfg->addColorSpace(cs), OCIO::Exception,
                          "A color space name 'cs1$VAR' cannot contain a context "
                          "variable reserved token i.e. % or $.");
}

OCIO_ADD_TEST(Config, context_variable_with_colorspacename)
{
    // Test some faulty context variable use cases.

    // Note: In predefined mode, the environment section must be self-contain and complete.
    // It means that all context variables must be present in the config i.e. in the environment
    // section.

    static constexpr char CONFIG[] = 
        "ocio_profile_version: 2\n"
        "\n"
        "environment: {ENV1: file.clf}\n"
        "\n"
        "search_path: luts\n"
        "\n"
        "roles:\n"
        "  default: cs1\n"
        "  reference: cs1\n"
        "\n"
        "displays:\n"
        "  disp1:\n"
        "    - !<View> {name: view1, colorspace: cs2}\n"
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: cs1\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: cs2\n";

    {
        // Add a new context variable not defined in the environment section.  The context does not
        // contain a value for this variable.

        std::string configStr 
            = std::string(CONFIG)
            + "    from_scene_reference: !<FileTransform> {src: $VAR3}\n";

        std::istringstream iss;
        iss.str(configStr);

        OCIO::ConfigRcPtr cfg;
        OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateFromStream(iss)->createEditableCopy());
        OCIO_CHECK_THROW_WHAT(cfg->validate(),
                              OCIO::Exception,
                              "The file Transform source cannot be resolved: '$VAR3'.");

        // Set $VAR3 and check again.

        OCIO_CHECK_NO_THROW(cfg->addEnvironmentVar("VAR3", "cs1"));
        OCIO_CHECK_NO_THROW(cfg->validate());
    }

    {
        std::string configStr 
            = std::string(CONFIG)
            + "    from_scene_reference: !<ColorSpaceTransform> {src: $VAR3, dst: cs1}\n";

        std::istringstream iss;
        iss.str(configStr);

        OCIO::ConfigRcPtr cfg;
        OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateFromStream(iss)->createEditableCopy());
        OCIO_CHECK_THROW_WHAT(cfg->validate(),
                              OCIO::Exception,
                              "This config references a color space '$VAR3' using"
                              " an unknown context variable.");

        // Set $VAR3 and check again.

        // Set a valid color space name.
        OCIO_CHECK_NO_THROW(cfg->addEnvironmentVar("VAR3", "cs1"));
        OCIO_CHECK_NO_THROW(cfg->validate());

        // Set a valid role name.
        OCIO_CHECK_NO_THROW(cfg->addEnvironmentVar("VAR3", "reference"));
        OCIO_CHECK_NO_THROW(cfg->validate());

        // Set an invalid color space name.
        OCIO_CHECK_NO_THROW(cfg->addEnvironmentVar("VAR3", "cs1234"));
        OCIO_CHECK_THROW_WHAT(cfg->validate(),
                              OCIO::Exception,
                              "This config references a color space, 'cs1234', "
                              "which is not defined.");

        // Set an invalid color space name.
        OCIO_CHECK_NO_THROW(cfg->addEnvironmentVar("VAR3", "reference1234"));
        OCIO_CHECK_THROW_WHAT(cfg->validate(),
                              OCIO::Exception,
                              "This config references a color space, 'reference1234', "
                              "which is not defined.");

        // Remove the context variable.
        OCIO_CHECK_NO_THROW(cfg->addEnvironmentVar("VAR3", nullptr));
        OCIO_CHECK_THROW_WHAT(cfg->validate(),
                              OCIO::Exception,
                              "This config references a color space '$VAR3' using"
                              " an unknown context variable.");
    }

    // Repeat the test using Config::getProcessor() with a non-default context.

    {
        std::string configStr 
            = std::string(CONFIG)
            + "    from_scene_reference: !<ColorSpaceTransform> {src: $VAR3, dst: cs1}\n";

        std::istringstream iss;
        iss.str(configStr);

        OCIO::ConfigRcPtr cfg;
        OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateFromStream(iss)->createEditableCopy());

        OCIO_CHECK_THROW_WHAT(cfg->getProcessor("cs1", "cs2"),
                              OCIO::Exception,
                              "Color space '$VAR3' could not be found.");

        OCIO::ContextRcPtr ctx;
        OCIO_CHECK_NO_THROW(ctx = cfg->getCurrentContext()->createEditableCopy());
        OCIO_CHECK_THROW_WHAT(cfg->getProcessor(ctx, "cs1", "cs2"),
                              OCIO::Exception,
                              "Color space '$VAR3' could not be found.");

        OCIO_CHECK_NO_THROW(ctx->setStringVar("VAR3", "cs1"));
        OCIO_CHECK_NO_THROW(cfg->getProcessor(ctx, "cs1", "cs2"));

        OCIO_CHECK_NO_THROW(ctx->setStringVar("VAR3", "reference"));
        OCIO_CHECK_NO_THROW(cfg->getProcessor(ctx, "cs1", "cs2"));

        OCIO_CHECK_NO_THROW(ctx->setStringVar("VAR3", ""));
        OCIO_CHECK_THROW_WHAT(cfg->getProcessor(ctx, "cs1", "cs2"),
                              OCIO::Exception,
                              "Color space '$VAR3' could not be found.");
    }
}

OCIO_ADD_TEST(Config, context_variable_with_role)
{
    // Test that a role cannot point to a context variable.

    static constexpr char CONFIG[] = 
        "ocio_profile_version: 2\n"
        "\n"
        "environment: {ENV1: cs1}\n"
        "\n"
        "search_path: luts\n"
        "\n"
        "roles:\n"
        "  default: cs1\n"
        "  reference: $ENV1\n"
        "\n"
        "displays:\n"
        "  disp1:\n"
        "    - !<View> {name: view1, colorspace: cs2}\n"
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: cs1\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: cs2\n"
        "    from_scene_reference: !<CDLTransform> {offset: [0.1, 0.1, 0.1]}\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: cs3\n"
        "    from_scene_reference: !<ColorSpaceTransform> {src: reference, dst: cs2}\n";
            
    {
        std::istringstream iss;
        iss.str(CONFIG);

        OCIO::ConfigRcPtr cfg;
        OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateFromStream(iss)->createEditableCopy());

        // The internal cache serializes the config throwing an exception because the role
        // color space does not exist so disable the internal cache.
        cfg->setProcessorCacheFlags(OCIO::PROCESSOR_CACHE_OFF);

        OCIO_CHECK_THROW_WHAT(cfg->validate(), OCIO::Exception,
                              "The role 'reference' refers to a color space, '$ENV1', which is not defined.");

        OCIO_CHECK_THROW_WHAT(cfg->getProcessor("cs1", "cs3"), OCIO::Exception,
                              "Color space 'reference' could not be found.");
    }
}

OCIO_ADD_TEST(Config, context_variable_with_display_view)
{
    // Test that a (display, view) pair cannot point to a context variable.

    static constexpr char CONFIG[] = 
        "ocio_profile_version: 2\n"
        "\n"
        "environment: {ENV1: cs2}\n"
        "\n"
        "search_path: luts\n"
        "\n"
        "roles:\n"
        "  default: cs1\n"
        "  reference: cs1\n"
        "\n"
        "displays:\n"
        "  disp1:\n"
        "    - !<View> {name: view1, colorspace: $ENV1}\n"
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: cs1\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: cs2\n"
        "    from_scene_reference: !<CDLTransform> {offset: [0.1, 0.1, 0.1]}\n";

    {
        std::istringstream iss;
        iss.str(CONFIG);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(iss));

        OCIO_CHECK_THROW_WHAT(config->validate(),
                              OCIO::Exception,
                              "Display 'disp1' has a view 'view1' that refers to a color space or "
                              "a named transform, '$ENV1', which is not defined.");

        OCIO_CHECK_THROW_WHAT(config->getProcessor("cs1", "disp1", "view1", OCIO::TRANSFORM_DIR_FORWARD),
                              OCIO::Exception,
                              "DisplayViewTransform error. Cannot find color space or "
                              "named transform, named '$ENV1'.");
    }
}

OCIO_ADD_TEST(Config, context_variable_with_search_path)
{
    // Test a search_path pointing to a context variable.

    static const std::string CONFIG = 
        "ocio_profile_version: 2\n"
        "\n"
        "environment: {ENV1: " + OCIO::GetTestFilesDir() + "}\n"
        "\n"
        "search_path: $ENV1\n"
        "\n"
        "roles:\n"
        "  default: cs1\n"
        "  reference: cs1\n"
        "\n"
        "displays:\n"
        "  disp1:\n"
        "    - !<View> {name: view1, colorspace: cs2}\n"
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: cs1\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: cs2\n"
        "    from_scene_reference: !<FileTransform> {src: lut1d_green.ctf}\n";

    std::istringstream iss;
    iss.str(CONFIG);

    OCIO::ConfigRcPtr cfg;
    OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateFromStream(iss)->createEditableCopy());
    OCIO_CHECK_NO_THROW(cfg->validate());
    OCIO_CHECK_NO_THROW(cfg->getProcessor("cs1", "cs2"));

    /// Remove the context variable.
    OCIO_CHECK_NO_THROW(cfg->addEnvironmentVar("ENV1", nullptr));

    OCIO_CHECK_THROW_WHAT(cfg->validate(), OCIO::Exception,
                          "The search_path '$ENV1' cannot be resolved.");

    OCIO_CHECK_THROW_WHAT(cfg->getProcessor("cs1", "cs2"), OCIO::Exception,
                          "The specified file reference 'lut1d_green.ctf' could not be located. ");
}

OCIO_ADD_TEST(Config, role_without_colorspace)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create()->createEditableCopy();
    config->setRole("reference", "UnknownColorSpace");

    std::ostringstream os;
    OCIO_CHECK_THROW_WHAT(config->serialize(os), OCIO::Exception,
                          "Colorspace associated to the role 'reference', does not exist");
}

OCIO_ADD_TEST(Config, env_colorspace_name)
{
    // Guard to automatically unset the env. variable.
    struct Guard
    {
        Guard() = default;
        ~Guard()
        {
            OCIO::Platform::Unsetenv("OCIO_TEST");
        }
    } guard;

    const std::string MY_OCIO_CONFIG =
        "ocio_profile_version: 1\n"
        "\n"
        "search_path: luts\n"
        "strictparsing: true\n"
        "luma: [0.2126, 0.7152, 0.0722]\n"
        "\n"
        "roles:\n"
        "  compositing_log: lgh\n"
        "  default: raw\n"
        "  scene_linear: lnh\n"
        "\n"
        "displays:\n"
        "  sRGB:\n"
        "    - !<View> {name: Raw, colorspace: raw}\n"
        "\n"
        "active_displays: []\n"
        "active_views: []\n"
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: raw\n"
        "    family: \"\"\n"
        "    equalitygroup: \"\"\n"
        "    bitdepth: unknown\n"
        "    isdata: false\n"
        "    allocation: uniform\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: lnh\n"
        "    family: \"\"\n"
        "    equalitygroup: \"\"\n"
        "    bitdepth: unknown\n"
        "    isdata: false\n"
        "    allocation: uniform\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: lgh\n"
        "    family: \"\"\n"
        "    equalitygroup: \"\"\n"
        "    bitdepth: unknown\n"
        "    isdata: false\n"
        "    allocation: uniform\n"
        "    allocationvars: [-0.125, 1.125]\n";


    {
        // Test when the env. variable is missing

        const std::string 
            myConfigStr = MY_OCIO_CONFIG
                + "    from_reference: !<ColorSpaceTransform> {src: raw, dst: $MISSING_ENV}\n";

        std::istringstream is;
        is.str(myConfigStr);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_THROW_WHAT(config->validate(), OCIO::Exception,
                              "This config references a color space '$MISSING_ENV' "
                              "using an unknown context variable");
        OCIO_CHECK_THROW_WHAT(config->getProcessor("raw", "lgh"), OCIO::Exception,
                              "Color space '$MISSING_ENV' could not be found");
    }

    {
        // Test when the env. variable exists but its content is wrong
        OCIO::Platform::Setenv("OCIO_TEST", "FaultyColorSpaceName");

        const std::string 
            myConfigStr = MY_OCIO_CONFIG
                + "    from_reference: !<ColorSpaceTransform> {src: raw, dst: $OCIO_TEST}\n";

        std::istringstream is;
        is.str(myConfigStr);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_THROW_WHAT(config->validate(), OCIO::Exception,
                              "color space, 'FaultyColorSpaceName', which is not defined");
        OCIO_CHECK_THROW_WHAT(config->getProcessor("raw", "lgh"), OCIO::Exception,
                              "Color space '$OCIO_TEST' could not be found");
    }

    {
        // Test when the env. variable exists and its content is right
        OCIO::Platform::Setenv("OCIO_TEST", "lnh");

        const std::string 
            myConfigStr = MY_OCIO_CONFIG
                + "    from_reference: !<ColorSpaceTransform> {src: raw, dst: $OCIO_TEST}\n";

        std::istringstream is;
        is.str(myConfigStr);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());
        OCIO_CHECK_NO_THROW(config->getProcessor("raw", "lgh"));
    }

    {
        // Check that the serialization preserves the env. variable
        OCIO::Platform::Setenv("OCIO_TEST", "lnh");

        const std::string
            myConfigStr = MY_OCIO_CONFIG
                + "    from_reference: !<ColorSpaceTransform> {src: raw, dst: $OCIO_TEST}\n";

        std::istringstream is;
        is.str(myConfigStr);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), myConfigStr);
    }
}

OCIO_ADD_TEST(Config, version)
{
    const std::string SIMPLE_PROFILE =
        "ocio_profile_version: 2\n"
        "environment:\n"
        "  {}\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "      name: raw\n"
        "strictparsing: false\n"
        "roles:\n"
        "  default: raw\n"
        "displays:\n"
        "  sRGB:\n"
        "  - !<View> {name: Raw, colorspace: raw}\n"
        "\n";

    std::istringstream is;
    is.str(SIMPLE_PROFILE);
    OCIO::ConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is)->createEditableCopy());

    OCIO_CHECK_NO_THROW(config->validate());

    OCIO_CHECK_NO_THROW(config->setMajorVersion(1));
    OCIO_CHECK_THROW_WHAT(config->setMajorVersion(20000), OCIO::Exception,
                          "version is 20000 where supported versions start at 1 and end at 2");

    {
        OCIO_CHECK_THROW_WHAT(config->setMinorVersion(1), OCIO::Exception,
                              "The minor version 1 is not supported for major version 1. "
                              "Maximum minor version is 0");
    }

    {
        OCIO_CHECK_NO_THROW(config->setMinorVersion(0));

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());   
        OCIO_CHECK_ASSERT(StringUtils::StartsWith(StringUtils::Lower(ss.str()),
                          "ocio_profile_version: 1"));
    }

    {
        OCIO_CHECK_NO_THROW(config->setMajorVersion(2));

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());   
        OCIO_CHECK_ASSERT(StringUtils::StartsWith(StringUtils::Lower(ss.str()),
                          "ocio_profile_version: 2"));
    }

    {
        OCIO_CHECK_THROW_WHAT(config->setVersion(2, 1), OCIO::Exception,
                              "The minor version 1 is not supported for major version 2. "
                              "Maximum minor version is 0");

        OCIO_CHECK_NO_THROW(config->setMajorVersion(2));
        OCIO_CHECK_THROW_WHAT(config->setMinorVersion(1), OCIO::Exception,
                              "The minor version 1 is not supported for major version 2. "
                              "Maximum minor version is 0");
    }

    {
        OCIO_CHECK_THROW_WHAT(config->setVersion(3, 4), OCIO::Exception,
                              "version is 3 where supported versions start at 1 and end at 2");
    }
}

OCIO_ADD_TEST(Config, version_validation)
{
    const std::string SIMPLE_PROFILE_END =
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "      name: raw\n"
        "strictparsing: false\n"
        "roles:\n"
        "  default: raw\n"
        "displays:\n"
        "  sRGB:\n"
        "  - !<View> {name: Raw, colorspace: raw}\n"
        "\n";

    {
        std::istringstream is;
        is.str("ocio_profile_version: 2.0.1\n" + SIMPLE_PROFILE_END);
        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                              "does not appear to have a valid version 2.0.1");
    }

    {
        std::istringstream is;
        is.str("ocio_profile_version: 2.1\n" + SIMPLE_PROFILE_END);
        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                              "The minor version 1 is not supported for major version 2");
    }

    {
        std::istringstream is;
        is.str("ocio_profile_version: 3\n" + SIMPLE_PROFILE_END);
        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                              "The version is 3 where supported versions start at 1 and end at 2");
    }

    {
        std::istringstream is;
        is.str("ocio_profile_version: 3.0\n" + SIMPLE_PROFILE_END);
        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                              "The version is 3 where supported versions start at 1 and end at 2");
    }

    {
        std::istringstream is;
        is.str("ocio_profile_version: 1.0\n" + SIMPLE_PROFILE_END);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_ASSERT(config);
        OCIO_CHECK_EQUAL(config->getMajorVersion(), 1);
        OCIO_CHECK_EQUAL(config->getMinorVersion(), 0);
    }

    {
        std::istringstream is;
        is.str("ocio_profile_version: 2.0\n" + SIMPLE_PROFILE_END);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_ASSERT(config);
        OCIO_CHECK_EQUAL(config->getMajorVersion(), 2);
        OCIO_CHECK_EQUAL(config->getMinorVersion(), 0);
    }
}

namespace
{

const std::string PROFILE_V1 = 
    "ocio_profile_version: 1\n"
    "\n";

const std::string PROFILE_V2 =
    "ocio_profile_version: 2\n"
    "\n"
    "environment:\n"
    "  {}\n";

const std::string SIMPLE_PROFILE_A =
    "search_path: luts\n"
    "strictparsing: true\n"
    "luma: [0.2126, 0.7152, 0.0722]\n"
    "\n"
    "roles:\n"
    "  default: raw\n"
    "  scene_linear: lnh\n"
    "\n";

const std::string SIMPLE_PROFILE_DISPLAYS_LOOKS =
    "displays:\n"
    "  sRGB:\n"
    "    - !<View> {name: RawView, colorspace: raw}\n"
    "    - !<View> {name: LnhView, colorspace: lnh, looks: beauty}\n"
    "\n"
    "active_displays: []\n"
    "active_views: []\n"
    "\n"
    "looks:\n"
    "  - !<Look>\n"
    "    name: beauty\n"
    "    process_space: lnh\n"
    "    transform: !<CDLTransform> {slope: [1, 2, 1]}\n"
    "\n";

const std::string SIMPLE_PROFILE_CS_V1 =
    "\n"
    "colorspaces:\n"
    "  - !<ColorSpace>\n"
    "    name: raw\n"
    "    family: \"\"\n"
    "    equalitygroup: \"\"\n"
    "    bitdepth: unknown\n"
    "    isdata: false\n"
    "    allocation: uniform\n"
    "\n"
    "  - !<ColorSpace>\n"
    "    name: log\n"
    "    family: \"\"\n"
    "    equalitygroup: \"\"\n"
    "    bitdepth: unknown\n"
    "    isdata: false\n"
    "    allocation: uniform\n"
    "    from_reference: !<LogTransform> {base: 10}\n"
    "\n"
    "  - !<ColorSpace>\n"
    "    name: lnh\n"
    "    family: \"\"\n"
    "    equalitygroup: \"\"\n"
    "    bitdepth: unknown\n"
    "    isdata: false\n"
    "    allocation: uniform\n";

const std::string SIMPLE_PROFILE_CS_V2 =
    "\n"
    "colorspaces:\n"
    "  - !<ColorSpace>\n"
    "    name: raw\n"
    "    family: \"\"\n"
    "    equalitygroup: \"\"\n"
    "    bitdepth: unknown\n"
    "    isdata: false\n"
    "    allocation: uniform\n"
    "\n"
    "  - !<ColorSpace>\n"
    "    name: log\n"
    "    family: \"\"\n"
    "    equalitygroup: \"\"\n"
    "    bitdepth: unknown\n"
    "    isdata: false\n"
    "    allocation: uniform\n"
    "    from_scene_reference: !<LogTransform> {base: 10}\n"
    "\n"
    "  - !<ColorSpace>\n"
    "    name: lnh\n"
    "    family: \"\"\n"
    "    equalitygroup: \"\"\n"
    "    bitdepth: unknown\n"
    "    isdata: false\n"
    "    allocation: uniform\n";

const std::string SIMPLE_PROFILE_B_V1 = SIMPLE_PROFILE_DISPLAYS_LOOKS + SIMPLE_PROFILE_CS_V1;
const std::string SIMPLE_PROFILE_B_V2 = SIMPLE_PROFILE_DISPLAYS_LOOKS + SIMPLE_PROFILE_CS_V2;

const std::string DEFAULT_RULES =
    "file_rules:\n"
    "  - !<Rule> {name: Default, colorspace: default}\n"
    "\n";

const std::string PROFILE_V2_START = PROFILE_V2 + SIMPLE_PROFILE_A +
                                     DEFAULT_RULES + SIMPLE_PROFILE_B_V2;
}

OCIO_ADD_TEST(Config, serialize_colorspace_displayview_transforms)
{
    // Validate that a ColorSpaceTransform and DisplayViewTransform are correctly serialized.
    {
        const std::string strEnd =
            "    from_scene_reference: !<GroupTransform>\n"
            "      children:\n"
            "        - !<ColorSpaceTransform> {src: raw, dst: log}\n"
            "        - !<ColorSpaceTransform> {src: raw, dst: log, direction: inverse}\n"
            "        - !<ColorSpaceTransform> {src: default, dst: log, data_bypass: false}\n"
            "        - !<DisplayViewTransform> {src: raw, display: sRGB, view: RawView}\n"
            "        - !<DisplayViewTransform> {src: default, display: sRGB, view: RawView, direction: inverse}\n"
            "        - !<DisplayViewTransform> {src: log, display: sRGB, view: RawView, looks_bypass: true, data_bypass: false}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        // Write the config.

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }
}

OCIO_ADD_TEST(Config, range_serialization)
{
    {
        const std::string strEnd =
            "    from_scene_reference: !<RangeTransform> {min_in_value: 0, min_out_value: 0}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        const std::string strEnd =
            "    from_scene_reference: !<RangeTransform> {min_in_value: 0, min_out_value: 0, "
            "direction: inverse}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        const std::string strEnd =
            "    from_scene_reference: !<RangeTransform> {min_in_value: 0, min_out_value: 0, "
            "style: noClamp}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_THROW_WHAT(config->validate(), OCIO::Exception,
                              "non clamping range must have min and max values defined");
    }

    {
        const std::string strEnd =
            "    from_scene_reference: !<RangeTransform> {min_in_value: 0, max_in_value: 1, "
            "min_out_value: 0, max_out_value: 1, style: noClamp, direction: inverse}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // Test Range with clamp style (i.e. default one)
        const std::string strEnd =
            "    from_scene_reference: !<RangeTransform> {min_in_value: -0.0109, "
            "max_in_value: 1.0505, min_out_value: 0.0009, max_out_value: 2.5001, "
            "direction: inverse}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // Test Range with clamp style
        const std::string in_strEnd =
            "    from_scene_reference: !<RangeTransform> {min_in_value: -0.0109, "
            "max_in_value: 1.0505, min_out_value: 0.0009, max_out_value: 2.5001, "
            "style: Clamp, direction: inverse}\n";
        const std::string in_str = PROFILE_V2_START + in_strEnd;

        std::istringstream is;
        is.str(in_str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        // Clamp style is not saved
        const std::string out_strEnd =
            "    from_scene_reference: !<RangeTransform> {min_in_value: -0.0109, "
            "max_in_value: 1.0505, min_out_value: 0.0009, max_out_value: 2.5001, "
            "direction: inverse}\n";
        const std::string out_str = PROFILE_V2_START + out_strEnd;

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), out_str);
    }

    {
        const std::string strEnd =
            "    from_scene_reference: !<RangeTransform> "
            "{min_in_value: 0, max_out_value: 1}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_THROW_WHAT(config->validate(), 
                              OCIO::Exception, 
                              "must be both set or both missing");

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // max_in_value has an illegal second number.
        const std::string strEndFail =
            "    from_scene_reference: !<RangeTransform> {min_in_value: -0.01, "
            "max_in_value: 1.05  10, min_out_value: 0.0009, max_out_value: 2.5}\n";
        const std::string strEnd =
            "    from_scene_reference: !<RangeTransform> {min_in_value: -0.01, "
            "max_in_value: 1.05, min_out_value: 0.0009, max_out_value: 2.5}\n";

        const std::string str = PROFILE_V2 + SIMPLE_PROFILE_A + SIMPLE_PROFILE_B_V2 + strEndFail;
        const std::string strSaved = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is),
                              OCIO::Exception, "parsing double failed");

        is.str(strSaved);
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        // Re-serialize and test that it matches the expected text.
        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), strSaved);
    }

    {
        // max_in_value & max_out_value have no value, they will not be defined.
        const std::string strEnd =
            "    from_scene_reference: !<RangeTransform> {min_in_value: -0.01, "
            "max_in_value: , min_out_value: -0.01, max_out_value: }\n";
        const std::string strEndSaved =
            "    from_scene_reference: !<RangeTransform> {min_in_value: -0.01, "
            "min_out_value: -0.01}\n";
        const std::string str = PROFILE_V2 + SIMPLE_PROFILE_A + SIMPLE_PROFILE_B_V2 + strEnd;
        const std::string strSaved = PROFILE_V2_START + strEndSaved;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        // Re-serialize and test that it matches the expected text.
        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), strSaved);
    }

    {
        const std::string strEnd =
            "    from_scene_reference: !<RangeTransform> "
            "{min_in_value: 0.12345678901234, max_out_value: 1.23456789012345}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_THROW_WHAT(config->validate(),
            OCIO::Exception,
            "must be both set or both missing");

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        const std::string strEnd =
            "    from_scene_reference: !<RangeTransform> {min_in_value: -0.01, "
            "max_in_value: 1.05, min_out_value: 0.0009, max_out_value: 2.5}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        // Re-serialize and test that it matches the original text.
        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        const std::string strEnd =
            "    from_scene_reference: !<RangeTransform> {min_out_value: 0.0009, "
            "max_out_value: 2.5}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_THROW_WHAT(config->validate(),
                              OCIO::Exception,
                              "must be both set or both missing");

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        const std::string strEnd =
            "    from_scene_reference: !<GroupTransform>\n"
            "      children:\n"
            "        - !<RangeTransform> {min_in_value: -0.01, max_in_value: 1.05, "
            "min_out_value: 0.0009, max_out_value: 2.5}\n"
            "        - !<RangeTransform> {min_out_value: 0.0009, max_out_value: 2.1}\n"
            "        - !<RangeTransform> {min_out_value: 0.1, max_out_value: 0.9}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_THROW_WHAT(config->validate(),
                              OCIO::Exception,
                              "must be both set or both missing");

        // Re-serialize and test that it matches the original text.
        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    // Some faulty cases

    {
        const std::string strEnd =
            "    from_scene_reference: !<GroupTransform>\n"
            "      children:\n"
            // missing { (and mInValue is wrong -> that's a warning)
            "        - !<RangeTransform> mInValue: -0.01, max_in_value: 1.05, "
            "min_out_value: 0.0009, max_out_value: 2.5}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is),
                              OCIO::Exception,
                              "Loading the OCIO profile failed");
    }

    {
        const std::string strEnd =
            // The comma is missing after the min_in_value value.
            "    from_scene_reference: !<RangeTransform> {min_in_value: -0.01 "
            "max_in_value: 1.05, min_out_value: 0.0009, max_out_value: 2.5}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is),
                              OCIO::Exception,
                              "Loading the OCIO profile failed");
    }

    {
        const std::string strEnd =
            "    from_scene_reference: !<RangeTransform> {min_in_value: -0.01, "
            // The comma is missing between the min_out_value value and
            // the max_out_value tag.
            "max_in_value: 1.05, min_out_value: 0.0009maxOutValue: 2.5}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is),
                              OCIO::Exception,
                              "Loading the OCIO profile failed");
    }
}

OCIO_ADD_TEST(Config, exponent_serialization)  
{
    const std::string SIMPLE_PROFILE_V1 = PROFILE_V1 + SIMPLE_PROFILE_A + SIMPLE_PROFILE_B_V1;
    {
        const std::string strEnd = 
            "    from_reference: !<ExponentTransform> " 
            "{value: [1.101, 1.202, 1.303, 1.404]}\n";  
        const std::string str = SIMPLE_PROFILE_V1 + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::stringstream ss;  
        OCIO_CHECK_NO_THROW(ss << *config.get());    
        OCIO_CHECK_EQUAL(ss.str(), str);    
    }

    // If R==G==B and A==1, and the version is > 1, it is serialized using a more compact syntax.
    {
        const std::string strEnd =
            "    from_scene_reference: !<ExponentTransform> "
            "{value: 1.101}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    // If version==1, then write all values for compatibility with the v1 library.
    {
        const std::string strEnd =
            "    from_reference: !<ExponentTransform> "
            "{value: [1.101, 1.101, 1.101, 1]}\n";
        const std::string str = SIMPLE_PROFILE_V1 + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

     {
        const std::string strEnd =  
            "    from_reference: !<ExponentTransform> " 
            "{value: [1.101, 1.202, 1.303, 1.404], direction: inverse}\n";  
        const std::string str = SIMPLE_PROFILE_V1 + strEnd;

        std::istringstream is;
        is.str(str); 
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::stringstream ss;  
        OCIO_CHECK_NO_THROW(ss << *config.get());    
        OCIO_CHECK_EQUAL(ss.str(), str);    
    }

     {
         const std::string strEnd =
             "    from_scene_reference: !<ExponentTransform> "
             "{value: [1.101, 1.202, 1.303, 1.404], style: mirror, direction: inverse}\n";
         const std::string str = PROFILE_V2_START + strEnd;

         std::istringstream is;
         is.str(str);
         OCIO::ConstConfigRcPtr config;
         OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
         OCIO_CHECK_NO_THROW(config->validate());

         std::stringstream ss;
         OCIO_CHECK_NO_THROW(ss << *config.get());
         OCIO_CHECK_EQUAL(ss.str(), str);
     }

     {
         const std::string strEnd =
             "    from_scene_reference: !<ExponentTransform> "
             "{value: [1.101, 1.202, 1.303, 1.404], style: pass_thru, direction: inverse}\n";
         const std::string str = PROFILE_V2_START + strEnd;

         std::istringstream is;
         is.str(str);
         OCIO::ConstConfigRcPtr config;
         OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
         OCIO_CHECK_NO_THROW(config->validate());

         std::stringstream ss;
         OCIO_CHECK_NO_THROW(ss << *config.get());
         OCIO_CHECK_EQUAL(ss.str(), str);
     }

    // Errors

    {
        // Some gamma values are missing.
        const std::string strEnd =
            "    from_reference: !<ExponentTransform> "
            "{value: [1.1, 1.2, 1.3]}\n";
        const std::string str = SIMPLE_PROFILE_V1 + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is),
                              OCIO::Exception,
                              "'value' values must be 4 floats. Found '3'");
    }

    {
        // Wrong style.
        const std::string strEnd =
            "    from_reference: !<ExponentTransform> "
            "{value: [1.101, 1.202, 1.303, 1.404], style: wrong,}\n";
        const std::string str = SIMPLE_PROFILE_V1 + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is),
                              OCIO::Exception,
                              "Unknown exponent style");
    }
}

OCIO_ADD_TEST(Config, exponent_with_linear_serialization)
{
    {
        const std::string strEnd =
            "    from_scene_reference: !<ExponentWithLinearTransform> "
            "{gamma: [1.1, 1.2, 1.3, 1.4], offset: [0.101, 0.102, 0.103, 0.1]}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        const std::string strEnd =
            "    from_scene_reference: !<ExponentWithLinearTransform> "
            "{gamma: [1.1, 1.2, 1.3, 1.4], offset: [0.101, 0.102, 0.103, 0.1], style: mirror}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        const std::string strEnd =
            "    from_scene_reference: !<ExponentWithLinearTransform> "
            "{gamma: [1.1, 1.2, 1.3, 1.4], offset: [0.101, 0.102, 0.103, 0.1], "
            "direction: inverse}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str().size(), str.size());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        const std::string strEnd =
            "    from_scene_reference: !<ExponentWithLinearTransform> "
            "{gamma: [1.1, 1.2, 1.3, 1.4], offset: [0.101, 0.102, 0.103, 0.1], style: mirror, "
            "direction: inverse}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        const std::string strEnd =
            "    from_scene_reference: !<ExponentWithLinearTransform> "
            "{gamma: 1.1, offset: 0.101, "
            "direction: inverse}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str().size(), str.size());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    // Errors

    {
        const std::string strEnd =
            "    from_scene_reference: !<ExponentWithLinearTransform> {}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_THROW_WHAT(config = OCIO::Config::CreateFromStream(is),
                              OCIO::Exception,
                              "ExponentWithLinear parse error, gamma and offset fields are missing");
    }

    {
        // Offset values are missing.
        const std::string strEnd =
            "    from_scene_reference: !<ExponentWithLinearTransform> "
            "{gamma: [1.1, 1.2, 1.3, 1.4]}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_THROW_WHAT(config = OCIO::Config::CreateFromStream(is),
                              OCIO::Exception,
                              "ExponentWithLinear parse error, offset field is missing");
    }

    {
        // Gamma values are missing.
        const std::string strEnd =
            "    from_scene_reference: !<ExponentWithLinearTransform> "
            "{offset: [1.1, 1.2, 1.3, 1.4]}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_THROW_WHAT(config = OCIO::Config::CreateFromStream(is),
                              OCIO::Exception,
                              "ExponentWithLinear parse error, gamma field is missing");
    }

    {
        // Some gamma values are missing.
        const std::string strEnd =
            "    from_scene_reference: !<ExponentWithLinearTransform> "
            "{gamma: [1.1, 1.2, 1.3]}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_THROW_WHAT(config = OCIO::Config::CreateFromStream(is),
                              OCIO::Exception,
                              "ExponentWithLinear parse error, gamma field must be 4 floats");
    }
    {
        // Some offset values are missing.
        const std::string strEnd =
            "    from_scene_reference: !<ExponentWithLinearTransform> "
            "{gamma: [1.1, 1.2, 1.3, 1.4], offset: [0.101, 0.102]}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_THROW_WHAT(config = OCIO::Config::CreateFromStream(is),
                              OCIO::Exception,
                              "ExponentWithLinear parse error, offset field must be 4 floats");
    }

    {
        const std::string strEnd =
            "    from_scene_reference: !<ExponentWithLinearTransform> "
            "{gamma: [1.1, 1.2, 1.3, 1.4], offset: [0.101, 0.102, 0.103, 0.1], "
            "direction: inverse, style: pass_thru}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                              "Pass thru negative extrapolation is not valid for MonCurve");
    }
}

OCIO_ADD_TEST(Config, exponent_vs_config_version)
{
    // The config i.e. SIMPLE_PROFILE is a version 2.

    std::istringstream is;
    OCIO::ConstConfigRcPtr config;
    OCIO::ConstProcessorRcPtr processor;

    // OCIO config file version == 1  and exponent == 1

    const std::string strEnd =
        "    from_reference: !<ExponentTransform> {value: [1, 1, 1, 1]}\n";
    const std::string str = PROFILE_V1 + SIMPLE_PROFILE_A + SIMPLE_PROFILE_B_V1 + strEnd;

    is.str(str);
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());

    OCIO_CHECK_NO_THROW(processor = config->getProcessor("raw", "lnh"));

    OCIO::ConstCPUProcessorRcPtr cpuProcessor;
    OCIO_CHECK_NO_THROW(cpuProcessor = processor->getDefaultCPUProcessor());

    float img1[4] = { -0.5f, 0.0f, 1.0f, 1.0f };
    OCIO_CHECK_NO_THROW(cpuProcessor->applyRGBA(img1));

    OCIO_CHECK_EQUAL(img1[0], -0.5f);
    OCIO_CHECK_EQUAL(img1[1],  0.0f);
    OCIO_CHECK_EQUAL(img1[2],  1.0f);
    OCIO_CHECK_EQUAL(img1[3],  1.0f);

    // OCIO config file version == 1  and exponent != 1

    const std::string strEnd2 =
        "    from_reference: !<ExponentTransform> {value: [2, 2, 2, 1]}\n";
    const std::string str2 = PROFILE_V1 + SIMPLE_PROFILE_A + SIMPLE_PROFILE_B_V1 + strEnd2;

    is.str(str2);
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());

    OCIO_CHECK_NO_THROW(processor = config->getProcessor("raw", "lnh"));
    OCIO_CHECK_NO_THROW(cpuProcessor = processor->getDefaultCPUProcessor());

    float img2[4] = { -0.5f, 0.0f, 1.0f, 1.0f };
    OCIO_CHECK_NO_THROW(cpuProcessor->applyRGBA(img2));

    OCIO_CHECK_EQUAL(img2[0],  0.0f);
    OCIO_CHECK_EQUAL(img2[1],  0.0f);
    OCIO_CHECK_EQUAL(img2[2],  1.0f);
    OCIO_CHECK_EQUAL(img2[3],  1.0f);

    // OCIO config file version > 1  and exponent == 1

    std::string str3 = PROFILE_V2_START + strEnd;
    is.str(str3);
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());

    OCIO_CHECK_NO_THROW(processor = config->getProcessor("raw", "lnh"));
    OCIO_CHECK_NO_THROW(cpuProcessor = processor->getDefaultCPUProcessor());

    float img3[4] = { -0.5f, 0.0f, 1.0f, 1.0f };
    OCIO_CHECK_NO_THROW(cpuProcessor->applyRGBA(img3));

    OCIO_CHECK_EQUAL(img3[0], 0.0f);
    OCIO_CHECK_EQUAL(img3[1], 0.0f);
    OCIO_CHECK_CLOSE(img3[2], 1.0f, 2e-5f); // Because of SSE optimizations.
    OCIO_CHECK_CLOSE(img3[3], 1.0f, 2e-5f); // Because of SSE optimizations.

    // OCIO config file version > 1  and exponent != 1

    std::string str4 = PROFILE_V2_START + strEnd2;
    is.str(str4);
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());

    OCIO_CHECK_NO_THROW(processor = config->getProcessor("raw", "lnh"));
    OCIO_CHECK_NO_THROW(cpuProcessor = processor->getDefaultCPUProcessor());

    float img4[4] = { -0.5f, 0.0f, 1.0f, 1.0f };
    OCIO_CHECK_NO_THROW(cpuProcessor->applyRGBA(img4));

    OCIO_CHECK_EQUAL(img4[0], 0.0f);
    OCIO_CHECK_EQUAL(img4[1], 0.0f);
    OCIO_CHECK_CLOSE(img4[2], 1.0f, 3e-5f); // Because of SSE optimizations.
    OCIO_CHECK_CLOSE(img4[3], 1.0f, 2e-5f); // Because of SSE optimizations.
}

OCIO_ADD_TEST(Config, categories)
{
    static const std::string MY_OCIO_CONFIG =
        "ocio_profile_version: 2\n"
        "\n"
        "environment:\n"
        "  {}\n"
        "search_path: luts\n"
        "strictparsing: true\n"
        "luma: [0.2126, 0.7152, 0.0722]\n"
        "\n"
        "roles:\n"
        "  default: raw1\n"
        "  scene_linear: raw1\n"
        "\n"
        "file_rules:\n"
        "  - !<Rule> {name: Default, colorspace: default}\n"
        "\n"
        "displays:\n"
        "  sRGB:\n"
        "    - !<View> {name: Raw, colorspace: raw1}\n"
        "\n"
        "active_displays: []\n"
        "active_views: []\n"
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: raw1\n"
        "    family: \"\"\n"
        "    equalitygroup: \"\"\n"
        "    bitdepth: unknown\n"
        "    isdata: false\n"
        "    categories: [rendering, linear]\n"
        "    encoding: scene-linear\n"
        "    allocation: uniform\n"
        "    allocationvars: [-0.125, 1.125]\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: raw2\n"
        "    family: \"\"\n"
        "    equalitygroup: \"\"\n"
        "    bitdepth: unknown\n"
        "    isdata: false\n"
        "    categories: [rendering]\n"
        "    encoding: data\n"
        "    allocation: uniform\n"
        "    allocationvars: [-0.125, 1.125]\n";

    std::istringstream is;
    is.str(MY_OCIO_CONFIG);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());

    // Test the serialization & deserialization.

    std::stringstream ss;
    OCIO_CHECK_NO_THROW(ss << *config.get());
    OCIO_CHECK_EQUAL(ss.str(), MY_OCIO_CONFIG);

    // Test the config content.

    OCIO::ColorSpaceSetRcPtr css = config->getColorSpaces(nullptr);
    OCIO_CHECK_EQUAL(css->getNumColorSpaces(), 2);
    OCIO::ConstColorSpaceRcPtr cs = css->getColorSpaceByIndex(0);
    OCIO_CHECK_EQUAL(cs->getNumCategories(), 2);
    OCIO_CHECK_EQUAL(std::string(cs->getCategory(0)), std::string("rendering"));
    OCIO_CHECK_EQUAL(std::string(cs->getCategory(1)), std::string("linear"));

    css = config->getColorSpaces("linear");
    OCIO_CHECK_EQUAL(css->getNumColorSpaces(), 1);
    cs = css->getColorSpaceByIndex(0);
    OCIO_CHECK_EQUAL(cs->getNumCategories(), 2);
    OCIO_CHECK_EQUAL(std::string(cs->getCategory(0)), std::string("rendering"));
    OCIO_CHECK_EQUAL(std::string(cs->getCategory(1)), std::string("linear"));

    css = config->getColorSpaces("rendering");
    OCIO_CHECK_EQUAL(css->getNumColorSpaces(), 2);

    OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 2);
    OCIO_CHECK_EQUAL(std::string(config->getColorSpaceNameByIndex(0)), std::string("raw1"));
    OCIO_CHECK_EQUAL(std::string(config->getColorSpaceNameByIndex(1)), std::string("raw2"));
    OCIO_CHECK_EQUAL(config->getIndexForColorSpace("raw1"), 0);
    OCIO_CHECK_EQUAL(config->getIndexForColorSpace("raw2"), 1);
    cs = config->getColorSpace("raw1");
    OCIO_CHECK_EQUAL(std::string(cs->getName()), std::string("raw1"));
    OCIO_CHECK_EQUAL(std::string(cs->getEncoding()), std::string("scene-linear"));
    cs = config->getColorSpace("raw2");
    OCIO_CHECK_EQUAL(std::string(cs->getName()), std::string("raw2"));
    OCIO_CHECK_EQUAL(std::string(cs->getEncoding()), std::string("data"));
}

OCIO_ADD_TEST(Config, display)
{
    // Guard to automatically unset the env. variable.
    struct Guard
    {
        Guard() = default;
        ~Guard()
        {
            OCIO::Platform::Unsetenv(OCIO::OCIO_ACTIVE_DISPLAYS_ENVVAR);
        }
    } guard;


    static const std::string SIMPLE_PROFILE_HEADER =
        "ocio_profile_version: 2\n"
        "\n"
        "environment:\n"
        "  {}\n"
        "search_path: luts\n"
        "strictparsing: true\n"
        "luma: [0.2126, 0.7152, 0.0722]\n"
        "\n"
        "roles:\n"
        "  default: raw\n"
        "  scene_linear: lnh\n"
        "\n"
        "file_rules:\n"
        "  - !<Rule> {name: Default, colorspace: default}\n"
        "\n"
        "displays:\n"
        "  sRGB_2:\n"
        "    - !<View> {name: Raw, colorspace: raw}\n"
        "  sRGB_F:\n"
        "    - !<View> {name: Raw, colorspace: raw}\n"
        "  sRGB_1:\n"
        "    - !<View> {name: Raw, colorspace: raw}\n"
        "  sRGB_3:\n"
        "    - !<View> {name: Raw, colorspace: raw}\n"
        "  sRGB_B:\n"
        "    - !<View> {name: Raw, colorspace: raw}\n"
        "  sRGB_A:\n"
        "    - !<View> {name: Raw, colorspace: raw}\n"
        "\n";

    static const std::string SIMPLE_PROFILE_FOOTER =
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: raw\n"
        "    family: \"\"\n"
        "    equalitygroup: \"\"\n"
        "    bitdepth: unknown\n"
        "    isdata: false\n"
        "    allocation: uniform\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: lnh\n"
        "    family: \"\"\n"
        "    equalitygroup: \"\"\n"
        "    bitdepth: unknown\n"
        "    isdata: false\n"
        "    allocation: uniform\n";

    {
        const std::string myProfile = 
            SIMPLE_PROFILE_HEADER
            +
            "active_displays: []\n"
            "active_views: []\n"
            + SIMPLE_PROFILE_FOOTER;

        std::istringstream is(myProfile);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        OCIO_REQUIRE_EQUAL(config->getNumDisplays(), 6);
        OCIO_CHECK_EQUAL(std::string(config->getDisplay(0)), std::string("sRGB_2"));
        OCIO_CHECK_EQUAL(std::string(config->getDisplay(1)), std::string("sRGB_F"));
        OCIO_CHECK_EQUAL(std::string(config->getDisplay(2)), std::string("sRGB_1"));
        OCIO_CHECK_EQUAL(std::string(config->getDisplay(3)), std::string("sRGB_3"));
        OCIO_CHECK_EQUAL(std::string(config->getDisplay(4)), std::string("sRGB_B"));
        OCIO_CHECK_EQUAL(std::string(config->getDisplay(5)), std::string("sRGB_A"));
        OCIO_CHECK_EQUAL(std::string(config->getDefaultDisplay()), "sRGB_2");

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), myProfile);
    }

    {
        const std::string myProfile = 
            SIMPLE_PROFILE_HEADER
            +
            "active_displays: [sRGB_1]\n"
            "active_views: []\n"
            + SIMPLE_PROFILE_FOOTER;

        std::istringstream is(myProfile);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        OCIO_REQUIRE_EQUAL(config->getNumDisplays(), 1);
        OCIO_CHECK_EQUAL(std::string(config->getDisplay(0)), std::string("sRGB_1"));
        OCIO_CHECK_EQUAL(std::string(config->getDefaultDisplay()), "sRGB_1");

        OCIO_REQUIRE_EQUAL(config->getNumDisplaysAll(), 6);

        // Test that all displays are saved.
        std::stringstream ss;
        ss << *config.get();
        OCIO_CHECK_EQUAL(ss.str(), myProfile);
    }

    {
        const std::string myProfile = 
            SIMPLE_PROFILE_HEADER
            +
            "active_displays: [sRGB_2, sRGB_1]\n"
            "active_views: []\n"
            + SIMPLE_PROFILE_FOOTER;

        std::istringstream is(myProfile);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));

        OCIO_REQUIRE_EQUAL(config->getNumDisplays(), 2);
        OCIO_CHECK_EQUAL(std::string(config->getDisplay(0)), std::string("sRGB_2"));
        OCIO_CHECK_EQUAL(std::string(config->getDisplay(1)), std::string("sRGB_1"));
        OCIO_CHECK_EQUAL(std::string(config->getDefaultDisplay()), "sRGB_2");
    }

    {
        const std::string myProfile = 
            SIMPLE_PROFILE_HEADER
            +
            "active_displays: []\n"
            "active_views: []\n"
            + SIMPLE_PROFILE_FOOTER;

        OCIO::Platform::Setenv(OCIO::OCIO_ACTIVE_DISPLAYS_ENVVAR, " sRGB_3, sRGB_2");

        std::istringstream is(myProfile);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        OCIO_REQUIRE_EQUAL(config->getNumDisplays(), 2);
        OCIO_CHECK_EQUAL(std::string(config->getDisplay(0)), std::string("sRGB_3"));
        OCIO_CHECK_EQUAL(std::string(config->getDisplay(1)), std::string("sRGB_2"));
        OCIO_CHECK_EQUAL(std::string(config->getDefaultDisplay()), "sRGB_3");
    }

    {
        const std::string myProfile = 
            SIMPLE_PROFILE_HEADER
            +
            "active_displays: [sRGB_2, sRGB_1]\n"
            "active_views: []\n"
            + SIMPLE_PROFILE_FOOTER;

        OCIO::Platform::Setenv(OCIO::OCIO_ACTIVE_DISPLAYS_ENVVAR, " sRGB_3, sRGB_2");

        std::istringstream is(myProfile);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        OCIO_REQUIRE_EQUAL(config->getNumDisplays(), 2);
        OCIO_CHECK_EQUAL(std::string(config->getDisplay(0)), std::string("sRGB_3"));
        OCIO_CHECK_EQUAL(std::string(config->getDisplay(1)), std::string("sRGB_2"));
        OCIO_CHECK_EQUAL(std::string(config->getDefaultDisplay()), "sRGB_3");
    }

    {
        OCIO::Platform::Setenv(OCIO::OCIO_ACTIVE_DISPLAYS_ENVVAR, ""); // No value

        const std::string myProfile = 
            SIMPLE_PROFILE_HEADER
            +
            "active_displays: [sRGB_2, sRGB_1]\n"
            "active_views: []\n"
            + SIMPLE_PROFILE_FOOTER;

        std::istringstream is(myProfile);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        OCIO_REQUIRE_EQUAL(config->getNumDisplays(), 2);
        OCIO_CHECK_EQUAL(std::string(config->getDisplay(0)), std::string("sRGB_2"));
        OCIO_CHECK_EQUAL(std::string(config->getDisplay(1)), std::string("sRGB_1"));
        OCIO_CHECK_EQUAL(std::string(config->getDefaultDisplay()), "sRGB_2");
    }

    {
        // No value, but misleading space.

        OCIO::Platform::Setenv(OCIO::OCIO_ACTIVE_DISPLAYS_ENVVAR, " ");

        const std::string myProfile = 
            SIMPLE_PROFILE_HEADER
            +
            "active_displays: [sRGB_2, sRGB_1]\n"
            "active_views: []\n"
            + SIMPLE_PROFILE_FOOTER;

        std::istringstream is(myProfile);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        OCIO_REQUIRE_EQUAL(config->getNumDisplays(), 2);
        OCIO_CHECK_EQUAL(std::string(config->getDisplay(0)), std::string("sRGB_2"));
        OCIO_CHECK_EQUAL(std::string(config->getDisplay(1)), std::string("sRGB_1"));
        OCIO_CHECK_EQUAL(std::string(config->getDefaultDisplay()), "sRGB_2");
    }

    {
        // Test an unknown display name using the env. variable.

        OCIO::Platform::Setenv(OCIO::OCIO_ACTIVE_DISPLAYS_ENVVAR, "ABCDEF");

        const std::string myProfile = 
            SIMPLE_PROFILE_HEADER
            +
            "active_displays: [sRGB_2, sRGB_1]\n"
            "active_views: []\n"
            + SIMPLE_PROFILE_FOOTER;

        std::istringstream is(myProfile);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_THROW_WHAT(config->validate(),
                              OCIO::Exception,
                              "The content of the env. variable for the list of active displays [ABCDEF] is invalid.");
    }

    {
        // Test an unknown display name using the env. variable.

        OCIO::Platform::Setenv(OCIO::OCIO_ACTIVE_DISPLAYS_ENVVAR, "sRGB_2, sRGB_1, ABCDEF");

        const std::string myProfile = 
            SIMPLE_PROFILE_HEADER
            +
            "active_displays: [sRGB_2, sRGB_1]\n"
            "active_views: []\n"
            + SIMPLE_PROFILE_FOOTER;

        std::istringstream is(myProfile);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_THROW_WHAT(config->validate(),
                              OCIO::Exception,
                              "The content of the env. variable for the list of active displays"\
                              " [sRGB_2, sRGB_1, ABCDEF] contains invalid display name(s).");
    }

    {
        // Test an unknown display name in the config active displays.

        OCIO::Platform::Unsetenv(OCIO::OCIO_ACTIVE_DISPLAYS_ENVVAR); // Remove the env. variable.

        const std::string myProfile = 
            SIMPLE_PROFILE_HEADER
            +
            "active_displays: [ABCDEF]\n"
            "active_views: []\n"
            + SIMPLE_PROFILE_FOOTER;

        std::istringstream is(myProfile);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_THROW_WHAT(config->validate(),
                              OCIO::Exception,
                              "The list of active displays [ABCDEF] from the config file is invalid.");
    }

    {
        // Test an unknown display name in the config active displays.

        OCIO::Platform::Unsetenv(OCIO::OCIO_ACTIVE_DISPLAYS_ENVVAR); // Remove the env. variable.

        const std::string myProfile = 
            SIMPLE_PROFILE_HEADER
            +
            "active_displays: [sRGB_2, sRGB_1, ABCDEF]\n"
            "active_views: []\n"
            + SIMPLE_PROFILE_FOOTER;

        std::istringstream is(myProfile);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_THROW_WHAT(config->validate(),
                              OCIO::Exception,
                              "The list of active displays [sRGB_2, sRGB_1, ABCDEF] "\
                              "from the config file contains invalid display name(s)");
    }
}

OCIO_ADD_TEST(Config, view)
{
    // Guard to automatically unset the env. variable.
    class Guard
    {
    public:
        Guard() = default;
        ~Guard()
        {
            OCIO::Platform::Unsetenv(OCIO::OCIO_ACTIVE_VIEWS_ENVVAR);
        }
    } guard;


    static const std::string SIMPLE_PROFILE_HEADER =
        "ocio_profile_version: 1\n"
        "\n"
        "search_path: luts\n"
        "strictparsing: true\n"
        "luma: [0.2126, 0.7152, 0.0722]\n"
        "\n"
        "roles:\n"
        "  default: raw\n"
        "  scene_linear: lnh\n"
        "\n"
        "displays:\n"
        "  sRGB_1:\n"
        "    - !<View> {name: View_1, colorspace: raw}\n"
        "    - !<View> {name: View_2, colorspace: raw}\n"
        "  sRGB_2:\n"
        "    - !<View> {name: View_2, colorspace: raw}\n"
        "    - !<View> {name: View_3, colorspace: raw}\n"
        "  sRGB_3:\n"
        "    - !<View> {name: View_3, colorspace: raw}\n"
        "    - !<View> {name: View_1, colorspace: raw}\n"
        "\n";

    static const std::string SIMPLE_PROFILE_FOOTER =
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: raw\n"
        "    family: \"\"\n"
        "    equalitygroup: \"\"\n"
        "    bitdepth: unknown\n"
        "    isdata: false\n"
        "    allocation: uniform\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: lnh\n"
        "    family: \"\"\n"
        "    equalitygroup: \"\"\n"
        "    bitdepth: unknown\n"
        "    isdata: false\n"
        "    allocation: uniform\n";

    {
        std::string myProfile =
            SIMPLE_PROFILE_HEADER
            +
            "active_displays: []\n"
            "active_views: []\n"
            + SIMPLE_PROFILE_FOOTER;

        std::istringstream is(myProfile);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_1")), "View_1");
        OCIO_REQUIRE_EQUAL(config->getNumViews("sRGB_1"), 2);
        OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_1", 0)), "View_1");
        OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_1", 1)), "View_2");
        // Invalid index.
        OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_1", 42)), "");

        OCIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_2")), "View_2");
        OCIO_REQUIRE_EQUAL(config->getNumViews("sRGB_2"), 2);
        OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_2", 0)), "View_2");
        OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_2", 1)), "View_3");
        OCIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_3")), "View_3");
        OCIO_REQUIRE_EQUAL(config->getNumViews("sRGB_3"), 2);
        OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_3", 0)), "View_3");
        OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_3", 1)), "View_1");

        std::stringstream ss;
        ss << *config.get();
        OCIO_CHECK_EQUAL(ss.str(), myProfile);
    }

    {
        std::string myProfile =
            SIMPLE_PROFILE_HEADER
            +
            "active_displays: []\n"
            "active_views: [View_3]\n"
            + SIMPLE_PROFILE_FOOTER;

        std::istringstream is(myProfile);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_1")), "View_1");
        OCIO_REQUIRE_EQUAL(config->getNumViews("sRGB_1"), 2);
        OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_1", 0)), "View_1");
        OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_1", 1)), "View_2");
        OCIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_2")), "View_3");
        OCIO_REQUIRE_EQUAL(config->getNumViews("sRGB_2"), 1);
        OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_2", 0)), "View_3");
        OCIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_3")), "View_3");
        OCIO_REQUIRE_EQUAL(config->getNumViews("sRGB_3"), 1);
        OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_3", 0)), "View_3");

        OCIO_REQUIRE_EQUAL(config->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, "sRGB_1"), 2);
        OCIO_REQUIRE_EQUAL(config->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, "sRGB_2"), 2);
        OCIO_REQUIRE_EQUAL(config->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, "sRGB_3"), 2);

        // Test that all views are saved.
        std::stringstream ss;
        ss << *config.get();
        OCIO_CHECK_EQUAL(ss.str(), myProfile);
    }

    {
        std::string myProfile = 
            SIMPLE_PROFILE_HEADER
            +
            "active_displays: []\n"
            "active_views: [View_3, View_2, View_1]\n"
            + SIMPLE_PROFILE_FOOTER;

        std::istringstream is(myProfile);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_1")), "View_2");
        OCIO_REQUIRE_EQUAL(config->getNumViews("sRGB_1"), 2);
        OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_1", 0)), "View_2");
        OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_1", 1)), "View_1");
        OCIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_2")), "View_3");
        OCIO_REQUIRE_EQUAL(config->getNumViews("sRGB_2"), 2);
        OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_2", 0)), "View_3");
        OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_2", 1)), "View_2");
        OCIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_3")), "View_3");
        OCIO_REQUIRE_EQUAL(config->getNumViews("sRGB_3"), 2);
        OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_3", 0)), "View_3");
        OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_3", 1)), "View_1");
    }

    {
        std::string myProfile = 
            SIMPLE_PROFILE_HEADER
            +
            "active_displays: []\n"
            "active_views: []\n"
            + SIMPLE_PROFILE_FOOTER;

        OCIO::Platform::Setenv(OCIO::OCIO_ACTIVE_VIEWS_ENVVAR, " View_3, View_2");

        std::istringstream is(myProfile);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_1")), "View_2");
        OCIO_REQUIRE_EQUAL(config->getNumViews("sRGB_1"), 1);
        OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_1", 0)), "View_2");
        OCIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_2")), "View_3");
        OCIO_REQUIRE_EQUAL(config->getNumViews("sRGB_2"), 2);
        OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_2", 0)), "View_3");
        OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_2", 1)), "View_2");
        OCIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_3")), "View_3");
        OCIO_REQUIRE_EQUAL(config->getNumViews("sRGB_3"), 1);
        OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_3", 0)), "View_3");
    }

    {
        std::string myProfile = 
            SIMPLE_PROFILE_HEADER
            +
            "active_displays: []\n"
            "active_views: []\n"
            + SIMPLE_PROFILE_FOOTER;

        OCIO::Platform::Setenv(OCIO::OCIO_ACTIVE_VIEWS_ENVVAR, ""); // No value.

        std::istringstream is(myProfile);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_1")), "View_1");
        OCIO_REQUIRE_EQUAL(config->getNumViews("sRGB_1"), 2);
        OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_1", 0)), "View_1");
        OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_1", 1)), "View_2");
        OCIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_2")), "View_2");
        OCIO_REQUIRE_EQUAL(config->getNumViews("sRGB_2"), 2);
        OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_2", 0)), "View_2");
        OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_2", 1)), "View_3");
        OCIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_3")), "View_3");
        OCIO_REQUIRE_EQUAL(config->getNumViews("sRGB_3"), 2);
        OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_3", 0)), "View_3");
        OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_3", 1)), "View_1");
    }

    {
        std::string myProfile = 
            SIMPLE_PROFILE_HEADER
            +
            "active_displays: []\n"
            "active_views: []\n"
            + SIMPLE_PROFILE_FOOTER;

        OCIO::Platform::Setenv(OCIO::OCIO_ACTIVE_VIEWS_ENVVAR, " "); // No value, but misleading space

        std::istringstream is(myProfile);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_1")), "View_1");
        OCIO_REQUIRE_EQUAL(config->getNumViews("sRGB_1"), 2);
        OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_1", 0)), "View_1");
        OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_1", 1)), "View_2");
        OCIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_2")), "View_2");
        OCIO_REQUIRE_EQUAL(config->getNumViews("sRGB_2"), 2);
        OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_2", 0)), "View_2");
        OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_2", 1)), "View_3");
        OCIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_3")), "View_3");
        OCIO_REQUIRE_EQUAL(config->getNumViews("sRGB_3"), 2);
        OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_3", 0)), "View_3");
        OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_3", 1)), "View_1");
    }
}

OCIO_ADD_TEST(Config, display_view_order)
{
    constexpr char SIMPLE_CONFIG[] { R"(
        ocio_profile_version: 2

        environment:
          {}

        displays:
          sRGB_B:
            - !<View> {name: View_2, colorspace: raw}
            - !<View> {name: View_1, colorspace: raw}
          sRGB_D:
            - !<View> {name: View_2, colorspace: raw}
            - !<View> {name: View_3, colorspace: raw}
          sRGB_A:
            - !<View> {name: View_3, colorspace: raw}
            - !<View> {name: View_1, colorspace: raw}
          sRGB_C:
            - !<View> {name: View_4, colorspace: raw}
            - !<View> {name: View_1, colorspace: raw}

        colorspaces:
          - !<ColorSpace>
            name: raw
            allocation: uniform

          - !<ColorSpace>
            name: lnh
            allocation: uniform

        file_rules:
          - !<Rule> {name: Default, colorspace: raw}
        )" };

    std::istringstream is(SIMPLE_CONFIG);
    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());

    OCIO_REQUIRE_EQUAL(config->getNumDisplays(), 4);

    // When active_displays is not defined, the displays are returned in config order.

    OCIO_CHECK_EQUAL(std::string(config->getDefaultDisplay()), "sRGB_B");

    OCIO_CHECK_EQUAL(std::string(config->getDisplay(0)), "sRGB_B");
    OCIO_CHECK_EQUAL(std::string(config->getDisplay(1)), "sRGB_D");
    OCIO_CHECK_EQUAL(std::string(config->getDisplay(2)), "sRGB_A");
    OCIO_CHECK_EQUAL(std::string(config->getDisplay(3)), "sRGB_C");

    // When active_views is not defined, the views are returned in config order.

    OCIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_B")), "View_2");

    OCIO_REQUIRE_EQUAL(config->getNumViews("sRGB_B"), 2);
    OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_B", 0)), "View_2");
    OCIO_CHECK_EQUAL(std::string(config->getView("sRGB_B", 1)), "View_1");
}

OCIO_ADD_TEST(Config, log_serialization)
{
    {
        // Log with default base value (saved in V1) and default direction.
        const std::string strEnd =
            "    from_reference: !<LogTransform> {base: 2}\n";
        const std::string str = PROFILE_V1 + SIMPLE_PROFILE_A + SIMPLE_PROFILE_B_V1 + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // Log with default base value (not saved in V2) and default direction.
        const std::string strEnd =
            "    from_scene_reference: !<LogTransform> {}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // Log with default base value.
        const std::string strEnd =
            "    from_reference: !<LogTransform> {base: 2, direction: inverse}\n";
        const std::string str = PROFILE_V1 + SIMPLE_PROFILE_A + SIMPLE_PROFILE_B_V1 + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // Log with default base value.
        const std::string strEnd =
            "    from_scene_reference: !<LogTransform> {direction: inverse}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // Log with specified base value.
        const std::string strEnd =
            "    from_reference: !<LogTransform> {base: 5}\n";
        const std::string str = PROFILE_V1 + SIMPLE_PROFILE_A + SIMPLE_PROFILE_B_V1 + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // Log with specified base value and direction.
        const std::string strEnd =
            "    from_reference: !<LogTransform> {base: 7, direction: inverse}\n";
        const std::string str = PROFILE_V1 + SIMPLE_PROFILE_A + SIMPLE_PROFILE_B_V1 + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // LogAffine with specified values 3 components.
        const std::string strEnd =
            "    from_scene_reference: !<LogAffineTransform> {"
            "base: 10, "
            "log_side_slope: [1.3, 1.4, 1.5], "
            "log_side_offset: [0, 0, 0.1], "
            "lin_side_slope: [1, 1, 1.1], "
            "lin_side_offset: [0.1234567890123, 0.5, 0.1]}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // LogAffine with default value for base.
        const std::string strEnd =
            "    from_scene_reference: !<LogAffineTransform> {"
            "log_side_slope: [1, 1, 1.1], "
            "log_side_offset: [0.1234567890123, 0.5, 0.1], "
            "lin_side_slope: [1.3, 1.4, 1.5], "
            "lin_side_offset: [0, 0, 0.1]}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // LogAffine with single value for lin_side_offset.
        const std::string strEnd =
            "    from_scene_reference: !<LogAffineTransform> {"
            "base: 10, "
            "log_side_slope: [1, 1, 1.1], "
            "log_side_offset: [0.1234567890123, 0.5, 0.1], "
            "lin_side_slope: [1.3, 1.4, 1.5], "
            "lin_side_offset: 0.5}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // LogAffine with single value for lin_side_slope.
        const std::string strEnd =
            "    from_scene_reference: !<LogAffineTransform> {"
            "log_side_slope: [1, 1, 1.1], "
            "lin_side_slope: 1.3, "
            "lin_side_offset: [0, 0, 0.1]}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // LogAffine with single value for log_side_offset.
        const std::string strEnd =
            "    from_scene_reference: !<LogAffineTransform> {"
            "log_side_slope: [1, 1, 1.1], "
            "log_side_offset: 0.5, "
            "lin_side_slope: [1.3, 1, 1], "
            "lin_side_offset: [0, 0, 0.1]}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // LogAffine with single value for log_side_slope.
        const std::string strEnd =
            "    from_scene_reference: !<LogAffineTransform> {"
            "log_side_slope: 1.1, "
            "log_side_offset: [0.5, 0, 0], "
            "lin_side_slope: [1.3, 1, 1], "
            "lin_side_offset: [0, 0, 0.1]}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // LogAffine with default value for log_side_slope.
        const std::string strEnd =
            "    from_scene_reference: !<LogAffineTransform> {"
            "log_side_offset: [0.1234567890123, 0.5, 0.1], "
            "lin_side_slope: [1.3, 1.4, 1.5], "
            "lin_side_offset: [0.1, 0, 0]}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // LogAffine with default value for all but base.
        const std::string strEnd =
            "    from_scene_reference: !<LogAffineTransform> {base: 10}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // LogAffine with wrong size for log_side_slope.
        const std::string strEnd =
            "    from_scene_reference: !<LogAffineTransform> {"
            "log_side_slope: [1, 1], "
            "log_side_offset: [0.1234567890123, 0.5, 0.1]}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_THROW_WHAT(config = OCIO::Config::CreateFromStream(is),
                              OCIO::Exception,
                              "log_side_slope value field must have 3 components");
    }

    {
        // LogAffine with 3 values for base.
        const std::string strEnd =
            "    from_scene_reference: !<LogAffineTransform> {"
            "base: [2, 2, 2], "
            "log_side_offset: [0.1234567890123, 0.5, 0.1]}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is),
                              OCIO::Exception,
                              "base must be a single double");
    }

    {
        // LogCamera with default value for base.
        const std::string strEnd =
            "    from_scene_reference: !<LogCameraTransform> {"
            "log_side_slope: [1, 1, 1.1], "
            "log_side_offset: [0.1234567890123, 0.5, 0.1], "
            "lin_side_slope: [1.3, 1.4, 1.5], "
            "lin_side_offset: [0, 0, 0.1], "
            "lin_side_break: [0.1, 0.2, 0.3]}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // LogCamera with default values and identical lin_side_break.
        const std::string strEnd =
            "    from_scene_reference: !<LogCameraTransform> {"
            "lin_side_break: 0.2}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // LogCamera with linear slope.
        const std::string strEnd =
            "    from_scene_reference: !<LogCameraTransform> {"
            "lin_side_break: 0.2, "
            "linear_slope: [1.1, 0.9, 1.2]}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // LogCamera with missing linSideBreak.
        const std::string strEnd =
            "    from_scene_reference: !<LogCameraTransform> {"
            "base: 5}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                              "lin_side_break values are missing");
    }
}

OCIO_ADD_TEST(Config, key_value_error)
{
    // Check the line number contained in the parser error messages.

    const std::string SHORT_PROFILE =
        "ocio_profile_version: 2\n"
        "strictparsing: false\n"
        "roles:\n"
        "  default: raw\n"
        "displays:\n"
        "  sRGB:\n"
        "  - !<View> {name: Raw, colorspace: raw}\n"
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: raw\n"
        "    to_scene_reference: !<MatrixTransform> \n"
        "                      {\n"
        "                           matrix: [1, 0, 0, 0, 0, 1]\n" // Missing values.
        "                      }\n"
        "    allocation: uniform\n"
        "\n";

    std::istringstream is;
    is.str(SHORT_PROFILE);

    OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is),
                          OCIO::Exception,
                          "Error: Loading the OCIO profile failed. At line 14, the value "
                          "parsing of the key 'matrix' from 'MatrixTransform' failed: "
                          "'matrix' values must be 16 numbers. Found '6'.");
}

OCIO_ADD_TEST(Config, unknown_key_error)
{
    std::ostringstream oss;
    oss << PROFILE_V2_START
        << "    dummyKey: dummyValue\n";

    std::istringstream is;
    is.str(oss.str());

    OCIO::LogGuard g;
    OCIO_CHECK_NO_THROW(OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_ASSERT(StringUtils::StartsWith(g.output(), 
                     "[OpenColorIO Warning]: At line 56, unknown key 'dummyKey' in 'ColorSpace'."));
}

OCIO_ADD_TEST(Config, grading_primary_serialization)
{
    {
        const std::string strEnd =
            "    from_scene_reference: !<GroupTransform>\n"
            "      children:\n"
            "        - !<GradingPrimaryTransform> {style: log}\n"
            "        - !<GradingPrimaryTransform> {style: log, contrast: {rgb: [1.1, 1, 1], master: 1.1}}\n"
            "        - !<GradingPrimaryTransform> {style: log, direction: inverse}\n"
            "        - !<GradingPrimaryTransform> {style: linear, saturation: 0.9}\n"
            "        - !<GradingPrimaryTransform> {style: linear, saturation: 1.1, direction: inverse}\n"
            "        - !<GradingPrimaryTransform> {name: test, style: video}\n"
            "        - !<GradingPrimaryTransform> {style: video, direction: inverse}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        // Write the config.

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());

        // Pivot contrast is always saved even if it is the default value (log & linear) when
        // contrast is not default. When controls are not default, transform is saved on separate
        // lines.
        const std::string strEndBack =
            "    from_scene_reference: !<GroupTransform>\n"
            "      children:\n"
            "        - !<GradingPrimaryTransform> {style: log}\n"
            "        - !<GradingPrimaryTransform>\n"
            "          style: log\n"
            "          contrast: {rgb: [1.1, 1, 1], master: 1.1}\n"
            "          pivot: {contrast: -0.2}\n"
            "        - !<GradingPrimaryTransform> {style: log, direction: inverse}\n"
            "        - !<GradingPrimaryTransform>\n"
            "          style: linear\n"
            "          saturation: 0.9\n"
            "        - !<GradingPrimaryTransform>\n"
            "          style: linear\n"
            "          saturation: 1.1\n"
            "          direction: inverse\n"
            "        - !<GradingPrimaryTransform> {name: test, style: video}\n"
            "        - !<GradingPrimaryTransform> {style: video, direction: inverse}\n";

        const std::string strBack = PROFILE_V2_START + strEndBack;
        OCIO_CHECK_EQUAL(ss.str(), strBack);
    }

    {
        // Pivot contrast value is included for log and linear even if it is the default value. 
        const std::string strEnd =
            "    from_scene_reference: !<GroupTransform>\n"
            "      children:\n"
            "        - !<GradingPrimaryTransform>\n"
            "          style: log\n"
            "          brightness: {rgb: [0.1, 0.12345678, 0], master: 0.1}\n"
            "        - !<GradingPrimaryTransform>\n"
            "          style: log\n"
            "          contrast: {rgb: [1.1, 1, 1], master: 1.1}\n"
            "          pivot: {contrast: -0.2}\n"
            "        - !<GradingPrimaryTransform>\n"
            "          style: log\n"
            "          gamma: {rgb: [1.1, 1.1, 1], master: 1.1}\n"
            "        - !<GradingPrimaryTransform>\n"
            "          style: log\n"
            "          saturation: 0.9\n"
            "        - !<GradingPrimaryTransform>\n"
            "          style: log\n"
            "          pivot: {contrast: -0.1, black: 0.1, white: 1.1}\n"
            "        - !<GradingPrimaryTransform>\n"
            "          style: log\n"
            "          pivot: {black: 0.1, white: 1.1}\n"
            "        - !<GradingPrimaryTransform>\n"
            "          style: log\n"
            "          pivot: {black: 0.1}\n"
            "        - !<GradingPrimaryTransform>\n"
            "          style: log\n"
            "          clamp: {black: 0.1, white: 1.1}\n"
            "        - !<GradingPrimaryTransform>\n"
            "          style: log\n"
            "          clamp: {black: 0.1}\n"
            "        - !<GradingPrimaryTransform>\n"
            "          style: linear\n"
            "          offset: {rgb: [0.1, 0.12345678, 0], master: 0.1}\n"
            "        - !<GradingPrimaryTransform>\n"
            "          style: linear\n"
            "          contrast: {rgb: [1.1, 1, 1], master: 1.1}\n"
            "          pivot: {contrast: 0.18}\n"
            "        - !<GradingPrimaryTransform>\n"
            "          style: linear\n"
            "          exposure: {rgb: [-1.1, 0.9, -0.01], master: 1.1}\n"
            "        - !<GradingPrimaryTransform>\n"
            "          style: linear\n"
            "          saturation: 0.9\n"
            "        - !<GradingPrimaryTransform>\n"
            "          style: linear\n"
            "          pivot: {contrast: -0.1}\n"
            "        - !<GradingPrimaryTransform>\n"
            "          style: linear\n"
            "          clamp: {black: 0.1, white: 1.1}\n"
            "        - !<GradingPrimaryTransform>\n"
            "          style: linear\n"
            "          clamp: {white: 1.1}\n"
            "        - !<GradingPrimaryTransform>\n"
            "          style: video\n"
            "          offset: {rgb: [0.1, 0.12345678, 0], master: 0.1}\n"
            "        - !<GradingPrimaryTransform>\n"
            "          style: video\n"
            "          gain: {rgb: [1.1, 1, 1], master: 1.1}\n"
            "        - !<GradingPrimaryTransform>\n"
            "          style: video\n"
            "          gamma: {rgb: [1.1, 1, 1], master: 1.1}\n"
            "        - !<GradingPrimaryTransform>\n"
            "          style: video\n"
            "          lift: {rgb: [0.1, 0.12345678, 0], master: 0.1}\n"
            "        - !<GradingPrimaryTransform>\n"
            "          style: video\n"
            "          pivot: {black: 0.1, white: 1.1}\n"
            "        - !<GradingPrimaryTransform>\n"
            "          style: video\n"
            "          pivot: {white: 1.1}\n"
            "        - !<GradingPrimaryTransform>\n"
            "          style: video\n"
            "          clamp: {black: 0.1, white: 1.1}\n"
            "        - !<GradingPrimaryTransform>\n"
            "          style: video\n"
            "          clamp: {black: 0.1}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        // Write the config.

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // Primary can be on one line or multiple lines (but is written on multiple lines).
        const std::string strEnd =
            "    from_scene_reference: !<GroupTransform>\n"
            "      children:\n"
            "        - !<GradingPrimaryTransform> {style: log, brightness: {rgb: [0.1, 0.12345678, 0], master: 0.1}, pivot: {contrast: -0.2}}\n"
            "        - !<GradingPrimaryTransform>\n"
            "          style: linear\n"
            "          offset:\n"
            "            rgb: [0.1, 0.12345678, 0]\n"
            "            master: 0.1\n"
            "          pivot: {contrast: 0.18}\n";

        const std::string strEndBack =
            "    from_scene_reference: !<GroupTransform>\n"
            "      children:\n"
            "        - !<GradingPrimaryTransform>\n"
            "          style: log\n"
            "          brightness: {rgb: [0.1, 0.12345678, 0], master: 0.1}\n"
            "        - !<GradingPrimaryTransform>\n"
            "          style: linear\n"
            "          offset: {rgb: [0.1, 0.12345678, 0], master: 0.1}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        // Write the config.

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());

        const std::string strBack = PROFILE_V2_START + strEndBack;

        OCIO_CHECK_EQUAL(ss.str(), strBack);
    }

    {
        // Rgb not enough values.
        const std::string strEnd =
            "    from_scene_reference: !<GradingPrimaryTransform> {style: log, brightness: {rgb: [0.1, 0], master: 0.1}}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                              "The RGB value needs to be a 3 doubles");
    }

    {
        // Rgb too many values.
        const std::string strEnd =
            "    from_scene_reference: !<GradingPrimaryTransform> {style: log, brightness: {rgb: [0.1, 0.12345678, 0, 0], master: 0.1}}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                              "The RGB value needs to be a 3 doubles");
    }

    {
        // Rgbm has to be a map.
        const std::string strEnd =
            "    from_scene_reference: !<GradingPrimaryTransform> "
            "{style: log, brightness: [0.1, 0.12345678, 0, 0]}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                              "'brightness' failed: The value needs to be a map");
    }

    {
        // Rgbm missing master.
        const std::string strEnd =
            "    from_scene_reference: !<GradingPrimaryTransform> "
            "{style: log, brightness: {rgb: [0.1, 0.12345678, 0]}}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                              "'brightness' failed: Both rgb and master values are required");
    }

    {
        // Rgbm master has too many values.
        const std::string strEnd =
            "    from_scene_reference: !<GradingPrimaryTransform> "
            "{style: log, brightness: {rgb: [0.1, 0.12345678, 0], master: [0.1, 0.2, 0.3]}}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                              "parsing double failed");
    }

    {
        // Rgbm missing rgb.
        const std::string strEnd =
            "    from_scene_reference: !<GradingPrimaryTransform> {style: log, brightness: {master: 0.1}}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                              "'brightness' failed: Both rgb and master values are required");
    }

    {
        // Pivot has to be a map.
        const std::string strEnd =
            "    from_scene_reference: !<GradingPrimaryTransform> {style: log, pivot: 0.1}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                              "'pivot' failed: The value needs to be a map");
    }

    {
        // Pivot has to define some values.
        const std::string strEnd =
            "    from_scene_reference: !<GradingPrimaryTransform> {style: log, pivot: {}}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                              "'pivot' failed: At least one of the pivot values must be provided");
    }

    {
        // Clamp has to be a map.
        const std::string strEnd =
            "    from_scene_reference: !<GradingPrimaryTransform> {style: log, clamp: 0.1}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                              "'clamp' failed: The value needs to be a map");
    }

    {
        // Clamp has to define some values.
        const std::string strEnd =
            "    from_scene_reference: !<GradingPrimaryTransform> {style: log, clamp: {}}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                              "'clamp' failed: At least one of the clamp values must be provided");
    }
}

OCIO_ADD_TEST(Config, grading_rgbcurve_serialization)
{
    {
        const std::string strEnd =
            "    from_scene_reference: !<GroupTransform>\n"
            "      children:\n"
            "        - !<GradingRGBCurveTransform> {style: log}\n"
            "        - !<GradingRGBCurveTransform> {style: log, direction: inverse}\n"
            "        - !<GradingRGBCurveTransform> {style: linear, lintolog_bypass: true}\n"
            "        - !<GradingRGBCurveTransform> {style: linear, direction: inverse}\n"
            "        - !<GradingRGBCurveTransform> {name: test, style: video}\n"
            "        - !<GradingRGBCurveTransform> {style: video, direction: inverse}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        // Write the config.

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        const std::string strEnd =
            "    from_scene_reference: !<GroupTransform>\n"
            "      children:\n"
            "        - !<GradingRGBCurveTransform>\n"
            "          style: log\n"
            "          red: {control_points: [0, 0, 0.5, 0.5, 1, 1.123456]}\n"
            "        - !<GradingRGBCurveTransform>\n"
            "          style: log\n"
            "          red: {control_points: [0, 0, 0.5, 0.5, 1, 1.5]}\n"
            "          green: {control_points: [-1, -1, 0, 0.1, 0.5, 0.6, 1, 1.1]}\n"
            "          direction: inverse\n"
            "        - !<GradingRGBCurveTransform>\n"
            "          style: linear\n"
            "          lintolog_bypass: true\n"
            "          red: {control_points: [0, 0, 0.1, 0.2, 0.5, 0.5, 0.7, 0.6, 1, 1.5]}\n"
            "          master: {control_points: [-1, -1, 0, 0.1, 0.5, 0.6, 1, 1.1]}\n"
            "        - !<GradingRGBCurveTransform>\n"
            "          style: video\n"
            "          red: {control_points: [-0.2, 0, 0.5, 0.5, 1.2, 1.5]}\n"
            "          green: {control_points: [0, 0, 0.2, 0.5, 1, 1.5]}\n"
            "          blue: {control_points: [0, 0, 0.1, 0.5, 1, 1.5], slopes: [0, 1, 1.1]}\n"
            "          master: {control_points: [-1, -1, 0, 0.1, 0.5, 0.6, 1, 1.1]}\n"
            "          direction: inverse\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        // Write the config.

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        const std::string strEnd =
            "    from_reference: !<GroupTransform>\n"
            "      children:\n"
            "        - !<GradingRGBCurveTransform>\n"
            "          style: log\n"
            "          blue: {control_points: [0, 0, 0.1, 0.5, 1, 1.5], slopes: [0, 1, 1.1, 1]}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                              "Number of slopes must match number of control points");
    }
}

OCIO_ADD_TEST(Config, grading_tone_serialization)
{
    {
        const std::string strEnd =
            "    from_scene_reference: !<GroupTransform>\n"
            "      children:\n"
            "        - !<GradingToneTransform> {style: log}\n"
            "        - !<GradingToneTransform> {style: log, s_contrast: 1.1}\n"
            "        - !<GradingToneTransform> {style: log, direction: inverse}\n"
            "        - !<GradingToneTransform> {style: linear}\n"
            "        - !<GradingToneTransform> {style: linear, direction: inverse}\n"
            "        - !<GradingToneTransform> {name: test, style: video}\n"
            "        - !<GradingToneTransform> {style: video, direction: inverse}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        // Write the config.

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());

        //  When controls are not default, transform is saved on separate lines.
        const std::string strEndBack =
            "    from_scene_reference: !<GroupTransform>\n"
            "      children:\n"
            "        - !<GradingToneTransform> {style: log}\n"
            "        - !<GradingToneTransform>\n"
            "          style: log\n"
            "          s_contrast: 1.1\n"
            "        - !<GradingToneTransform> {style: log, direction: inverse}\n"
            "        - !<GradingToneTransform> {style: linear}\n"
            "        - !<GradingToneTransform> {style: linear, direction: inverse}\n"
            "        - !<GradingToneTransform> {name: test, style: video}\n"
            "        - !<GradingToneTransform> {style: video, direction: inverse}\n";

        const std::string strBack = PROFILE_V2_START + strEndBack;
        OCIO_CHECK_EQUAL(ss.str(), strBack);
    }

    {
        const std::string strEnd =
            "    from_scene_reference: !<GroupTransform>\n"
            "      children:\n"
            "        - !<GradingToneTransform>\n"
            "          style: log\n"
            "          blacks: {rgb: [0.1, 0.12345678, 0.9], master: 1, start: 0.1, width: 0.9}\n"
            "          shadows: {rgb: [1, 1.1, 1.1111], master: 1.1, start: 0.9, pivot: 0.1}\n"
            "          midtones: {rgb: [0.85, 0.98, 1], master: 1.11, center: 0.1, width: 0.9}\n"
            "          highlights: {rgb: [1.1, 1.1111, 1], master: 1.2, start: 0.15, pivot: 1.1}\n"
            "          whites: {rgb: [0.95, 0.96, 0.95], master: 1.1, start: 0.1, width: 0.9}\n"
            "          s_contrast: 1.1\n"
            "        - !<GradingToneTransform>\n"
            "          style: log\n"
            "          midtones: {rgb: [0.85, 0.98, 1], master: 1.11, center: 0.1, width: 0.9}\n"
            "          highlights: {rgb: [1.1, 1.1111, 1], master: 1.2, start: 0.15, pivot: 1.1}\n"
            "          whites: {rgb: [0.95, 0.96, 0.95], master: 1.1, start: 0.1, width: 0.9}\n"
            "          s_contrast: 1.1\n"
            "        - !<GradingToneTransform>\n"
            "          style: linear\n"
            "          blacks: {rgb: [0.1, 0.12345678, 0.9], master: 1, start: 0.1, width: 0.9}\n"
            "          shadows: {rgb: [1, 1.1, 1.1111], master: 1.1, start: 0.9, pivot: 0.1}\n"
            "          whites: {rgb: [0.95, 0.96, 0.95], master: 1.1, start: 0.1, width: 0.9}\n"
            "          s_contrast: 1.1\n"
            "        - !<GradingToneTransform>\n"
            "          style: video\n"
            "          shadows: {rgb: [1, 1.1, 1.1111], master: 1.1, start: 0.9, pivot: 0.1}\n"
            "          midtones: {rgb: [0.85, 0.98, 1], master: 1.11, center: 0.1, width: 0.9}\n"
            "          highlights: {rgb: [1.1, 1.1111, 1], master: 1.2, start: 0.15, pivot: 1.1}\n"
            "          direction: inverse\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        // Write the config.

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // Rgb not enough values.
        const std::string strEnd =
            "    from_scene_reference: !<GradingToneTransform> {style: log, whites: {rgb: [0.1, 1], master: 1, start: 1, width: 1}}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                              "The RGB value needs to be a 3 doubles");
    }

    {
        // Rgb too many values.
        const std::string strEnd =
            "    from_scene_reference: !<GradingToneTransform> {style: log, whites: {rgb: [0.1, 0.12345678, 1, 1], master: 0.1, start: 1, width: 1}}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                              "The RGB value needs to be a 3 doubles");
    }

    {
        // Rgbm has to be a map.
        const std::string strEnd =
            "    from_scene_reference: !<GradingToneTransform> "
            "{style: log, whites: [0.1, 0.12345678, 0, 0]}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                             "'whites' failed: The value needs to be a map");
    }

    {
        // Rgbmsw missing start.
        const std::string strEnd =
            "    from_scene_reference: !<GradingToneTransform> "
            "{style: log, whites: {rgb: [0.1, 1, 1], master: 0.1, width: 1}}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                              "'whites' failed: Rgb, master, start, and width values are required");
    }

    {
        // Rgbmsw missing center.
        const std::string strEnd =
            "    from_scene_reference: !<GradingToneTransform> "
            "{style: log, midtones: {rgb: [0.1, 1, 1], master: 0.1, width: 1}}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                              "'midtones' failed: Rgb, master, center, and width values are "
                              "required");
    }

    {
        // Rgbmsw start has too many values.
        const std::string strEnd =
            "    from_scene_reference: !<GradingToneTransform> "
            "{style: log, whites: {rgb: [0.1, 1, 1], master: 0.1, start: [1, 1.1], width: 1}}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                              "parsing double failed");
    }
}

OCIO_ADD_TEST(Config, fixed_function_serialization)
{
    {
        const std::string strEnd =
            "    from_scene_reference: !<GroupTransform>\n"
            "      children:\n"
            "        - !<FixedFunctionTransform> {style: ACES_RedMod03}\n"
            "        - !<FixedFunctionTransform> {style: ACES_RedMod03, direction: inverse}\n"
            "        - !<FixedFunctionTransform> {style: ACES_RedMod10}\n"
            "        - !<FixedFunctionTransform> {style: ACES_RedMod10, direction: inverse}\n"
            "        - !<FixedFunctionTransform> {style: ACES_Glow03}\n"
            "        - !<FixedFunctionTransform> {style: ACES_Glow03, direction: inverse}\n"
            "        - !<FixedFunctionTransform> {style: ACES_Glow10}\n"
            "        - !<FixedFunctionTransform> {style: ACES_Glow10, direction: inverse}\n"
            "        - !<FixedFunctionTransform> {style: ACES_DarkToDim10}\n"
            "        - !<FixedFunctionTransform> {style: ACES_DarkToDim10, direction: inverse}\n"
            "        - !<FixedFunctionTransform> {style: REC2100_Surround, params: [0.75]}\n"
            "        - !<FixedFunctionTransform> {style: REC2100_Surround, params: [0.75], direction: inverse}\n"
            "        - !<FixedFunctionTransform> {style: RGB_TO_HSV}\n"
            "        - !<FixedFunctionTransform> {style: RGB_TO_HSV, direction: inverse}\n"
            "        - !<FixedFunctionTransform> {style: XYZ_TO_xyY}\n"
            "        - !<FixedFunctionTransform> {style: XYZ_TO_xyY, direction: inverse}\n"
            "        - !<FixedFunctionTransform> {style: XYZ_TO_uvY}\n"
            "        - !<FixedFunctionTransform> {style: XYZ_TO_uvY, direction: inverse}\n"
            "        - !<FixedFunctionTransform> {style: XYZ_TO_LUV}\n"
            "        - !<FixedFunctionTransform> {style: XYZ_TO_LUV, direction: inverse}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        // Write the config.

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        const std::string strEnd =
            "    from_scene_reference: !<GroupTransform>\n"
            "      children:\n"
            "        - !<FixedFunctionTransform> {style: ACES_DarkToDim10, params: [0.75]}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_THROW_WHAT(config->validate(), OCIO::Exception,
            "The style 'ACES_DarkToDim10 (Forward)' must have zero parameters but 1 found.");
    }

    {
        const std::string strEnd =
            "    from_scene_reference: !<GroupTransform>\n"
            "      children:\n"
            "        - !<FixedFunctionTransform> {style: REC2100_Surround, direction: inverse}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_THROW_WHAT(config->validate(), OCIO::Exception, 
            "The style 'REC2100_Surround (Inverse)' must "
                              "have one parameter but 0 found.");
    }

    {
        const std::string strEnd =
            "    from_scene_reference: !<GroupTransform>\n"
            "      children:\n"
            "        - !<FixedFunctionTransform> {direction: inverse}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_THROW_WHAT(config = OCIO::Config::CreateFromStream(is), OCIO::Exception,
                              "'FixedFunctionTransform' parsing failed: style value is missing.");
    }
}

OCIO_ADD_TEST(Config, exposure_contrast_serialization)
{
    {
        const std::string strEnd =
            "    from_scene_reference: !<GroupTransform>\n"
            "      children:\n"
            "        - !<ExposureContrastTransform> {style: video,"
                       " contrast: 0.5, gamma: 1.1, pivot: 0.18}\n"
            "        - !<ExposureContrastTransform> {style: video, exposure: 1.5,"
                       " gamma: 1.1, pivot: 0.18}\n"
            "        - !<ExposureContrastTransform> {style: video, exposure: 1.5,"
                       " contrast: 0.5, pivot: 0.18}\n"
            "        - !<ExposureContrastTransform> {style: video, exposure: 1.5,"
                       " contrast: 0.5, gamma: 1.1, pivot: 0.18}\n"
            "        - !<ExposureContrastTransform> {style: video,"
                       " exposure: 1.5, contrast: 0.5,"
                       " gamma: 1.1, pivot: 0.18}\n"
            "        - !<ExposureContrastTransform> {style: video, exposure: -1.4,"
                       " contrast: 0.6, gamma: 1.2, pivot: 0.2,"
                       " direction: inverse}\n"
            "        - !<ExposureContrastTransform> {style: log, exposure: 1.5,"
                       " contrast: 0.6, gamma: 1.2, pivot: 0.18}\n"
            "        - !<ExposureContrastTransform> {style: log, exposure: 1.5,"
                       " contrast: 0.5, gamma: 1.1, pivot: 0.18,"
                       " direction: inverse}\n"
            "        - !<ExposureContrastTransform> {style: log, exposure: 1.5,"
                       " contrast: 0.6, gamma: 1.2,"
                       " pivot: 0.18}\n"
            "        - !<ExposureContrastTransform> {style: linear, exposure: 1.5,"
                       " contrast: 0.5, gamma: 1.1, pivot: 0.18}\n"
            "        - !<ExposureContrastTransform> {style: linear, exposure: 1.5,"
                       " contrast: 0.5, gamma: 1.1, pivot: 0.18,"
                       " direction: inverse}\n"
            "        - !<ExposureContrastTransform> {style: linear, exposure: 1.5,"
                       " contrast: 0.5, gamma: 1.1,"
                       " pivot: 0.18}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);

        // For exposure contrast transforms, no value for exposure, contrast or gamma means dynamic.
        auto cs = config->getColorSpace("lnh");
        OCIO_REQUIRE_ASSERT(cs);
        auto cst = cs->getTransform(OCIO::COLORSPACE_DIR_FROM_REFERENCE);
        OCIO_REQUIRE_ASSERT(cst);
        auto grp = OCIO_DYNAMIC_POINTER_CAST<const OCIO::GroupTransform>(cst);
        OCIO_REQUIRE_ASSERT(grp);
        OCIO_REQUIRE_EQUAL(grp->getNumTransforms(), 12);
        OCIO::ConstTransformRcPtr t;
        OCIO_CHECK_NO_THROW(t = grp->getTransform(0));
        OCIO_REQUIRE_ASSERT(t);
        auto ec = OCIO_DYNAMIC_POINTER_CAST<const OCIO::ExposureContrastTransform>(t);
        OCIO_REQUIRE_ASSERT(ec);
        OCIO_CHECK_ASSERT(ec->isExposureDynamic());
        OCIO_CHECK_ASSERT(!ec->isContrastDynamic());
        OCIO_CHECK_ASSERT(!ec->isGammaDynamic());
        OCIO_CHECK_NO_THROW(t = grp->getTransform(1));
        OCIO_REQUIRE_ASSERT(t);
        ec = OCIO_DYNAMIC_POINTER_CAST<const OCIO::ExposureContrastTransform>(t);
        OCIO_REQUIRE_ASSERT(ec);
        OCIO_CHECK_ASSERT(!ec->isExposureDynamic());
        OCIO_CHECK_ASSERT(ec->isContrastDynamic());
        OCIO_CHECK_ASSERT(!ec->isGammaDynamic());
        OCIO_CHECK_NO_THROW(t = grp->getTransform(2));
        OCIO_REQUIRE_ASSERT(t);
        ec = OCIO_DYNAMIC_POINTER_CAST<const OCIO::ExposureContrastTransform>(t);
        OCIO_REQUIRE_ASSERT(ec);
        OCIO_CHECK_ASSERT(!ec->isExposureDynamic());
        OCIO_CHECK_ASSERT(!ec->isContrastDynamic());
        OCIO_CHECK_ASSERT(ec->isGammaDynamic());
    }

    {
        const std::string strEnd =
            "    from_scene_reference: !<GroupTransform>\n"
            "      children:\n"
            "        - !<ExposureContrastTransform> {style: wrong}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is),
                              OCIO::Exception,
                              "Unknown exposure contrast style");
    }
}

OCIO_ADD_TEST(Config, matrix_serialization)
{
    const std::string strEnd =
        "    from_reference: !<GroupTransform>\n"
        "      children:\n"
                 // Check the value serialization.
        "        - !<MatrixTransform> {matrix: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15],"\
                                     " offset: [-1, -2, -3, -4]}\n"
                 // Check the value precision.
        "        - !<MatrixTransform> {offset: [0.123456789876, 1.23456789876, 12.3456789876, 123.456789876]}\n"
        "        - !<MatrixTransform> {matrix: [0.123456789876, 1.23456789876, 12.3456789876, 123.456789876, "\
                                                "1234.56789876, 12345.6789876, 123456.789876, 1234567.89876, "\
                                                "0, 0, 1, 0, 0, 0, 0, 1]}\n";

    const std::string str = PROFILE_V1 + SIMPLE_PROFILE_A + SIMPLE_PROFILE_B_V1 + strEnd;

    std::istringstream is;
    is.str(str);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());

    std::stringstream ss;
    OCIO_CHECK_NO_THROW(ss << *config.get());
    OCIO_CHECK_EQUAL(ss.str(), str);
}

OCIO_ADD_TEST(Config, cdl_serialization)
{
    // Config v2.
    {
        const std::string strEnd =
            "    from_scene_reference: !<GroupTransform>\n"
            "      children:\n"
            "        - !<CDLTransform> {slope: [1, 2, 1]}\n"
            "        - !<CDLTransform> {offset: [0.1, 0.2, 0.1]}\n"
            "        - !<CDLTransform> {power: [1.1, 1.2, 1.1]}\n"
            "        - !<CDLTransform> {sat: 0.1, direction: inverse}\n"
            "        - !<CDLTransform> {slope: [2, 2, 3], offset: [0.2, 0.3, 0.1], power: [1.2, 1.1, 1], sat: 0.2, style: asc}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::ostringstream oss;
        OCIO_CHECK_NO_THROW(oss << *config.get());
        OCIO_CHECK_EQUAL(oss.str(), str);
    }

    // Config v1.
    {
        const std::string strEnd =
            "    from_reference: !<GroupTransform>\n"
            "      children:\n"
            "        - !<CDLTransform> {slope: [1, 2, 1]}\n"
            "        - !<CDLTransform> {offset: [0.1, 0.2, 0.1]}\n"
            "        - !<CDLTransform> {power: [1.1, 1.2, 1.1]}\n"
            "        - !<CDLTransform> {sat: 0.1}\n";

        const std::string str = PROFILE_V1 + SIMPLE_PROFILE_A + SIMPLE_PROFILE_B_V1 + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::ostringstream oss;
        OCIO_CHECK_NO_THROW(oss << *config.get());
        OCIO_CHECK_EQUAL(oss.str(), str);
    }
}

OCIO_ADD_TEST(Config, file_transform_serialization)
{
    // Config v2.
    const std::string strEnd =
        "    from_scene_reference: !<GroupTransform>\n"
        "      children:\n"
        "        - !<FileTransform> {src: a.clf}\n"
        "        - !<FileTransform> {src: b.ccc, cccid: cdl1, interpolation: best}\n"
        "        - !<FileTransform> {src: b.ccc, cccid: cdl2, cdl_style: asc, interpolation: linear}\n"
        "        - !<FileTransform> {src: a.clf, direction: inverse}\n";

    const std::string str = PROFILE_V2_START + strEnd;

    std::istringstream is;
    is.str(str);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());

    std::ostringstream oss;
    OCIO_CHECK_NO_THROW(oss << *config.get());
    OCIO_CHECK_EQUAL(oss.str(), str);
}

OCIO_ADD_TEST(Config, file_transform_serialization_v1)
{
    OCIO::ConfigRcPtr cfg;
    OCIO_CHECK_NO_THROW(cfg = OCIO::Config::Create());
    OCIO_REQUIRE_ASSERT(cfg);
    cfg->setMajorVersion(1);
    auto ft = OCIO::FileTransform::Create();
    ft->setSrc("file");
    auto cs = OCIO::ColorSpace::Create();
    // Note that ft has no interpolation set.  In a v2 config, this is not a problem and is taken
    // to mean default interpolation.  However, in this case the config version is 1 and if the
    // config were read by a v1 library (rather than v2), this could cause a failure.  So the
    // interp is set to linear during serialization to avoid problems.
    cs->setTransform(ft, OCIO::COLORSPACE_DIR_TO_REFERENCE);
    ft->setSrc("other");
    ft->setInterpolation(OCIO::INTERP_TETRAHEDRAL);
    cs->setTransform(ft, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
    cs->setName("cs");
    cfg->addColorSpace(cs);
    std::ostringstream os;
    cfg->serialize(os);
    OCIO_CHECK_EQUAL(os.str(), R"(ocio_profile_version: 1

search_path: ""
strictparsing: true
luma: [0.2126, 0.7152, 0.0722]

roles:
  {}

displays:
  {}

active_displays: []
active_views: []

colorspaces:
  - !<ColorSpace>
    name: cs
    family: ""
    equalitygroup: ""
    bitdepth: unknown
    isdata: false
    allocation: uniform
    to_reference: !<FileTransform> {src: file, interpolation: linear}
    from_reference: !<FileTransform> {src: other, interpolation: tetrahedral}
)" );
}

OCIO_ADD_TEST(Config, add_color_space)
{
    // The unit test validates that the color space is correctly added to the configuration.

    // Note that the new C++11 u8 notation for UTF-8 string literals is used
    // to partially validate non-english language support.

    const std::string str
        = PROFILE_V2_START
            + u8"    from_scene_reference: !<MatrixTransform> {offset: [-1, -2, -3, -4]}\n";

    std::istringstream is;
    is.str(str);

    OCIO::ConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is)->createEditableCopy());
    OCIO_CHECK_NO_THROW(config->validate());
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 3);

    OCIO::ColorSpaceRcPtr cs;
    OCIO_CHECK_NO_THROW(cs = OCIO::ColorSpace::Create());
    cs->setName(u8"astéroïde");                           // Color space name with accents.
    cs->setDescription(u8"é À Â Ç É È ç -- $ € 円 £ 元"); // Some accents and some money symbols.

    OCIO::FixedFunctionTransformRcPtr tr;
    OCIO_CHECK_NO_THROW(tr = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_RED_MOD_03));

    OCIO_CHECK_NO_THROW(cs->setTransform(tr, OCIO::COLORSPACE_DIR_TO_REFERENCE));

    constexpr char csName[] = u8"astéroïde";

    OCIO_CHECK_EQUAL(config->getIndexForColorSpace(csName), -1);
    OCIO_CHECK_NO_THROW(config->addColorSpace(cs));
    OCIO_CHECK_EQUAL(config->getIndexForColorSpace(csName), 3);

    const std::string res 
        = str
        + u8"\n"
        + u8"  - !<ColorSpace>\n"
        + u8"    name: " + csName + u8"\n"
        + u8"    family: \"\"\n"
        + u8"    equalitygroup: \"\"\n"
        + u8"    bitdepth: unknown\n"
        + u8"    description: é À Â Ç É È ç -- $ € 円 £ 元\n"
        + u8"    isdata: false\n"
        + u8"    allocation: uniform\n"
        + u8"    to_scene_reference: !<FixedFunctionTransform> {style: ACES_RedMod03}\n";

    std::stringstream ss;
    OCIO_CHECK_NO_THROW(ss << *config.get());
    OCIO_CHECK_EQUAL(ss.str(), res);

    OCIO_CHECK_NO_THROW(config->removeColorSpace(csName));
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 3);
    OCIO_CHECK_EQUAL(config->getIndexForColorSpace(csName), -1);

    OCIO_CHECK_NO_THROW(config->clearColorSpaces());
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 0);
}

OCIO_ADD_TEST(Config, faulty_config_file)
{
    std::istringstream is("/usr/tmp/not_existing.ocio");

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_THROW_WHAT(config = OCIO::Config::CreateFromStream(is),
                          OCIO::Exception,
                          "Error: Loading the OCIO profile failed.");
}

OCIO_ADD_TEST(Config, remove_color_space)
{
    // The unit test validates that a color space is correctly removed from a configuration.

    const std::string str
        = PROFILE_V2_START
            + "    from_scene_reference: !<MatrixTransform> {offset: [-1, -2, -3, -4]}\n"
            + "\n"
            + "  - !<ColorSpace>\n"
            + "    name: cs5\n"
            + "    allocation: uniform\n"
            + "    to_scene_reference: !<FixedFunctionTransform> {style: ACES_RedMod03}\n";

    std::istringstream is;
    is.str(str);

    OCIO::ConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is)->createEditableCopy());
    OCIO_CHECK_NO_THROW(config->validate());
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 4);

    // Step 1 - Validate the remove.

    OCIO_CHECK_EQUAL(config->getIndexForColorSpace("cs5"), 3);
    OCIO_CHECK_NO_THROW(config->removeColorSpace("cs5"));
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 3);
    OCIO_CHECK_EQUAL(config->getIndexForColorSpace("cs5"), -1);

    // Step 2 - Validate some faulty removes.

    // As documented, removing a color space that doesn't exist fails without any notice.
    OCIO_CHECK_NO_THROW(config->removeColorSpace("cs5"));
    OCIO_CHECK_NO_THROW(config->validate());

    // Since the method does not support role names, a role name removal fails 
    // without any notice except if it's also an existing color space.
    OCIO_CHECK_NO_THROW(config->removeColorSpace("scene_linear"));
    OCIO_CHECK_NO_THROW(config->validate());

    // Successfully remove a color space unfortunately used by a role.
    OCIO_CHECK_NO_THROW(config->removeColorSpace("raw"));
    // As discussed only validation traps the issue.
    OCIO_CHECK_THROW_WHAT(config->validate(),
                          OCIO::Exception,
                          "Config failed validation. The role 'default' refers to"\
                          " a color space, 'raw', which is not defined.");
}

namespace
{

constexpr char InactiveCSConfigStart[] =
    "ocio_profile_version: 2\n"
    "\n"
    "environment:\n"
    "  {}\n"
    "search_path: luts\n"
    "strictparsing: true\n"
    "luma: [0.2126, 0.7152, 0.0722]\n"
    "\n"
    "roles:\n"
    "  default: raw\n"
    "  scene_linear: lnh\n"
    "\n"
    "file_rules:\n"
    "  - !<Rule> {name: Default, colorspace: default}\n"
    "\n"
    "displays:\n"
    "  sRGB:\n"
    "    - !<View> {name: Raw, colorspace: raw}\n"
    "    - !<View> {name: Lnh, colorspace: lnh, looks: beauty}\n"
    "\n"
    "active_displays: []\n"
    "active_views: []\n";

constexpr char InactiveCSConfigEnd[] =
    "\n"
    "looks:\n"
    "  - !<Look>\n"
    "    name: beauty\n"
    "    process_space: lnh\n"
    "    transform: !<CDLTransform> {slope: [1, 2, 1]}\n"
    "\n"
    "\n"
    "colorspaces:\n"
    "  - !<ColorSpace>\n"
    "    name: raw\n"
    "    family: \"\"\n"
    "    equalitygroup: \"\"\n"
    "    bitdepth: unknown\n"
    "    isdata: false\n"
    "    allocation: uniform\n"
    "\n"
    "  - !<ColorSpace>\n"
    "    name: lnh\n"
    "    family: \"\"\n"
    "    equalitygroup: \"\"\n"
    "    bitdepth: unknown\n"
    "    isdata: false\n"
    "    allocation: uniform\n"
    "\n"
    "  - !<ColorSpace>\n"
    "    name: cs1\n"
    "    aliases: [alias1]\n"
    "    family: \"\"\n"
    "    equalitygroup: \"\"\n"
    "    bitdepth: unknown\n"
    "    isdata: false\n"
    "    categories: [file-io]\n"
    "    allocation: uniform\n"
    "    from_scene_reference: !<CDLTransform> {offset: [0.1, 0.1, 0.1]}\n"
    "\n"
    "  - !<ColorSpace>\n"
    "    name: cs2\n"
    "    family: \"\"\n"
    "    equalitygroup: \"\"\n"
    "    bitdepth: unknown\n"
    "    isdata: false\n"
    "    categories: [working-space]\n"
    "    allocation: uniform\n"
    "    from_scene_reference: !<CDLTransform> {offset: [0.2, 0.2, 0.2]}\n"
    "\n"
    "  - !<ColorSpace>\n"
    "    name: cs3\n"
    "    family: \"\"\n"
    "    equalitygroup: \"\"\n"
    "    bitdepth: unknown\n"
    "    isdata: false\n"
    "    categories: [cat3]\n"
    "    allocation: uniform\n"
    "    from_scene_reference: !<CDLTransform> {offset: [0.3, 0.3, 0.3]}\n";

class InactiveCSGuard
{
public:
    InactiveCSGuard()
    {
        OCIO::Platform::Setenv(OCIO::OCIO_INACTIVE_COLORSPACES_ENVVAR, "cs3, cs1, lnh");
    }
    ~InactiveCSGuard()
    {
        OCIO::Platform::Unsetenv(OCIO::OCIO_INACTIVE_COLORSPACES_ENVVAR);
    }
};

} // anon.

OCIO_ADD_TEST(Config, inactive_color_space)
{
    // The unit test validates the inactive color space behavior.

    std::string configStr;
    configStr += InactiveCSConfigStart;
    configStr += InactiveCSConfigEnd;

    std::istringstream is;
    is.str(configStr);

    OCIO::ConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is)->createEditableCopy());
    OCIO_REQUIRE_ASSERT(config);
    OCIO_CHECK_NO_THROW(config->validate());


    // Step 1 - No inactive color spaces.

    OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                 OCIO::COLORSPACE_INACTIVE), 0);
    OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                 OCIO::COLORSPACE_ACTIVE), 5);
    OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                 OCIO::COLORSPACE_ALL), 5);

    OCIO_CHECK_EQUAL(std::string("raw"),
                     config->getColorSpaceNameByIndex(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                      OCIO::COLORSPACE_ALL, 0));
    OCIO_CHECK_EQUAL(std::string("lnh"),
                     config->getColorSpaceNameByIndex(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                      OCIO::COLORSPACE_ALL, 1));
    OCIO_CHECK_EQUAL(std::string("cs1"),
                     config->getColorSpaceNameByIndex(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                      OCIO::COLORSPACE_ALL, 2));
    OCIO_CHECK_EQUAL(std::string("cs2"),
                     config->getColorSpaceNameByIndex(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                      OCIO::COLORSPACE_ALL, 3));
    OCIO_CHECK_EQUAL(std::string("cs3"),
                     config->getColorSpaceNameByIndex(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                      OCIO::COLORSPACE_ALL, 4));
    // Check a faulty call.
    OCIO_CHECK_EQUAL(std::string(""),
                     config->getColorSpaceNameByIndex(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                      OCIO::COLORSPACE_ALL, 5));

    OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(), 5);
    OCIO_CHECK_EQUAL(std::string("raw"), config->getColorSpaceNameByIndex(0));
    OCIO_CHECK_EQUAL(std::string("lnh"), config->getColorSpaceNameByIndex(1));
    OCIO_CHECK_EQUAL(std::string("cs1"), config->getColorSpaceNameByIndex(2));
    OCIO_CHECK_EQUAL(std::string("cs2"), config->getColorSpaceNameByIndex(3));
    OCIO_CHECK_EQUAL(std::string("cs3"), config->getColorSpaceNameByIndex(4));
    // Check a faulty call.
    OCIO_CHECK_EQUAL(std::string(""), config->getColorSpaceNameByIndex(5));

    OCIO::ColorSpaceSetRcPtr css;
    OCIO_CHECK_NO_THROW(css = config->getColorSpaces(nullptr));
    OCIO_CHECK_EQUAL(css->getNumColorSpaces(), 5);

    OCIO::ConstColorSpaceRcPtr cs;
    OCIO_CHECK_NO_THROW(cs = config->getColorSpace("scene_linear"));
    OCIO_REQUIRE_ASSERT(cs);
    OCIO_CHECK_EQUAL(std::string("lnh"), cs->getName());

    OCIO_CHECK_EQUAL(config->getIndexForColorSpace("scene_linear"), 1);
    OCIO_CHECK_EQUAL(config->getIndexForColorSpace("lnh"), 1);

    // Step 2 - Some inactive color spaces.

    OCIO_CHECK_NO_THROW(config->setInactiveColorSpaces("lnh, cs1"));
    OCIO_CHECK_EQUAL(config->getInactiveColorSpaces(), std::string("lnh, cs1"));

    OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                 OCIO::COLORSPACE_INACTIVE), 2);
    OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                 OCIO::COLORSPACE_ACTIVE), 3);
    OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                 OCIO::COLORSPACE_ALL), 5);

    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_SCENE,
                                               OCIO::COLORSPACE_INACTIVE), 2);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_SCENE,
                                               OCIO::COLORSPACE_ACTIVE), 3);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_SCENE,
                                               OCIO::COLORSPACE_ALL), 5);

    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_DISPLAY,
                                               OCIO::COLORSPACE_INACTIVE), 0);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_DISPLAY,
                                               OCIO::COLORSPACE_ACTIVE), 0);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_DISPLAY,
                                               OCIO::COLORSPACE_ALL), 0);

    // Check methods working on all color spaces.
    OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, 
                                                 OCIO::COLORSPACE_ALL), 5);
    OCIO_CHECK_EQUAL(std::string("raw"),
                     config->getColorSpaceNameByIndex(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                      OCIO::COLORSPACE_ALL, 0));
    OCIO_CHECK_EQUAL(std::string("lnh"),
                     config->getColorSpaceNameByIndex(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                      OCIO::COLORSPACE_ALL, 1));
    OCIO_CHECK_EQUAL(std::string("cs1"),
                     config->getColorSpaceNameByIndex(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                      OCIO::COLORSPACE_ALL, 2));
    OCIO_CHECK_EQUAL(std::string("cs2"),
                     config->getColorSpaceNameByIndex(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                      OCIO::COLORSPACE_ALL, 3));
    OCIO_CHECK_EQUAL(std::string("cs3"),
                     config->getColorSpaceNameByIndex(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                      OCIO::COLORSPACE_ALL, 4));

    // Check methods working on only active color spaces.
    OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(), 3);
    OCIO_CHECK_EQUAL(std::string("raw"), config->getColorSpaceNameByIndex(0));
    OCIO_CHECK_EQUAL(std::string("cs2"), config->getColorSpaceNameByIndex(1));
    OCIO_CHECK_EQUAL(std::string("cs3"), config->getColorSpaceNameByIndex(2));

    // Asking for a color space set with no categories returns active color spaces only.
    OCIO_CHECK_NO_THROW(css = config->getColorSpaces(nullptr));
    OCIO_CHECK_EQUAL(css->getNumColorSpaces(), 3);

    // Search using a category 'file-io' with no active color space.
    OCIO_CHECK_NO_THROW(css = config->getColorSpaces("file-io"));
    OCIO_CHECK_EQUAL(css->getNumColorSpaces(), 0);

    // Search using a category 'working-space' with some active color spaces.
    OCIO_CHECK_NO_THROW(css = config->getColorSpaces("working-space"));
    OCIO_CHECK_EQUAL(css->getNumColorSpaces(), 1);

    // Request an active color space.
    OCIO_CHECK_NO_THROW(cs = config->getColorSpace("cs2"));
    OCIO_CHECK_ASSERT(cs);
    OCIO_CHECK_EQUAL(std::string("cs2"), cs->getName());

    // Request an inactive color space.
    OCIO_CHECK_NO_THROW(cs = config->getColorSpace("cs1"));
    OCIO_CHECK_ASSERT(cs);
    OCIO_CHECK_EQUAL(std::string("cs1"), cs->getName());

    // Request a role with an active color space.
    OCIO_CHECK_NO_THROW(cs = config->getColorSpace("default"));
    OCIO_REQUIRE_ASSERT(cs);
    OCIO_CHECK_EQUAL(std::string("raw"), cs->getName());

    // Request a role with an inactive color space.
    OCIO_CHECK_NO_THROW(cs = config->getColorSpace("scene_linear"));
    OCIO_CHECK_ASSERT(cs);
    OCIO_CHECK_EQUAL(std::string("lnh"), cs->getName());
    // ... the color is not an active color space.
    OCIO_CHECK_EQUAL(config->getIndexForColorSpace("scene_linear"), -1);
    OCIO_CHECK_EQUAL(config->getIndexForColorSpace("lnh"), -1);

    // Request a (display, view) processor with an inactive color space and
    // a look with an inactive process space.
    {
        OCIO::LookTransformRcPtr lookTransform = OCIO::LookTransform::Create();
        lookTransform->setLooks("beauty"); // Process space (i.e. lnh) inactive.
        lookTransform->setSrc("raw");

        const char * csName = config->getDisplayViewColorSpaceName("sRGB", "Lnh");
        lookTransform->setDst(csName); // Color space inactive (i.e. lnh).

        OCIO_CHECK_NO_THROW(config->getProcessor(lookTransform, OCIO::TRANSFORM_DIR_FORWARD));
    }

    // Check a faulty call.
    OCIO_CHECK_EQUAL(config->getColorSpaceNameByIndex(3), std::string(""));
    // ... but getColorSpace() must still succeed.
    OCIO_CHECK_NO_THROW(cs = config->getColorSpace("cs1"));
    OCIO_CHECK_ASSERT(cs);

    // Create a processor with one or more inactive color spaces.
    OCIO_CHECK_NO_THROW(config->getProcessor("lnh", "cs1"));
    OCIO_CHECK_NO_THROW(config->getProcessor("raw", "cs1"));
    OCIO_CHECK_NO_THROW(config->getProcessor("lnh", "cs2"));
    OCIO_CHECK_NO_THROW(config->getProcessor("cs2", "scene_linear"));

    // Step 3 - Same as 2, but using role name.
    
    // Setting a role to an inactive space is actually setting the space that it points to as
    // being inactive.  In this case, scene_linear is lnh.

    OCIO_CHECK_NO_THROW(config->setInactiveColorSpaces("scene_linear, cs1"));
    OCIO_CHECK_EQUAL(config->getInactiveColorSpaces(), std::string("scene_linear, cs1"));

    OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                 OCIO::COLORSPACE_INACTIVE), 2);
    OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                 OCIO::COLORSPACE_ACTIVE), 3);
    OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                 OCIO::COLORSPACE_ALL), 5);

    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_SCENE,
                                               OCIO::COLORSPACE_INACTIVE), 2);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_SCENE,
                                               OCIO::COLORSPACE_ACTIVE), 3);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_SCENE,
                                               OCIO::COLORSPACE_ALL), 5);

    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_DISPLAY,
                                               OCIO::COLORSPACE_INACTIVE), 0);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_DISPLAY,
                                               OCIO::COLORSPACE_ACTIVE), 0);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_DISPLAY,
                                               OCIO::COLORSPACE_ALL), 0);

    // Check methods working on only active color spaces.
    OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(), 3);
    OCIO_CHECK_EQUAL(std::string("raw"), config->getColorSpaceNameByIndex(0));
    OCIO_CHECK_EQUAL(std::string("cs2"), config->getColorSpaceNameByIndex(1));
    OCIO_CHECK_EQUAL(std::string("cs3"), config->getColorSpaceNameByIndex(2));

    OCIO_CHECK_ASSERT(config->hasRole("scene_linear"));

    // Step 4 - Same as 2, but using an alias.

    // Setting an alias to an inactive space is actually setting the space that it refers to as
    // being inactive.  In this case, alias1 is cs1.

    OCIO_CHECK_NO_THROW(config->setInactiveColorSpaces("lnh, alias1"));
    OCIO_CHECK_EQUAL(config->getInactiveColorSpaces(), std::string("lnh, alias1"));

    OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                 OCIO::COLORSPACE_INACTIVE), 2);
    OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                 OCIO::COLORSPACE_ACTIVE), 3);
    OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                 OCIO::COLORSPACE_ALL), 5);

    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_SCENE,
                                               OCIO::COLORSPACE_INACTIVE), 2);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_SCENE,
                                               OCIO::COLORSPACE_ACTIVE), 3);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_SCENE,
                                               OCIO::COLORSPACE_ALL), 5);

    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_DISPLAY,
                                               OCIO::COLORSPACE_INACTIVE), 0);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_DISPLAY,
                                               OCIO::COLORSPACE_ACTIVE), 0);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_DISPLAY,
                                               OCIO::COLORSPACE_ALL), 0);

    // Check methods working on only active color spaces.
    OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(), 3);
    OCIO_CHECK_EQUAL(std::string("raw"), config->getColorSpaceNameByIndex(0));
    OCIO_CHECK_EQUAL(std::string("cs2"), config->getColorSpaceNameByIndex(1));
    OCIO_CHECK_EQUAL(std::string("cs3"), config->getColorSpaceNameByIndex(2));

    // Step 5 - No inactive color spaces.

    OCIO_CHECK_NO_THROW(config->setInactiveColorSpaces(""));
    OCIO_CHECK_EQUAL(config->getInactiveColorSpaces(), std::string(""));

    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                               OCIO::COLORSPACE_ALL), 5);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 5);

    // Step 6 - No inactive color spaces.

    OCIO_CHECK_NO_THROW(config->setInactiveColorSpaces(nullptr));
    OCIO_CHECK_EQUAL(config->getInactiveColorSpaces(), std::string(""));

    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                               OCIO::COLORSPACE_ALL), 5);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 5);

    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_SCENE,
                                               OCIO::COLORSPACE_ALL), 5);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_DISPLAY,
                                               OCIO::COLORSPACE_ALL), 0);

    // Step 7 - Add display color spaces.

    auto dcs0 = OCIO::ColorSpace::Create(OCIO::REFERENCE_SPACE_DISPLAY);
    dcs0->setName("display0");
    config->addColorSpace(dcs0);
    auto dcs1 = OCIO::ColorSpace::Create(OCIO::REFERENCE_SPACE_DISPLAY);
    dcs1->setName("display1");
    config->addColorSpace(dcs1);
    auto dcs2 = OCIO::ColorSpace::Create(OCIO::REFERENCE_SPACE_DISPLAY);
    dcs2->setName("display2");
    config->addColorSpace(dcs2);

    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                               OCIO::COLORSPACE_ALL), 8);

    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_SCENE,
                                               OCIO::COLORSPACE_ALL), 5);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_DISPLAY,
                                               OCIO::COLORSPACE_ALL), 3);

    // Step 8 - Some inactive color spaces.

    OCIO_CHECK_NO_THROW(config->setInactiveColorSpaces("cs1, display1"));
    OCIO_CHECK_EQUAL(config->getInactiveColorSpaces(), std::string("cs1, display1"));

    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_SCENE,
                                               OCIO::COLORSPACE_INACTIVE), 1);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_DISPLAY,
                                               OCIO::COLORSPACE_INACTIVE), 1);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                               OCIO::COLORSPACE_INACTIVE), 2);
    OCIO_CHECK_EQUAL(std::string("cs1"),
                     config->getColorSpaceNameByIndex(OCIO::SEARCH_REFERENCE_SPACE_SCENE,
                                                      OCIO::COLORSPACE_INACTIVE, 0));
    OCIO_CHECK_EQUAL(std::string("display1"),
                     config->getColorSpaceNameByIndex(OCIO::SEARCH_REFERENCE_SPACE_DISPLAY,
                                                      OCIO::COLORSPACE_INACTIVE, 0));
    OCIO_CHECK_EQUAL(std::string(""),
                     config->getColorSpaceNameByIndex(OCIO::SEARCH_REFERENCE_SPACE_SCENE,
                                                      OCIO::COLORSPACE_INACTIVE, 1));
    OCIO_CHECK_EQUAL(std::string(""),
                     config->getColorSpaceNameByIndex(OCIO::SEARCH_REFERENCE_SPACE_DISPLAY,
                                                      OCIO::COLORSPACE_INACTIVE, 1));

    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_SCENE,
                                               OCIO::COLORSPACE_ACTIVE), 4);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_DISPLAY,
                                               OCIO::COLORSPACE_ACTIVE), 2);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                               OCIO::COLORSPACE_ACTIVE), 6);
    OCIO_CHECK_EQUAL(std::string("cs2"),
                     config->getColorSpaceNameByIndex(OCIO::SEARCH_REFERENCE_SPACE_SCENE,
                                                      OCIO::COLORSPACE_ACTIVE, 2));
    OCIO_CHECK_EQUAL(std::string("display2"),
                     config->getColorSpaceNameByIndex(OCIO::SEARCH_REFERENCE_SPACE_DISPLAY,
                                                      OCIO::COLORSPACE_ACTIVE, 1));

    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_SCENE,
                                               OCIO::COLORSPACE_ALL), 5);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_DISPLAY,
                                               OCIO::COLORSPACE_ALL), 3);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                               OCIO::COLORSPACE_ALL), 8);
    OCIO_CHECK_EQUAL(std::string("raw"),
                     config->getColorSpaceNameByIndex(OCIO::SEARCH_REFERENCE_SPACE_SCENE,
                                                      OCIO::COLORSPACE_ALL, 0));
    OCIO_CHECK_EQUAL(std::string("cs2"),
                     config->getColorSpaceNameByIndex(OCIO::SEARCH_REFERENCE_SPACE_SCENE,
                                                      OCIO::COLORSPACE_ALL, 3));
    OCIO_CHECK_EQUAL(std::string(""),
                     config->getColorSpaceNameByIndex(OCIO::SEARCH_REFERENCE_SPACE_SCENE,
                                                      OCIO::COLORSPACE_ALL, 10));
    OCIO_CHECK_EQUAL(std::string("display1"),
                     config->getColorSpaceNameByIndex(OCIO::SEARCH_REFERENCE_SPACE_DISPLAY,
                                                      OCIO::COLORSPACE_ALL, 1));
}

OCIO_ADD_TEST(Config, inactive_color_space_precedence)
{
    // The test demonstrates that an API request supersedes the env. variable and the
    // config file contents.

    std::string configStr;
    configStr += InactiveCSConfigStart;
    configStr += "inactive_colorspaces: [cs2]\n";
    configStr += InactiveCSConfigEnd;

    std::istringstream is;
    is.str(configStr);

    OCIO::ConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is)->createEditableCopy());
    OCIO_CHECK_NO_THROW(config->validate());

    OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                 OCIO::COLORSPACE_INACTIVE), 1);
    OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                 OCIO::COLORSPACE_ACTIVE), 4);
    OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                 OCIO::COLORSPACE_ALL), 5);

    OCIO_CHECK_EQUAL(config->getColorSpaceNameByIndex(0), std::string("raw"));
    OCIO_CHECK_EQUAL(config->getColorSpaceNameByIndex(1), std::string("lnh"));
    OCIO_CHECK_EQUAL(config->getColorSpaceNameByIndex(2), std::string("cs1"));
    OCIO_CHECK_EQUAL(config->getColorSpaceNameByIndex(3), std::string("cs3"));

    // Env. variable supersedes the config content.

    InactiveCSGuard guard;

    is.str(configStr);
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is)->createEditableCopy());
    OCIO_CHECK_NO_THROW(config->validate());

    OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                 OCIO::COLORSPACE_INACTIVE), 3);
    OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                 OCIO::COLORSPACE_ACTIVE), 2);
    OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                 OCIO::COLORSPACE_ALL), 5);

    OCIO_CHECK_EQUAL(config->getColorSpaceNameByIndex(0), std::string("raw"));
    OCIO_CHECK_EQUAL(config->getColorSpaceNameByIndex(1), std::string("cs2"));

    // An API request supersedes the lists from the env. variable and the config file.

    OCIO_CHECK_NO_THROW(config->setInactiveColorSpaces("cs1, lnh"));

    OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                 OCIO::COLORSPACE_INACTIVE), 2);
    OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                 OCIO::COLORSPACE_ACTIVE), 3);
    OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                 OCIO::COLORSPACE_ALL), 5);

    OCIO_CHECK_EQUAL(config->getColorSpaceNameByIndex(0), std::string("raw"));
    OCIO_CHECK_EQUAL(config->getColorSpaceNameByIndex(1), std::string("cs2"));
    OCIO_CHECK_EQUAL(config->getColorSpaceNameByIndex(2), std::string("cs3"));
}

OCIO_ADD_TEST(Config, inactive_color_space_read_write)
{
    // The unit tests validate the read/write.

    {
        std::string configStr;
        configStr += InactiveCSConfigStart;
        configStr += "inactive_colorspaces: [cs2]\n";
        configStr += InactiveCSConfigEnd;

        std::istringstream is;
        is.str(configStr);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                     OCIO::COLORSPACE_ALL), 5);
        OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(), 4);

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), configStr);
    }

    {
        InactiveCSGuard guard; // Where inactive color spaces are "cs3, cs1, lnh".

        std::string configStr;
        configStr += InactiveCSConfigStart;
        configStr += "inactive_colorspaces: [cs2]\n";
        configStr += InactiveCSConfigEnd;

        std::istringstream is;
        is.str(configStr);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        {
            OCIO::LogGuard log; // Mute the warnings.
            OCIO_CHECK_NO_THROW(config->validate());
        }

        OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                     OCIO::COLORSPACE_ALL), 5);
        OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(), 2);

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), configStr);
    }

    {
        std::string configStr;
        configStr += InactiveCSConfigStart;
        // Test a multi-line list.
        configStr += "inactive_colorspaces: [cs1\t\n   \n,   \ncs2]\n";
        configStr += InactiveCSConfigEnd;

        std::istringstream is;
        is.str(configStr);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                     OCIO::COLORSPACE_ALL), 5);
        OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(), 3);

        std::string resultStr;
        resultStr += InactiveCSConfigStart;
        resultStr += "inactive_colorspaces: [cs1, cs2]\n";
        resultStr += InactiveCSConfigEnd;

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), resultStr);
    }

    // Do not save an empty 'inactive_colorspaces'.
    {
        std::string configStr;
        configStr += InactiveCSConfigStart;
        configStr += "inactive_colorspaces: []\n";
        configStr += InactiveCSConfigEnd;

        std::istringstream is;
        is.str(configStr);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                   OCIO::COLORSPACE_ALL), 5);
        OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 5);

        std::string resultStr;
        resultStr += InactiveCSConfigStart;
        resultStr += InactiveCSConfigEnd;

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), resultStr);
    }

    // Inactive 'unknown' color space ends up to not filter out any color space
    // but still preserved by the read/write.
    {
        std::string configStr;
        configStr += InactiveCSConfigStart;
        configStr += "inactive_colorspaces: [unknown]\n";
        configStr += InactiveCSConfigEnd;

        std::istringstream is;
        is.str(configStr);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));

        {
            OCIO::LogGuard log;
            OCIO_CHECK_NO_THROW(config->validate());
            OCIO_CHECK_EQUAL(log.output(), 
                             "[OpenColorIO Warning]: Inactive 'unknown' is neither a color "
                             "space nor a named transform.\n");
        }

        OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                   OCIO::COLORSPACE_ALL), 5);
        OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 5);

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), configStr);
    }
}

OCIO_ADD_TEST(Config, two_configs)
{
    constexpr const char * SIMPLE_CONFIG1{ R"(
ocio_profile_version: 2

environment:
  {}

roles:
  default: raw1
  aces_interchange: aces1
  cie_xyz_d65_interchange: display1

colorspaces:
  - !<ColorSpace>
    name: raw1
    allocation: uniform

  - !<ColorSpace>
    name: test1
    allocation: uniform
    to_scene_reference: !<MatrixTransform> {offset: [0.01, 0.02, 0.03, 0]}

  - !<ColorSpace>
    name: aces1
    allocation: uniform
    from_scene_reference: !<ExponentTransform> {value: [1.101, 1.202, 1.303, 1.404]}

display_colorspaces:
  - !<ColorSpace>
    name: display1
    allocation: uniform
    from_display_reference: !<CDLTransform> {slope: [1, 2, 1]}

  - !<ColorSpace>
    name: display2
    allocation: uniform
    from_display_reference: !<FixedFunctionTransform> {style: ACES_RedMod03}

)" };

    constexpr const char * SIMPLE_CONFIG2{ R"(
ocio_profile_version: 2

environment:
  {}

roles:
  default: raw2
  aces_interchange: aces2
  cie_xyz_d65_interchange: display3
  test_role: test2

colorspaces:
  - !<ColorSpace>
    name: raw2
    allocation: uniform

  - !<ColorSpace>
    name: test2
    allocation: uniform
    from_scene_reference: !<MatrixTransform> {offset: [0.11, 0.12, 0.13, 0]}

  - !<ColorSpace>
    name: aces2
    allocation: uniform
    to_scene_reference: !<RangeTransform> {min_in_value: -0.0109, max_in_value: 1.0505, min_out_value: 0.0009, max_out_value: 2.5001}

display_colorspaces:
  - !<ColorSpace>
    name: display3
    allocation: uniform
    from_display_reference: !<ExponentTransform> {value: 2.4}

  - !<ColorSpace>
    name: display4
    allocation: uniform
    from_display_reference: !<LogTransform> {base: 5}
)" };

    std::istringstream is;
    is.str(SIMPLE_CONFIG1);
    OCIO::ConstConfigRcPtr config1, config2;
    OCIO_CHECK_NO_THROW(config1 = OCIO::Config::CreateFromStream(is));
    is.clear();
    is.str(SIMPLE_CONFIG2);
    OCIO_CHECK_NO_THROW(config2 = OCIO::Config::CreateFromStream(is));

    OCIO::ConstProcessorRcPtr p;
    // NB: Although they have the same name, they are in different configs and are different ColorSpaces.
    OCIO_CHECK_NO_THROW(p = OCIO::Config::GetProcessorFromConfigs(config1, "test1", config2, "test2"));
    OCIO_REQUIRE_ASSERT(p);
    auto group = p->createGroupTransform();
    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 4);
    auto t0 = group->getTransform(0);
    auto m0 = OCIO_DYNAMIC_POINTER_CAST<OCIO::MatrixTransform>(t0);
    OCIO_CHECK_ASSERT(m0);
    auto t1 = group->getTransform(1);
    auto e1 = OCIO_DYNAMIC_POINTER_CAST<OCIO::ExponentTransform>(t1);
    OCIO_CHECK_ASSERT(e1);
    auto t2 = group->getTransform(2);
    auto r2 = OCIO_DYNAMIC_POINTER_CAST<OCIO::RangeTransform>(t2);
    OCIO_CHECK_ASSERT(r2);
    auto t3 = group->getTransform(3);
    auto m3 = OCIO_DYNAMIC_POINTER_CAST<OCIO::MatrixTransform>(t3);
    OCIO_CHECK_ASSERT(m3);

    // Or interchange spaces can be specified.
    OCIO_CHECK_NO_THROW(p = OCIO::Config::GetProcessorFromConfigs(
        config1, "test1", "aces1", config2, "test2", "aces2"));
    OCIO_REQUIRE_ASSERT(p);
    OCIO_REQUIRE_ASSERT(p);
    group = p->createGroupTransform();
    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 4);

    // Or interchange space can be specified using role.
    OCIO_CHECK_NO_THROW(p = OCIO::Config::GetProcessorFromConfigs(
        config1, "test1", OCIO::ROLE_INTERCHANGE_SCENE, config2, "test2", "aces2"));
    OCIO_REQUIRE_ASSERT(p);
    OCIO_REQUIRE_ASSERT(p);
    group = p->createGroupTransform();
    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 4);

    // Or color space can be specified using role.
    OCIO_CHECK_NO_THROW(p = OCIO::Config::GetProcessorFromConfigs(
        config1, "test1", OCIO::ROLE_INTERCHANGE_SCENE, config2, "test_role", "aces2"));
    OCIO_REQUIRE_ASSERT(p);
    OCIO_REQUIRE_ASSERT(p);
    group = p->createGroupTransform();
    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 4);

    // Display-referred interchange space.
    OCIO_CHECK_NO_THROW(p = OCIO::Config::GetProcessorFromConfigs(config1, "display2", config2, "display4"));
    OCIO_REQUIRE_ASSERT(p);
    group = p->createGroupTransform();
    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 4);
    t0 = group->getTransform(0);
    auto f0 = OCIO_DYNAMIC_POINTER_CAST<OCIO::FixedFunctionTransform>(t0);
    OCIO_CHECK_ASSERT(f0);
    t1 = group->getTransform(1);
    auto c1 = OCIO_DYNAMIC_POINTER_CAST<OCIO::CDLTransform>(t1);
    OCIO_CHECK_ASSERT(c1);
    t2 = group->getTransform(2);
    auto e2 = OCIO_DYNAMIC_POINTER_CAST<OCIO::ExponentTransform>(t2);
    OCIO_CHECK_ASSERT(e2);
    t3 = group->getTransform(3);
    auto l3 = OCIO_DYNAMIC_POINTER_CAST<OCIO::LogTransform>(t3);
    OCIO_CHECK_ASSERT(l3);

    OCIO_CHECK_THROW_WHAT(OCIO::Config::GetProcessorFromConfigs(config1, "display2", config2, "test2"),
                          OCIO::Exception,
                          "There is no view transform between the main scene-referred space "
                          "and the display-referred space");

    constexpr const char * SIMPLE_CONFIG3{ R"(
ocio_profile_version: 2

environment:
  {}

roles:
  default: raw

colorspaces:
  - !<ColorSpace>
    name: raw
    allocation: uniform

  - !<ColorSpace>
    name: test
    allocation: uniform
    from_scene_reference: !<MatrixTransform> {offset: [0.11, 0.12, 0.13, 0]}
)" };

    is.clear();
    is.str(SIMPLE_CONFIG3);
    OCIO::ConstConfigRcPtr config3;
    OCIO_CHECK_NO_THROW(config3 = OCIO::Config::CreateFromStream(is));

    OCIO_CHECK_THROW_WHAT(OCIO::Config::GetProcessorFromConfigs(config1, "test1", config3, "test"),
                          OCIO::Exception,
                          "The role 'aces_interchange' is missing in the destination config");

    OCIO_CHECK_THROW_WHAT(OCIO::Config::GetProcessorFromConfigs(config1, "display1", config3, "test"),
                          OCIO::Exception,
                          "The role 'cie_xyz_d65_interchange' is missing in the destination config");
}


const std::string PROFILE_V2_DCS_START = PROFILE_V2 + SIMPLE_PROFILE_A + DEFAULT_RULES +
                                         SIMPLE_PROFILE_DISPLAYS_LOOKS;

OCIO_ADD_TEST(Config, display_color_spaces_serialization)
{
    {
        const std::string strDCS =
            "\n"
            "view_transforms:\n"
            "  - !<ViewTransform>\n"
            "    name: display\n"
            "    from_display_reference: !<MatrixTransform> {}\n"
            "\n"
            "  - !<ViewTransform>\n"
            "    name: scene\n"
            "    from_scene_reference: !<MatrixTransform> {}\n"
            "\n"
            "display_colorspaces:\n"
            "  - !<ColorSpace>\n"
            "    name: dcs1\n"
            "    family: \"\"\n"
            "    equalitygroup: \"\"\n"
            "    bitdepth: unknown\n"
            "    isdata: false\n"
            "    allocation: uniform\n"
            "    from_display_reference: !<ExponentTransform> {value: 2.4, direction: inverse}\n"
            "\n"
            "  - !<ColorSpace>\n"
            "    name: dcs2\n"
            "    family: \"\"\n"
            "    equalitygroup: \"\"\n"
            "    bitdepth: unknown\n"
            "    isdata: false\n"
            "    allocation: uniform\n"
            "    to_display_reference: !<ExponentTransform> {value: 2.4}\n";

        const std::string str = PROFILE_V2_DCS_START + strDCS + SIMPLE_PROFILE_CS_V2;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str().size(), str.size());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }
}

OCIO_ADD_TEST(Config, display_color_spaces_errors)
{
    {
        const std::string strDCS =
            "\n"
            "display_colorspaces:\n"
            "  - !<ColorSpace>\n"
            "    name: dcs1\n"
            "    family: \"\"\n"
            "    equalitygroup: \"\"\n"
            "    bitdepth: unknown\n"
            "    isdata: false\n"
            "    allocation: uniform\n"
            "    from_scene_reference: !<ExponentTransform> {value: [2.4, 2.4, 2.4, 1], direction: inverse}\n"
            "\n"
            "  - !<ColorSpace>\n"
            "    name: dcs2\n"
            "    family: \"\"\n"
            "    equalitygroup: \"\"\n"
            "    bitdepth: unknown\n"
            "    isdata: false\n"
            "    allocation: uniform\n"
            "    to_display_reference: !<ExponentTransform> {value: [2.4, 2.4, 2.4, 1]}\n";
        const std::string str = PROFILE_V2_DCS_START + strDCS + SIMPLE_PROFILE_CS_V2;

        std::istringstream is;
        is.str(str);

        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                              "'from_scene_reference' cannot be used for a display color space");
    }
    {
        const std::string strDCS =
            "\n"
            "display_colorspaces:\n"
            "  - !<ColorSpace>\n"
            "    name: dcs1\n"
            "    family: \"\"\n"
            "    equalitygroup: \"\"\n"
            "    bitdepth: unknown\n"
            "    isdata: false\n"
            "    allocation: uniform\n"
            "    from_display_reference: !<ExponentTransform> {value: [2.4, 2.4, 2.4, 1], direction: inverse}\n"
            "\n"
            "  - !<ColorSpace>\n"
            "    name: dcs2\n"
            "    family: \"\"\n"
            "    equalitygroup: \"\"\n"
            "    bitdepth: unknown\n"
            "    isdata: false\n"
            "    allocation: uniform\n"
            "    to_scene_reference: !<ExponentTransform> {value: [2.4, 2.4, 2.4, 1]}\n";
        const std::string str = PROFILE_V2_DCS_START + strDCS + SIMPLE_PROFILE_CS_V2;

        std::istringstream is;
        is.str(str);

        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                              "'to_scene_reference' cannot be used for a display color space");
    }
}

OCIO_ADD_TEST(Config, config_v1)
{
    static const char CONFIG[] = 
        "ocio_profile_version: 1\n"
        "strictparsing: false\n"
        "roles:\n"
        "  default: raw\n"
        "displays:\n"
        "  sRGB:\n"
        "  - !<View> {name: Raw, colorspace: raw}\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "      name: raw\n";

    std::istringstream is;
    is.str(CONFIG);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());

    OCIO_CHECK_EQUAL(config->getNumViewTransforms(), 0);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_DISPLAY, OCIO::COLORSPACE_ALL), 
                     0);
}

OCIO_ADD_TEST(Config, view_transforms)
{
    const std::string str = PROFILE_V2_DCS_START + SIMPLE_PROFILE_CS_V2;

    std::istringstream is;
    is.str(str);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());

    auto configEdit = config->createEditableCopy();
    // Create display-referred view transform and add it to the config.
    auto vt = OCIO::ViewTransform::Create(OCIO::REFERENCE_SPACE_DISPLAY);
    OCIO_CHECK_THROW_WHAT(configEdit->addViewTransform(vt), OCIO::Exception,
                          "Cannot add view transform with an empty name");
    const std::string vtDisplay{ "display" };
    vt->setName(vtDisplay.c_str());
    OCIO_CHECK_THROW_WHAT(configEdit->addViewTransform(vt), OCIO::Exception,
                          "Cannot add view transform 'display' with no transform");
    OCIO_CHECK_NO_THROW(vt->setTransform(OCIO::MatrixTransform::Create(),
                                         OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE));
    OCIO_CHECK_NO_THROW(configEdit->addViewTransform(vt));
    OCIO_CHECK_EQUAL(configEdit->getNumViewTransforms(), 1);
    // Need at least one scene-referred view transform.
    OCIO_CHECK_THROW_WHAT(configEdit->validate(), OCIO::Exception,
                          "at least one must use the scene reference space");
    OCIO_CHECK_ASSERT(!configEdit->getDefaultSceneToDisplayViewTransform());

    // Create scene-referred view transform and add it to the config.
    vt = OCIO::ViewTransform::Create(OCIO::REFERENCE_SPACE_SCENE);
    const std::string vtScene{ "scene" };
    vt->setName(vtScene.c_str());
    OCIO_CHECK_NO_THROW(vt->setTransform(OCIO::MatrixTransform::Create(),
                                         OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE));
    OCIO_CHECK_NO_THROW(configEdit->addViewTransform(vt));
    OCIO_REQUIRE_EQUAL(configEdit->getNumViewTransforms(), 2);
    OCIO_CHECK_NO_THROW(configEdit->validate());

    auto sceneVT = configEdit->getDefaultSceneToDisplayViewTransform();
    OCIO_CHECK_ASSERT(sceneVT);

    OCIO_CHECK_EQUAL(vtDisplay, configEdit->getViewTransformNameByIndex(0));
    OCIO_CHECK_EQUAL(vtScene, configEdit->getViewTransformNameByIndex(1));
    OCIO_CHECK_EQUAL(std::string(""), configEdit->getViewTransformNameByIndex(42));
    OCIO_CHECK_ASSERT(configEdit->getViewTransform(vtScene.c_str()));
    OCIO_CHECK_ASSERT(!configEdit->getViewTransform("not a view transform"));

    // Default view transform.

    OCIO_CHECK_EQUAL(std::string(""), configEdit->getDefaultViewTransformName());

    configEdit->setDefaultViewTransformName("not valid");
    OCIO_CHECK_EQUAL(std::string("not valid"), configEdit->getDefaultViewTransformName());

    OCIO_CHECK_THROW_WHAT(configEdit->validate(), OCIO::Exception,
                          "Default view transform is defined as: 'not valid' but this does not "
                          "correspond to an existing scene-referred view transform");

    configEdit->setDefaultViewTransformName(vtDisplay.c_str());
    OCIO_CHECK_THROW_WHAT(configEdit->validate(), OCIO::Exception,
                          "Default view transform is defined as: 'display' but this does not "
                          "correspond to an existing scene-referred view transform");

    auto newSceneVT = sceneVT->createEditableCopy();
    newSceneVT->setName("NotFirst");
    configEdit->addViewTransform(newSceneVT);

    configEdit->setDefaultViewTransformName("NotFirst");
    OCIO_CHECK_NO_THROW(configEdit->validate());

    // Save and reload to test file io for viewTransform.
    std::stringstream os;
    os << *configEdit.get();

    is.clear();
    is.str(os.str());

    OCIO::ConstConfigRcPtr configReloaded;
    OCIO_CHECK_NO_THROW(configReloaded = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(configReloaded->validate());

    // Setting a view transform with the same name replaces the earlier one.
    OCIO_CHECK_NO_THROW(vt->setTransform(OCIO::LogTransform::Create(),
                                         OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE));
    OCIO_CHECK_NO_THROW(configEdit->addViewTransform(vt));
    OCIO_REQUIRE_EQUAL(configEdit->getNumViewTransforms(), 3);
    sceneVT = configEdit->getViewTransform(vtScene.c_str());
    auto trans = sceneVT->getTransform(OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE);
    OCIO_REQUIRE_ASSERT(trans);
    OCIO_CHECK_ASSERT(OCIO_DYNAMIC_POINTER_CAST<const OCIO::LogTransform>(trans));

    OCIO_CHECK_EQUAL(configReloaded->getNumViewTransforms(), 3);

    OCIO_CHECK_EQUAL(std::string("NotFirst"), configReloaded->getDefaultViewTransformName());

    // Clear all view transforms does not clear the config's default view transform string.

    configEdit->clearViewTransforms();
    OCIO_CHECK_EQUAL(configEdit->getNumViewTransforms(), 0);

    OCIO_CHECK_EQUAL(std::string("NotFirst"), configEdit->getDefaultViewTransformName());
}

OCIO_ADD_TEST(Config, display_view)
{
    // Create a config with a display that has 2 kinds of views.
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    {
        // Add default color space.
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("default");
        cs->setIsData(true);
        config->addColorSpace(cs);
    }

    // Add a scene-referred and a display-referred color space.
    auto cs = OCIO::ColorSpace::Create(OCIO::REFERENCE_SPACE_SCENE);
    cs->setName("scs");
    config->addColorSpace(cs);
    cs = OCIO::ColorSpace::Create(OCIO::REFERENCE_SPACE_DISPLAY);
    cs->setName("dcs");
    config->addColorSpace(cs);

    // Add a scene-referred and a display-referred view transform.
    auto vt = OCIO::ViewTransform::Create(OCIO::REFERENCE_SPACE_DISPLAY);
    vt->setName("display");
    OCIO_CHECK_NO_THROW(vt->setTransform(OCIO::MatrixTransform::Create(),
                                         OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE));
    OCIO_CHECK_NO_THROW(config->addViewTransform(vt));
    vt = OCIO::ViewTransform::Create(OCIO::REFERENCE_SPACE_SCENE);
    vt->setName("view_transform");
    OCIO_CHECK_NO_THROW(vt->setTransform(OCIO::MatrixTransform::Create(),
                                         OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE));
    OCIO_CHECK_NO_THROW(config->addViewTransform(vt));

    config->setDefaultViewTransformName("view_transform");

    // Add a simple view.
    const std::string display{ "display" };
    OCIO_CHECK_NO_THROW(config->addDisplayView(display.c_str(), "view1", "scs", ""));

    OCIO_CHECK_NO_THROW(config->validate());

    OCIO_CHECK_NO_THROW(config->addDisplayView(display.c_str(), "view2", "view_transform", "scs",
                                               "", "", ""));
    OCIO_CHECK_THROW_WHAT(config->validate(), OCIO::Exception,
                          "color space, 'scs', that is not a display-referred");

    OCIO_CHECK_NO_THROW(config->addDisplayView(display.c_str(), "view2", "view_transform", "dcs",
                                               "", "", ""));
    OCIO_CHECK_NO_THROW(config->validate());

    // Validate how the config is serialized.

    std::stringstream os;
    os << *config.get();
    constexpr char expected[]{ R"(ocio_profile_version: 2

environment:
  {}
search_path: ""
strictparsing: true
luma: [0.2126, 0.7152, 0.0722]

roles:
  {}

file_rules:
  - !<Rule> {name: Default, colorspace: default}

displays:
  display:
    - !<View> {name: view1, colorspace: scs}
    - !<View> {name: view2, view_transform: view_transform, display_colorspace: dcs}

active_displays: []
active_views: []

default_view_transform: view_transform

view_transforms:
  - !<ViewTransform>
    name: display
    from_display_reference: !<MatrixTransform> {}

  - !<ViewTransform>
    name: view_transform
    from_scene_reference: !<MatrixTransform> {}

display_colorspaces:
  - !<ColorSpace>
    name: dcs
    family: ""
    equalitygroup: ""
    bitdepth: unknown
    isdata: false
    allocation: uniform

colorspaces:
  - !<ColorSpace>
    name: default
    family: ""
    equalitygroup: ""
    bitdepth: unknown
    isdata: true
    allocation: uniform

  - !<ColorSpace>
    name: scs
    family: ""
    equalitygroup: ""
    bitdepth: unknown
    isdata: false
    allocation: uniform
)" };

    OCIO_CHECK_EQUAL(os.str(), expected);

    OCIO::ConstConfigRcPtr configRead;
    OCIO_CHECK_NO_THROW(configRead = OCIO::Config::CreateFromStream(os));
    OCIO_CHECK_EQUAL(configRead->getNumViews("display"), 2);
    const std::string v1{ configRead->getView("display", 0) };
    OCIO_CHECK_EQUAL(v1, "view1");
    OCIO_CHECK_EQUAL(std::string("scs"),
                     configRead->getDisplayViewColorSpaceName("display", v1.c_str()));
    OCIO_CHECK_EQUAL(std::string(""),
                     configRead->getDisplayViewTransformName("display", v1.c_str()));
    const std::string v2{ configRead->getView("display", 1) };
    OCIO_CHECK_EQUAL(v2, "view2");
    OCIO_CHECK_EQUAL(std::string("dcs"),
                     configRead->getDisplayViewColorSpaceName("display", v2.c_str()));
    OCIO_CHECK_EQUAL(std::string("view_transform"),
                     configRead->getDisplayViewTransformName("display", v2.c_str()));
    OCIO_CHECK_EQUAL(std::string("view_transform"), configRead->getDefaultViewTransformName());

    // Check some faulty calls related to displays & views.

    // Using nullptr or empty string for required parameters with throw.
    OCIO_CHECK_THROW_WHAT(config->addDisplayView(nullptr, "view1", "scs", ""),
                          OCIO::Exception, "a non-empty display name is needed");
    OCIO_CHECK_THROW_WHAT(config->addDisplayView(display.c_str(), nullptr, "scs", ""),
                          OCIO::Exception, "a non-empty view name is needed");
    OCIO_CHECK_THROW_WHAT(config->addDisplayView(display.c_str(), "view3", nullptr, ""),
                          OCIO::Exception, "a non-empty color space name is needed");
    OCIO_CHECK_THROW_WHAT(config->addDisplayView(display.c_str(), "view4", "view_transform", nullptr,
                                                 "", "", ""),
                          OCIO::Exception, "a non-empty color space name is needed");
    OCIO_CHECK_THROW_WHAT(config->addDisplayView("", "view1", "scs", ""),
                          OCIO::Exception, "a non-empty display name is needed");
    OCIO_CHECK_THROW_WHAT(config->addDisplayView(display.c_str(), "", "scs", ""),
                          OCIO::Exception, "a non-empty view name is needed");
    OCIO_CHECK_THROW_WHAT(config->addDisplayView(display.c_str(), "view3", "", ""),
                          OCIO::Exception, "a non-empty color space name is needed");
    OCIO_CHECK_THROW_WHAT(config->addDisplayView(display.c_str(), "view4", "view_transform", "",
                                                 "", "", ""),
                          OCIO::Exception, "a non-empty color space name is needed");
}

OCIO_ADD_TEST(Config, not_case_sensitive)
{
    // Validate that the color spaces and roles are case insensitive.

    std::istringstream is;
    is.str(PROFILE_V2_START);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());

    OCIO::ConstColorSpaceRcPtr cs;
    OCIO_CHECK_NO_THROW(cs = config->getColorSpace("lnh"));
    OCIO_CHECK_ASSERT(cs);

    OCIO_CHECK_NO_THROW(cs = config->getColorSpace("LNH"));
    OCIO_CHECK_ASSERT(cs);

    OCIO_CHECK_NO_THROW(cs = config->getColorSpace("RaW"));
    OCIO_CHECK_ASSERT(cs);

    OCIO_CHECK_ASSERT(config->hasRole("default"));
    OCIO_CHECK_ASSERT(config->hasRole("Default"));
    OCIO_CHECK_ASSERT(config->hasRole("DEFAULT"));

    OCIO_CHECK_ASSERT(config->hasRole("scene_linear"));
    OCIO_CHECK_ASSERT(config->hasRole("Scene_Linear"));

    OCIO_CHECK_ASSERT(!config->hasRole("reference"));
    OCIO_CHECK_ASSERT(!config->hasRole("REFERENCE"));
}

OCIO_ADD_TEST(Config, transform_with_roles)
{
    // Validate that Config::validate() on config file containing transforms 
    // with color space names (such as ColorSpaceTransform), correctly checks for role names
    // for those transforms.

    constexpr const char * OCIO_CONFIG{ R"(
ocio_profile_version: 1

roles:
  DEFAULT: raw
  scene_linear: cs1

displays:
  Disp1:
  - !<View> {name: View1, colorspace: RaW, looks: beauty}

looks:
  - !<Look>
    name: beauty
    process_space: SCENE_LINEAR
    transform: !<ColorSpaceTransform> {src: SCENE_LINEAR, dst: raw}

colorspaces:
  - !<ColorSpace>
    name: RAW
    allocation: uniform

  - !<ColorSpace>
    name: CS1
    allocation: uniform
    from_reference: !<MatrixTransform> {offset: [0.11, 0.12, 0.13, 0]}

  - !<ColorSpace>
    name: cs2
    allocation: uniform
    to_reference: !<ColorSpaceTransform> {src: SCENE_LINEAR, dst: raw}

  - !<ColorSpace>
    name: cs3
    allocation: uniform
    to_reference: !<ColorSpaceTransform> {src: SCENE_LINEAR, dst: raw, data_bypass: false}
)" };

    std::istringstream is;
    is.str(OCIO_CONFIG);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());

    // Validate the color spaces.

    OCIO::ConstProcessorRcPtr processor;
    OCIO_CHECK_NO_THROW(processor = config->getProcessor("raw", "cs1"));
    OCIO_CHECK_ASSERT(processor);

    OCIO_CHECK_NO_THROW(processor = config->getProcessor("raw", "cs2"));
    OCIO_CHECK_ASSERT(processor);

    OCIO_CHECK_NO_THROW(processor = config->getProcessor("cs1", "cs2"));
    OCIO_CHECK_ASSERT(processor);

    OCIO::ConstColorSpaceRcPtr cs2 = config->getColorSpace("cs2");
    OCIO_REQUIRE_ASSERT(cs2);
    auto tr2 = cs2->getTransform(OCIO::COLORSPACE_DIR_TO_REFERENCE);
    OCIO_REQUIRE_ASSERT(tr2);
    auto cs2Tr = OCIO::DynamicPtrCast<const OCIO::ColorSpaceTransform>(tr2);
    OCIO_REQUIRE_ASSERT(cs2Tr);
    OCIO_CHECK_ASSERT(cs2Tr->getDataBypass());

    OCIO::ConstColorSpaceRcPtr cs3 = config->getColorSpace("cs3");
    OCIO_REQUIRE_ASSERT(cs3);
    auto tr3 = cs3->getTransform(OCIO::COLORSPACE_DIR_TO_REFERENCE);
    OCIO_REQUIRE_ASSERT(tr3);
    auto cs3Tr = OCIO::DynamicPtrCast<const OCIO::ColorSpaceTransform>(tr3);
    OCIO_REQUIRE_ASSERT(cs3Tr);
    OCIO_CHECK_ASSERT(!cs3Tr->getDataBypass());

    // Validate the (display, view) pair with looks.

    OCIO::DisplayViewTransformRcPtr display = OCIO::DisplayViewTransform::Create();
    display->setSrc("raw");
    display->setDisplay("Disp1");
    display->setView("View1");

    OCIO_CHECK_NO_THROW(processor = config->getProcessor(display));
    OCIO_CHECK_ASSERT(processor);

    display->setSrc("cs1");

    OCIO_CHECK_NO_THROW(processor = config->getProcessor(display));
    OCIO_CHECK_ASSERT(processor);

    display->setSrc("cs2");

    OCIO_CHECK_NO_THROW(processor = config->getProcessor(display));
    OCIO_CHECK_ASSERT(processor);
}    

OCIO_ADD_TEST(Config, look_transform)
{
    // Validate Config::validate() on config file containing look transforms.

    constexpr const char * OCIO_CONFIG{ R"(
ocio_profile_version: 2

environment:
  {}

roles:
  default: raw

file_rules:
  - !<Rule> {name: Default, colorspace: default}

displays:
  Disp1:
  - !<View> {name: View1, colorspace: raw, looks: look1}

looks:
  - !<Look>
    name: look1
    process_space: default
    transform: !<ColorSpaceTransform> {src: default, dst: raw}
  - !<Look>
    name: look2
    process_space: default
    transform: !<LookTransform> {src: default, dst: raw, looks:+look1}

colorspaces:
  - !<ColorSpace>
    name: raw
    allocation: uniform
)" };

    std::istringstream is;
    is.str(OCIO_CONFIG);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());
}

OCIO_ADD_TEST(Config, family_separator)
{
    // Test the family separator.

    OCIO::ConfigRcPtr cfg;
    OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateRaw()->createEditableCopy());
    OCIO_CHECK_NO_THROW(cfg->validate());

    OCIO_CHECK_EQUAL(cfg->getFamilySeparator(), '/');

    OCIO_CHECK_NO_THROW(cfg->setFamilySeparator(' '));
    OCIO_CHECK_EQUAL(cfg->getFamilySeparator(), ' ');

    OCIO_CHECK_NO_THROW(cfg->setFamilySeparator(0));
    OCIO_CHECK_EQUAL(cfg->getFamilySeparator(), 0);

    // Reset to its default value.
    OCIO_CHECK_EQUAL(OCIO::Config::GetDefaultFamilySeparator(), '/');
    OCIO_CHECK_NO_THROW(cfg->setFamilySeparator(OCIO::Config::GetDefaultFamilySeparator()));
    OCIO_CHECK_EQUAL(cfg->getFamilySeparator(), '/');

    OCIO_CHECK_THROW(cfg->setFamilySeparator((char)127), OCIO::Exception);
    OCIO_CHECK_THROW(cfg->setFamilySeparator((char)31) , OCIO::Exception);

    // Test read/write.

    static const std::string CONFIG = 
        "ocio_profile_version: 2\n"
        "\n"
        "environment:\n"
        "  {}\n"
        "search_path: \"\"\n"
        "strictparsing: false\n"
        "family_separator: \" \"\n"
        "luma: [0.2126, 0.7152, 0.0722]\n"
        "\n"
        "roles:\n"
        "  default: raw\n"
        "\n"
        "file_rules:\n"
        "  - !<Rule> {name: Default, colorspace: default}\n"
        "\n"
        "displays:\n"
        "  sRGB:\n"
        "    - !<View> {name: Raw, colorspace: raw}\n"
        "\n"
        "active_displays: []\n"
        "active_views: []\n"
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: raw\n"
        "    family: raw\n"
        "    equalitygroup: \"\"\n"
        "    bitdepth: 32f\n"
        "    description: A raw color space. Conversions to and from this space are no-ops.\n"
        "    isdata: true\n"
        "    allocation: uniform\n";

    OCIO_CHECK_NO_THROW(cfg->setFamilySeparator(' '));

    std::ostringstream oss;
    OCIO_CHECK_NO_THROW(oss << *cfg.get());

    OCIO_CHECK_EQUAL(oss.str(), CONFIG);

    // v1 does not support family separators different from the default value i.e. '/'.

    static const std::string CONFIG_V1 = 
        "ocio_profile_version: 1\n"
        "\n"
        "search_path: \"\"\n"
        "\n"
        "roles:\n"
        "  reference: raw\n"
        "\n"
        "displays:\n"
        "  sRGB:\n"
        "    - !<View> {name: Raw, colorspace: raw}\n"
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: raw\n"
        "    allocation: uniform\n";

    std::istringstream iss;
    iss.str(CONFIG_V1);

    OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateFromStream(iss)->createEditableCopy());
    OCIO_REQUIRE_EQUAL(cfg->getFamilySeparator(), '/'); // v1 default family separator

    OCIO_CHECK_NO_THROW(cfg->setFamilySeparator('&'));
    OCIO_CHECK_THROW_WHAT(cfg->validate(),
                          OCIO::Exception,
                          "Only version 2 (or higher) can have a family separator.");

    oss.str("");
    OCIO_CHECK_THROW_WHAT((oss << *cfg),
                          OCIO::Exception,
                          "Only version 2 (or higher) can have a family separator.");

    // Even with the default value, v1 config file must not contain the family_separator key.

    static const std::string CONFIG_V1bis = 
        "ocio_profile_version: 1\n"
        "\n"
        "search_path: \"\"\n"
        "family_separator: \"/\"\n"
        "\n"
        "roles:\n"
        "  reference: raw\n"
        "\n"
        "displays:\n"
        "  sRGB:\n"
        "    - !<View> {name: Raw, colorspace: raw}\n"
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: raw\n"
        "    allocation: uniform\n";

    iss.str(CONFIG_V1bis);

    OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(iss),
                          OCIO::Exception,
                          "Config v1 can't have 'family_separator'.");
}

OCIO_ADD_TEST(Config, add_remove_display)
{
    OCIO::ConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateRaw()->createEditableCopy());
    OCIO_CHECK_NO_THROW(config->validate());

    OCIO_REQUIRE_EQUAL(config->getNumDisplays(), 1);
    OCIO_REQUIRE_EQUAL(std::string(config->getDisplay(0)), std::string("sRGB"));
    OCIO_REQUIRE_EQUAL(config->getNumViews("sRGB"), 1);
    OCIO_REQUIRE_EQUAL(std::string(config->getView("sRGB", 0)), std::string("Raw"));

    // Add a (display, view) pair.

    OCIO_CHECK_NO_THROW(config->addDisplayView("disp1", "view1", "raw", nullptr));
    OCIO_REQUIRE_EQUAL(config->getNumDisplays(), 2);
    OCIO_CHECK_EQUAL(std::string(config->getDisplay(0)), std::string("sRGB"));
    OCIO_CHECK_EQUAL(std::string(config->getDisplay(1)), std::string("disp1"));
    OCIO_REQUIRE_EQUAL(config->getNumViews("disp1"), 1);

    // Remove a (display, view) pair.

    OCIO_CHECK_NO_THROW(config->removeDisplayView("disp1", "view1"));
    OCIO_REQUIRE_EQUAL(config->getNumDisplays(), 1);
    OCIO_CHECK_EQUAL(std::string(config->getDisplay(0)), std::string("sRGB"));
}

OCIO_ADD_TEST(Config, is_colorspace_used)
{
    // Test Config::isColorSpaceUsed() i.e. a color space could be defined but not used.

    constexpr char CONFIG[] = 
        "ocio_profile_version: 2\n"
        "\n"
        "environment:\n"
        "  {}\n"
        "\n"
        "search_path: luts\n"
        "strictparsing: true\n"
        "luma: [0.2126, 0.7152, 0.0722]\n"
        "\n"
        "roles:\n"
        "  default: cs1\n"
        "\n"
        "view_transforms:\n"
        "  - !<ViewTransform>\n"
        "    name: vt1\n"
        "    from_scene_reference: !<ColorSpaceTransform> {src: cs11, dst: cs11}\n"
        "\n"
        "displays:\n"
        "  disp1:\n"
        "    - !<View> {name: view1, colorspace: cs2}\n"
        "    - !<View> {name: view2, colorspace: cs9}\n"
        "\n"
        "active_displays: [disp1]\n"
        "active_views: [view1]\n"
        "\n"
        "file_rules:\n"
        "  - !<Rule> {name: rule1, colorspace: cs10, pattern: \"*\", extension: \"*\"}\n"
        "  - !<Rule> {name: Default, colorspace: default}\n"
        "\n"
        "looks:\n"
        "  - !<Look>\n"
        "    name: beauty\n"
        "    process_space: cs5\n"
        "    transform: !<ColorSpaceTransform> {src: cs6, dst: cs6}\n"
        "\n"
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: cs1\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: cs2\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: cs3\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: cs4\n"
        "    from_scene_reference: !<ColorSpaceTransform> {src: cs3, dst: cs3}\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: cs5\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: cs6\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: cs7\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: cs8\n"
        "    from_scene_reference: !<GroupTransform>\n"
        "      children:\n"
        "        - !<ColorSpaceTransform> {src: cs7, dst: cs7}\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: cs9\n"
        "    from_scene_reference: !<GroupTransform>\n"
        "      children:\n"
        "        - !<GroupTransform>\n"
        "             children:\n"
        "               - !<LookTransform> {src: cs8, dst: cs8}\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: cs10\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: cs11\n";

    std::istringstream iss;
    iss.str(CONFIG);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(iss));
    OCIO_CHECK_NO_THROW(config->validate());

    OCIO_CHECK_ASSERT(config->isColorSpaceUsed("cs1" )); // Used by a role.
    OCIO_CHECK_ASSERT(config->isColorSpaceUsed("cs2" )); // Used by a (display, view) pair.
    OCIO_CHECK_ASSERT(config->isColorSpaceUsed("cs3" )); // Used by another color space.
    OCIO_CHECK_ASSERT(config->isColorSpaceUsed("cs5" )); // Used by a look i.e. process_space.
    OCIO_CHECK_ASSERT(config->isColorSpaceUsed("cs6" )); // Used by a look i.e. ColorSpaceTransform.
    OCIO_CHECK_ASSERT(config->isColorSpaceUsed("cs7" )); // Indirectly used by a ColorSpaceTransform.
    OCIO_CHECK_ASSERT(config->isColorSpaceUsed("cs8" )); // Indirectly used by a LookTransform.
    OCIO_CHECK_ASSERT(config->isColorSpaceUsed("cs9" )); // Used by a inactive (display, view) pair.
    OCIO_CHECK_ASSERT(config->isColorSpaceUsed("cs10")); // Used by a file rule.
    OCIO_CHECK_ASSERT(config->isColorSpaceUsed("cs11")); // Used by a view transform.

    OCIO_CHECK_ASSERT(!config->isColorSpaceUsed("cs4")); // Present but not used.

    OCIO_CHECK_ASSERT(!config->isColorSpaceUsed(nullptr));
    OCIO_CHECK_ASSERT(!config->isColorSpaceUsed(""));
    OCIO_CHECK_ASSERT(!config->isColorSpaceUsed("cs65")); // Unknown color spaces are not used.
}

OCIO_ADD_TEST(Config, transform_versions)
{
    // Saving a v1 config containing v2 transforms must fail.

    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    OCIO_CHECK_EQUAL(config->getMajorVersion(), OCIO_VERSION_MAJOR);

    config->setMajorVersion(OCIO::FirstSupportedMajorVersion);
    config->setMinorVersion(0);

    OCIO_CHECK_EQUAL(config->getMajorVersion(), 1);

    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();

    OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
    cs->setName("range");
    cs->setTransform(range, OCIO::COLORSPACE_DIR_TO_REFERENCE);

    OCIO_CHECK_NO_THROW(config->addColorSpace(cs));

    std::ostringstream oss;
    OCIO_CHECK_THROW_WHAT((oss << *config),
                          OCIO::Exception,
                          "Error building YAML: Only config version 2 (or higher) can have RangeTransform.");

    // Loading a v1 config containing v2 transforms must fail.

    constexpr const char * OCIO_CONFIG{ R"(
ocio_profile_version: 1

roles:
  default: raw

colorspaces:
  - !<ColorSpace>
    name: raw
    allocation: uniform
    from_reference: !<GroupTransform>
       children:
         - !<RangeTransform> {min_in_value: 0, min_out_value: 0}
)" };

    std::istringstream is;
    is.str(OCIO_CONFIG);
    OCIO::ConstConfigRcPtr cfg;
    OCIO_CHECK_THROW_WHAT(cfg = OCIO::Config::CreateFromStream(is),
                          OCIO::Exception,
                          "Only config version 2 (or higher) can have RangeTransform.");
}

OCIO_ADD_TEST(Config, dynamic_properties)
{
    OCIO::ConfigRcPtr config = OCIO::Config::CreateRaw()->createEditableCopy();

    OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
    cs->setName("test");

    OCIO::ExposureContrastTransformRcPtr ec = OCIO::ExposureContrastTransform::Create();
    ec->makeExposureDynamic();
    cs->setTransform(ec, OCIO::COLORSPACE_DIR_TO_REFERENCE);

    OCIO_CHECK_NO_THROW(config->addColorSpace(cs));
    OCIO_CHECK_NO_THROW(config->validate());

    OCIO::GradingPrimaryTransformRcPtr gp = OCIO::GradingPrimaryTransform::Create(OCIO::GRADING_LOG);
    gp->makeDynamic();
    cs->setTransform(gp, OCIO::COLORSPACE_DIR_FROM_REFERENCE);

    OCIO_CHECK_NO_THROW(config->addColorSpace(cs));
    OCIO_CHECK_NO_THROW(config->validate());

    // Save config and load it back.

    std::ostringstream os;
    config->serialize(os);
    std::istringstream is;
    is.str(os.str());

    OCIO::ConstConfigRcPtr configBack;
    OCIO_CHECK_NO_THROW(configBack = OCIO::Config::CreateFromStream(is));
    OCIO_REQUIRE_ASSERT(configBack);
    OCIO::ConstColorSpaceRcPtr csBack;
    OCIO_CHECK_NO_THROW(csBack = configBack->getColorSpace("test"));
    OCIO_REQUIRE_ASSERT(csBack);
    auto toTr = csBack->getTransform(OCIO::COLORSPACE_DIR_TO_REFERENCE);
    OCIO_REQUIRE_ASSERT(toTr);
    auto ecBack = OCIO_DYNAMIC_POINTER_CAST<const OCIO::ExposureContrastTransform>(toTr);
    OCIO_REQUIRE_ASSERT(ecBack);
    // Exposure contrast is dynamic when loaded back.
    OCIO_CHECK_ASSERT(ecBack->isExposureDynamic());
    auto fromTr = csBack->getTransform(OCIO::COLORSPACE_DIR_FROM_REFERENCE);
    OCIO_REQUIRE_ASSERT(fromTr);
    auto gpBack = OCIO_DYNAMIC_POINTER_CAST<const OCIO::GradingPrimaryTransform>(fromTr);
    OCIO_REQUIRE_ASSERT(gpBack);
    // Grading primary is not dynamic when loaded back.
    OCIO_CHECK_ASSERT(!gpBack->isDynamic());
}

OCIO_ADD_TEST(Config, builtin_transforms)
{
    // Test some default built-in transforms.

    constexpr const char * CONFIG_BUILTIN_TRANSFORMS{
R"(ocio_profile_version: 2

environment:
  {}
search_path: ""
strictparsing: true
luma: [0.2126, 0.7152, 0.0722]

roles:
  default: ref

file_rules:
  - !<Rule> {name: Default, colorspace: default}

displays:
  Disp1:
    - !<View> {name: View1, colorspace: test}

active_displays: []
active_views: []

colorspaces:
  - !<ColorSpace>
    name: ref
    family: ""
    equalitygroup: ""
    bitdepth: unknown
    isdata: false
    allocation: uniform

  - !<ColorSpace>
    name: test
    family: ""
    equalitygroup: ""
    bitdepth: unknown
    isdata: false
    allocation: uniform
    from_scene_reference: !<GroupTransform>
      children:
        - !<BuiltinTransform> {style: ACEScct_to_ACES2065-1}
        - !<BuiltinTransform> {style: ACEScct_to_ACES2065-1, direction: inverse}
)"};

    std::istringstream iss;
    iss.str(CONFIG_BUILTIN_TRANSFORMS);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(iss));

    {
        // Test loading the config.

        OCIO_CHECK_NO_THROW(config->validate());
        OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 2);

        OCIO::ConstProcessorRcPtr processor;
        OCIO_CHECK_NO_THROW(processor = config->getProcessor("ref", "test"));
    }

    {
        // Test saving the config.

        std::ostringstream oss;
        oss << *config.get();
        OCIO_CHECK_EQUAL(oss.str(), CONFIG_BUILTIN_TRANSFORMS);
    }
}

OCIO_ADD_TEST(Config, config_context_cacheids)
{
    // Validate the cacheID computation from Config & Context classes when OCIO Context
    // variables are present. In the config below, there is one in a color space i.e. $CS3 
    // and one undeclared in a look i.e. $LOOK1.

    static constexpr char CONFIG[] = 
        "ocio_profile_version: 2\n"
        "\n"
        "search_path: luts\n"
        "\n"
        "environment: {CS3: lut1d_green.ctf}\n"
        "\n"
        "roles:\n"
        "  default: cs1\n"
        "\n"
        "displays:\n"
        "  disp1:\n"
        "    - !<View> {name: view1, colorspace: cs3}\n"
        "    - !<View> {name: view2, colorspace: cs3, looks: look1}\n"
        "\n"
        "looks:\n"
        "  - !<Look>\n"
        "    name: look1\n"
        "    process_space: cs2\n"
        "    transform: !<FileTransform> {src: $LOOK1}\n"
        "\n"
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: cs1\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: cs2\n"
        "    from_scene_reference: !<MatrixTransform> {offset: [0.11, 0.12, 0.13, 0]}\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: cs3\n"
        "    from_scene_reference: !<FileTransform> {src: $CS3}\n";

    std::istringstream iss;
    iss.str(CONFIG);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(iss));

    // Set the right search_path.
    OCIO::ConfigRcPtr cfg = config->createEditableCopy();
    OCIO_CHECK_NO_THROW(cfg->clearSearchPaths());
    OCIO_CHECK_NO_THROW(cfg->addSearchPath(OCIO::GetTestFilesDir().c_str()));

    // Lets say there is a need for several processors built from the same config 
    // with same or different contexts.

    const std::string contextCacheID = cfg->getCurrentContext()->getCacheID();
    const std::string configCacheID  = cfg->getCacheID();

    // Using the default context variables.
    {
        OCIO_CHECK_NO_THROW(cfg->getProcessor("cs2", "disp1", "view1", OCIO::TRANSFORM_DIR_FORWARD));
    }

    // Set the context variable to its default value on a new context instance.
    {
        OCIO::ContextRcPtr ctx = cfg->getCurrentContext()->createEditableCopy();
        ctx->setStringVar("CS3", "lut1d_green.ctf");
 
        OCIO_CHECK_NO_THROW(cfg->getProcessor(ctx, "cs2", "disp1", "view1", OCIO::TRANSFORM_DIR_FORWARD));

        OCIO_CHECK_EQUAL(contextCacheID, ctx->getCacheID());
        OCIO_CHECK_EQUAL(configCacheID,  cfg->getCacheID(ctx));
    }

    // Set the context variable to its default value.
    {
        OCIO_CHECK_NO_THROW(cfg->addEnvironmentVar("CS3", "lut1d_green.ctf")); 
        OCIO_CHECK_NO_THROW(cfg->getProcessor("cs2", "disp1", "view1", OCIO::TRANSFORM_DIR_FORWARD));

        OCIO_CHECK_EQUAL(contextCacheID, cfg->getCurrentContext()->getCacheID());
        OCIO_CHECK_EQUAL(configCacheID,  cfg->getCacheID());
    }

    // Set the context variable to a different file using the context.
    {
        OCIO::ContextRcPtr ctx = cfg->getCurrentContext()->createEditableCopy();
        ctx->setStringVar("CS3", "exposure_contrast_log.ctf");
 
        OCIO_CHECK_NO_THROW(cfg->getProcessor(ctx, "cs2", "disp1", "view1", OCIO::TRANSFORM_DIR_FORWARD));

        OCIO_CHECK_NE(contextCacheID, ctx->getCacheID());
        OCIO_CHECK_NE(configCacheID,  cfg->getCacheID(ctx));

        // As expected the 'current' context is unchanged.
        OCIO_CHECK_EQUAL(configCacheID, cfg->getCacheID());
    }

    // Set the context variable to a different file using the config i.e. add a new value.
    {
        OCIO_CHECK_NO_THROW(cfg->addEnvironmentVar("CS3", "exposure_contrast_log.ctf"));
        OCIO_CHECK_NO_THROW(cfg->getProcessor("cs2", "disp1", "view1", OCIO::TRANSFORM_DIR_FORWARD));

        OCIO_CHECK_NE(contextCacheID, cfg->getCurrentContext()->getCacheID());
        OCIO_CHECK_NE(configCacheID,  cfg->getCacheID());
    }

    // $LOOK1 was missing so set to something.
    {
        OCIO_CHECK_NO_THROW(cfg->addEnvironmentVar("LOOK1", "lut1d_green.ctf"));
        OCIO_CHECK_NO_THROW(cfg->getProcessor("cs2", "disp1", "view2", OCIO::TRANSFORM_DIR_FORWARD));

        OCIO_CHECK_NE(contextCacheID, cfg->getCurrentContext()->getCacheID());
        OCIO_CHECK_NE(configCacheID,  cfg->getCacheID());
    }

    // Set $CS3 to its default value.
    {
        OCIO_CHECK_NO_THROW(cfg->addEnvironmentVar("CS3", "lut1d_green.ctf"));
        OCIO_CHECK_NO_THROW(cfg->getProcessor("cs2", "disp1", "view2", OCIO::TRANSFORM_DIR_FORWARD));

        OCIO_CHECK_NE(contextCacheID, cfg->getCurrentContext()->getCacheID());
        OCIO_CHECK_NE(configCacheID,  cfg->getCacheID());
    }

    // Remove $LOOK1 from context.
    {
        OCIO_CHECK_NO_THROW(cfg->addEnvironmentVar("CS3", "lut1d_green.ctf")); 
        OCIO_CHECK_NO_THROW(cfg->addEnvironmentVar("LOOK1", nullptr));

        OCIO_CHECK_EQUAL(contextCacheID, cfg->getCurrentContext()->getCacheID());
        OCIO_CHECK_EQUAL(configCacheID,  cfg->getCacheID());
    }
}

OCIO_ADD_TEST(Config, processor_cache_with_context_variables)
{
    // Validation of the processor cache of the Config class with context variables.

    constexpr const char * CONFIG_CUSTOM {
R"(ocio_profile_version: 2

environment: { VAR: cs1 }

search_path: ""
strictparsing: true
luma: [0.2126, 0.7152, 0.0722]

roles:
  default: ref

file_rules:
  - !<Rule> {name: Default, colorspace: default}

displays:
  Disp1:
    - !<View> {name: View1, colorspace: cs1}

colorspaces:
  - !<ColorSpace>
    name: ref

  - !<ColorSpace>
    name: cs1
    from_scene_reference: !<BuiltinTransform> {style: ACEScct_to_ACES2065-1}

  - !<ColorSpace>
    name: cs2
    from_scene_reference: !<ColorSpaceTransform> {src: ref, dst: cs1}

  - !<ColorSpace>
    name: cs3
    from_scene_reference: !<ColorSpaceTransform> {src: ref, dst: $VAR}
)"};

    std::istringstream iss;
    iss.str(CONFIG_CUSTOM);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(iss));

    {
        // Some basic validations before testing anything else.

        OCIO_CHECK_NO_THROW(config->validate());
        OCIO_CHECK_NO_THROW(config->getProcessor("ref", "cs1"));
    }

    {
        // Test that the cache detects identical processors (e.g. when $VAR == cs1)
        // even if the cache keys are different.

        // Keys are identical i.e. normal case.
        OCIO_CHECK_EQUAL(config->getProcessor("ref", "cs1").get(),
                         config->getProcessor("ref", "cs1").get());

        // Keys are different but processors are identical so it returns the same instance.
        OCIO_CHECK_EQUAL(config->getProcessor("ref", "cs1").get(),
                         config->getProcessor("ref", "cs2").get());

        // Keys are different but processors are identical.
        OCIO_CHECK_EQUAL(config->getProcessor("ref", "cs2").get(),
                         config->getProcessor("ref", "cs3").get());

        // Making a copy also flushes the internal processor cache.
        OCIO::ConfigRcPtr cfg = config->createEditableCopy();

        // Check that caches are different between Config instances.
        OCIO_CHECK_NE(config->getProcessor("ref", "cs1").get(),
                      cfg->getProcessor("ref", "cs1").get());

        OCIO_CHECK_NO_THROW(cfg->addEnvironmentVar("VAR", "ref"));

        // Keys are different but processors are identical.
        OCIO_CHECK_EQUAL(cfg->getProcessor("ref", "cs1").get(),
                         cfg->getProcessor("ref", "cs2").get());

        // Keys are different but processors are now different because $VAR != cs1.
        OCIO_CHECK_NE(cfg->getProcessor("ref", "cs2").get(),
                      cfg->getProcessor("ref", "cs3").get());
    }
}

OCIO_ADD_TEST(Config, context_variables_typical_use_cases)
{
    // Case 1 - No context variables used in the config.

    {
        static const std::string CONFIG = 
            "ocio_profile_version: 2\n"
            "\n"
            "search_path: " + OCIO::GetTestFilesDir() + "\n"
            "\n"
            "roles:\n"
            "  default: cs1\n"
            "\n"
            "displays:\n"
            "  disp1:\n"
            "    - !<View> {name: view1, colorspace: cs2}\n"
            "    - !<View> {name: view2, colorspace: cs3}\n"
            "\n"
            "colorspaces:\n"
            "  - !<ColorSpace>\n"
            "    name: cs1\n"
            "\n"
            "  - !<ColorSpace>\n"
            "    name: cs2\n"
            "    from_scene_reference: !<FileTransform> {src: exposure_contrast_linear.ctf}\n"
            "\n"
            "  - !<ColorSpace>\n"
            "    name: cs3\n"
            "    from_scene_reference: !<MatrixTransform> {offset: [0.11, 0.12, 0.13, 0]}\n";

        std::istringstream iss;
        iss.str(CONFIG);

        OCIO::ConfigRcPtr cfg;
        OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateFromStream(iss)->createEditableCopy());
        OCIO_CHECK_NO_THROW(cfg->validate());

        // If consecutive calls to getProcessor return the same pointer, it means that the cache
        // is working.

        OCIO_CHECK_EQUAL(cfg->getProcessor("cs1", "cs2").get(),
                         cfg->getProcessor("cs1", "cs2").get());

        OCIO_CHECK_EQUAL(cfg->getProcessor("cs1", "cs3").get(),
                         cfg->getProcessor("cs1", "cs3").get());


        OCIO_CHECK_EQUAL(cfg->getProcessor("cs1", "disp1", "view1", OCIO::TRANSFORM_DIR_FORWARD).get(),
                         cfg->getProcessor("cs1", "disp1", "view1", OCIO::TRANSFORM_DIR_FORWARD).get());

        OCIO_CHECK_EQUAL(cfg->getProcessor("cs1", "disp1", "view2", OCIO::TRANSFORM_DIR_FORWARD).get(),
                         cfg->getProcessor("cs1", "disp1", "view2", OCIO::TRANSFORM_DIR_FORWARD).get());

        // Create a different context instance but still identical to the current one.
        OCIO::ContextRcPtr ctx = cfg->getCurrentContext()->createEditableCopy();

        OCIO_CHECK_EQUAL(cfg->getProcessor(ctx, "cs1", "cs2").get(),
                         cfg->getProcessor(ctx, "cs1", "cs2").get());

        OCIO_CHECK_EQUAL(cfg->getProcessor(ctx, "cs1", "cs3").get(),
                         cfg->getProcessor(ctx, "cs1", "cs3").get());

        OCIO_CHECK_EQUAL(cfg->getProcessor(ctx, "cs1", "cs2").get(),
                         cfg->getProcessor("cs1", "cs2").get());


        OCIO_CHECK_EQUAL(cfg->getProcessor(ctx, "cs1", "disp1", "view1", OCIO::TRANSFORM_DIR_FORWARD).get(),
                         cfg->getProcessor(ctx, "cs1", "disp1", "view1", OCIO::TRANSFORM_DIR_FORWARD).get());

        OCIO_CHECK_EQUAL(cfg->getProcessor(ctx, "cs1", "disp1", "view2", OCIO::TRANSFORM_DIR_FORWARD).get(),
                         cfg->getProcessor(ctx, "cs1", "disp1", "view2", OCIO::TRANSFORM_DIR_FORWARD).get());

        OCIO_CHECK_EQUAL(cfg->getProcessor(ctx, "cs1", "disp1", "view1", OCIO::TRANSFORM_DIR_FORWARD).get(),
                         cfg->getProcessor("cs1", "disp1", "view1", OCIO::TRANSFORM_DIR_FORWARD).get());

        // Add an unused context variable in the context. The cache is still used.
        ctx->setStringVar("ENV", "xxx");

        OCIO_CHECK_EQUAL(cfg->getProcessor(ctx, "cs1", "cs2").get(),
                         cfg->getProcessor(ctx, "cs1", "cs2").get());

        OCIO_CHECK_EQUAL(cfg->getProcessor(ctx, "cs1", "cs3").get(),
                         cfg->getProcessor(ctx, "cs1", "cs3").get());

        OCIO_CHECK_EQUAL(cfg->getProcessor(ctx, "cs1", "cs2").get(),
                         cfg->getProcessor("cs1", "cs2").get());


        OCIO_CHECK_EQUAL(cfg->getProcessor(ctx, "cs1", "disp1", "view1", OCIO::TRANSFORM_DIR_FORWARD).get(),
                         cfg->getProcessor(ctx, "cs1", "disp1", "view1", OCIO::TRANSFORM_DIR_FORWARD).get());

        OCIO_CHECK_EQUAL(cfg->getProcessor(ctx, "cs1", "disp1", "view2", OCIO::TRANSFORM_DIR_FORWARD).get(),
                         cfg->getProcessor(ctx, "cs1", "disp1", "view2", OCIO::TRANSFORM_DIR_FORWARD).get());

        OCIO_CHECK_EQUAL(cfg->getProcessor(ctx, "cs1", "disp1", "view1", OCIO::TRANSFORM_DIR_FORWARD).get(),
                         cfg->getProcessor("cs1", "disp1", "view1", OCIO::TRANSFORM_DIR_FORWARD).get());
    }

    // Case 2 - Context variables used anywhere but in the search_path.

    {
        static const std::string CONFIG = 
            "ocio_profile_version: 2\n"
            "\n"
            "environment: {FILE: exposure_contrast_linear.ctf }\n"
            "\n"
            "search_path: " + OCIO::GetTestFilesDir() + "\n"
            "\n"
            "roles:\n"
            "  default: cs1\n"
            "\n"
            "displays:\n"
            "  disp1:\n"
            "    - !<View> {name: view1, colorspace: cs2}\n"
            "\n"
            "colorspaces:\n"
            "  - !<ColorSpace>\n"
            "    name: cs1\n"
            "\n"
            "  - !<ColorSpace>\n"
            "    name: cs2\n"
            "    from_scene_reference: !<FileTransform> {src: $FILE}\n";

        std::istringstream iss;
        iss.str(CONFIG);

        OCIO::ConfigRcPtr cfg;
        OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateFromStream(iss)->createEditableCopy());
        OCIO_CHECK_NO_THROW(cfg->validate());

        OCIO_CHECK_EQUAL(cfg->getProcessor("cs1", "cs2").get(),
                         cfg->getProcessor("cs1", "cs2").get());

        OCIO_CHECK_EQUAL(cfg->getProcessor("cs1", "disp1", "view1", OCIO::TRANSFORM_DIR_FORWARD).get(),
                         cfg->getProcessor("cs1", "disp1", "view1", OCIO::TRANSFORM_DIR_FORWARD).get());

        // Add an unused context variable in the context. The cache is still used.
        OCIO::ContextRcPtr ctx = cfg->getCurrentContext()->createEditableCopy();
        ctx->setStringVar("ENV", "xxx");

        OCIO_CHECK_EQUAL(cfg->getProcessor(ctx, "cs1", "cs2").get(),
                         cfg->getProcessor(ctx, "cs1", "cs2").get());

        OCIO_CHECK_EQUAL(cfg->getProcessor(ctx, "cs1", "cs2").get(),
                         cfg->getProcessor("cs1", "cs2").get());


        OCIO_CHECK_EQUAL(cfg->getProcessor(ctx, "cs1", "disp1", "view1", OCIO::TRANSFORM_DIR_FORWARD).get(),
                         cfg->getProcessor(ctx, "cs1", "disp1", "view1", OCIO::TRANSFORM_DIR_FORWARD).get());

        OCIO_CHECK_EQUAL(cfg->getProcessor(ctx, "cs1", "disp1", "view1", OCIO::TRANSFORM_DIR_FORWARD).get(),
                         cfg->getProcessor("cs1", "disp1", "view1", OCIO::TRANSFORM_DIR_FORWARD).get());

        // Change the value of the used context variable. The original cached value is *not* used.
        ctx->setStringVar("FILE", "exposure_contrast_log.ctf");

        OCIO_CHECK_EQUAL(cfg->getProcessor(ctx, "cs1", "cs2").get(),
                         cfg->getProcessor(ctx, "cs1", "cs2").get());

        OCIO_CHECK_EQUAL(cfg->getProcessor("cs1", "cs2").get(),
                         cfg->getProcessor("cs1", "cs2").get());

        OCIO_CHECK_NE(cfg->getProcessor(ctx, "cs1", "cs2").get(),
                      cfg->getProcessor("cs1", "cs2").get());


        OCIO_CHECK_EQUAL(cfg->getProcessor(ctx, "cs1", "disp1", "view1", OCIO::TRANSFORM_DIR_FORWARD).get(),
                         cfg->getProcessor(ctx, "cs1", "disp1", "view1", OCIO::TRANSFORM_DIR_FORWARD).get());

        OCIO_CHECK_EQUAL(cfg->getProcessor("cs1", "disp1", "view1", OCIO::TRANSFORM_DIR_FORWARD).get(),
                         cfg->getProcessor("cs1", "disp1", "view1", OCIO::TRANSFORM_DIR_FORWARD).get());

        OCIO_CHECK_NE(cfg->getProcessor(ctx, "cs1", "disp1", "view1", OCIO::TRANSFORM_DIR_FORWARD).get(),
                      cfg->getProcessor("cs1", "disp1", "view1", OCIO::TRANSFORM_DIR_FORWARD).get());
    }

    // Helper to disable the fallback mechanism.
    struct DisableFallback
    {
        DisableFallback()  { OCIO::SetEnvVariable(OCIO::OCIO_DISABLE_CACHE_FALLBACK ,"1"); }
        ~DisableFallback() { OCIO::UnsetEnvVariable(OCIO::OCIO_DISABLE_CACHE_FALLBACK);   }
    };

    // Case 3 - Context variables used on the search_path, but that variable is unchanged.

    {
        static const std::string CONFIG = 
            "ocio_profile_version: 2\n"
            "\n"
            "environment:\n"
            "  SHOW: " + OCIO::GetTestFilesDir() + "\n"
            "  SHOT: exposure_contrast_linear.ctf\n"
            "\n"
            "search_path: $SHOW\n"
            "\n"
            "roles:\n"
            "  default: cs1\n"
            "\n"
            "displays:\n"
            "  disp1:\n"
            "    - !<View> {name: view1, colorspace: cs2}\n"
            "\n"
            "colorspaces:\n"
            "  - !<ColorSpace>\n"
            "    name: cs1\n"
            "\n"
            "  - !<ColorSpace>\n"
            "    name: cs2\n"
            "    from_scene_reference: !<FileTransform> {src: exposure_contrast_linear.ctf}\n"
            "\n"
            "  - !<ColorSpace>\n"
            "    name: cs3\n"
            "    from_scene_reference: !<FileTransform> {src: $SHOT}\n";

        {
            std::istringstream iss;
            iss.str(CONFIG);

            OCIO::ConfigRcPtr cfg;
            OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateFromStream(iss)->createEditableCopy());
            OCIO_CHECK_NO_THROW(cfg->validate());

            // Change $SHOT to lut1d_green.ctf but $SHOT is not used.
            OCIO::ContextRcPtr ctx = cfg->getCurrentContext()->createEditableCopy();
            ctx->setStringVar("SHOT", "lut1d_green.ctf");

            // Here is the important validation: same processor because $SHOT is not used.
            OCIO_CHECK_EQUAL(cfg->getProcessor("cs1", "cs2").get(),
                             cfg->getProcessor(ctx, "cs1", "cs2").get());


            // The cache mechanism is also looking for identical processors (i.e. diff. contexts
            // or color spaces but producing the same color transformation). The following check is
            // validating the behavior.

            // Note that using this fall-back mechanism in the cache is much slower than if the
            // cache is able to find a hit based on the arguments alone since it much calculate a
            // cacheID of the two processors.  The ocioperf tool may be used to measure cache speed
            // in various situations.

            // Same processor because $SHOT is equal to 'exposure_contrast_linear.ctf'.
            OCIO_CHECK_EQUAL(cfg->getProcessor("cs1", "cs2").get(),
                             cfg->getProcessor("cs1", "cs3").get());
        }

        {
            // If the fallback is disabled (using the env. variable OCIO_DISABLE_CACHE_FALLBACK)
            // then the processor cache returns different instances as the cache keys are different.

            DisableFallback guard;

            std::istringstream iss;
            iss.str(CONFIG);

            OCIO::ConfigRcPtr cfg;
            OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateFromStream(iss)->createEditableCopy());
            OCIO_CHECK_NO_THROW(cfg->validate());

            // Fail to find the identical processor because the fallback is now disabled i.e. but
            // it succeeds when fallback is enabled as demonstrated above.
            OCIO_CHECK_NE(cfg->getProcessor("cs1", "cs2").get(), 
                          cfg->getProcessor("cs1", "cs3").get());

            // Change $SHOT to lut1d_green.ctf but $SHOT is not used.
            OCIO::ContextRcPtr ctx = cfg->getCurrentContext()->createEditableCopy();
            ctx->setStringVar("SHOT", "lut1d_green.ctf");

            // Here is the important validation: As the fallback is not used for the computation
            // of the cs1 to cs2 color transformation the same processor is still found.
            OCIO_CHECK_EQUAL(cfg->getProcessor("cs1", "cs2").get(),
                             cfg->getProcessor(ctx, "cs1", "cs2").get());
        }
    }

    // Case 4 - Context vars used in the search_path and they are changing per shot, but no
    // FileTransforms are used.

    {
        static const std::string CONFIG = 
            "ocio_profile_version: 2\n"
            "\n"
            "environment:\n"
            "  SHOW: " + OCIO::GetTestFilesDir() + "\n"
            "\n"
            "search_path: $SHOW\n"
            "\n"
            "roles:\n"
            "  default: cs1\n"
            "\n"
            "displays:\n"
            "  disp1:\n"
            "    - !<View> {name: view1, colorspace: cs2}\n"
            "\n"
            "colorspaces:\n"
            "  - !<ColorSpace>\n"
            "    name: cs1\n"
            "\n"
            "  - !<ColorSpace>\n"
            "    name: cs2\n"
            "    from_scene_reference: !<MatrixTransform> {offset: [0.11, 0.12, 0.13, 0]}\n";

        {
            std::istringstream iss;
            iss.str(CONFIG);

            OCIO::ConfigRcPtr cfg;
            OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateFromStream(iss)->createEditableCopy());
            OCIO_CHECK_NO_THROW(cfg->validate());

            OCIO::ContextRcPtr ctx = cfg->getCurrentContext()->createEditableCopy();
            ctx->setStringVar("SHOW", "/some/arbitrary/path");

            // As the context does not impact the color transformation computation use two different
            // context instances i.e. context keys are then different.
            OCIO_CHECK_NE(cfg->getCurrentContext()->getCacheID(), ctx->getCacheID());

            // Here is the important validation: same processor because $SHOW is not used.
            OCIO_CHECK_EQUAL(cfg->getProcessor("cs1", "cs2").get(),
                             cfg->getProcessor(ctx, "cs1", "cs2").get());
        }

        {
            // Demonstrate that the fallback is not used here i.e. context variables are not
            // impacting the cache.

            DisableFallback guard;

            std::istringstream iss;
            iss.str(CONFIG);

            OCIO::ConfigRcPtr cfg;
            OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateFromStream(iss)->createEditableCopy());
            OCIO_CHECK_NO_THROW(cfg->validate());

            OCIO::ContextRcPtr ctx = cfg->getCurrentContext()->createEditableCopy();
            ctx->setStringVar("SHOW", "/some/arbitrary/path");

            // Here is the demonstration that the fallback is not used i.e. disabled but the right
            // processor is still found.
            OCIO_CHECK_EQUAL(cfg->getProcessor("cs1", "cs2").get(),
                             cfg->getProcessor(ctx, "cs1", "cs2").get());
        }
    }

    // Case 5 - Context vars in the search_path and they are changing but the changed vars are not
    // used to resolve the file transform.

    // TODO: The collect of context variables currently lacks the heuristic to find which search_path
    // is effectively used so, as soon as one path (from the search_paths) is used all the paths are
    // then collected changing the cache key computation (even if the extra search_paths are useless).
    // To mitigate that limitation the fallback is then used to find if an existing identical
    // processor instance already exists.  

    {
        static const std::string CONFIG = 
            "ocio_profile_version: 2\n"
            "\n"
            "environment:\n"
            "  TRANSFORM_DIR: " + OCIO::GetTestFilesDir() + "\n"
            "\n"
            "search_path:\n"
            "  - /bogus/unknown/path\n"
            "  - $TRANSFORM_DIR\n"
            "  - $SHOT\n"
            "\n"
            "roles:\n"
            "  default: cs1\n"
            "\n"
            "displays:\n"
            "  disp1:\n"
            "    - !<View> {name: view1, colorspace: cs2}\n"
            "\n"
            "colorspaces:\n"
            "  - !<ColorSpace>\n"
            "    name: cs1\n"
            "\n"
            "  - !<ColorSpace>\n"
            "    name: cs2\n"
            "    from_scene_reference: !<FileTransform> {src: exposure_contrast_linear.ctf}\n";

        std::istringstream iss;
        iss.str(CONFIG);

        OCIO::ConfigRcPtr cfg;
        OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateFromStream(iss)->createEditableCopy());
        OCIO_CHECK_NO_THROW(cfg->validate());

        OCIO::ContextRcPtr ctx1 = cfg->getCurrentContext()->createEditableCopy();
        ctx1->setStringVar("SHOT", "/unknow/path/for_path_1");

        OCIO::ContextRcPtr ctx2 = cfg->getCurrentContext()->createEditableCopy();
        ctx2->setStringVar("SHOT", "/unknow/path/for_path_2");

        // Even if the two context instances are different the changed context variable is useless
        // so the same processor instance is returned. 
        OCIO_CHECK_EQUAL(cfg->getProcessor(ctx1, "cs1", "cs2").get(),
                         cfg->getProcessor(ctx2, "cs1", "cs2").get());

        {
            // If the fallback is disabled (using the env. variable OCIO_DISABLE_CACHE_FALLBACK)
            // then the processor cache returns different instances because of the search_path
            // heuristic limitation. It demonstrates the fallback is needed to mitigate the heuristic
            // limitation. As soon as the heuristic is enhanced, the following test must return
            // the same processor instance.

            DisableFallback guard;

            std::istringstream iss;
            iss.str(CONFIG);

            OCIO::ConfigRcPtr cfg;
            OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateFromStream(iss)->createEditableCopy());
            OCIO_CHECK_NO_THROW(cfg->validate());

            OCIO_CHECK_NE(cfg->getProcessor(ctx1, "cs1", "cs2").get(), 
                          cfg->getProcessor(ctx2, "cs1", "cs2").get());
        }
    }

    // Case 6 - Context vars in the search_path, the vars on the path to the file do change, but the
    // resulting file is the same.

    {
        static const std::string CONFIG = 
            "ocio_profile_version: 2\n"
            "\n"
            "environment:\n"
            "  PATH_1: " + OCIO::GetTestFilesDir() + "\n"
            "  PATH_2: " + OCIO::GetTestFilesDir() + "\n"
            "\n"
            "search_path:\n"
            "  - $PATH_1\n"
            "  - $PATH_2\n"
            "\n"
            "roles:\n"
            "  default: cs1\n"
            "\n"
            "displays:\n"
            "  disp1:\n"
            "    - !<View> {name: view1, colorspace: cs2}\n"
            "\n"
            "colorspaces:\n"
            "  - !<ColorSpace>\n"
            "    name: cs1\n"
            "\n"
            "  - !<ColorSpace>\n"
            "    name: cs2\n"
            "    from_scene_reference: !<FileTransform> {src: exposure_contrast_linear.ctf}\n";

        std::istringstream iss;
        iss.str(CONFIG);

        OCIO::ConfigRcPtr cfg;
        OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateFromStream(iss)->createEditableCopy());
        OCIO_CHECK_NO_THROW(cfg->validate());

        OCIO::ContextRcPtr ctx1 = cfg->getCurrentContext()->createEditableCopy();
        ctx1->setStringVar("PATH_1", "/unknow/path/for_path_1");

        OCIO::ContextRcPtr ctx2 = cfg->getCurrentContext()->createEditableCopy();
        ctx2->setStringVar("PATH_2", "/unknow/path/for_path_2");

        // It demonstrates that the cache keys will be different.
        OCIO_CHECK_NE(std::string(ctx1->getCacheID()), std::string(ctx2->getCacheID()));

        // Even if a different context variable is used the color transform remains identical so 
        // the processor cache returns the same processor instance because of the fallback. 
        OCIO_CHECK_EQUAL(cfg->getProcessor(ctx1, "cs1", "cs2").get(), // FileTransform uses PATH_2
                         cfg->getProcessor(ctx2, "cs1", "cs2").get());// FileTransform uses PATH_1

        {
            // If the fallback is disabled (using the env. variable OCIO_DISABLE_CACHE_FALLBACK)
            // then the processor cache returns different instances as the cache keys are different
            // i.e. cs1 needs PATH_2 while cs2 needs PATH_1. It demonstrates that only the fallback
            // can find the processor instance.

            DisableFallback guard;

            std::istringstream iss;
            iss.str(CONFIG);

            OCIO::ConfigRcPtr cfg;
            OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateFromStream(iss)->createEditableCopy());
            OCIO_CHECK_NO_THROW(cfg->validate());

            // The processor cache without the fallback fails to find the identical processor.
            OCIO_CHECK_NE(cfg->getProcessor(ctx1, "cs1", "cs2").get(), 
                          cfg->getProcessor(ctx2, "cs1", "cs2").get());
        }
    }
}

OCIO_ADD_TEST(Config, virtual_display)
{
    // Test the virtual display instantiation.

    static constexpr char CONFIG[]{ R"(ocio_profile_version: 2

environment:
  {}
search_path: ""
strictparsing: true
luma: [0.2126, 0.7152, 0.0722]

roles:
  default: raw

file_rules:
  - !<Rule> {name: Default, colorspace: default}

shared_views:
  - !<View> {name: sview1, colorspace: raw}
  - !<View> {name: sview2, colorspace: raw}

displays:
  sRGB:
    - !<View> {name: Raw, colorspace: raw}
    - !<View> {name: view, view_transform: display_vt, display_colorspace: display_cs}
    - !<Views> [sview1]

virtual_display:
  - !<View> {name: Raw, colorspace: raw}
  - !<View> {name: Film, view_transform: display_vt, display_colorspace: <USE_DISPLAY_NAME>}
  - !<Views> [sview2]

active_displays: []
active_views: []

view_transforms:
  - !<ViewTransform>
    name: default_vt
    to_scene_reference: !<CDLTransform> {sat: 1.5}

  - !<ViewTransform>
    name: display_vt
    to_display_reference: !<CDLTransform> {sat: 1.5}

display_colorspaces:
  - !<ColorSpace>
    name: display_cs
    family: ""
    equalitygroup: ""
    bitdepth: unknown
    isdata: false
    allocation: uniform
    to_display_reference: !<CDLTransform> {sat: 1.5}

colorspaces:
  - !<ColorSpace>
    name: raw
    family: ""
    equalitygroup: ""
    bitdepth: unknown
    isdata: true
    allocation: uniform
)" };

    std::istringstream iss;
    iss.str(CONFIG);

    // Step 1 - Validate a config containing a virtual display.

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(iss));
    OCIO_CHECK_NO_THROW(config->validate());


    // Step 2 - The virtual display is correctly loaded & saved.

    std::ostringstream oss;
    OCIO_CHECK_NO_THROW(oss << *config.get());
    OCIO_CHECK_EQUAL(oss.str(), CONFIG);

    // Some basic checks.
    OCIO_CHECK_EQUAL(3, config->getNumViews("sRGB"));
    OCIO_CHECK_EQUAL(2, config->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, "sRGB"));
    OCIO_CHECK_EQUAL(1, config->getNumViews(OCIO::VIEW_SHARED, "sRGB"));

    // Step 3 - Validate the virtual display information.

    {
        OCIO::ConfigRcPtr cfg = config->createEditableCopy();

        OCIO_REQUIRE_EQUAL(2, cfg->getVirtualDisplayNumViews(OCIO::VIEW_DISPLAY_DEFINED));

        const char * viewName = cfg->getVirtualDisplayView(OCIO::VIEW_DISPLAY_DEFINED, 0);

        OCIO_CHECK_EQUAL(std::string("Raw"), viewName);
        OCIO_CHECK_EQUAL(std::string(""), cfg->getVirtualDisplayViewTransformName(viewName));
        OCIO_CHECK_EQUAL(std::string("raw"), cfg->getVirtualDisplayViewColorSpaceName(viewName));
        OCIO_CHECK_EQUAL(std::string(""), cfg->getVirtualDisplayViewLooks(viewName));
        OCIO_CHECK_EQUAL(std::string(""), cfg->getVirtualDisplayViewRule(viewName));
        OCIO_CHECK_EQUAL(std::string(""), cfg->getVirtualDisplayViewDescription(viewName));

        viewName = cfg->getVirtualDisplayView(OCIO::VIEW_DISPLAY_DEFINED, 1);

        OCIO_CHECK_EQUAL(std::string("Film"), cfg->getVirtualDisplayView(OCIO::VIEW_DISPLAY_DEFINED, 1));
        OCIO_CHECK_EQUAL(std::string("display_vt"), cfg->getVirtualDisplayViewTransformName(viewName));
        OCIO_CHECK_EQUAL(std::string("<USE_DISPLAY_NAME>"), cfg->getVirtualDisplayViewColorSpaceName(viewName));
        OCIO_CHECK_EQUAL(std::string(""), cfg->getVirtualDisplayViewLooks(viewName));
        OCIO_CHECK_EQUAL(std::string(""), cfg->getVirtualDisplayViewRule(viewName));
        OCIO_CHECK_EQUAL(std::string(""), cfg->getVirtualDisplayViewDescription(viewName));

        OCIO_REQUIRE_EQUAL(1, cfg->getVirtualDisplayNumViews(OCIO::VIEW_SHARED));
        OCIO_CHECK_EQUAL(std::string("sview2"), cfg->getVirtualDisplayView(OCIO::VIEW_SHARED, 0));

        // Remove a view from the Virtual Display.

        cfg->removeVirtualDisplayView("Raw");

        OCIO_REQUIRE_EQUAL(1, cfg->getVirtualDisplayNumViews(OCIO::VIEW_DISPLAY_DEFINED));
        OCIO_CHECK_EQUAL(std::string("Film"), cfg->getVirtualDisplayView(OCIO::VIEW_DISPLAY_DEFINED, 0));

        OCIO_REQUIRE_EQUAL(1, cfg->getVirtualDisplayNumViews(OCIO::VIEW_SHARED));
        OCIO_CHECK_EQUAL(std::string("sview2"), cfg->getVirtualDisplayView(OCIO::VIEW_SHARED, 0));

        // Remove a shared view from the Virtual Display.

        cfg->removeVirtualDisplayView("sview2");
        OCIO_REQUIRE_EQUAL(1, cfg->getVirtualDisplayNumViews(OCIO::VIEW_DISPLAY_DEFINED));
        OCIO_REQUIRE_EQUAL(0, cfg->getVirtualDisplayNumViews(OCIO::VIEW_SHARED));

        {
            // Extra serialize & deserialize validation.

            std::ostringstream oss2;
            OCIO_CHECK_NO_THROW(oss2 << *cfg.get());

            std::istringstream iss2;
            iss2.str(oss2.str());

            OCIO::ConstConfigRcPtr config2;
            OCIO_CHECK_NO_THROW(config2 = OCIO::Config::CreateFromStream(iss2));

            OCIO_REQUIRE_EQUAL(1, config2->getVirtualDisplayNumViews(OCIO::VIEW_DISPLAY_DEFINED));
            OCIO_REQUIRE_EQUAL(0, config2->getVirtualDisplayNumViews(OCIO::VIEW_SHARED));
        }

        cfg->addVirtualDisplaySharedView("sview2");
        OCIO_REQUIRE_EQUAL(1, cfg->getVirtualDisplayNumViews(OCIO::VIEW_DISPLAY_DEFINED));
        OCIO_REQUIRE_EQUAL(1, cfg->getVirtualDisplayNumViews(OCIO::VIEW_SHARED));

        // Remove the Virtual Display.

        cfg->clearVirtualDisplay();
        OCIO_REQUIRE_EQUAL(0, cfg->getVirtualDisplayNumViews(OCIO::VIEW_DISPLAY_DEFINED));
        OCIO_REQUIRE_EQUAL(0, cfg->getVirtualDisplayNumViews(OCIO::VIEW_SHARED));

        {
            // Extra serialize & deserialize validation.

            std::ostringstream oss2;
            OCIO_CHECK_NO_THROW(oss2 << *cfg.get());

            std::istringstream iss2;
            iss2.str(oss2.str());

            OCIO::ConstConfigRcPtr config2;
            OCIO_CHECK_NO_THROW(config2 = OCIO::Config::CreateFromStream(iss2));

            OCIO_REQUIRE_EQUAL(0, config2->getVirtualDisplayNumViews(OCIO::VIEW_DISPLAY_DEFINED));
            OCIO_REQUIRE_EQUAL(0, config2->getVirtualDisplayNumViews(OCIO::VIEW_SHARED));
        }
    }


    // Step 4 - When present the virtual display instantiation works for MacOS and Windows but
    // throws for headless machines and Linux.

    static const std::string ICCProfileFilepath
        = std::string(OCIO::GetTestFilesDir()) + "/icc-test-1.icc";


#if !defined(OCIO_HEADLESS_ENABLED) && ( defined(__APPLE__) || defined(_WIN32) )

    OCIO_CHECK_ASSERT(OCIO::SystemMonitors::Get()->isSupported());

    const std::string monitorName = OCIO::SystemMonitors::Get()->getMonitorName(0);

    // Step 4 - 1 - Check the virtual display instantiation.

    OCIO::ConfigRcPtr cfg = config->createEditableCopy();
    OCIO_CHECK_NO_THROW(cfg->instantiateDisplayFromMonitorName(monitorName.c_str()));

    OCIO_CHECK_ASSERT((1 + config->getNumDisplays()) == cfg->getNumDisplays());

    // One more display exists in the changed config instance.
    const int numColorSpaces
        = config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_DISPLAY, OCIO::COLORSPACE_ACTIVE);
    OCIO_CHECK_ASSERT((1 + numColorSpaces)
                        == cfg->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_DISPLAY, OCIO::COLORSPACE_ACTIVE));

    // Some basic checks of the new display.

    // New display is the last one.
    const std::string displayName = cfg->getDisplay(config->getNumDisplays());
    OCIO_CHECK_EQUAL(3, cfg->getNumViews(displayName.c_str()));
    OCIO_CHECK_EQUAL(2, cfg->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, displayName.c_str()));
    OCIO_CHECK_EQUAL(1, cfg->getNumViews(OCIO::VIEW_SHARED, displayName.c_str()));

    // Check the created display color space.

    OCIO::ConstColorSpaceRcPtr cs = cfg->getColorSpace(displayName.c_str());
    OCIO_CHECK_ASSERT(cs);
    
    OCIO::ConstTransformRcPtr tr = cs->getTransform(OCIO::COLORSPACE_DIR_TO_REFERENCE);
    OCIO_CHECK_ASSERT(!tr);

    tr = cs->getTransform(OCIO::COLORSPACE_DIR_FROM_REFERENCE);
    OCIO_CHECK_ASSERT(tr);

    OCIO::ConstFileTransformRcPtr file = OCIO::DynamicPtrCast<const OCIO::FileTransform>(tr);
    OCIO_CHECK_ASSERT(file);

    int displayPos = -1;

    // If the display already exists it only udpates existing (display, view) pair and the
    // corresponding display color space.
    OCIO_CHECK_NO_THROW(displayPos = cfg->instantiateDisplayFromMonitorName(monitorName.c_str()));
    OCIO_CHECK_EQUAL(displayPos, config->getNumDisplays()); // Added at the last position.

    OCIO_CHECK_EQUAL((1 + config->getNumDisplays()), cfg->getNumDisplays());

    OCIO_CHECK_EQUAL((1 + numColorSpaces),
                     cfg->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_DISPLAY, OCIO::COLORSPACE_ACTIVE));

    // Check that the (display, view) pairs instantiated from a virtual display are not saved
    // which includes to not save the associated display color spaces.

    {
        std::ostringstream oss2;
        OCIO_CHECK_NO_THROW(oss2 << *cfg.get()); // With an instantiated virtual display.

        std::istringstream iss2;
        iss2.str(oss2.str());

        OCIO::ConstConfigRcPtr config2;
        OCIO_CHECK_NO_THROW(config2 = OCIO::Config::CreateFromStream(iss2));

        // Check that (display, view) pair created by the virtual display instantiation is gone.
 
        OCIO_CHECK_EQUAL(config->getNumDisplays(),  config2->getNumDisplays());
        OCIO_CHECK_EQUAL(cfg->getNumDisplays() - 1, config2->getNumDisplays());

        // And the display color space is also gone.

        OCIO_CHECK_EQUAL(config->getNumColorSpaces(),  config2->getNumColorSpaces());
        OCIO_CHECK_EQUAL(cfg->getNumColorSpaces() - 1, config2->getNumColorSpaces());
    }

    // Step 4 - 2 - Create a (display, view) using a custom ICC profile.

    cfg = config->createEditableCopy(); // Reset the instance to the original content.
    OCIO_CHECK_NO_THROW(displayPos = cfg->instantiateDisplayFromICCProfile(ICCProfileFilepath.c_str()));
    OCIO_CHECK_EQUAL(displayPos, config->getNumDisplays()); // Added at the last position.

    OCIO_CHECK_EQUAL((1 + config->getNumDisplays()), cfg->getNumDisplays());
    OCIO_CHECK_EQUAL((1 + numColorSpaces),
                     cfg->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_DISPLAY, OCIO::COLORSPACE_ACTIVE));

    // Some basic checks of the new display.

    // New display is the last one.
    const std::string customDisplayName = cfg->getDisplay(config->getNumDisplays());
    OCIO_CHECK_EQUAL(3, cfg->getNumViews(customDisplayName.c_str()));
    OCIO_CHECK_EQUAL(2, cfg->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, customDisplayName.c_str()));
    OCIO_CHECK_EQUAL(1, cfg->getNumViews(OCIO::VIEW_SHARED, customDisplayName.c_str()));

#elif !defined(OCIO_HEADLESS_ENABLED) && defined (__linux__)

    OCIO_CHECK_ASSERT(!OCIO::SystemMonitors::Get()->isSupported());

    // There is no uniform way to retrieve the monitor information.
    OCIO_CHECK_THROW_WHAT(OCIO::SystemMonitors::Get()->getMonitorName(0),
                          OCIO::Exception,
                          "Invalid index for the monitor name 0 where the number of monitors is 0.");

    // Step 4 - 2 - Create a (display, view) using a custom ICC profile.

    OCIO::ConfigRcPtr cfg = config->createEditableCopy();
    OCIO_CHECK_NO_THROW(cfg->instantiateDisplayFromICCProfile(ICCProfileFilepath.c_str()));

    OCIO_CHECK_EQUAL((1 + config->getNumDisplays()), cfg->getNumDisplays());

    const int numColorSpaces
        = config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_DISPLAY, OCIO::COLORSPACE_ACTIVE);
    OCIO_CHECK_EQUAL((1 + numColorSpaces),
                     cfg->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_DISPLAY, OCIO::COLORSPACE_ACTIVE));

    // Some basic checks of the new display.

    // New display is the last one.
    const std::string customDisplayName = cfg->getDisplay(config->getNumDisplays());
    OCIO_CHECK_EQUAL(3, cfg->getNumViews(customDisplayName.c_str()));
    OCIO_CHECK_EQUAL(2, cfg->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, customDisplayName.c_str()));
    OCIO_CHECK_EQUAL(1, cfg->getNumViews(OCIO::VIEW_SHARED, customDisplayName.c_str()));

#else

    OCIO_CHECK_ASSERT(!OCIO::SystemMonitors::Get()->isSupported());

#endif

}

OCIO_ADD_TEST(Config, virtual_display_with_active_displays)
{
    // Test the virtual display instantiation when active displays & views are defined.

    static constexpr char CONFIG[]{ R"(ocio_profile_version: 2

roles:
  default: raw

file_rules:
  - !<Rule> {name: Default, colorspace: default}

shared_views:
  - !<View> {name: sview1, colorspace: raw}

displays:
  Raw:
    - !<View> {name: Raw, colorspace: raw}
  sRGB:
    - !<View> {name: Raw, colorspace: raw}
    - !<View> {name: view, view_transform: display_vt, display_colorspace: display_cs}

virtual_display:
  - !<View> {name: Raw, colorspace: raw}
  - !<Views> [sview1]

active_displays: [sRGB]
active_views: [view]

view_transforms:
  - !<ViewTransform>
    name: default_vt
    to_scene_reference: !<CDLTransform> {sat: 1.5}

  - !<ViewTransform>
    name: display_vt
    to_display_reference: !<CDLTransform> {sat: 1.5}

display_colorspaces:
  - !<ColorSpace>
    name: display_cs
    to_display_reference: !<CDLTransform> {sat: 1.5}

colorspaces:
  - !<ColorSpace>
    name: raw
)" };

    std::istringstream iss;
    iss.str(CONFIG);

    // Validate a config containing a virtual display.

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(iss));
    OCIO_CHECK_NO_THROW(config->validate());

    // Only the 'sRGB' display is active.
    OCIO_CHECK_EQUAL(1, config->getNumDisplays());
    // Only the 'view' view is active.
    OCIO_CHECK_EQUAL(1, config->getNumViews("sRGB"));

#if !defined(OCIO_HEADLESS_ENABLED) && ( defined(__APPLE__) || defined(_WIN32) )

    OCIO_CHECK_ASSERT(OCIO::SystemMonitors::Get()->isSupported());

    const std::string monitorName = OCIO::SystemMonitors::Get()->getMonitorName(0);

    // Instantiate a Virtual Display.

    OCIO::ConfigRcPtr cfg = config->createEditableCopy();

    int displayIndex = -1;
    OCIO_CHECK_NO_THROW(displayIndex = cfg->instantiateDisplayFromMonitorName(monitorName.c_str()));

    OCIO_CHECK_EQUAL(2, cfg->getNumDisplays());

    // Now, the views 'Raw' & 'view' are active (Since 'Raw' is used by both the new display and sRGB.)
    OCIO_CHECK_EQUAL(2, cfg->getNumViews("sRGB"));
    // All the views from the new display are active.
    OCIO_CHECK_EQUAL(2, cfg->getNumViews(cfg->getDisplay(displayIndex)));

#endif

}

OCIO_ADD_TEST(Config, virtual_display_v2_only)
{
    // Test that the virtual display is only supported by v2 or higher.

    static constexpr char CONFIG[]{ R"(ocio_profile_version: 1

roles:
  default: raw

displays:
  sRGB:
    - !<View> {name: Raw, colorspace: raw}

virtual_display:
  - !<View> {name: Raw, colorspace: raw}

colorspaces:
  - !<ColorSpace>
    name: raw
)" };

    std::istringstream iss;
    iss.str(CONFIG);

    OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(iss),
                          OCIO::Exception,
                          "Only version 2 (or higher) can have a virtual display.");

    OCIO::ConfigRcPtr cfg = OCIO::Config::CreateRaw()->createEditableCopy();
    cfg->addVirtualDisplaySharedView("sview");
    cfg->setMajorVersion(1);
    cfg->setFileRules(OCIO::FileRules::Create());

    OCIO_CHECK_THROW_WHAT(cfg->validate(),
                          OCIO::Exception,
                          "Only version 2 (or higher) can have a virtual display.");

    std::ostringstream oss;
    OCIO_CHECK_THROW_WHAT(oss << *cfg.get(),
                          OCIO::Exception,
                          "Only version 2 (or higher) can have a virtual display.");
}

OCIO_ADD_TEST(Config, virtual_display_exceptions)
{
    // Test the validations around the virtual display definition.

    static constexpr char CONFIG[]{ R"(ocio_profile_version: 2

roles:
  default: raw

file_rules:
  - !<Rule> {name: Default, colorspace: default}

shared_views:
  - !<View> {name: sview1, colorspace: raw}

displays:
  Raw:
    - !<View> {name: Raw, colorspace: raw}

virtual_display:
  - !<View> {name: Raw, colorspace: raw}
  - !<Views> [sview1]

view_transforms:
  - !<ViewTransform>
    name: default_vt
    to_scene_reference: !<CDLTransform> {sat: 1.5}

  - !<ViewTransform>
    name: display_vt
    to_display_reference: !<CDLTransform> {sat: 1.5}

display_colorspaces:
  - !<ColorSpace>
    name: display_cs
    to_display_reference: !<CDLTransform> {sat: 1.5}

colorspaces:
  - !<ColorSpace>
    name: raw
)" };

    std::istringstream iss;
    iss.str(CONFIG);

    OCIO::ConfigRcPtr cfg;
    OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateFromStream(iss)->createEditableCopy());
    OCIO_CHECK_NO_THROW(cfg->validate());

    // Test failures for shared views.

    OCIO_CHECK_THROW_WHAT(cfg->addVirtualDisplaySharedView("sview1"),
                          OCIO::Exception,
                          "Shared view could not be added to virtual_display: There is already a"
                          " shared view named 'sview1'.");

    OCIO_CHECK_NO_THROW(cfg->addVirtualDisplaySharedView("sview2"));
    OCIO_CHECK_THROW_WHAT(cfg->validate(),
                          OCIO::Exception,
                          "The display 'virtual_display' contains a shared view 'sview2' that is"
                          " not defined.");

    cfg->removeVirtualDisplayView("sview2");
    OCIO_CHECK_NO_THROW(cfg->validate());

    // Test failures for views.

    OCIO_CHECK_THROW_WHAT(cfg->addVirtualDisplayView("Raw", nullptr, "raw", nullptr, nullptr, nullptr),
                          OCIO::Exception,
                          "View could not be added to virtual_display in config: View 'Raw' already"
                          " exists.");

    OCIO_CHECK_NO_THROW(cfg->addVirtualDisplayView("Raw1", nullptr, "raw1", nullptr, nullptr, nullptr));
    OCIO_CHECK_THROW_WHAT(cfg->validate(),
                          OCIO::Exception,
                          "Display 'virtual_display' has a view 'Raw1' that refers to a color space"
                          " or a named transform, 'raw1', which is not defined.");

    cfg->removeVirtualDisplayView("Raw1");
    OCIO_CHECK_NO_THROW(cfg->validate());

    OCIO_CHECK_NO_THROW(cfg->addVirtualDisplayView("Raw1", nullptr, "raw", "look", nullptr, nullptr));
    OCIO_CHECK_THROW_WHAT(cfg->validate(),
                          OCIO::Exception,
                          "Display 'virtual_display' has a view 'Raw1' refers to a look, 'look',"
                          " which is not defined.");
}

OCIO_ADD_TEST(Config, description_and_name)
{
    auto cfg = OCIO::Config::CreateRaw()->createEditableCopy();
    std::ostringstream oss;
    cfg->serialize(oss);
    static constexpr char CONFIG_NO_DESC[]{ R"(ocio_profile_version: 2

environment:
  {}
search_path: ""
strictparsing: false
luma: [0.2126, 0.7152, 0.0722]

roles:
  default: raw

file_rules:
  - !<Rule> {name: Default, colorspace: default}

displays:
  sRGB:
    - !<View> {name: Raw, colorspace: raw}

active_displays: []
active_views: []

colorspaces:
  - !<ColorSpace>
    name: raw
    family: raw
    equalitygroup: ""
    bitdepth: 32f
    description: A raw color space. Conversions to and from this space are no-ops.
    isdata: true
    allocation: uniform
)" };
    OCIO_CHECK_EQUAL(oss.str(), std::string(CONFIG_NO_DESC));

    oss.clear();
    oss.str("");

    cfg->setDescription("single line description");
    cfg->setName("Test config name");

    // Verify name is copied.
    {
        auto cfg2 = cfg->createEditableCopy();
        OCIO_CHECK_EQUAL(std::string(cfg2->getName()), "Test config name");
    }

    cfg->serialize(oss);
    static constexpr char CONFIG_DESC_SINGLELINE[]{ R"(ocio_profile_version: 2

environment:
  {}
search_path: ""
strictparsing: false
luma: [0.2126, 0.7152, 0.0722]
name: Test config name
description: single line description

roles:
  default: raw

file_rules:
  - !<Rule> {name: Default, colorspace: default}

displays:
  sRGB:
    - !<View> {name: Raw, colorspace: raw}

active_displays: []
active_views: []

colorspaces:
  - !<ColorSpace>
    name: raw
    family: raw
    equalitygroup: ""
    bitdepth: 32f
    description: A raw color space. Conversions to and from this space are no-ops.
    isdata: true
    allocation: uniform
)" };
    OCIO_CHECK_EQUAL(oss.str(), std::string(CONFIG_DESC_SINGLELINE));

    oss.clear();
    oss.str("");

    cfg->setDescription("multi line description\n\nother line");
    cfg->setName("");
    cfg->serialize(oss);

    static constexpr char CONFIG_DESC_MULTILINES[]{ R"(ocio_profile_version: 2

environment:
  {}
search_path: ""
strictparsing: false
luma: [0.2126, 0.7152, 0.0722]
description: |
  multi line description
  
  other line

roles:
  default: raw

file_rules:
  - !<Rule> {name: Default, colorspace: default}

displays:
  sRGB:
    - !<View> {name: Raw, colorspace: raw}

active_displays: []
active_views: []

colorspaces:
  - !<ColorSpace>
    name: raw
    family: raw
    equalitygroup: ""
    bitdepth: 32f
    description: A raw color space. Conversions to and from this space are no-ops.
    isdata: true
    allocation: uniform
)" };
    OCIO_CHECK_EQUAL(oss.str(), std::string(CONFIG_DESC_MULTILINES));
}

OCIO_ADD_TEST(Config, alias_validation)
{
    // NB: This tests ColorSpaceSet::addColorSpace.

    auto cfg = OCIO::Config::CreateRaw()->createEditableCopy();
    auto cs = OCIO::ColorSpace::Create();
    cs->setName("colorspace1");
    OCIO_CHECK_NO_THROW(cfg->addColorSpace(cs));
    cs->setName("colorspace2");
    OCIO_CHECK_NO_THROW(cfg->addColorSpace(cs));
    OCIO_CHECK_NO_THROW(cfg->validate());
    cs->setName("colorspace3");
    cs->addAlias("colorspace1");
    OCIO_CHECK_THROW_WHAT(cfg->addColorSpace(cs), OCIO::Exception, "Cannot add 'colorspace3' "
                          "color space, it has 'colorspace1' alias and existing color space, "
                          "'colorspace1' is using the same alias");
    cs->removeAlias("colorspace1");

    OCIO_CHECK_NO_THROW(cfg->setRole("alias", "colorspace2"));
    OCIO_CHECK_NO_THROW(cs->addAlias("alias"));
    OCIO_CHECK_THROW_WHAT(cfg->addColorSpace(cs), OCIO::Exception,
                          "Cannot add 'colorspace3' color space, it has an alias 'alias' and "
                          "there is already a role with this name");
    cs->removeAlias("alias");
    OCIO_CHECK_NO_THROW(cs->addAlias("test%test"));
    OCIO_CHECK_THROW_WHAT(cfg->addColorSpace(cs), OCIO::Exception,
                          "Cannot add 'colorspace3' color space, it has an alias 'test%test' "
                          "that cannot contain a context variable reserved token i.e. % or $");

    cs->removeAlias("test%test");
    OCIO_CHECK_NO_THROW(cs->addAlias("namedtransform"));
    OCIO_CHECK_NO_THROW(cfg->addColorSpace(cs));
    auto nt = OCIO::NamedTransform::Create();
    nt->setTransform(OCIO::MatrixTransform::Create(), OCIO::TRANSFORM_DIR_FORWARD);
    nt->setName("namedtransform");
    OCIO_CHECK_THROW_WHAT(cfg->addNamedTransform(nt), OCIO::Exception,
                          "Cannot add 'namedtransform' named transform, there is already a color "
                          "space using this name as a name or as an alias: 'colorspace3");

    nt->setName("nt");
    OCIO_CHECK_NO_THROW(cfg->addNamedTransform(nt));
    OCIO_CHECK_NO_THROW(cfg->validate());

    nt->addAlias("namedtransform");
    OCIO_CHECK_THROW_WHAT(cfg->addNamedTransform(nt), OCIO::Exception,
                          "Cannot add 'nt' named transform, it has an alias 'namedtransform' and "
                          "there is already a color space using this name as a name or as an "
                          "alias: 'colorspace3'");

    nt->removeAlias("namedtransform");
    nt->addAlias("colorspace3");
    OCIO_CHECK_THROW_WHAT(cfg->addNamedTransform(nt), OCIO::Exception,
                          "Cannot add 'nt' named transform, it has an alias 'colorspace3' and "
                          "there is already a color space using this name as a name or as an "
                          "alias: 'colorspace3'");

    nt->removeAlias("colorspace3");
    nt->addAlias("alias");
    OCIO_CHECK_THROW_WHAT(cfg->addNamedTransform(nt), OCIO::Exception,
                          "Cannot add 'nt' named transform, it has an alias 'alias' and there "
                          "is already a role with this name");

    nt->removeAlias("alias");
    nt->addAlias("test%test");
    OCIO_CHECK_THROW_WHAT(cfg->addNamedTransform(nt), OCIO::Exception,
                          "Cannot add 'nt' named transform, it has an alias 'test%test' that "
                          "cannot contain a context variable reserved token i.e. % or $");
}

OCIO_ADD_TEST(Config, get_processor_alias)
{
    OCIO::ConfigRcPtr config = OCIO::Config::CreateRaw()->createEditableCopy();
    auto csSceneToRef = OCIO::ColorSpace::Create(OCIO::REFERENCE_SPACE_SCENE);
    csSceneToRef->setName("source");
    auto mat = OCIO::MatrixTransform::Create();
    const double offset[4]{ 0., 0.1, 0.2, 0. };
    mat->setOffset(offset);
    csSceneToRef->setTransform(mat, OCIO::COLORSPACE_DIR_TO_REFERENCE);
    csSceneToRef->addAlias("alias source");
    csSceneToRef->addAlias("src");
    OCIO_CHECK_NO_THROW(config->addColorSpace(csSceneToRef));

    auto csSceneFromRef = OCIO::ColorSpace::Create(OCIO::REFERENCE_SPACE_SCENE);
    csSceneFromRef->setName("destination");
    auto ff = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_GLOW_03);
    csSceneFromRef->setTransform(ff, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
    csSceneFromRef->addAlias("alias destination");
    csSceneFromRef->addAlias("dst");
    OCIO_CHECK_NO_THROW(config->addColorSpace(csSceneFromRef));

    OCIO_CHECK_NO_THROW(config->validate());

    OCIO::ConstProcessorRcPtr refProc;
    OCIO_CHECK_NO_THROW(refProc = config->getProcessor("source", "destination"));
    OCIO_REQUIRE_ASSERT(refProc);
    {
        const auto grp = refProc->createGroupTransform();
        OCIO_CHECK_EQUAL(grp->getNumTransforms(), 2);
        OCIO_CHECK_EQUAL(grp->getTransform(0)->getTransformType(), OCIO::TRANSFORM_TYPE_MATRIX);
        OCIO_CHECK_EQUAL(grp->getTransform(1)->getTransformType(),
                         OCIO::TRANSFORM_TYPE_FIXED_FUNCTION);
    }

    {
        OCIO::ConstProcessorRcPtr withAlias;
        OCIO_CHECK_NO_THROW(withAlias = config->getProcessor("alias source", "destination"));
        OCIO_REQUIRE_ASSERT(withAlias);
        // TODO: Resolve the aliases before creating the new processor. Code currently creates a
        // second processor but only keeps the first one because they have the same cacheID.
        OCIO_CHECK_EQUAL(withAlias.get(), refProc.get());
    }

    config->setProcessorCacheFlags(OCIO::PROCESSOR_CACHE_OFF);
    {
        OCIO::ConstProcessorRcPtr withAlias;
        OCIO_CHECK_NO_THROW(withAlias = config->getProcessor("alias source", "destination"));
        OCIO_REQUIRE_ASSERT(withAlias);
        const auto grp = withAlias->createGroupTransform();
        OCIO_CHECK_EQUAL(grp->getNumTransforms(), 2);
        OCIO_CHECK_EQUAL(grp->getTransform(0)->getTransformType(), OCIO::TRANSFORM_TYPE_MATRIX);
        OCIO_CHECK_EQUAL(grp->getTransform(1)->getTransformType(),
                         OCIO::TRANSFORM_TYPE_FIXED_FUNCTION);
    }

    {
        OCIO::ConstProcessorRcPtr withAlias;
        OCIO_CHECK_NO_THROW(withAlias = config->getProcessor("alias source", "dst"));
        OCIO_REQUIRE_ASSERT(withAlias);
        const auto grp = withAlias->createGroupTransform();
        OCIO_CHECK_EQUAL(grp->getNumTransforms(), 2);
        OCIO_CHECK_EQUAL(grp->getTransform(0)->getTransformType(), OCIO::TRANSFORM_TYPE_MATRIX);
        OCIO_CHECK_EQUAL(grp->getTransform(1)->getTransformType(),
                         OCIO::TRANSFORM_TYPE_FIXED_FUNCTION);
    }

    auto nt = OCIO::NamedTransform::Create();
    nt->setName("named_transform");
    nt->addAlias("nt");
    nt->setTransform(OCIO::ExponentTransform::Create(), OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_NO_THROW(config->addNamedTransform(nt));

    {
        OCIO::ConstProcessorRcPtr withAlias;
        OCIO_CHECK_NO_THROW(withAlias = config->getProcessor("nt", "dst"));
        OCIO_REQUIRE_ASSERT(withAlias);
        const auto grp = withAlias->createGroupTransform();
        OCIO_CHECK_EQUAL(grp->getNumTransforms(), 1);
        OCIO_CHECK_EQUAL(grp->getTransform(0)->getTransformType(), OCIO::TRANSFORM_TYPE_EXPONENT);
    }

    config->addDisplayView("display", "view", "alias destination", nullptr);

    {
        OCIO::ConstProcessorRcPtr withAlias;
        OCIO_CHECK_NO_THROW(withAlias = config->getProcessor("alias source", "display", "view",
                                                             OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_REQUIRE_ASSERT(withAlias);
        const auto grp = withAlias->createGroupTransform();
        OCIO_CHECK_EQUAL(grp->getNumTransforms(), 2);
        OCIO_CHECK_EQUAL(grp->getTransform(0)->getTransformType(), OCIO::TRANSFORM_TYPE_MATRIX);
        OCIO_CHECK_EQUAL(grp->getTransform(1)->getTransformType(),
                         OCIO::TRANSFORM_TYPE_FIXED_FUNCTION);
    }
}
