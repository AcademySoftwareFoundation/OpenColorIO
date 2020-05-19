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
    OCIO_CHECK_NO_THROW(config->sanityCheck());
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 1);
    OCIO_CHECK_EQUAL(std::string(config->getColorSpaceNameByIndex(0)), std::string("raw"));

    OCIO::ConstProcessorRcPtr proc;
    OCIO_CHECK_NO_THROW(proc = config->getProcessor("raw", "raw"));
    OCIO_CHECK_NO_THROW(proc->getDefaultCPUProcessor());

    OCIO_CHECK_THROW_WHAT(config->getProcessor("not_found", "raw"), OCIO::Exception,
                          "Could not find source color space");
    OCIO_CHECK_THROW_WHAT(config->getProcessor("raw", "not_found"), OCIO::Exception,
                          "Could not find destination color space");
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
    OCIO_CHECK_NO_THROW(config->sanityCheck());
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
        "    to_reference: !<CDLTransform> {slope: [1, 2, 1], slope: [1, 2, 1]}\n"
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
        OCIO::FileTransformRcPtr transform1 = \
            OCIO::FileTransform::Create();
        OCIO::GroupTransformRcPtr groupTransform = OCIO::GroupTransform::Create();
        groupTransform->appendTransform(transform1);
        OCIO_CHECK_NO_THROW(cs->setTransform(groupTransform, OCIO::COLORSPACE_DIR_FROM_REFERENCE));
        config->addColorSpace(cs);
        config->setRole( OCIO::ROLE_COMPOSITING_LOG, cs->getName() );
    }
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("testing2");
        cs->setFamily("test");
        OCIO::ExponentTransformRcPtr transform1 = \
            OCIO::ExponentTransform::Create();
        OCIO::GroupTransformRcPtr groupTransform = OCIO::GroupTransform::Create();
        groupTransform->appendTransform(transform1);
        OCIO_CHECK_NO_THROW(cs->setTransform(groupTransform, OCIO::COLORSPACE_DIR_TO_REFERENCE));
        config->addColorSpace(cs);
        config->setRole( OCIO::ROLE_COMPOSITING_LOG, cs->getName() );
    }

    std::ostringstream os;
    config->serialize(os);

    std::string PROFILE_OUT =
    "ocio_profile_version: 1\n"
    "\n"
    "search_path: \"\"\n"
    "strictparsing: true\n"
    "luma: [0.2126, 0.7152, 0.0722]\n"
    "\n"
    "roles:\n"
    "  compositing_log: testing2\n"
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
    "    from_reference: !<GroupTransform>\n"
    "      children:\n"
    "        - !<FileTransform> {src: \"\"}\n"
    "\n"
    "  - !<ColorSpace>\n"
    "    name: testing2\n"
    "    family: test\n"
    "    equalitygroup: \"\"\n"
    "    bitdepth: unknown\n"
    "    isdata: false\n"
    "    allocation: uniform\n"
    "    to_reference: !<GroupTransform>\n"
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

        std::ostringstream os;
        config->serialize(os);

        std::string PROFILE_OUT =
            "ocio_profile_version: 1\n"
            "\n"
            "search_path: \"\"\n"
            "strictparsing: true\n"
            "luma: [0.2126, 0.7152, 0.0722]\n"
            "\n"
            "roles:\n"
            "  {}\n"
            "\n"
            "displays:\n"
            "  {}\n"
            "\n"
            "active_displays: []\n"
            "active_views: []\n"
            "\n"
            "colorspaces:\n"
            "  []";

        const StringUtils::StringVec osvec          = StringUtils::SplitByLines(os.str());
        const StringUtils::StringVec PROFILE_OUTvec = StringUtils::SplitByLines(PROFILE_OUT);

        OCIO_CHECK_EQUAL(osvec.size(), PROFILE_OUTvec.size());
        for (unsigned int i = 0; i < PROFILE_OUTvec.size(); ++i)
            OCIO_CHECK_EQUAL(osvec[i], PROFILE_OUTvec[i]);
    }

    {
        OCIO::ConfigRcPtr config = OCIO::Config::Create();

        std::string searchPath("a:b:c");
        config->setSearchPath(searchPath.c_str());

        std::ostringstream os;
        config->serialize(os);

        StringUtils::StringVec osvec = StringUtils::SplitByLines(os.str());

        const std::string expected1{ "search_path: a:b:c" };
        OCIO_CHECK_EQUAL(osvec[2], expected1);

        config->setMajorVersion(2);
        os.clear();
        os.str("");
        config->serialize(os);

        osvec = StringUtils::SplitByLines(os.str());

        const std::string expected2[] = { "search_path:", "  - a", "  - b", "  - c" };
        OCIO_CHECK_EQUAL(osvec[2], expected2[0]);
        OCIO_CHECK_EQUAL(osvec[3], expected2[1]);
        OCIO_CHECK_EQUAL(osvec[4], expected2[2]);
        OCIO_CHECK_EQUAL(osvec[5], expected2[3]);

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
        OCIO_CHECK_EQUAL(osvec[2], expected3[0]);
        OCIO_CHECK_EQUAL(osvec[3], expected3[1]);
        OCIO_CHECK_EQUAL(osvec[4], expected3[2]);
        OCIO_CHECK_EQUAL(osvec[5], expected3[3]);
        OCIO_CHECK_EQUAL(osvec[6], expected3[4]);

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

OCIO_ADD_TEST(Config, sanity_check)
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

    OCIO_CHECK_NO_THROW(config->sanityCheck());
    }
}


OCIO_ADD_TEST(Config, env_check)
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

    OCIO::Platform::Setenv("SHOW", "bar");
    OCIO::Platform::Setenv("TASK", "lighting");

    std::istringstream is;
    is.str(SIMPLE_PROFILE);
    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_EQUAL(config->getNumEnvironmentVars(), 5);
    OCIO_CHECK_ASSERT(strcmp(config->getCurrentContext()->resolveStringVar("test${test}"),
        "testbarchedder") == 0);
    OCIO_CHECK_ASSERT(strcmp(config->getCurrentContext()->resolveStringVar("${SHOW}"),
        "bar") == 0);
    OCIO_CHECK_ASSERT(strcmp(config->getEnvironmentVarDefault("SHOW"), "super") == 0);

    OCIO::ConfigRcPtr edit = config->createEditableCopy();
    edit->clearEnvironmentVars();
    OCIO_CHECK_EQUAL(edit->getNumEnvironmentVars(), 0);

    edit->addEnvironmentVar("testing", "dupvar");
    edit->addEnvironmentVar("testing", "dupvar");
    edit->addEnvironmentVar("foobar", "testing");
    edit->addEnvironmentVar("blank", "");
    edit->addEnvironmentVar("dontadd", NULL);
    OCIO_CHECK_EQUAL(edit->getNumEnvironmentVars(), 3);
    edit->addEnvironmentVar("foobar", NULL); // remove
    OCIO_CHECK_EQUAL(edit->getNumEnvironmentVars(), 2);
    edit->clearEnvironmentVars();

    edit->addEnvironmentVar("SHOW", "super");
    edit->addEnvironmentVar("SHOT", "test");
    edit->addEnvironmentVar("SEQ", "foo");
    edit->addEnvironmentVar("test", "bar${cheese}");
    edit->addEnvironmentVar("cheese", "chedder");

    // As a warning message is expected, please mute it.
    OCIO::MuteLogging mute;

    // Test
    OCIO::LoggingLevel loglevel = OCIO::GetLoggingLevel();
    OCIO::SetLoggingLevel(OCIO::LOGGING_LEVEL_DEBUG);
    is.str(SIMPLE_PROFILE2);
    OCIO::ConstConfigRcPtr noenv;
    OCIO_CHECK_NO_THROW(noenv = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_ASSERT(strcmp(noenv->getCurrentContext()->resolveStringVar("${TASK}"),
        "lighting") == 0);
    OCIO::SetLoggingLevel(loglevel);

    OCIO_CHECK_EQUAL(edit->getEnvironmentMode(), OCIO::ENV_ENVIRONMENT_LOAD_PREDEFINED);
    edit->setEnvironmentMode(OCIO::ENV_ENVIRONMENT_LOAD_ALL);
    OCIO_CHECK_EQUAL(edit->getEnvironmentMode(), OCIO::ENV_ENVIRONMENT_LOAD_ALL);
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
        OCIO_CHECK_THROW_WHAT(config->sanityCheck(), OCIO::Exception,
                              "This config references a color space, '$MISSING_ENV', "
                              "which is not defined");
        OCIO_CHECK_THROW_WHAT(config->getProcessor("raw", "lgh"), OCIO::Exception,
                              "BuildColorSpaceOps failed: destination color space '$MISSING_ENV' "
                              "could not be found");
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
        OCIO_CHECK_THROW_WHAT(config->sanityCheck(), OCIO::Exception,
                              "color space, 'FaultyColorSpaceName', which is not defined");
        OCIO_CHECK_THROW_WHAT(config->getProcessor("raw", "lgh"), OCIO::Exception,
                              "BuildColorSpaceOps failed: destination color space '$OCIO_TEST' "
                              "could not be found");
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
        OCIO_CHECK_NO_THROW(config->sanityCheck());
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
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), myConfigStr);
    }
}

OCIO_ADD_TEST(Config, version)
{
    const std::string SIMPLE_PROFILE =
        "ocio_profile_version: 2\n"
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

    OCIO_CHECK_NO_THROW(config->sanityCheck());

    OCIO_CHECK_NO_THROW(config->setMajorVersion(1));
    OCIO_CHECK_THROW_WHAT(config->setMajorVersion(20000), OCIO::Exception,
                          "version is 20000 where supported versions start at 1 and end at 2");

    {
        OCIO_CHECK_NO_THROW(config->setMinorVersion(2));
        OCIO_CHECK_NO_THROW(config->setMinorVersion(20));

        std::stringstream ss;
        ss << *config.get();   
        StringUtils::StartsWith(StringUtils::Lower(ss.str()), "ocio_profile_version: 2.20");
    }

    {
        OCIO_CHECK_NO_THROW(config->setMinorVersion(0));

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());   
        StringUtils::StartsWith(StringUtils::Lower(ss.str()), "ocio_profile_version: 2");
    }

    {
        OCIO_CHECK_NO_THROW(config->setMinorVersion(1));

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());   
        StringUtils::StartsWith(StringUtils::Lower(ss.str()), "ocio_profile_version: 1");
    }
}

OCIO_ADD_TEST(Config, version_faulty_1)
{
    const std::string SIMPLE_PROFILE =
        "ocio_profile_version: 2.0.1\n"
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
    OCIO_CHECK_THROW_WHAT(config = OCIO::Config::CreateFromStream(is), OCIO::Exception,
                          "does not appear to have a valid version 2.0.1");
}

namespace
{

const std::string PROFILE_V1 = "ocio_profile_version: 1\n";

const std::string PROFILE_V2 = "ocio_profile_version: 2\n";

const std::string SIMPLE_PROFILE_A =
    "\n"
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
    "    - !<View> {name: Raw, colorspace: raw}\n"
    "    - !<View> {name: Lnh, colorspace: lnh, looks: beauty}\n"
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

const std::string SIMPLE_PROFILE_CS =
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

const std::string SIMPLE_PROFILE_B = SIMPLE_PROFILE_DISPLAYS_LOOKS + SIMPLE_PROFILE_CS;

const std::string DEFAULT_RULES =
    "file_rules:\n"
    "  - !<Rule> {name: Default, colorspace: default}\n"
    "\n";

const std::string PROFILE_V2_START = PROFILE_V2 + SIMPLE_PROFILE_A +
                                     DEFAULT_RULES + SIMPLE_PROFILE_B;
}

OCIO_ADD_TEST(Config, range_serialization)
{
    {
        const std::string strEnd =
            "    from_reference: !<RangeTransform> {minInValue: 0, minOutValue: 0}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        const std::string strEnd =
            "    from_reference: !<RangeTransform> {minInValue: 0, minOutValue: 0, "
            "direction: inverse}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        const std::string strEnd =
            "    from_reference: !<RangeTransform> {minInValue: 0, minOutValue: 0, "
            "style: noClamp}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_THROW_WHAT(config->sanityCheck(), OCIO::Exception,
                              "non clamping range must have min and max values defined");
    }

    {
        const std::string strEnd =
            "    from_reference: !<RangeTransform> {minInValue: 0, maxInValue: 1, "
            "minOutValue: 0, maxOutValue: 1, style: noClamp, direction: inverse}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // Test Range with clamp style (i.e. default one)
        const std::string strEnd =
            "    from_reference: !<RangeTransform> {minInValue: -0.0109, "
            "maxInValue: 1.0505, minOutValue: 0.0009, maxOutValue: 2.5001, "
            "direction: inverse}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // Test Range with clamp style
        const std::string in_strEnd =
            "    from_reference: !<RangeTransform> {minInValue: -0.0109, "
            "maxInValue: 1.0505, minOutValue: 0.0009, maxOutValue: 2.5001, "
            "style: Clamp, direction: inverse}\n";
        const std::string in_str = PROFILE_V2_START + in_strEnd;

        std::istringstream is;
        is.str(in_str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        // Clamp style is not saved
        const std::string out_strEnd =
            "    from_reference: !<RangeTransform> {minInValue: -0.0109, "
            "maxInValue: 1.0505, minOutValue: 0.0009, maxOutValue: 2.5001, "
            "direction: inverse}\n";
        const std::string out_str = PROFILE_V2_START + out_strEnd;

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), out_str);
    }

    {
        const std::string strEnd =
            "    from_reference: !<RangeTransform> "
            "{minInValue: 0, maxOutValue: 1}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_THROW_WHAT(config->sanityCheck(), 
                              OCIO::Exception, 
                              "must be both set or both missing");

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // maxInValue has an illegal second number.
        const std::string strEndFail =
            "    from_reference: !<RangeTransform> {minInValue: -0.01, "
            "maxInValue: 1.05  10, minOutValue: 0.0009, maxOutValue: 2.5}\n";
        const std::string strEnd =
            "    from_reference: !<RangeTransform> {minInValue: -0.01, "
            "maxInValue: 1.05, minOutValue: 0.0009, maxOutValue: 2.5}\n";

        const std::string str = PROFILE_V2 + SIMPLE_PROFILE_A + SIMPLE_PROFILE_B + strEndFail;
        const std::string strSaved = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is),
                              OCIO::Exception, "parsing double failed");

        is.str(strSaved);
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        // Re-serialize and test that it matches the expected text.
        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), strSaved);
    }

    {
        // maxInValue & maxOutValue have no value, they will not be defined.
        const std::string strEnd =
            "    from_reference: !<RangeTransform> {minInValue: -0.01, "
            "maxInValue: , minOutValue: -0.01, maxOutValue: }\n";
        const std::string strEndSaved =
            "    from_reference: !<RangeTransform> {minInValue: -0.01, "
            "minOutValue: -0.01}\n";
        const std::string str = PROFILE_V2 + SIMPLE_PROFILE_A + SIMPLE_PROFILE_B + strEnd;
        const std::string strSaved = PROFILE_V2_START + strEndSaved;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        // Re-serialize and test that it matches the expected text.
        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), strSaved);
    }

    {
        const std::string strEnd =
            "    from_reference: !<RangeTransform> "
            "{minInValue: 0.12345678901234, maxOutValue: 1.23456789012345}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_THROW_WHAT(config->sanityCheck(),
            OCIO::Exception,
            "must be both set or both missing");

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        const std::string strEnd =
            "    from_reference: !<RangeTransform> {minInValue: -0.01, "
            "maxInValue: 1.05, minOutValue: 0.0009, maxOutValue: 2.5}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        // Re-serialize and test that it matches the original text.
        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        const std::string strEnd =
            "    from_reference: !<RangeTransform> {minOutValue: 0.0009, "
            "maxOutValue: 2.5}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_THROW_WHAT(config->sanityCheck(),
                              OCIO::Exception,
                              "must be both set or both missing");

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        const std::string strEnd =
            "    from_reference: !<GroupTransform>\n"
            "      children:\n"
            "        - !<RangeTransform> {minInValue: -0.01, maxInValue: 1.05, "
            "minOutValue: 0.0009, maxOutValue: 2.5}\n"
            "        - !<RangeTransform> {minOutValue: 0.0009, maxOutValue: 2.1}\n"
            "        - !<RangeTransform> {minOutValue: 0.1, maxOutValue: 0.9}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_THROW_WHAT(config->sanityCheck(),
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
            "    from_reference: !<GroupTransform>\n"
            "      children:\n"
            // missing { (and mInValue is wrong -> that's a warning)
            "        - !<RangeTransform> mInValue: -0.01, maxInValue: 1.05, "
            "minOutValue: 0.0009, maxOutValue: 2.5}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is),
                              OCIO::Exception,
                              "Loading the OCIO profile failed");
    }

    {
        const std::string strEnd =
            // The comma is missing after the minInValue value.
            "    from_reference: !<RangeTransform> {minInValue: -0.01 "
            "maxInValue: 1.05, minOutValue: 0.0009, maxOutValue: 2.5}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is),
                              OCIO::Exception,
                              "Loading the OCIO profile failed");
    }

    {
        const std::string strEnd =
            "    from_reference: !<RangeTransform> {minInValue: -0.01, "
            // The comma is missing between the minOutValue value and
            // the maxOutValue tag.
            "maxInValue: 1.05, minOutValue: 0.0009maxOutValue: 2.5}\n";
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
    const std::string SIMPLE_PROFILE = SIMPLE_PROFILE_A + SIMPLE_PROFILE_B;
    {
        const std::string strEnd = 
            "    from_reference: !<ExponentTransform> " 
            "{value: [1.101, 1.202, 1.303, 1.404]}\n";  
        const std::string str = PROFILE_V1 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;  
        OCIO_CHECK_NO_THROW(ss << *config.get());    
        OCIO_CHECK_EQUAL(ss.str(), str);    
    }

    {
        const std::string strEnd =
            "    from_reference: !<ExponentTransform> "
            "{value: 1.101}\n";
        const std::string str = PROFILE_V1 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

     {
        const std::string strEnd =  
            "    from_reference: !<ExponentTransform> " 
            "{value: [1.101, 1.202, 1.303, 1.404], direction: inverse}\n";  
        const std::string str = PROFILE_V1 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str); 
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;  
        OCIO_CHECK_NO_THROW(ss << *config.get());    
        OCIO_CHECK_EQUAL(ss.str(), str);    
    }

     {
         const std::string strEnd =
             "    from_reference: !<ExponentTransform> "
             "{value: [1.101, 1.202, 1.303, 1.404], style: mirror, direction: inverse}\n";
         const std::string str = PROFILE_V2_START + strEnd;

         std::istringstream is;
         is.str(str);
         OCIO::ConstConfigRcPtr config;
         OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
         OCIO_CHECK_NO_THROW(config->sanityCheck());

         std::stringstream ss;
         OCIO_CHECK_NO_THROW(ss << *config.get());
         OCIO_CHECK_EQUAL(ss.str(), str);
     }

     {
         const std::string strEnd =
             "    from_reference: !<ExponentTransform> "
             "{value: [1.101, 1.202, 1.303, 1.404], style: pass_thru, direction: inverse}\n";
         const std::string str = PROFILE_V2_START + strEnd;

         std::istringstream is;
         is.str(str);
         OCIO::ConstConfigRcPtr config;
         OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
         OCIO_CHECK_NO_THROW(config->sanityCheck());

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
        const std::string str = PROFILE_V1 + SIMPLE_PROFILE + strEnd;

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
        const std::string str = PROFILE_V1 + SIMPLE_PROFILE + strEnd;

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
            "    from_reference: !<ExponentWithLinearTransform> "
            "{gamma: [1.1, 1.2, 1.3, 1.4], offset: [0.101, 0.102, 0.103, 0.1]}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        const std::string strEnd =
            "    from_reference: !<ExponentWithLinearTransform> "
            "{gamma: [1.1, 1.2, 1.3, 1.4], offset: [0.101, 0.102, 0.103, 0.1], style: mirror}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        const std::string strEnd =
            "    from_reference: !<ExponentWithLinearTransform> "
            "{gamma: [1.1, 1.2, 1.3, 1.4], offset: [0.101, 0.102, 0.103, 0.1], "
            "direction: inverse}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str().size(), str.size());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        const std::string strEnd =
            "    from_reference: !<ExponentWithLinearTransform> "
            "{gamma: [1.1, 1.2, 1.3, 1.4], offset: [0.101, 0.102, 0.103, 0.1], style: mirror, "
            "direction: inverse}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        const std::string strEnd =
            "    from_reference: !<ExponentWithLinearTransform> "
            "{gamma: 1.1, offset: 0.101, "
            "direction: inverse}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str().size(), str.size());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    // Errors

    {
        const std::string strEnd =
            "    from_reference: !<ExponentWithLinearTransform> {}\n";
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
            "    from_reference: !<ExponentWithLinearTransform> "
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
            "    from_reference: !<ExponentWithLinearTransform> "
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
            "    from_reference: !<ExponentWithLinearTransform> "
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
            "    from_reference: !<ExponentWithLinearTransform> "
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
            "    from_reference: !<ExponentWithLinearTransform> "
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
    const std::string str = PROFILE_V1 + SIMPLE_PROFILE_A + SIMPLE_PROFILE_B + strEnd;

    is.str(str);
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->sanityCheck());

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
    const std::string str2 = PROFILE_V1 + SIMPLE_PROFILE_A + SIMPLE_PROFILE_B + strEnd2;

    is.str(str2);
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->sanityCheck());

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
    OCIO_CHECK_NO_THROW(config->sanityCheck());

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
    OCIO_CHECK_NO_THROW(config->sanityCheck());

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
        "    allocation: uniform\n"
        "    allocationvars: [-0.125, 1.125]\n";

    std::istringstream is;
    is.str(MY_OCIO_CONFIG);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->sanityCheck());

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
    cs = config->getColorSpace("raw2");
    OCIO_CHECK_EQUAL(std::string(cs->getName()), std::string("raw2"));
}

OCIO_ADD_TEST(Config, display)
{
    // Guard to automatically unset the env. variable.
    class Guard
    {
    public:
        Guard() = default;
        ~Guard()
        {
            OCIO::Platform::Setenv(OCIO::OCIO_ACTIVE_DISPLAYS_ENVVAR, "");
        }
    } guard;


    static const std::string SIMPLE_PROFILE_HEADER =
        "ocio_profile_version: 2\n"
        "\n"
        "search_path: luts\n"
        "strictparsing: true\n"
        "luma: [0.2126, 0.7152, 0.0722]\n"
        "\n"
        "roles:\n"
        "  default: raw\n"
        "  scene_linear: lnh\n"
        "\n"
        "file_rules:\n"
        "  - !<Rule> {name: ColorSpaceNamePathSearch}\n"
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
        OCIO_CHECK_NO_THROW(config->sanityCheck());

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
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        OCIO_REQUIRE_EQUAL(config->getNumDisplays(), 1);
        OCIO_CHECK_EQUAL(std::string(config->getDisplay(0)), std::string("sRGB_1"));
        OCIO_CHECK_EQUAL(std::string(config->getDefaultDisplay()), "sRGB_1");
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
        OCIO_CHECK_NO_THROW(config->sanityCheck());

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
        OCIO_CHECK_NO_THROW(config->sanityCheck());

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
        OCIO_CHECK_NO_THROW(config->sanityCheck());

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
        OCIO_CHECK_NO_THROW(config->sanityCheck());

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
        OCIO_CHECK_THROW_WHAT(config->sanityCheck(),
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
        OCIO_CHECK_THROW_WHAT(config->sanityCheck(),
                              OCIO::Exception,
                              "The content of the env. variable for the list of active displays"\
                              " [sRGB_2, sRGB_1, ABCDEF] contains invalid display name(s).");
    }

    {
        // Test an unknown display name in the config active displays.

        OCIO::Platform::Setenv(OCIO::OCIO_ACTIVE_DISPLAYS_ENVVAR, ""); // Unset the env. variable.

        const std::string myProfile = 
            SIMPLE_PROFILE_HEADER
            +
            "active_displays: [ABCDEF]\n"
            "active_views: []\n"
            + SIMPLE_PROFILE_FOOTER;

        std::istringstream is(myProfile);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_THROW_WHAT(config->sanityCheck(),
                              OCIO::Exception,
                              "The list of active displays [ABCDEF] from the config file is invalid.");
    }

    {
        // Test an unknown display name in the config active displays.

        OCIO::Platform::Setenv(OCIO::OCIO_ACTIVE_DISPLAYS_ENVVAR, ""); // Unset the env. variable.

        const std::string myProfile = 
            SIMPLE_PROFILE_HEADER
            +
            "active_displays: [sRGB_2, sRGB_1, ABCDEF]\n"
            "active_views: []\n"
            + SIMPLE_PROFILE_FOOTER;

        std::istringstream is(myProfile);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_THROW_WHAT(config->sanityCheck(),
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
            OCIO::Platform::Setenv(OCIO::OCIO_ACTIVE_VIEWS_ENVVAR, "");
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
          - !<Rule> {name: ColorSpaceNamePathSearch}
          - !<Rule> {name: Default, colorspace: raw}
        )" };

    std::istringstream is(SIMPLE_CONFIG);
    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->sanityCheck());

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
        // Log with default base value and default direction.
        const std::string strEnd =
            "    from_reference: !<LogTransform> {}\n";
        const std::string str = PROFILE_V1 + SIMPLE_PROFILE_A + SIMPLE_PROFILE_B + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // Log with default base value.
        const std::string strEnd =
            "    from_reference: !<LogTransform> {direction: inverse}\n";
        const std::string str = PROFILE_V1 + SIMPLE_PROFILE_A + SIMPLE_PROFILE_B + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // Log with specified base value.
        const std::string strEnd =
            "    from_reference: !<LogTransform> {base: 5}\n";
        const std::string str = PROFILE_V1 + SIMPLE_PROFILE_A + SIMPLE_PROFILE_B + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // Log with specified base value and direction.
        const std::string strEnd =
            "    from_reference: !<LogTransform> {base: 7, direction: inverse}\n";
        const std::string str = PROFILE_V1 + SIMPLE_PROFILE_A + SIMPLE_PROFILE_B + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // LogAffine with specified values 3 components.
        const std::string strEnd =
            "    from_reference: !<LogAffineTransform> {"
            "base: 10, "
            "logSideSlope: [1.3, 1.4, 1.5], "
            "logSideOffset: [0, 0, 0.1], "
            "linSideSlope: [1, 1, 1.1], "
            "linSideOffset: [0.1234567890123, 0.5, 0.1]}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // LogAffine with default value for base.
        const std::string strEnd =
            "    from_reference: !<LogAffineTransform> {"
            "logSideSlope: [1, 1, 1.1], "
            "logSideOffset: [0.1234567890123, 0.5, 0.1], "
            "linSideSlope: [1.3, 1.4, 1.5], "
            "linSideOffset: [0, 0, 0.1]}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // LogAffine with single value for linSideOffset.
        const std::string strEnd =
            "    from_reference: !<LogAffineTransform> {"
            "base: 10, "
            "logSideSlope: [1, 1, 1.1], "
            "logSideOffset: [0.1234567890123, 0.5, 0.1], "
            "linSideSlope: [1.3, 1.4, 1.5], "
            "linSideOffset: 0.5}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // LogAffine with single value for linSideSlope.
        const std::string strEnd =
            "    from_reference: !<LogAffineTransform> {"
            "logSideSlope: [1, 1, 1.1], "
            "linSideSlope: 1.3, "
            "linSideOffset: [0, 0, 0.1]}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // LogAffine with single value for logSideOffset.
        const std::string strEnd =
            "    from_reference: !<LogAffineTransform> {"
            "logSideSlope: [1, 1, 1.1], "
            "logSideOffset: 0.5, "
            "linSideSlope: [1.3, 1, 1], "
            "linSideOffset: [0, 0, 0.1]}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // LogAffine with single value for logSideSlope.
        const std::string strEnd =
            "    from_reference: !<LogAffineTransform> {"
            "logSideSlope: 1.1, "
            "logSideOffset: [0.5, 0, 0], "
            "linSideSlope: [1.3, 1, 1], "
            "linSideOffset: [0, 0, 0.1]}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // LogAffine with default value for logSideSlope.
        const std::string strEnd =
            "    from_reference: !<LogAffineTransform> {"
            "logSideOffset: [0.1234567890123, 0.5, 0.1], "
            "linSideSlope: [1.3, 1.4, 1.5], "
            "linSideOffset: [0.1, 0, 0]}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // LogAffine with default value for all but base.
        const std::string strEnd =
            "    from_reference: !<LogAffineTransform> {base: 10}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // LogAffine with wrong size for logSideSlope.
        const std::string strEnd =
            "    from_reference: !<LogAffineTransform> {"
            "logSideSlope: [1, 1], "
            "logSideOffset: [0.1234567890123, 0.5, 0.1]}\n";
        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_THROW_WHAT(config = OCIO::Config::CreateFromStream(is),
                              OCIO::Exception,
                              "logSideSlope value field must have 3 components");
    }

    {
        // LogAffine with 3 values for base.
        const std::string strEnd =
            "    from_reference: !<LogAffineTransform> {"
            "base: [2, 2, 2], "
            "logSideOffset: [0.1234567890123, 0.5, 0.1]}\n";
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
            "    from_reference: !<LogCameraTransform> {"
            "logSideSlope: [1, 1, 1.1], "
            "logSideOffset: [0.1234567890123, 0.5, 0.1], "
            "linSideSlope: [1.3, 1.4, 1.5], "
            "linSideOffset: [0, 0, 0.1], "
            "linSideBreak: [0.1, 0.2, 0.3]}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // LogCamera with default values and identical linSideBreak.
        const std::string strEnd =
            "    from_reference: !<LogCameraTransform> {"
            "linSideBreak: 0.2}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // LogCamera with linear slope.
        const std::string strEnd =
            "    from_reference: !<LogCameraTransform> {"
            "linSideBreak: 0.2, "
            "linearSlope: [1.1, 0.9, 1.2]}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // LogCamera with missing linSideBreak.
        const std::string strEnd =
            "    from_reference: !<LogCameraTransform> {"
            "base: 5}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                              "linSideBreak values are missing");
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
        "    to_reference: !<MatrixTransform> \n"
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
                     "[OpenColorIO Warning]: At line 45, unknown key 'dummyKey' in 'ColorSpace'."));
}

OCIO_ADD_TEST(Config, fixed_function_serialization)
{
    {
        const std::string strEnd =
            "    from_reference: !<GroupTransform>\n"
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
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        // Write the config.

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        const std::string strEnd =
            "    from_reference: !<GroupTransform>\n"
            "      children:\n"
            "        - !<FixedFunctionTransform> {style: ACES_DarkToDim10, params: [0.75]}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_THROW_WHAT(config->sanityCheck(), OCIO::Exception,
            "The style 'ACES_DarkToDim10 (Forward)' must have zero parameters but 1 found.");
    }

    {
        const std::string strEnd =
            "    from_reference: !<GroupTransform>\n"
            "      children:\n"
            "        - !<FixedFunctionTransform> {style: REC2100_Surround, direction: inverse}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_THROW_WHAT(config->sanityCheck(), OCIO::Exception, 
            "The style 'REC2100_Surround (Inverse)' must "
                              "have one parameter but 0 found.");
    }
}

OCIO_ADD_TEST(Config, exposure_contrast_serialization)
{
    {
        const std::string strEnd =
            "    from_reference: !<GroupTransform>\n"
            "      children:\n"
            "        - !<ExposureContrastTransform> {style: video, exposure: 1.5,"
                       " contrast: 0.5, gamma: 1.1, pivot: 0.18}\n"
            "        - !<ExposureContrastTransform> {style: video,"
                       " exposure: {value: 1.5, dynamic: true}, contrast: 0.5,"
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
                       " contrast: {value: 0.6, dynamic: true}, gamma: 1.2,"
                       " pivot: 0.18}\n"
            "        - !<ExposureContrastTransform> {style: linear, exposure: 1.5,"
                       " contrast: 0.5, gamma: 1.1, pivot: 0.18}\n"
            "        - !<ExposureContrastTransform> {style: linear, exposure: 1.5,"
                       " contrast: 0.5, gamma: 1.1, pivot: 0.18,"
                       " direction: inverse}\n"
            "        - !<ExposureContrastTransform> {style: linear, exposure: 1.5,"
                       " contrast: 0.5, gamma: {value: 1.1, dynamic: true},"
                       " pivot: 0.18}\n";

        const std::string str = PROFILE_V2_START + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        const std::string strEnd =
            "    from_reference: !<GroupTransform>\n"
            "      children:\n";

        const std::string strEndEC =
            "        - !<ExposureContrastTransform> {style: video,"
                       " exposure: {value: 1.5},"
                       " contrast: {value: 0.5, dynamic: false},"
                       " gamma: {value: 1.1}, pivot: 0.18}\n";

        const std::string strEndECExpected =
            "        - !<ExposureContrastTransform> {style: video, exposure: 1.5,"
                       " contrast: 0.5, gamma: 1.1, pivot: 0.18}\n";

        const std::string str = PROFILE_V2_START + strEnd + strEndEC;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        const std::string strExpected = PROFILE_V2_START + strEnd + strEndECExpected;

        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), strExpected);

    }

    {
        const std::string strEnd =
            "    from_reference: !<GroupTransform>\n"
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

    const std::string str = PROFILE_V1 + SIMPLE_PROFILE_A + SIMPLE_PROFILE_B + strEnd;

    std::istringstream is;
    is.str(str);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->sanityCheck());

    std::stringstream ss;
    OCIO_CHECK_NO_THROW(ss << *config.get());
    OCIO_CHECK_EQUAL(ss.str(), str);
}

OCIO_ADD_TEST(Config, cdl_serialization)
{
    // Config v2.
    {
        const std::string strEnd =
            "    from_reference: !<GroupTransform>\n"
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
        OCIO_CHECK_NO_THROW(config->sanityCheck());

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

        const std::string str = PROFILE_V1 + SIMPLE_PROFILE_A + SIMPLE_PROFILE_B + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        std::ostringstream oss;
        OCIO_CHECK_NO_THROW(oss << *config.get());
        OCIO_CHECK_EQUAL(oss.str(), str);
    }
}

OCIO_ADD_TEST(Config, file_transform_serialization)
{
    // Config v2.
    const std::string strEnd =
        "    from_reference: !<GroupTransform>\n"
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
    OCIO_CHECK_NO_THROW(config->sanityCheck());

    std::ostringstream oss;
    OCIO_CHECK_NO_THROW(oss << *config.get());
    OCIO_CHECK_EQUAL(oss.str(), str);
}

OCIO_ADD_TEST(Config, add_color_space)
{
    // The unit test validates that the color space is correctly added to the configuration.

    // Note that the new C++11 u8 notation for UTF-8 string literals is used
    // to partially validate non-english language support.

    const std::string str
        = PROFILE_V2_START
            + u8"    from_reference: !<MatrixTransform> {offset: [-1, -2, -3, -4]}\n";

    std::istringstream is;
    is.str(str);

    OCIO::ConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is)->createEditableCopy());
    OCIO_CHECK_NO_THROW(config->sanityCheck());
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 2);

    OCIO::ColorSpaceRcPtr cs;
    OCIO_CHECK_NO_THROW(cs = OCIO::ColorSpace::Create());
    cs->setName(u8"astéroïde");                           // Color space name with accents.
    cs->setDescription(u8"é À Â Ç É È ç -- $ € 円 £ 元"); // Some accents and some money symbols.

    OCIO::FixedFunctionTransformRcPtr tr;
    OCIO_CHECK_NO_THROW(tr = OCIO::FixedFunctionTransform::Create());

    OCIO_CHECK_NO_THROW(cs->setTransform(tr, OCIO::COLORSPACE_DIR_TO_REFERENCE));

    constexpr char csName[] = u8"astéroïde";

    OCIO_CHECK_EQUAL(config->getIndexForColorSpace(csName), -1);
    OCIO_CHECK_NO_THROW(config->addColorSpace(cs));
    OCIO_CHECK_EQUAL(config->getIndexForColorSpace(csName), 2);

    const std::string res 
        = str
        + u8"\n"
        + u8"  - !<ColorSpace>\n"
        + u8"    name: " + csName + u8"\n"
        + u8"    family: \"\"\n"
        + u8"    equalitygroup: \"\"\n"
        + u8"    bitdepth: unknown\n"
        + u8"    description: |\n"
        + u8"      é À Â Ç É È ç -- $ € 円 £ 元\n"
        + u8"    isdata: false\n"
        + u8"    allocation: uniform\n"
        + u8"    to_reference: !<FixedFunctionTransform> {style: ACES_RedMod03}\n";

    std::stringstream ss;
    OCIO_CHECK_NO_THROW(ss << *config.get());
    OCIO_CHECK_EQUAL(ss.str(), res);

    OCIO_CHECK_NO_THROW(config->removeColorSpace(csName));
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 2);
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
            + "    from_reference: !<MatrixTransform> {offset: [-1, -2, -3, -4]}\n"
            + "\n"
            + "  - !<ColorSpace>\n"
            + "    name: cs5\n"
            + "    allocation: uniform\n"
            + "    to_reference: !<FixedFunctionTransform> {style: ACES_RedMod03}\n";

    std::istringstream is;
    is.str(str);

    OCIO::ConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is)->createEditableCopy());
    OCIO_CHECK_NO_THROW(config->sanityCheck());
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 3);

    // Step 1 - Validate the remove.

    OCIO_CHECK_EQUAL(config->getIndexForColorSpace("cs5"), 2);
    OCIO_CHECK_NO_THROW(config->removeColorSpace("cs5"));
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 2);
    OCIO_CHECK_EQUAL(config->getIndexForColorSpace("cs5"), -1);

    // Step 2 - Validate some faulty removes.

    // As documented, removing a color space that doesn't exist fails without any notice.
    OCIO_CHECK_NO_THROW(config->removeColorSpace("cs5"));
    OCIO_CHECK_NO_THROW(config->sanityCheck());

    // Since the method does not support role names, a role name removal fails 
    // without any notice except if it's also an existing color space.
    OCIO_CHECK_NO_THROW(config->removeColorSpace("scene_linear"));
    OCIO_CHECK_NO_THROW(config->sanityCheck());

    // Sucessfully remove a color space unfortunately used by a role.
    OCIO_CHECK_NO_THROW(config->removeColorSpace("raw"));
    // As discussed only the sanity check traps the issue.
    OCIO_CHECK_THROW_WHAT(config->sanityCheck(),
                          OCIO::Exception,
                          "Config failed sanitycheck. The role 'default' refers to"\
                          " a color space, 'raw', which is not defined.");
}

namespace
{

constexpr char InactiveCSConfigStart[] =
    "ocio_profile_version: 2\n"
    "\n"
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
    "    family: \"\"\n"
    "    equalitygroup: \"\"\n"
    "    bitdepth: unknown\n"
    "    isdata: false\n"
    "    categories: [cat1]\n"
    "    allocation: uniform\n"
    "\n"
    "  - !<ColorSpace>\n"
    "    name: cs2\n"
    "    family: \"\"\n"
    "    equalitygroup: \"\"\n"
    "    bitdepth: unknown\n"
    "    isdata: false\n"
    "    categories: [cat2]\n"
    "    allocation: uniform\n"
    "\n"
    "  - !<ColorSpace>\n"
    "    name: cs3\n"
    "    family: \"\"\n"
    "    equalitygroup: \"\"\n"
    "    bitdepth: unknown\n"
    "    isdata: false\n"
    "    categories: [cat3]\n"
    "    allocation: uniform\n";

class InactiveCSGuard
{
public:
    InactiveCSGuard()
    {
        OCIO::Platform::Setenv(OCIO::OCIO_INACTIVE_COLORSPACES_ENVVAR, "cs3, cs1, lnh");
    }
    ~InactiveCSGuard()
    {
        OCIO::Platform::Setenv(OCIO::OCIO_INACTIVE_COLORSPACES_ENVVAR, "");
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
    OCIO_CHECK_NO_THROW(config->sanityCheck());


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

    // Search using a category 'cat1' with no active color space.
    OCIO_CHECK_NO_THROW(css = config->getColorSpaces("cat1"));
    OCIO_CHECK_EQUAL(css->getNumColorSpaces(), 0);

    // Search using a category 'cat2' with some active color spaces.
    OCIO_CHECK_NO_THROW(css = config->getColorSpaces("cat2"));
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

        const char * csName = config->getDisplayColorSpaceName("sRGB", "Lnh");
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

    // Step 3 - No inactive color spaces.

    OCIO_CHECK_NO_THROW(config->setInactiveColorSpaces(""));
    OCIO_CHECK_EQUAL(config->getInactiveColorSpaces(), std::string(""));

    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                               OCIO::COLORSPACE_ALL), 5);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 5);

    // Step 4 - No inactive color spaces.

    OCIO_CHECK_NO_THROW(config->setInactiveColorSpaces(nullptr));
    OCIO_CHECK_EQUAL(config->getInactiveColorSpaces(), std::string(""));

    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                               OCIO::COLORSPACE_ALL), 5);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 5);

    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_SCENE,
                                               OCIO::COLORSPACE_ALL), 5);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_DISPLAY,
                                               OCIO::COLORSPACE_ALL), 0);

    // Step 5 - Add display color spaces.
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

    // Step 6 - Some inactive color spaces.
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
    OCIO_CHECK_NO_THROW(config->sanityCheck());

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
    OCIO_CHECK_NO_THROW(config->sanityCheck());

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
        OCIO_CHECK_NO_THROW(config->sanityCheck());

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
            OCIO_CHECK_NO_THROW(config->sanityCheck());
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
        OCIO_CHECK_NO_THROW(config->sanityCheck());

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
        OCIO_CHECK_NO_THROW(config->sanityCheck());

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
            OCIO_CHECK_NO_THROW(config->sanityCheck());
            OCIO_CHECK_EQUAL(log.output(), 
                             "[OpenColorIO Warning]: Inactive color space 'unknown' does not exist.\n");
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
    to_reference: !<MatrixTransform> {offset: [0.01, 0.02, 0.03, 0]}

  - !<ColorSpace>
    name: aces1
    allocation: uniform
    from_reference: !<ExponentTransform> {value: [1.101, 1.202, 1.303, 1.404]}

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
    from_reference: !<MatrixTransform> {offset: [0.11, 0.12, 0.13, 0]}

  - !<ColorSpace>
    name: aces2
    allocation: uniform
    to_reference: !<RangeTransform> {minInValue: -0.0109, maxInValue: 1.0505, minOutValue: 0.0009, maxOutValue: 2.5001}

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
    OCIO_CHECK_NO_THROW(p = OCIO::Config::GetProcessor(config1, "test1", config2, "test2"));
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
    OCIO_CHECK_NO_THROW(p = OCIO::Config::GetProcessor(config1, "test1", "aces1", config2, "test2", "aces2"));
    OCIO_REQUIRE_ASSERT(p);
    OCIO_REQUIRE_ASSERT(p);
    group = p->createGroupTransform();
    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 4);

    // Or interchange space can be specified using role.
    OCIO_CHECK_NO_THROW(p = OCIO::Config::GetProcessor(config1, "test1", "aces_interchange", config2, "test2", "aces2"));
    OCIO_REQUIRE_ASSERT(p);
    OCIO_REQUIRE_ASSERT(p);
    group = p->createGroupTransform();
    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 4);

    // Or color space can be specified using role.
    OCIO_CHECK_NO_THROW(p = OCIO::Config::GetProcessor(config1, "test1", "aces_interchange", config2, "test_role", "aces2"));
    OCIO_REQUIRE_ASSERT(p);
    OCIO_REQUIRE_ASSERT(p);
    group = p->createGroupTransform();
    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 4);

    // Display-referred interchange space.
    OCIO_CHECK_NO_THROW(p = OCIO::Config::GetProcessor(config1, "display2", config2, "display4"));
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

    OCIO_CHECK_THROW_WHAT(OCIO::Config::GetProcessor(config1, "display2", config2, "test2"),
                          OCIO::Exception,
                          "There is no view transform between the main scene-referred space "
                          "and the display-referred space");

    constexpr const char * SIMPLE_CONFIG3{ R"(
ocio_profile_version: 2

roles:
  default: raw

colorspaces:
  - !<ColorSpace>
    name: raw
    allocation: uniform

  - !<ColorSpace>
    name: test
    allocation: uniform
    from_reference: !<MatrixTransform> {offset: [0.11, 0.12, 0.13, 0]}
)" };

    is.clear();
    is.str(SIMPLE_CONFIG3);
    OCIO::ConstConfigRcPtr config3;
    OCIO_CHECK_NO_THROW(config3 = OCIO::Config::CreateFromStream(is));

    OCIO_CHECK_THROW_WHAT(OCIO::Config::GetProcessor(config1, "test1", config3, "test"),
                          OCIO::Exception,
                          "The role 'aces_interchange' is missing in the destination config");

    OCIO_CHECK_THROW_WHAT(OCIO::Config::GetProcessor(config1, "display1", config3, "test"),
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
            "    from_reference: !<MatrixTransform> {}\n"
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

        const std::string str = PROFILE_V2_DCS_START + strDCS + SIMPLE_PROFILE_CS;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

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
            "    from_reference: !<ExponentTransform> {value: [2.4, 2.4, 2.4, 1], direction: inverse}\n"
            "\n"
            "  - !<ColorSpace>\n"
            "    name: dcs2\n"
            "    family: \"\"\n"
            "    equalitygroup: \"\"\n"
            "    bitdepth: unknown\n"
            "    isdata: false\n"
            "    allocation: uniform\n"
            "    to_display_reference: !<ExponentTransform> {value: [2.4, 2.4, 2.4, 1]}\n";
        const std::string str = PROFILE_V2_DCS_START + strDCS + SIMPLE_PROFILE_CS;

        std::istringstream is;
        is.str(str);

        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                              "'from_reference' cannot be used for a display color space");
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
            "    to_reference: !<ExponentTransform> {value: [2.4, 2.4, 2.4, 1]}\n";
        const std::string str = PROFILE_V2_DCS_START + strDCS + SIMPLE_PROFILE_CS;

        std::istringstream is;
        is.str(str);

        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                              "'to_reference' cannot be used for a display color space");
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
    OCIO_CHECK_NO_THROW(config->sanityCheck());

    OCIO_CHECK_EQUAL(config->getNumViewTransforms(), 0);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_DISPLAY, OCIO::COLORSPACE_ALL), 
                     0);
}

OCIO_ADD_TEST(Config, view_transforms)
{
    const std::string str = PROFILE_V2_DCS_START + SIMPLE_PROFILE_CS;

    std::istringstream is;
    is.str(str);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->sanityCheck());

    auto configEdit = config->createEditableCopy();
    // Create display-referred view transform and add it to the config.
    auto vt = OCIO::ViewTransform::Create(OCIO::REFERENCE_SPACE_DISPLAY);
    OCIO_CHECK_THROW_WHAT(configEdit->addViewTransform(vt), OCIO::Exception,
                          "Cannot add view transform with an empty name");
    const std::string vtDisplay{ "display" };
    vt->setName(vtDisplay.c_str());
    OCIO_CHECK_THROW_WHAT(configEdit->addViewTransform(vt), OCIO::Exception,
                          "Cannot add view transform with no transform");
    OCIO_CHECK_NO_THROW(vt->setTransform(OCIO::MatrixTransform::Create(),
                                         OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE));
    OCIO_CHECK_NO_THROW(configEdit->addViewTransform(vt));
    OCIO_CHECK_EQUAL(configEdit->getNumViewTransforms(), 1);
    // Need at least one scene-referred view transform.
    OCIO_CHECK_THROW_WHAT(configEdit->sanityCheck(), OCIO::Exception,
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
    OCIO_CHECK_NO_THROW(configEdit->sanityCheck());

    auto sceneVT = configEdit->getDefaultSceneToDisplayViewTransform();
    OCIO_CHECK_ASSERT(sceneVT);

    OCIO_CHECK_EQUAL(vtDisplay, configEdit->getViewTransformNameByIndex(0));
    OCIO_CHECK_EQUAL(vtScene, configEdit->getViewTransformNameByIndex(1));
    OCIO_CHECK_EQUAL(std::string(""), configEdit->getViewTransformNameByIndex(42));
    OCIO_CHECK_ASSERT(configEdit->getViewTransform(vtScene.c_str()));
    OCIO_CHECK_ASSERT(!configEdit->getViewTransform("not a view transform"));

    // Save and reload to test file io for viewTransform.
    std::stringstream os;
    os << *configEdit.get();

    is.clear();
    is.str(os.str());

    OCIO::ConstConfigRcPtr configReloaded;
    OCIO_CHECK_NO_THROW(configReloaded = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(configReloaded->sanityCheck());

    // Setting a view transform with the same name replaces the earlier one.
    OCIO_CHECK_NO_THROW(vt->setTransform(OCIO::LogTransform::Create(),
                                         OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE));
    OCIO_CHECK_NO_THROW(configEdit->addViewTransform(vt));
    OCIO_REQUIRE_EQUAL(configEdit->getNumViewTransforms(), 2);
    sceneVT = configEdit->getViewTransform(vtScene.c_str());
    auto trans = sceneVT->getTransform(OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE);
    OCIO_REQUIRE_ASSERT(trans);
    OCIO_CHECK_ASSERT(OCIO_DYNAMIC_POINTER_CAST<const OCIO::LogTransform>(trans));

    OCIO_CHECK_EQUAL(configReloaded->getNumViewTransforms(), 2);

    configEdit->clearViewTransforms();
    OCIO_CHECK_EQUAL(configEdit->getNumViewTransforms(), 0);
}

OCIO_ADD_TEST(Config, display_view)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    auto cs = OCIO::ColorSpace::Create(OCIO::REFERENCE_SPACE_SCENE);
    cs->setName("scs");
    config->addColorSpace(cs);
    cs = OCIO::ColorSpace::Create(OCIO::REFERENCE_SPACE_DISPLAY);
    cs->setName("dcs");
    config->addColorSpace(cs);

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

    const std::string display{ "display" };
    OCIO_CHECK_NO_THROW(config->addDisplay(display.c_str(), "view1", "scs", ""));

    OCIO_CHECK_NO_THROW(config->sanityCheck());

    OCIO_CHECK_NO_THROW(config->addDisplay(display.c_str(), "view2", "view_transform", "scs", ""));
    OCIO_CHECK_THROW_WHAT(config->sanityCheck(), OCIO::Exception,
                          "color space, 'scs', that is not a display-referred");

    OCIO_CHECK_NO_THROW(config->addDisplay(display.c_str(), "view2", "view_transform", "dcs", ""));
    OCIO_CHECK_NO_THROW(config->sanityCheck());

    OCIO_CHECK_EQUAL(config->getNumDisplays(), 1);
    OCIO_CHECK_EQUAL(config->getNumViews(display.c_str()), 2);

    // Check that views are saved and loaded properly.
    config->setMajorVersion(2);
    std::ostringstream oss;
    config->serialize(oss);

    std::istringstream is;
    is.str(oss.str());
    OCIO::ConstConfigRcPtr configRead;
    OCIO_CHECK_NO_THROW(configRead = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_EQUAL(configRead->getNumViews("display"), 2);
    const std::string v1{ configRead->getView("display", 0) };
    OCIO_CHECK_EQUAL(v1, "view1");
    OCIO_CHECK_EQUAL(std::string("scs"),
                     configRead->getDisplayColorSpaceName("display", v1.c_str()));
    OCIO_CHECK_EQUAL(std::string(""),
                     configRead->getDisplayViewTransformName("display", v1.c_str()));
    const std::string v2{ configRead->getView("display", 1) };
    OCIO_CHECK_EQUAL(v2, "view2");
    OCIO_CHECK_EQUAL(std::string("dcs"),
                     configRead->getDisplayColorSpaceName("display", v2.c_str()));
    OCIO_CHECK_EQUAL(std::string("view_transform"),
                     configRead->getDisplayViewTransformName("display", v2.c_str()));

    // Using nullptr for any parameter does nothing.
    OCIO_CHECK_NO_THROW(config->addDisplay(nullptr, "view1", "scs", ""));
    OCIO_CHECK_NO_THROW(config->addDisplay(display.c_str(), nullptr, "scs", ""));
    OCIO_CHECK_NO_THROW(config->addDisplay(display.c_str(), "view3", nullptr, ""));
    OCIO_CHECK_NO_THROW(config->addDisplay(display.c_str(), "view4", "view_transform", nullptr, ""));
    OCIO_CHECK_EQUAL(config->getNumDisplays(), 1);
    OCIO_CHECK_EQUAL(config->getNumViews(display.c_str()), 2);

    OCIO_CHECK_THROW_WHAT(config->addDisplay("", "view1", "scs", ""), OCIO::Exception,
                          "Can't add a (display, view) pair with empty display name");
    OCIO_CHECK_THROW_WHAT(config->addDisplay(display.c_str(), "", "scs", ""), OCIO::Exception,
                          "Can't add a (display, view) pair with empty view name");
    OCIO_CHECK_THROW_WHAT(config->addDisplay(display.c_str(), "view1", "", ""), OCIO::Exception,
                          "Can't add a (display, view) pair with empty color space name");
}

OCIO_ADD_TEST(Config, not_case_sensitive)
{
    // Validate that the color spaces and roles are case insensitive.

    std::istringstream is;
    is.str(PROFILE_V2_START);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->sanityCheck());

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
    // Validate that Config::sanityCheck() on config file containing transforms 
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
)" };

    std::istringstream is;
    is.str(OCIO_CONFIG);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->sanityCheck());

    // Validate the color spaces.

    OCIO::ConstProcessorRcPtr processor;
    OCIO_CHECK_NO_THROW(processor = config->getProcessor("raw", "cs1"));
    OCIO_CHECK_ASSERT(processor);

    OCIO_CHECK_NO_THROW(processor = config->getProcessor("raw", "cs2"));
    OCIO_CHECK_ASSERT(processor);

    OCIO_CHECK_NO_THROW(processor = config->getProcessor("cs1", "cs2"));
    OCIO_CHECK_ASSERT(processor);

    // Validate the (display, view) pair with looks.

    OCIO::DisplayTransformRcPtr display = OCIO::DisplayTransform::Create();
    display->setInputColorSpaceName("raw");
    display->setDisplay("Disp1");
    display->setView("View1");

    OCIO_CHECK_NO_THROW(processor = config->getProcessor(display));
    OCIO_CHECK_ASSERT(processor);

    display->setInputColorSpaceName("cs1");

    OCIO_CHECK_NO_THROW(processor = config->getProcessor(display));
    OCIO_CHECK_ASSERT(processor);

    display->setInputColorSpaceName("cs2");

    OCIO_CHECK_NO_THROW(processor = config->getProcessor(display));
    OCIO_CHECK_ASSERT(processor);
}    

OCIO_ADD_TEST(Config, look_transform)
{
    // Validate Config::sanityCheck() on config file containing look transforms.

    constexpr const char * OCIO_CONFIG{ R"(
ocio_profile_version: 2

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
    OCIO_CHECK_NO_THROW(config->sanityCheck());
}

OCIO_ADD_TEST(Config, family_separator)
{
    OCIO::ConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateRaw()->createEditableCopy());
    OCIO_CHECK_NO_THROW(config->sanityCheck());

    OCIO_CHECK_EQUAL(config->getFamilySeparator(), 0x0); // Default value i.e. no separator.

    OCIO_CHECK_NO_THROW(config->setFamilySeparator('/'));
    OCIO_CHECK_EQUAL(config->getFamilySeparator(), '/');

    OCIO_CHECK_THROW(config->setFamilySeparator((char)127), OCIO::Exception);
    OCIO_CHECK_THROW(config->setFamilySeparator((char)31) , OCIO::Exception);
}

OCIO_ADD_TEST(Config, add_remove_display)
{
    OCIO::ConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateRaw()->createEditableCopy());
    OCIO_CHECK_NO_THROW(config->sanityCheck());

    OCIO_REQUIRE_EQUAL(config->getNumDisplays(), 1);
    OCIO_REQUIRE_EQUAL(std::string(config->getDisplay(0)), std::string("sRGB"));
    OCIO_REQUIRE_EQUAL(config->getNumViews("sRGB"), 1);
    OCIO_REQUIRE_EQUAL(std::string(config->getView("sRGB", 0)), std::string("Raw"));

    // Add a (display, view) pair.

    OCIO_CHECK_NO_THROW(config->addDisplay("disp1", "view1", "raw", nullptr));
    OCIO_REQUIRE_EQUAL(config->getNumDisplays(), 2);
    OCIO_CHECK_EQUAL(std::string(config->getDisplay(0)), std::string("sRGB"));
    OCIO_CHECK_EQUAL(std::string(config->getDisplay(1)), std::string("disp1"));
    OCIO_REQUIRE_EQUAL(config->getNumViews("disp1"), 1);

    // Remove a (display, view) pair.

    OCIO_CHECK_NO_THROW(config->removeDisplay("disp1", "view1"));
    OCIO_REQUIRE_EQUAL(config->getNumDisplays(), 1);
    OCIO_CHECK_EQUAL(std::string(config->getDisplay(0)), std::string("sRGB"));
}

OCIO_ADD_TEST(Config, is_colorspace_used)
{
    // Test Config::isColorSpaceUsed() i.e. a color space could be defined but not used.

    constexpr char CONFIG[] = 
        "ocio_profile_version: 2\n"
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
        "    from_reference: !<ColorSpaceTransform> {src: cs11, dst: cs11}\n"
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
        "    from_reference: !<ColorSpaceTransform> {src: cs3, dst: cs3}\n"
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
        "    from_reference: !<GroupTransform>\n"
        "      children:\n"
        "        - !<ColorSpaceTransform> {src: cs7, dst: cs7}\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: cs9\n"
        "    from_reference: !<GroupTransform>\n"
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
    OCIO_CHECK_NO_THROW(config->sanityCheck());

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
         - !<RangeTransform> {minInValue: 0, minOutValue: 0}
)" };

    std::istringstream is;
    is.str(OCIO_CONFIG);
    OCIO::ConstConfigRcPtr cfg;
    OCIO_CHECK_THROW_WHAT(cfg = OCIO::Config::CreateFromStream(is),
                          OCIO::Exception,
                          "Only config version 2 (or higher) can have RangeTransform.");
}

OCIO_ADD_TEST(Config, builtin_transforms)
{
    // Test some default built-in transforms.

    constexpr const char * CONFIG_BUILTIN_TRANSFORMS{
R"(ocio_profile_version: 2

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
    from_reference: !<GroupTransform>
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

        OCIO_CHECK_NO_THROW(config->sanityCheck());
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
