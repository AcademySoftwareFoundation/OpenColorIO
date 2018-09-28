
import junit.framework.TestCase;
import org.OpenColorIO.*;

public class GpuShaderDescTest extends TestCase {
    
    protected void setUp() {
    }
    
    protected void tearDown() {
    }
    
    public void test_interface() {
        GpuShaderDesc desc = new GpuShaderDesc();
        desc.setLanguage(GpuLanguage.GPU_LANGUAGE_GLSL_1_3);
        assertEquals(GpuLanguage.GPU_LANGUAGE_GLSL_1_3, desc.getLanguage());
        desc.setFunctionName("foo123");
        assertEquals("foo123", desc.getFunctionName());
        desc.setLut3DEdgeLen(32);
        assertEquals(32, desc.getLut3DEdgeLen());
        assertEquals("glsl_1.3 foo123 32", desc.getCacheID());
    }
    
}
