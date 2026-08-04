package uk.ac.dur.cosma.hdfstream;

import java.util.Objects;
import uk.ac.dur.cosma.virtual_directory.VirtualFile;


public final class CacheKey {

    // Path to a virtual file
    private final String path;

    // Last modification time of the file
    private final long last_modified;

    // HDF5 object name within the file
    private final String object;

    // Maximum recursion depth for returning sub-groups
    private final int max_depth;

    // Maximum size of dataset contents to return inline (bytes)
    private final long data_size_limit;

    public CacheKey(VirtualFile file, String object, int max_depth, long data_size_limit) {
        this.path = file.filesystem_path;
        this.last_modified = file.last_modified;
        this.object = object;
        this.max_depth = max_depth;
        this.data_size_limit = data_size_limit;
    }

    public String path() { return path; }
    public long last_modified() { return last_modified; }
    public String object() { return object; }
    public int max_depth() { return max_depth; }
    public long data_size_limit() { return data_size_limit; }

    @Override
    public boolean equals(Object o) {

        // Object is equal to itself
        if (this == o) return true;

        // An instance of a different class cannot be equal to this object
        if (!(o instanceof CacheKey)) return false;

        // Compare data fields
        CacheKey other = (CacheKey) o;
        return max_depth == other.max_depth &&
               data_size_limit == other.data_size_limit &&
               last_modified == other.last_modified &&
               Objects.equals(path, other.path) &&
               Objects.equals(object, other.object);
    }

    @Override
    public int hashCode() {
        return Objects.hash(path, last_modified, object, max_depth, data_size_limit);
    }

    @Override
    public String toString() {
        return "CacheKey[path=" + path +
               ", last_modified=" + last_modified +
               ", object=" + object +
               ", max_depth=" + max_depth +
               ", data_size_limit=" + data_size_limit + "]";
    }
}
