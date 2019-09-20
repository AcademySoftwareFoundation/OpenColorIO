
import junit.framework.TestCase;
import org.OpenColorIO.*;

public class GpuShaderDescTest extends TestCase {
    
    protected void setUp() {
    }
    
    protected void tearDown() {
    }
    
    public void test_interface() {
        GpuShaderDesc desc = new GpuShaderDesc();

        desc.setFunctionName("foo123");
        assertEquals("foo123", desc.getFunctionName());
        desc.setLut3DEdgeLen(32);
        assertEquals(32, desc.getLut3DEdgeLen());

        desc.setLanguage(GpuLanguage.GPU_LANGUAGE_GLSL_1_0);
        assertEquals(GpuLanguage.GPU_LANGUAGE_GLSL_1_0, desc.getLanguage());
        assertEquals("glsl_1.0 foo123 32", desc.getCacheID());

        desc.setLanguage(GpuLanguage.GPU_LANGUAGE_GLSL_1_3);
        assertEquals(GpuLanguage.GPU_LANGUAGE_GLSL_1_3, desc.getLanguage());
        assertEquals("glsl_1.3 foo123 32", desc.getCacheID());

        desc.setLanguage(GpuLanguage.GPU_LANGUAGE_GLSL_4_0);
        assertEquals(GpuLanguage.GPU_LANGUAGE_GLSL_4_0, desc.getLanguage());
        assertEquals("glsl_4.0 foo123 32", desc.getCacheID());
    }
    
}
