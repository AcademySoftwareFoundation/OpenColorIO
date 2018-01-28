
import junit.framework.TestCase;
import org.OpenColorIO.*;
import java.nio.*; 

public class PackedImageDescTest extends TestCase {
    
    protected void setUp() {
    }
    
    protected void tearDown() {
    }
    
    public void test_interface() {
        
        int width = 2;
        int height = 2;
        int channels = 4;
        float packedpix[] = new float[]{0.1f, 0.1f, 0.1f, 1.0f,
                                        0.2f, 0.2f, 0.2f, 1.0f,
                                        0.3f, 0.3f, 0.3f, 1.0f,
                                        0.4f, 0.4f, 0.4f, 1.0f };
        FloatBuffer buf = ByteBuffer.allocateDirect(width * height * channels
            * Float.SIZE / 8).asFloatBuffer();
        buf.put(packedpix);
        //
        PackedImageDesc foo = new PackedImageDesc(buf, width, height, channels);
        FloatBuffer wee = foo.getData();
        assertEquals(0.3f, wee.get(10));
        assertEquals(2, foo.getWidth());
        assertEquals(2, foo.getHeight());
        assertEquals(4, foo.getNumChannels());
        assertEquals(4, foo.getChanStrideBytes());
        assertEquals(16, foo.getXStrideBytes());
        assertEquals(32, foo.getYStrideBytes());
        
    }
    
}
