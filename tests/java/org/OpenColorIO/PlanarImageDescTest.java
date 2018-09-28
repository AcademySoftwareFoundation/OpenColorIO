
import junit.framework.TestCase;
import org.OpenColorIO.*;
import java.nio.*;

public class PlanarImageDescTest extends TestCase {
    
    protected void setUp() {
    }
    
    protected void tearDown() {
    }
    
    public void test_interface() {
        
        int width = 2;
        int height = 2;
        int channels = 4;
        //
        float rpix[] = new float[]{0.1f, 0.2f, 0.3f, 0.4f};
        float gpix[] = new float[]{0.1f, 0.2f, 0.3f, 0.4f};
        float bpix[] = new float[]{0.1f, 0.2f, 0.3f, 0.4f};
        float apix[] = new float[]{1.0f, 1.0f, 1.0f, 1.0f};
        int alloc = width * height * Float.SIZE / 8;
        //
        FloatBuffer rbuf = ByteBuffer.allocateDirect(alloc).asFloatBuffer();
        rbuf.put(rpix);
        FloatBuffer gbuf = ByteBuffer.allocateDirect(alloc).asFloatBuffer();
        gbuf.put(gpix);
        FloatBuffer bbuf = ByteBuffer.allocateDirect(alloc).asFloatBuffer();
        bbuf.put(bpix);
        FloatBuffer abuf = ByteBuffer.allocateDirect(alloc).asFloatBuffer();
        abuf.put(apix);
        //
        PlanarImageDesc foo = new PlanarImageDesc(rbuf, gbuf, bbuf, abuf,
                                                  width, height);
        FloatBuffer trb = foo.getRData();
        FloatBuffer tgb = foo.getGData();
        FloatBuffer tbb = foo.getBData();
        FloatBuffer tab = foo.getAData();
        assertEquals(0.1f, trb.get(0));
        assertEquals(0.2f, tgb.get(1));
        assertEquals(0.3f, tbb.get(2));
        assertEquals(1.0f, tab.get(3));
        assertEquals(2, foo.getWidth());
        assertEquals(2, foo.getHeight());
        assertEquals(8, foo.getYStrideBytes());
        
    }
    
}
