import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
import com.github.benmanes.caffeine.cache.Caffeine;
import uk.ac.dur.cosma.hdfstream.CacheKey;
import uk.ac.dur.cosma.virtual_directory.VirtualFile;


class TestCacheKey {

    @Test
    void equalKeysAreEqualSameFile() {
        VirtualFile file = new VirtualFile("/virtual_path", "/filesystem_path", 10L, 10L, "mediatype");
        var k1 = new CacheKey(file, "b", 1, 10L);
        var k2 = new CacheKey(file, "b", 1, 10L);
        assertEquals(k1, k2);
        assertEquals(k1.hashCode(), k2.hashCode());
    }

    @Test
    void equalKeysAreEqualIdenticalFile() {
        VirtualFile file1 = new VirtualFile("/virtual_path", "/filesystem_path", 10L, 10L, "mediatype");
        VirtualFile file2 = new VirtualFile("/virtual_path", "/filesystem_path", 10L, 10L, "mediatype");
        var k1 = new CacheKey(file1, "b", 1, 10L);
        var k2 = new CacheKey(file2, "b", 1, 10L);
        assertEquals(k1, k2);
        assertEquals(k1.hashCode(), k2.hashCode());
    }

    @Test
    void differentFileSystemPathMakesKeysUnequal() {
        VirtualFile file1 = new VirtualFile("/virtual_path", "/filesystem_path1", 10L, 10L, "mediatype");
        VirtualFile file2 = new VirtualFile("/virtual_path", "/filesystem_path2", 10L, 10L, "mediatype");
        var k1 = new CacheKey(file1, "b", 1, 10L);
        var k2 = new CacheKey(file2, "b", 1, 10L);
        assertNotEquals(k1, k2);
    }

    @Test
    void differentObjectMakesKeysUnequal() {
        VirtualFile file = new VirtualFile("/virtual_path", "/filesystem_path", 10L, 10L, "mediatype");
        var k1 = new CacheKey(file, "b", 1, 10L);
        var k2 = new CacheKey(file, "z", 1, 10L);
        assertNotEquals(k1, k2);
    }

    @Test
    void differentMaxDepthMakesKeysUnequal() {
        VirtualFile file = new VirtualFile("/virtual_path", "/filesystem_path", 10L, 10L, "mediatype");
        var k1 = new CacheKey(file, "b", 1, 10L);
        var k2 = new CacheKey(file, "b", 2, 10L);
        assertNotEquals(k1, k2);
    }

    @Test
    void differentDataSizeLimitMakesKeysUnequal() {
        VirtualFile file = new VirtualFile("/virtual_path", "/filesystem_path", 10L, 10L, "mediatype");
        var k1 = new CacheKey(file, "b", 1, 10L);
        var k2 = new CacheKey(file, "b", 1, 20L);
        assertNotEquals(k1, k2);
    }

    @Test
    void differentLastModifiedMakesKeysUnequal() {
        VirtualFile file1 = new VirtualFile("/virtual_path", "/filesystem_path", 10L, 10L, "mediatype");
        VirtualFile file2 = new VirtualFile("/virtual_path", "/filesystem_path", 10L, 11L, "mediatype");
        var k1 = new CacheKey(file1, "b", 1, 10L);
        var k2 = new CacheKey(file2, "b", 1, 20L);
        assertNotEquals(k1, k2);
    }

    @Test
    void cacheDistinguishesDifferentKeys() {
        var cache = Caffeine.newBuilder().build();
        VirtualFile file = new VirtualFile("/virtual_path", "/filesystem_path", 10L, 10L, "mediatype");
        var k1 = new CacheKey(file, "o", 1, 100L);
        var k2 = new CacheKey(file, "o", 2, 100L);
        assertNotEquals(k1, k2);
        cache.put(k1, "value1");
        cache.put(k2, "value2");
        assertEquals("value1", cache.getIfPresent(k1));
        assertEquals("value2", cache.getIfPresent(k2));
    }

    @Test
    void cacheReturnsSameValueForEqualKeys() {
        var cache = Caffeine.newBuilder().build();
        VirtualFile file = new VirtualFile("/virtual_path", "/filesystem_path", 10L, 10L, "mediatype");
        var k1 = new CacheKey(file, "o", 1, 100L);
        var k2 = new CacheKey(file, "o", 1, 100L);
        cache.put(k1, "value");
        assertEquals("value", cache.getIfPresent(k2));
    }
}
