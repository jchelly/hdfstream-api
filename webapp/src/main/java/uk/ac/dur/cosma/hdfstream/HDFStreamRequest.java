package uk.ac.dur.cosma.hdfstream;

import java.lang.Long;
import java.lang.Integer;

import javax.servlet.http.HttpServletResponse;
import java.util.Map;
import java.util.LinkedHashMap;
import java.lang.Math;
import java.io.IOException;
import java.io.OutputStream;
import uk.ac.dur.cosma.virtual_directory.VirtualDirectory;
import uk.ac.dur.cosma.virtual_directory.VirtualFile;
import uk.ac.dur.cosma.virtual_directory.VirtualDirectoryException;
import uk.ac.dur.cosma.virtual_directory.VirtualPathInfo;
import uk.ac.dur.cosma.virtual_directory.CheckRole;
import uk.ac.dur.cosma.virtual_directory.DirectoryMetadata;
import org.msgpack.core.MessagePacker;
import org.msgpack.core.MessagePack;
import com.github.benmanes.caffeine.cache.Cache;

import uk.ac.dur.cosma.libhdfstream.*;

/*
  Class for handling requests for msgpack encoded data
*/
public class HDFStreamRequest {

    protected VirtualFile file = null;
    protected VirtualDirectory directory = null;
    protected String object = null;
    protected int max_depth = 0;
    protected long data_size_limit = Integer.MAX_VALUE;
    protected SliceInfo slice_info = null;
    protected CheckRole in_role = null;
    protected CacheInfo cache_info = null;

    /*
      Requests are initialized using parameters extracted from a http get or
      post request.

      virtual_directory: root of the virtual directory structure
      in_role: in_role(name) returns True if user is in role 'name'
      path: virtual path to the requested object
      object: name of the requested HDF5 object (or null)
      max_depth: maximum recursion depth
      data_size_limit: maximum size of dataset bodies to return
    */
    public HDFStreamRequest(VirtualDirectory virtual_directory, CheckRole in_role, CacheInfo cache_info,
                            String path, String object, int max_depth, long data_size_limit, SliceInfo slice,
                            int max_hdf5_name_length) throws HDFStreamRequestException {

        // Keep a reference to the cache
        this.cache_info = cache_info;

        // Reject very long paths
        if(path != null) {
            if((max_hdf5_name_length > 0) && (path.length() > max_hdf5_name_length)) {
                throw new HDFStreamRequestException(HttpServletResponse.SC_BAD_REQUEST, "Path is too long: "+path);
            }
        }

        // Reject negative size limits
        if(max_depth < 0) {
            throw new HDFStreamRequestException(HttpServletResponse.SC_BAD_REQUEST, "max_depth must not be negative");
        }
        if(data_size_limit < 0) {
            throw new HDFStreamRequestException(HttpServletResponse.SC_BAD_REQUEST, "data_size_limit must not be negative");
        }

        // Look up the specified virtual path, if we have one
        if((path != null) && (path.length() > 0)) {
            // We have a path, so find the corresponding virtual file or directory
            VirtualPathInfo path_info = null;
            try {
                path_info = virtual_directory.resolvePath(path, in_role);
            } catch (VirtualDirectoryException e) {
                throw new HDFStreamRequestException(HttpServletResponse.SC_NOT_FOUND, "Invalid path specified: "+path);
            }
            this.file = path_info.file;
            this.directory = path_info.directory;
        } else {
            // A zero length path will return a listing of the root directory
            this.file = null;
            this.directory = virtual_directory;
        }

        // Sanity check and store the HDF5 object name, if any
        if(object != null) {
            // Check the name doesn't exceed the length limit
            if((max_hdf5_name_length > 0) && (object.length() > max_hdf5_name_length)) {
                throw new HDFStreamRequestException(HttpServletResponse.SC_BAD_REQUEST, "Object name is too long");
            }
            // Require object names to be ascii only:
            // Unicode support in HDF5 was added relatively recently and we don't have any files with
            // unicode names, so disallow them in case of vulnerabilities in libhdf5.
            if(!StringSanitizer.isPrintableAscii(object)) {
                throw new HDFStreamRequestException(HttpServletResponse.SC_BAD_REQUEST, "Object names must contain printable ascii characters only");
            }
            this.object = object;
        }

        // Check slicing information
        if(slice != null) {
            if(object == null)
                throw new HDFStreamRequestException(HttpServletResponse.SC_BAD_REQUEST, "Can't specify slicing parameters without object name");
            this.slice_info = slice;
        }

        // Store other parameters
        this.in_role = in_role;
        this.max_depth = max_depth;
        this.data_size_limit = data_size_limit;
    }

    protected void packDirectory(MessagePacker packer, VirtualDirectory directory, int max_depth, CheckRole in_role) throws IOException {

        // A directory is a map with "type", "size", "files" and "directories" entries.
        // It might also have "description" and "labels" entries.
        String description = null;
        LinkedHashMap<String, String> labels = null;
        DirectoryMetadata md = directory.getMetadata();
        if(md != null) {
            description = md.description;
            labels = md.labels;
        }
        int nr_entries = 6;
        packer.packMapHeader(nr_entries);

        // Indicate that this is a directory
        packer.packString("type");
        packer.packString("directory");

        // Store the directory size
        packer.packString("size");
        packer.packLong(directory.getTotalSize(in_role));

        // Store the description, if we have one
        packer.packString("description");
        if(description != null) {
            packer.packString(description);
        } else {
            packer.packNil();
        }

        // Store the labels, if we have them
        packer.packString("labels");
        if(labels != null) {
            packer.packMapHeader(labels.size());
            for(Map.Entry<String, String> entry : labels.entrySet()) {
                String label_key = entry.getKey();
                String label_value = entry.getValue();
                packer.packString(label_key);
                packer.packString(label_value);
            }
        } else {
            packer.packNil();
        }

        // Pack map of {name : file} pairs for files in this directory
        packer.packString("files");
        LinkedHashMap<String, VirtualFile> files = directory.getFiles(in_role);
        packer.packMapHeader(files.size());
        for(Map.Entry<String, VirtualFile> entry : files.entrySet()) {
            String name = entry.getKey();
            VirtualFile file = entry.getValue();
            packer.packString(name);
            packFile(packer, file);
        }

        // Recursively pack the array of directories:
        // Also pack subdirectories until we hit the recursion limit, in which
        // case we pack a msgpack nil.
        packer.packString("directories");
        LinkedHashMap<String, VirtualDirectory> subdirs = directory.getDirectories(in_role);
        packer.packMapHeader(subdirs.size());
        for(Map.Entry<String, VirtualDirectory> entry : subdirs.entrySet()) {
            String name = entry.getKey();
            VirtualDirectory subdir = entry.getValue();
            packer.packString(name);
            if(max_depth > 0) {
                packDirectory(packer, subdir, max_depth-1, in_role);
            } else {
                packer.packNil();
            }
        }
        return;
    }

    protected void packFile(MessagePacker packer, VirtualFile file) throws IOException {

        // A file is a map with "type", "size" and "last_modified" entries.
        packer.packMapHeader(3);

        // Indicate type of this file
        packer.packString("type");
        packer.packString(file.media_type);

        // Pack the file size
        packer.packString("size");
        packer.packLong(file.length);

        // Pack the last modification time
        packer.packString("last_modified");
        packer.packLong(file.last_modified);

        return;
    }

    protected void streamDirectory(OutputStream out) throws IOException {

        MessagePacker packer = MessagePack.newDefaultPacker(out);
        packDirectory(packer, directory, max_depth, in_role);
        packer.close();
        return;
    }

    protected void streamFile(OutputStream out) throws IOException {

        MessagePacker packer = MessagePack.newDefaultPacker(out);
        packFile(packer, file);
        packer.close();
        return;
    }

    protected void streamObject(HDFStream hs, OutputStream out, int buffer_size, boolean write_body)
        throws IOException, HDFStreamRequestException {

        // Return an internal server error if hdfstream didn't start
        if(hs==null) {
            throw new HDFStreamRequestException(HttpServletResponse.SC_INTERNAL_SERVER_ERROR, "Hdfstream library initialization failed");
        }

        // Check we have an object name and path
        if(object==null) {
            throw new HDFStreamRequestException(HttpServletResponse.SC_INTERNAL_SERVER_ERROR, "Null object name in streamObject!");
        }
        if(file==null) {
            throw new HDFStreamRequestException(HttpServletResponse.SC_INTERNAL_SERVER_ERROR, "Null file in streamObject!");
        }

        // Open the object to stream
        if(slice_info == null) {

            // In this case we're returning a whole object, possibly recursively. Only the object name is compulsory
            // and we shouldn't be here if it wasn't specified. Make a cache key for this request.
            String cache_key = file.filesystem_path + ";" + object + ";" + Integer.toString(max_depth) + ";" + Long.toString(data_size_limit);

            // Return a cached response if we can
            byte[] cached_data = cache_info.request_cache.getIfPresent(cache_key);
            if(cached_data != null) {
                // Response is in the cache
                if(write_body)out.write(cached_data);
                return;
            }

            // Otherwise we need to read the data
            byte[] data = null;
            try (DataStream stream = hs.openObject(file.filesystem_path, object, max_depth, buffer_size, data_size_limit)) {
                if(write_body) {
                    data = StreamCopier.copyStreamAndReturnIfSmall(stream, out, buffer_size, cache_info.max_cached_response_size);
                }
            } catch (IOException e) {
                throw new HDFStreamRequestException(HttpServletResponse.SC_NOT_FOUND, e.getMessage());
            }

            // Cache the response, if it was below the size threshold
            if(data != null)cache_info.request_cache.put(cache_key, data);

        } else {
            // In this case we're taking one or more dataset slices. These are not cached.
            try (DataStream stream = hs.openDatasetSlices(file.filesystem_path, object, slice_info.nr_slices,
                                                          slice_info.rank, slice_info.starts, slice_info.counts, buffer_size)) {
                if(write_body)StreamCopier.copyStream(stream, out, buffer_size);
            } catch (IOException e) {
                throw new HDFStreamRequestException(HttpServletResponse.SC_NOT_FOUND, e.getMessage());
            }
        }
	return;
    }

    public void streamResponse(HDFStream hs, OutputStream out, int buffer_size, boolean write_body) throws IOException, HDFStreamRequestException {

        if(file == null) {
            // Stream directory listing in msgpack format
            if(write_body)streamDirectory(out);
        } else if(object == null) {
            // Stream file metadata in msgpack format
            if(write_body)streamFile(out);
        } else {
            // Stream a HDF5 object in msgpack format. Path must be a HDF5 file in this case.
            streamObject(hs, out, buffer_size, write_body);
        }
    }
}
