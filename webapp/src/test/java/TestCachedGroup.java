import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.ValueSource;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.AfterAll;
import static org.junit.jupiter.api.Assertions.*;

import java.util.Random;

/*
  Test requesting a HDF5 group from the server.

  Make repeated requests for a few files and groups to
  catch incorrect/non-unique cache keys.
*/
public class TestCachedGroup extends BasicUnitTest {

    private static int nr_files = 5;
    private static int nr_groups = 5;

    @BeforeAll
    public static void setUp() throws Exception {
        long max_response_cache_size = 10*1024;
        int max_cached_response_size = 1024;
        startServer("../../data/tests/cache/config.csv",
                    max_response_cache_size,
                    max_cached_response_size);
    }

    private void tryRead(int file_nr, int group_nr, int max_depth, long data_size_limit) throws Exception {

        // Read the group
        String filename = "/tests/cache/test_data"+Integer.toString(file_nr)+".hdf5";
        String object = "Group"+Integer.toString(group_nr);
        HDF5Object root = client.requestObject(filename, object, Integer.toString(max_depth), Long.toString(data_size_limit), 200, true);

        // Check we read the right one!
        long file_nr_attr = root.attributes.get("file_nr").getElementAsLong(0);
        assertEquals(file_nr, file_nr_attr);
        long group_nr_attr = root.attributes.get("group_nr").getElementAsLong(0);
        assertEquals(group_nr, group_nr_attr);

        // If data_size_limit >= 8, we should have downloaded the dataset body
        HDF5Object dataset = root.members.get("Dataset"+Integer.toString(group_nr));
        assertNotNull(dataset);
        if(data_size_limit >= 8) {
            assertNotNull(dataset.data);
        } else {
            assertNull(dataset.data);
        }

        // If max_depth > 0, we should have a sub-group, too
        HDF5Object subgroup =  root.members.get("SubGroup"+Integer.toString(group_nr));
        if(max_depth > 0) {
            assertNotNull(subgroup);
        } else {
            assertNull(subgroup);
        }
    }

    @Test
    public void testReadGroupRandom() throws Exception {

        Random random = new Random(0);
        int nr_reps = 200;
        for(int rep_nr=0; rep_nr<nr_reps; rep_nr+=1) {

            // Pick a random file and group to read
            int file_nr = random.nextInt(nr_files);
            int group_nr = random.nextInt(nr_groups);
            // Random depth and size limits
            int max_depth = random.nextInt(2);
            long data_size_limit = random.nextInt(2) * 10;
            // Read the group and check we get the right one
            tryRead(file_nr, group_nr, max_depth, data_size_limit);
        }
    }

    @Test
    public void testReadGroupSequential() throws Exception {

        int nr_reps = 20;
        for(int rep_nr=0; rep_nr<nr_reps; rep_nr+=1) {
            for(int file_nr=0; file_nr<nr_files; file_nr+=1) {
                for(int group_nr=0; group_nr<nr_groups; group_nr+=1) {
                    for(int max_depth=0; max_depth<2; max_depth+=1) {
                        for(long data_size_limit=0; data_size_limit<11; data_size_limit+=10) {
                            // Read the group and check we get the right one
                            tryRead(file_nr, group_nr, max_depth, data_size_limit);
                        }
                    }
                }
            }
        }
    }
}
