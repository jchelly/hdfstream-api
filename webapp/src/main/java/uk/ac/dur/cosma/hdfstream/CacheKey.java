package uk.ac.dur.cosma.hdfstream;

public record CacheKey(
    String path,
    String object,
    int max_depth,
    long data_size_limit
) {}
