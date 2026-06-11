import org.junit.jupiter.api.Test;
import uk.ac.dur.cosma.hdfstream.RequestCounter;

public class TestRequestCounter {
    @Test
    public void test() throws Exception {
        RequestCounter rc = new RequestCounter();
        rc.logRequest(0,0);
    }
}
