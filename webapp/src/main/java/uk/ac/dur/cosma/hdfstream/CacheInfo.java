package uk.ac.dur.cosma.hdfstream;

import com.github.benmanes.caffeine.cache.Cache;
import com.github.benmanes.caffeine.cache.Caffeine;


public class CacheInfo {

    // Cache for metadata requests
    public Cache<String, byte []> request_cache;

    // Maximum size of response to cache
    public long max_cached_response_size;;

    // Maximum allowed size of the cache
    public long max_cache_size;;

    public CacheInfo(long max_cached_response_size, long max_cache_size) {

        // Store cache parameters
        this.max_cached_response_size = max_cached_response_size;
        this.max_cache_size = max_cache_size;

        // Set up the cache with a maximum size in bytes
        this.request_cache = Caffeine.newBuilder()
            .maximumWeight(max_cache_size)
            .weigher((String key, byte[] val) -> val.length)
            .build();
    }
}
