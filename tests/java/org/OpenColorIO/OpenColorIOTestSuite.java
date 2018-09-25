
import junit.framework.Test;
import junit.framework.TestSuite;

public class OpenColorIOTestSuite {
    
    public static Test suite() {
        
        TestSuite suite = new TestSuite();
        
        // Core
        suite.addTestSuite(GlobalsTest.class);
        suite.addTestSuite(ConfigTest.class);
        suite.addTestSuite(ColorSpaceTest.class);
        suite.addTestSuite(LookTest.class);
        suite.addTestSuite(BakerTest.class);
        suite.addTestSuite(PackedImageDescTest.class);
        suite.addTestSuite(PlanarImageDescTest.class);
        suite.addTestSuite(GpuShaderDescTest.class);
        suite.addTestSuite(ContextTest.class);
        suite.addTestSuite(TransformsTest.class);
        
        return suite;
    }
    
    public static void main(String[] args) {
        junit.textui.TestRunner.run(suite());
    }
    
}
