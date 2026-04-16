import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.ValueSource;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.AfterAll;
import static org.junit.jupiter.api.Assertions.*;
import org.junit.jupiter.params.provider.Arguments;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.nio.charset.StandardCharsets;
import java.util.stream.Stream;
import java.util.Arrays;

import uk.ac.dur.cosma.hdfstream.StreamCopier;

/*
  Test stream copier utility methods.
*/
public class TestStreamCopier {

    // Stream lengths to test
    private static int dataSizes[] = {0, 1, 2, 3, 4, 10, 20, 32, 64, 1000, 1024};

    // Buffer sizes to test
    private static int bufferSizes[] = {1, 2, 3, 4, 8, 16, 19, 20, 21, 999, 1000, 1001, 10000};

    // Size thresholds to test
    private static int maxSizes[] = {0, 1, 2, 3, 4, 8, 16, 19, 20, 21, 999, 1000, 1001, 10000};

    private void runCopyStreamTest(int dataSize, int bufferSize) throws Exception {

        // Generate predictable test data
        byte[] data = new byte[dataSize];
        for (int i = 0; i < dataSize; i++) {
            data[i] = (byte) (i % 256);
        }

        // Create input and output streams
        InputStream in = new ByteArrayInputStream(data);
        ByteArrayOutputStream out = new ByteArrayOutputStream();

        // Copy the data
        StreamCopier.copyStream(in, out, bufferSize);

        // Check the result
        assertArrayEquals(data, out.toByteArray());
    }

    @Test
    public void testCopyStream() throws Exception {

        // Test all combinations
        for (int dataSize : dataSizes) {
            for (int bufferSize : bufferSizes) {
                runCopyStreamTest(dataSize, bufferSize);
            }
        }
    }

    private void runCopyStreamAndReturnIfSmallTest(int dataSize, int bufferSize, int maxSize) throws Exception {

        // Generate predictable test data
        byte[] data = new byte[dataSize];
        for (int i = 0; i < dataSize; i++) {
            data[i] = (byte) (i % 256);
        }

        // Create input and output streams
        InputStream in = new ByteArrayInputStream(data);
        ByteArrayOutputStream out = new ByteArrayOutputStream();

        // Copy the data
        byte[] kept = StreamCopier.copyStreamAndReturnIfSmall(in, out, bufferSize, maxSize);

        // Check we really copied the stream
        assertArrayEquals(data, out.toByteArray());

        // Check the buffered data
        if(dataSize > maxSize) {
            // If the stream is too long, nothing is kept
            assertNull(kept);
        } else {
            // Otherwise we should have the original data as a new byte array
            assertNotNull(kept);
            assertEquals(data.length, kept.length);
            assertArrayEquals(kept, data);
        }
    }

    @Test
    public void testCopyStreamAndReturnIfSmall() throws Exception {

        // Test all combinations
        for (int dataSize : dataSizes) {
            for (int bufferSize : bufferSizes) {
                for (int maxSize : maxSizes) {
                    runCopyStreamAndReturnIfSmallTest(dataSize, bufferSize, maxSize);
                }
            }
        }
    }
}
