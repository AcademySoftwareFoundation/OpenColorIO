
import junit.framework.TestCase;
import org.OpenColorIO.*;

public class BakerTest extends TestCase {
    
    String SIMPLE_PROFILE = ""
    + "ocio_profile_version: 1\n"
    + "\n"
    + "strictparsing: false\n"
    + "\n"
    + "colorspaces:\n"
    + "\n"
    + "  - !<ColorSpace>\n"
    + "    name: lnh\n"
    + "    bitdepth: 16f\n"
    + "    isdata: false\n"
    + "    allocation: lg2\n"
    + "\n"
    + "  - !<ColorSpace>\n"
    + "    name: test\n"
    + "    bitdepth: 8ui\n"
    + "    isdata: false\n"
    + "    allocation: uniform\n"
    + "    to_reference: !<ExponentTransform> {value: [2.2, 2.2, 2.2, 1]}\n";
    
    String EXPECTED_LUT = ""
    + "CSPLUTV100\n"
    + "3D\n"
    + "BEGIN METADATA\n"
    + "this is some metadata!\n"
    + "END METADATA\n"
    + "\n"
    + "4\n"
    + "0.000977 0.039373 1.587401 64.000000\n"
    + "0.000000 0.333333 0.666667 1.000000\n"
    + "4\n"
    + "0.000977 0.039373 1.587401 64.000000\n"
    + "0.000000 0.333333 0.666667 1.000000\n"
    + "4\n"
    + "0.000977 0.039373 1.587401 64.000000\n"
    + "0.000000 0.333333 0.666667 1.000000\n"
    + "\n"
    + "2 2 2\n"
    + "0.042823 0.042823 0.042823\n"
    + "6.622026 0.042823 0.042823\n"
    + "0.042823 6.622026 0.042823\n"
    + "6.622026 6.622026 0.042823\n"
    + "0.042823 0.042823 6.622026\n"
    + "6.622026 0.042823 6.622026\n"
    + "0.042823 6.622026 6.622026\n"
    + "6.622026 6.622026 6.622026\n"
    + "\n";
    
    protected void setUp() {
    }
    
    protected void tearDown() {
    }
    
    public void test_interface() {
        Baker bake = new Baker().Create();
        Baker bakee = bake.createEditableCopy();
        Config cfg = new Config().CreateFromStream(SIMPLE_PROFILE);
        bakee.setConfig(cfg);
        bakee.setFormat("cinespace");
        assertEquals("cinespace", bakee.getFormat());
        bakee.setType("3D");
        assertEquals("3D", bakee.getType());
        bakee.setMetadata("this is some metadata!");
        assertEquals("this is some metadata!", bakee.getMetadata());
        bakee.setInputSpace("lnh");
        assertEquals("lnh", bakee.getInputSpace());
        bakee.setTargetSpace("test");
        assertEquals("test", bakee.getTargetSpace());
        bakee.setShaperSize(4);
        assertEquals(4, bakee.getShaperSize());
        bakee.setCubeSize(2);
        assertEquals(2, bakee.getCubeSize());
        String output = bakee.bake();
        assertEquals(EXPECTED_LUT, output);
    }
    
}
