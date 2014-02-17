
import junit.framework.TestCase;
import org.OpenColorIO.*;

public class GlobalsTest extends TestCase {
    
    String FOO = ""
    + "ocio_profile_version: 1\n"
    + "\n"
    + "search_path: \"\"\n"
    + "strictparsing: false\n"
    + "luma: [0.2126, 0.7152, 0.0722]\n"
    + "\n"
    + "roles:\n"
    + "  default: raw\n"
    + "\n"
    + "displays:\n"
    + "  sRGB:\n"
    + "    - !<View> {name: Raw, colorspace: raw}\n"
    + "\n"
    + "active_displays: []\n"
    + "active_views: []\n"
    + "\n"
    + "colorspaces:\n"
    + "  - !<ColorSpace>\n"
    + "    name: raw\n"
    + "    family: raw\n"
    + "    equalitygroup: \"\"\n" 
    + "    bitdepth: 32f\n"
    + "    description: |\n"
    + "      A raw color space. Conversions to and from this space are no-ops.\n"
    + "    isdata: true\n"
    + "    allocation: uniform\n";
    
    protected void setUp() {
    }
    
    public void test_interface() {
        
        Globals globals = new Globals();
        
        // Globals
        globals.ClearAllCaches();
        //assertEquals("1.0.1", globals.GetVersion());
        //assertEquals(16777472, globals.GetVersionHex());
        assertEquals(LoggingLevel.LOGGING_LEVEL_INFO, globals.GetLoggingLevel());
        globals.SetLoggingLevel(LoggingLevel.LOGGING_LEVEL_NONE);
        assertEquals(LoggingLevel.LOGGING_LEVEL_NONE, globals.GetLoggingLevel());
        Config foo = globals.GetCurrentConfig();
        assertEquals(FOO, foo.serialize());
        globals.SetLoggingLevel(LoggingLevel.LOGGING_LEVEL_INFO);
        Config bar = new Config().CreateFromStream(foo.serialize());
        globals.SetCurrentConfig(bar);
        Config wee = globals.GetCurrentConfig();
        
        // LoggingLevel
        assertEquals(globals.LoggingLevelToString(LoggingLevel.LOGGING_LEVEL_NONE), "none");
        assertEquals(globals.LoggingLevelFromString("none"), LoggingLevel.LOGGING_LEVEL_NONE);
        assertEquals(globals.LoggingLevelToString(LoggingLevel.LOGGING_LEVEL_WARNING), "warning");
        assertEquals(globals.LoggingLevelFromString("warning"), LoggingLevel.LOGGING_LEVEL_WARNING);
        assertEquals(globals.LoggingLevelToString(LoggingLevel.LOGGING_LEVEL_INFO), "info");
        assertEquals(globals.LoggingLevelFromString("info"), LoggingLevel.LOGGING_LEVEL_INFO);
        assertEquals(globals.LoggingLevelToString(LoggingLevel.LOGGING_LEVEL_DEBUG), "debug");
        assertEquals(globals.LoggingLevelFromString("debug"), LoggingLevel.LOGGING_LEVEL_DEBUG);
        assertEquals(globals.LoggingLevelToString(LoggingLevel.LOGGING_LEVEL_UNKNOWN), "unknown");
        assertEquals(globals.LoggingLevelFromString("unknown"), LoggingLevel.LOGGING_LEVEL_UNKNOWN);
        
        // TransformDirection
        assertEquals(globals.TransformDirectionToString(TransformDirection.TRANSFORM_DIR_UNKNOWN), "unknown");
        assertEquals(globals.TransformDirectionFromString("unknown"), TransformDirection.TRANSFORM_DIR_UNKNOWN);
        assertEquals(globals.TransformDirectionToString(TransformDirection.TRANSFORM_DIR_FORWARD), "forward");
        assertEquals(globals.TransformDirectionFromString("forward"), TransformDirection.TRANSFORM_DIR_FORWARD);
        assertEquals(globals.TransformDirectionToString(TransformDirection.TRANSFORM_DIR_INVERSE), "inverse");
        assertEquals(globals.TransformDirectionFromString("inverse"), TransformDirection.TRANSFORM_DIR_INVERSE);
        assertEquals(globals.GetInverseTransformDirection(TransformDirection.TRANSFORM_DIR_UNKNOWN), TransformDirection.TRANSFORM_DIR_UNKNOWN);
        assertEquals(globals.GetInverseTransformDirection(TransformDirection.TRANSFORM_DIR_FORWARD), TransformDirection.TRANSFORM_DIR_INVERSE);
        assertEquals(globals.GetInverseTransformDirection(TransformDirection.TRANSFORM_DIR_INVERSE), TransformDirection.TRANSFORM_DIR_FORWARD);
        
        // ColorSpaceDirection
        assertEquals(globals.ColorSpaceDirectionToString(ColorSpaceDirection.COLORSPACE_DIR_UNKNOWN), "unknown");
        assertEquals(globals.ColorSpaceDirectionFromString("unknown"), ColorSpaceDirection.COLORSPACE_DIR_UNKNOWN);
        assertEquals(globals.ColorSpaceDirectionToString(ColorSpaceDirection.COLORSPACE_DIR_TO_REFERENCE), "to_reference");
        assertEquals(globals.ColorSpaceDirectionFromString("to_reference"), ColorSpaceDirection.COLORSPACE_DIR_TO_REFERENCE);
        assertEquals(globals.ColorSpaceDirectionToString(ColorSpaceDirection.COLORSPACE_DIR_FROM_REFERENCE), "from_reference");
        assertEquals(globals.ColorSpaceDirectionFromString("from_reference"), ColorSpaceDirection.COLORSPACE_DIR_FROM_REFERENCE);
        
        // BitDepth
        assertEquals(globals.BitDepthToString(BitDepth.BIT_DEPTH_UNKNOWN), "unknown");
        assertEquals(globals.BitDepthFromString("unknown"), BitDepth.BIT_DEPTH_UNKNOWN);
        assertEquals(globals.BitDepthToString(BitDepth.BIT_DEPTH_UINT8), "8ui");
        assertEquals(globals.BitDepthFromString("8ui"), BitDepth.BIT_DEPTH_UINT8);
        assertEquals(globals.BitDepthToString(BitDepth.BIT_DEPTH_UINT10), "10ui");
        assertEquals(globals.BitDepthFromString("10ui"), BitDepth.BIT_DEPTH_UINT10);
        assertEquals(globals.BitDepthToString(BitDepth.BIT_DEPTH_UINT12), "12ui");
        assertEquals(globals.BitDepthFromString("12ui"), BitDepth.BIT_DEPTH_UINT12);
        assertEquals(globals.BitDepthToString(BitDepth.BIT_DEPTH_UINT14), "14ui");
        assertEquals(globals.BitDepthFromString("14ui"), BitDepth.BIT_DEPTH_UINT14);
        assertEquals(globals.BitDepthToString(BitDepth.BIT_DEPTH_UINT16), "16ui");
        assertEquals(globals.BitDepthFromString("16ui"), BitDepth.BIT_DEPTH_UINT16);
        assertEquals(globals.BitDepthToString(BitDepth.BIT_DEPTH_UINT32), "32ui");
        assertEquals(globals.BitDepthFromString("32ui"), BitDepth.BIT_DEPTH_UINT32);
        assertEquals(globals.BitDepthToString(BitDepth.BIT_DEPTH_F16), "16f");
        assertEquals(globals.BitDepthFromString("16f"), BitDepth.BIT_DEPTH_F16);
        assertEquals(globals.BitDepthToString(BitDepth.BIT_DEPTH_F32), "32f");
        assertEquals(globals.BitDepthFromString("32f"), BitDepth.BIT_DEPTH_F32);
        
        // Allocation
        assertEquals(globals.AllocationToString(Allocation.ALLOCATION_UNKNOWN), "unknown");
        assertEquals(globals.AllocationFromString("unknown"), Allocation.ALLOCATION_UNKNOWN);
        assertEquals(globals.AllocationToString(Allocation.ALLOCATION_UNIFORM), "uniform");
        assertEquals(globals.AllocationFromString("uniform"), Allocation.ALLOCATION_UNIFORM);
        assertEquals(globals.AllocationToString(Allocation.ALLOCATION_LG2), "lg2");
        assertEquals(globals.AllocationFromString("lg2"), Allocation.ALLOCATION_LG2);
        
        // Interpolation
        assertEquals(globals.InterpolationToString(Interpolation.INTERP_UNKNOWN), "unknown");
        assertEquals(globals.InterpolationFromString("unknown"), Interpolation.INTERP_UNKNOWN);
        assertEquals(globals.InterpolationToString(Interpolation.INTERP_NEAREST), "nearest");
        assertEquals(globals.InterpolationFromString("nearest"), Interpolation.INTERP_NEAREST);
        assertEquals(globals.InterpolationToString(Interpolation.INTERP_LINEAR), "linear");
        assertEquals(globals.InterpolationFromString("linear"), Interpolation.INTERP_LINEAR);
        assertEquals(globals.InterpolationToString(Interpolation.INTERP_TETRAHEDRAL), "tetrahedral");
        assertEquals(globals.InterpolationFromString("tetrahedral"), Interpolation.INTERP_TETRAHEDRAL);
        assertEquals(globals.InterpolationToString(Interpolation.INTERP_BEST), "best");
        assertEquals(globals.InterpolationFromString("best"), Interpolation.INTERP_BEST);
        
        // GpuLanguage
        assertEquals(globals.GpuLanguageToString(GpuLanguage.GPU_LANGUAGE_UNKNOWN), "unknown");
        assertEquals(globals.GpuLanguageFromString("unknown"), GpuLanguage.GPU_LANGUAGE_UNKNOWN);
        assertEquals(globals.GpuLanguageToString(GpuLanguage.GPU_LANGUAGE_CG), "cg");
        assertEquals(globals.GpuLanguageFromString("cg"), GpuLanguage.GPU_LANGUAGE_CG);
        assertEquals(globals.GpuLanguageToString(GpuLanguage.GPU_LANGUAGE_GLSL_1_0), "glsl_1.0");
        assertEquals(globals.GpuLanguageFromString("glsl_1.0"), GpuLanguage.GPU_LANGUAGE_GLSL_1_0);
        assertEquals(globals.GpuLanguageToString(GpuLanguage.GPU_LANGUAGE_GLSL_1_3), "glsl_1.3");
        assertEquals(globals.GpuLanguageFromString("glsl_1.3"), GpuLanguage.GPU_LANGUAGE_GLSL_1_3);
        
        // EnvironmentMode
        assertEquals(globals.EnvironmentModeToString(EnvironmentMode.ENV_ENVIRONMENT_UNKNOWN), "unknown");
        assertEquals(globals.EnvironmentModeFromString("unknown"), EnvironmentMode.ENV_ENVIRONMENT_UNKNOWN);
        assertEquals(globals.EnvironmentModeToString(EnvironmentMode.ENV_ENVIRONMENT_LOAD_PREDEFINED), "loadpredefined");
        assertEquals(globals.EnvironmentModeFromString("loadpredefined"), EnvironmentMode.ENV_ENVIRONMENT_LOAD_PREDEFINED);
        assertEquals(globals.EnvironmentModeToString(EnvironmentMode.ENV_ENVIRONMENT_LOAD_ALL), "loadall");
        assertEquals(globals.EnvironmentModeFromString("loadall"), EnvironmentMode.ENV_ENVIRONMENT_LOAD_ALL);
        
        // Roles
        assertEquals(globals.ROLE_DEFAULT, "default");
        assertEquals(globals.ROLE_REFERENCE, "reference");
        assertEquals(globals.ROLE_DATA, "data");
        assertEquals(globals.ROLE_COLOR_PICKING, "color_picking");
        assertEquals(globals.ROLE_SCENE_LINEAR, "scene_linear");
        assertEquals(globals.ROLE_COMPOSITING_LOG, "compositing_log");
        assertEquals(globals.ROLE_COLOR_TIMING, "color_timing");
        assertEquals(globals.ROLE_TEXTURE_PAINT, "texture_paint");
        assertEquals(globals.ROLE_MATTE_PAINT, "matte_paint");
        
    }
    
}
