package uk.ac.dur.cosma.hdfstream;

import java.util.Objects;

public final class CacheKey {

    // Path to a virtual file
    private final String path;

    // HDF5 object name within the file
    private final String object;

    // Maximum recursion depth for returning sub-groups
    private final int max_depth;

    // Maximum size of dataset contents to return inline (bytes)
    private final long data_size_limit;

    public CacheKey(String path, String object, int max_depth, long data_size_limit) {
        this.path = path;
        this.object = object;
        this.max_depth = max_depth;
        this.data_size_limit = data_size_limit;
    }

    public String path() { return path; }
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
               Objects.equals(path, other.path) &&
               Objects.equals(object, other.object);
    }

    @Override
    public int hashCode() {
        return Objects.hash(path, object, max_depth, data_size_limit);
    }

    @Override
    public String toString() {
        return "CacheKey[path=" + path +
               ", object=" + object +
               ", max_depth=" + max_depth +
               ", data_size_limit=" + data_size_limit + "]";
    }
}
