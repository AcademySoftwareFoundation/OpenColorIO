
import junit.framework.TestCase;
import org.OpenColorIO.*;

public class ColorSpaceTest extends TestCase {
    
    protected void setUp() {
    }
    
    protected void tearDown() {
    }
    
    public void test_interface() {
        ColorSpace cs = new ColorSpace().Create();
        cs.setName("mynewcolspace");
        assertEquals("mynewcolspace", cs.getName());
        cs.setFamily("fam1");
        assertEquals("fam1", cs.getFamily());
        cs.setEqualityGroup("match1");
        assertEquals("match1", cs.getEqualityGroup());
        cs.setDescription("this is a test");
        assertEquals("this is a test", cs.getDescription());
        cs.setBitDepth(BitDepth.BIT_DEPTH_F16);
        assertEquals(BitDepth.BIT_DEPTH_F16, cs.getBitDepth());
        cs.setIsData(false);
        assertEquals(false, cs.isData());
        cs.setAllocation(Allocation.ALLOCATION_LG2);
        assertEquals(Allocation.ALLOCATION_LG2, cs.getAllocation());
        float test[] = new float[]{0.1f, 0.2f, 0.3f};
        cs.setAllocationVars(3, test);
        assertEquals(3, cs.getAllocationNumVars());
        float out[] = new float[3];
        cs.getAllocationVars(out);
        LogTransform lt = new LogTransform().Create();
        lt.setBase(10.f);
        cs.setTransform(lt, ColorSpaceDirection.COLORSPACE_DIR_TO_REFERENCE);
        LogTransform ott = (LogTransform)cs.getTransform(ColorSpaceDirection.COLORSPACE_DIR_TO_REFERENCE);
        assertEquals(10.f, ott.getBase());
    }
    
}
