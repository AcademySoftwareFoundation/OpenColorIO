
import junit.framework.TestCase;
import org.OpenColorIO.*;

public class LookTest extends TestCase {
    
    protected void setUp() {
    }
    
    protected void tearDown() {
    }
    
    public void test_interface() {
        Look lk = new Look().Create();
        lk.setName("coollook");
        assertEquals("coollook", lk.getName());
        lk.setProcessSpace("somespace");
        assertEquals("somespace", lk.getProcessSpace());
        lk.setDescription("this is a test");
        assertEquals("this is a test", lk.getDescription());
        ExponentTransform et = new ExponentTransform().Create();
        et.setValue(new float[]{0.1f, 0.2f, 0.3f, 0.4f});
        lk.setTransform(et);
        ExponentTransform oet = (ExponentTransform)lk.getTransform();
        float vals[] = new float[4];
        oet.getValue(vals);
        assertEquals(0.2f, vals[1], 1e-8);
        ExponentTransform iet = new ExponentTransform().Create();
        iet.setValue(new float[]{-0.1f, -0.2f, -0.3f, -0.4f});
        lk.setInverseTransform(iet);
        ExponentTransform ioet = (ExponentTransform)lk.getInverseTransform();
        float vals2[] = new float[4];
        ioet.getValue(vals2);
        assertEquals(-0.2f, vals2[1], 1e-8);
    }
    
}
