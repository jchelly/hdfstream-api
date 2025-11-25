import uk.ac.dur.cosma.virtual_directory.VirtualDirectory;
import uk.ac.dur.cosma.virtual_directory.VirtualDirectoryException;

import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

// Very basic test to check that we can compile and run test cases
public class TestInit {

    @Test
    public void main() throws VirtualDirectoryException {
        VirtualDirectory root = new VirtualDirectory();
    }
}
