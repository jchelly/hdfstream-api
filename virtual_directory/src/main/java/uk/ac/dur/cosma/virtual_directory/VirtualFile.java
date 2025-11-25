package uk.ac.dur.cosma.virtual_directory;

import java.io.File;

public class VirtualFile {

    // Location of this file on the real file system
    public String virtual_path;
    public String filesystem_path;
    public long length;
    public long last_modified;
    public String media_type;

    public VirtualFile(String virtual_path, String filesystem_path, long length, long last_modified, String media_type) {
        this.virtual_path = virtual_path;
        this.filesystem_path = filesystem_path;
        this.length = length;
        this.last_modified = last_modified;
        this.media_type = media_type;
    }

    public long getLength() {
        return length;
    }

    public long getLastModified() {
        return last_modified;
    }
}
