package uk.ac.dur.cosma.virtual_directory;

public class VirtualPathInfo {

    public VirtualDirectory directory;
    public VirtualFile file;
    public String basename;

    public VirtualPathInfo() {
        this.directory = null;
        this.file = null;
        this.basename = null;
    }

    public VirtualPathInfo(VirtualDirectory directory, VirtualFile file, String basename) {
        this.directory = directory;
        this.file = file;
        this.basename = basename;
    }
}
