import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
import com.github.benmanes.caffeine.cache.Caffeine;
import uk.ac.dur.cosma.hdfstream.CacheKey;

class TestCacheKey {

    @Test
    void equalKeysAreEqual() {
        var k1 = new CacheKey("a", "b", 1, 10L);
        var k2 = new CacheKey("a", "b", 1, 10L);

        assertEquals(k1, k2);
        assertEquals(k1.hashCode(), k2.hashCode());
    }

    @Test
    void differentPathMakesKeysUnequal() {
        var k1 = new CacheKey("a", "b", 1, 10L);
        var k2 = new CacheKey("x", "b", 1, 10L);

        assertNotEquals(k1, k2);
    }

    @Test
    void differentObjectMakesKeysUnequal() {
        var k1 = new CacheKey("a", "b", 1, 10L);
        var k2 = new CacheKey("a", "z", 1, 10L);

        assertNotEquals(k1, k2);
    }

    @Test
    void differentMaxDepthMakesKeysUnequal() {
        var k1 = new CacheKey("a", "b", 1, 10L);
        var k2 = new CacheKey("a", "b", 2, 10L);

        assertNotEquals(k1, k2);
    }

    @Test
    void differentDataSizeLimitMakesKeysUnequal() {
        var k1 = new CacheKey("a", "b", 1, 10L);
        var k2 = new CacheKey("a", "b", 1, 20L);

        assertNotEquals(k1, k2);
    }

    @Test
    void cacheDistinguishesDifferentKeys() {
        var cache = Caffeine.newBuilder().build();

        var k1 = new CacheKey("p", "o", 1, 100L);
        var k2 = new CacheKey("p", "o", 2, 100L);

        cache.put(k1, "value1");
        cache.put(k2, "value2");

        assertEquals("value1", cache.getIfPresent(k1));
        assertEquals("value2", cache.getIfPresent(k2));
    }

    @Test
    void cacheReturnsSameValueForEqualKeys() {
        var cache = Caffeine.newBuilder().build();

        var k1 = new CacheKey("p", "o", 1, 100L);
        var k2 = new CacheKey("p", "o", 1, 100L);

        cache.put(k1, "value");

        assertEquals("value", cache.getIfPresent(k2));
    }
}
