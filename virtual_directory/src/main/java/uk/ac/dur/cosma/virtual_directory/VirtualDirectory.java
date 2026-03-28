package uk.ac.dur.cosma.virtual_directory;

import java.util.Set;
import java.util.HashSet;
import java.util.Map;
import java.util.LinkedHashMap;
import java.util.LinkedList;
import java.io.FileReader;
import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.text.DecimalFormat;

public class VirtualDirectory {

    public String virtual_path = null;
    private String real_path = null;
    private volatile DirectoryMetadata metadata;
    private LinkedHashMap<String, VirtualFile> files = null;
    private LinkedHashMap<String, VirtualDirectory> directories = null;
    private VirtualDirectory parent = null;
    private long size_of_files = 0;

    // Directories can only be accessed by users belonging to all roles in the set.
    // All roles from the parent directory are inherited.
    public HashSet<String> required_roles;

    // Check if we can access this directory
    private boolean canAccess(CheckRole in_role) {
        for(String role: required_roles) {
            if(!in_role.check(role))return false;
        }
        return true;
    }

    public void setRealPath(String real_path) throws VirtualDirectoryException {
	if(this.real_path != null)throw new VirtualDirectoryException("Attempted to set directory metadata path more than once");
	if(!real_path.trim().isEmpty()) {
	    this.real_path = real_path.trim();
	} else {
	    this.real_path = null;
	}
    }

    public DirectoryMetadata getMetadata() throws IOException {

	// Handle the case where there is no metadata
	if(real_path==null)return null;

	// Read the metadata file, if we didn't already
	if (metadata == null) {
            synchronized (this) {
                if (metadata == null) {		    
                    metadata = new DirectoryMetadata(real_path);
                }
            }
        }
	return metadata;
    }
    
    public static String formatSize(long size) {
        final String[] units = new String[] { "B", "kB", "MB", "GB", "TB", "PB", "EB" };
        int digitGroups = (int) (Math.log10(size)/Math.log10(1024));
        return new DecimalFormat("#,##0.#").format(size/Math.pow(1024, digitGroups)) + " " + units[digitGroups];
    }

    /*
      Create an empty root VirtualDirectory with no parent and no access restrictions
    */
    public VirtualDirectory() {
        files = new LinkedHashMap<String, VirtualFile>();
        directories = new LinkedHashMap<String, VirtualDirectory>();
        parent = null;
        virtual_path = "";
        required_roles = new HashSet<String>(); // Accessible to all by default
        size_of_files = 0;
    }

    /*
      Create an empty VirtualDirectory at the specified path, inheriting access restrictions
    */
    public VirtualDirectory(VirtualDirectory parent, String virtual_path) {
        files = new LinkedHashMap<String, VirtualFile>();
        directories = new LinkedHashMap<String, VirtualDirectory>();
        this.parent = parent;
        this.virtual_path = virtual_path;
        // Required roles for this directory are the parent's roles
        required_roles = new HashSet<String>(parent.required_roles);
        size_of_files = 0;
    }

    /*
      Create an empty VirtualDirectory at the specified path, inheriting access restrictions
      and possibly adding new restrictions.
    */
    public VirtualDirectory(VirtualDirectory parent, String virtual_path, Set<String> roles) {
        files = new LinkedHashMap<String, VirtualFile>();
        directories = new LinkedHashMap<String, VirtualDirectory>();
        this.parent = parent;
        this.virtual_path = virtual_path;
        // Required roles for this directory are the parent's roles
        // plus any passed in the roles parameter.
        required_roles = new HashSet<String>(parent.required_roles);
        for (String role : roles) {
            required_roles.add(role);
        }
        size_of_files = 0;
    }

    /*
      Split a path into a leading directory name and remainder.

      Path must not start or end with a slash. Returns a two
      element array of strings. The first element is null if
      there are no path separators in the string.

      Path components must not have zero length (so no '//')
    */
    private String[] splitPath(String path) throws VirtualDirectoryException {

        // Do some sanity checks on the path
        if(path.length()==0)
            throw new VirtualDirectoryException("Paths must not have zero length");
        if(path.startsWith("/"))
            throw new VirtualDirectoryException("Path "+path+" must not start with a slash");
        if(path.endsWith("/"))
            throw new VirtualDirectoryException("Path "+path+" must not end with a slash");

        String[] result = new String[2];

        // Split into directory name and rest of path at the first slash
	String components[] = path.split("/", 2);
	if(components.length == 1) {
            result[0] = null;
            result[1] = components[0];
            if(result[1].length()==0)
                throw new VirtualDirectoryException("Path component has zero length!");
        } else {
            result[0] = components[0];
            if(result[0].length()==0)
                throw new VirtualDirectoryException("Path component has zero length!");
            result[1] = components[1];
            if(result[1].length()==0)
                throw new VirtualDirectoryException("Path component has zero length!");
        }
        return result;
    }

    /*
      Create a virtual file at the specified location relative to
      this directory. Creates intermediate directories as needed.

      virtual_path must be relative to this directory and cannot
      start or end with a slash.
    */
    private void addFile(String virtual_path, String real_path,
                         long length, long last_modified,
                         String media_type,
                         Set<String> required_roles) throws VirtualDirectoryException {

	/* Split path into directory name and rest of the path */
	String components[] = splitPath(virtual_path);
        String subdir_name = components[0];
        String remainder = components[1];

        if(subdir_name == null) {
            if(media_type.equals("directory")) {
                /* Path is a directory in this directory. */
                if(files.containsKey(remainder))
                    throw new VirtualDirectoryException("Attempted to add directory with same name as file!");
                VirtualDirectory subdir = null;
                if(directories.containsKey(remainder) == false) {
                    subdir = new VirtualDirectory(this, this.virtual_path+"/"+remainder, required_roles);
		    subdir.setRealPath(real_path);
                    directories.put(remainder, subdir);
                } else {
                    // Directory already exists. Add any new access roles and the metadata path in this case.
                    subdir = directories.get(remainder);
		    subdir.setRealPath(real_path);
                    for(String role : required_roles) {
                        subdir.required_roles.add(role);
                    }
                }
                return;
            } else {
                /* Path is a file in this directory */
                if(files.containsKey(remainder))
                    throw new VirtualDirectoryException("Attempted to add duplicate file!");
                if(directories.containsKey(remainder))
                    throw new VirtualDirectoryException("Attempted to add file with same name as directory!");
                VirtualFile new_file = new VirtualFile(this.virtual_path+"/"+remainder, real_path, length, last_modified, media_type);
                files.put(remainder, new_file);
                // Accumulate total size of files in this directory
                this.size_of_files += new_file.length;
                return;
            }
        } else {
            /* Path is in some subdirectory */
            VirtualDirectory subdir = directories.get(subdir_name);
            if(subdir == null) {
                /* Need to create the subdirectory */
                if(this.virtual_path.length() > 0)
                    subdir = new VirtualDirectory(this, this.virtual_path+"/"+subdir_name);
                else
                    subdir = new VirtualDirectory(this, subdir_name);
                if(files.containsKey(subdir_name))
                    throw new VirtualDirectoryException("Attempted to add directory with same name as file: "+subdir_name+" : "+media_type);
                directories.put(subdir_name, subdir);
            }
            subdir.addFile(remainder, real_path, length, last_modified, media_type, required_roles);
            return;
        }
    }

    /*
      Set up a directory tree by reading a CSV file
    */
    public void addFromFile(String filename) throws VirtualDirectoryException {
        try {
            BufferedReader br = new BufferedReader(new FileReader(filename));
            addFromReader(br);
        } catch (FileNotFoundException e) {
            throw new VirtualDirectoryException("Csv config file " + filename + " not found");
        }
    }

    /*
      Set up a directory tree by reading CSV from a buffered reader.

      Columns in the csv file:

      0. Virtual filename
      1. Real filename
      2. File size in bytes
      3. Last modification time (seconds since the epoch)
      4. Media type
      5. Required security roles (optional, semicolon separated, only affects directories)
    */
    public void addFromReader(BufferedReader br) throws VirtualDirectoryException {
        try {
            String line;
            while ((line = br.readLine()) != null) {
                String fields[] = line.split(",");
                if((fields.length < 5) || (fields.length > 7))
                    throw new VirtualDirectoryException("Unable to interpret line in csv config file");
                String virtual_filename = fields[0].trim();
                String real_filename = fields[1].trim();
                String media_type = fields[4].trim();
                HashSet<String> required_roles = new HashSet<String>();
                if(fields.length >= 6) {
                    for(String role : fields[5].trim().split(";")) {
                        required_roles.add(role);
                    }
                }
                long length;
                long last_modified;
                try {
                    // File size in bytes
                    length = Long.valueOf(fields[2].trim());
                    // Last modification time.
                    // In the input file this should be in seconds. Here we convert to milliseconds.
                    last_modified = Long.valueOf(fields[3].trim()) * 1000;
                } catch (NumberFormatException e) {
                    throw new VirtualDirectoryException(e.getMessage());
                }
                addFile(virtual_filename, real_filename, length, last_modified, media_type, required_roles);
            }
        } catch (IOException e) {
            throw new VirtualDirectoryException("Unable to read csv config file");
        }
    }

    /*
      Return a map of files in this directory, or an empty map if
      this directory is not accessible.
    */
    public LinkedHashMap<String, VirtualFile> getFiles(CheckRole in_role) {
        if(canAccess(in_role)) {
            return files;
        } else {
            return new LinkedHashMap<String, VirtualFile>();
        }
    }

    /*
      Return a map of subdirectories the user can access.
      Return an empty map if this directory is not accessible.
    */
    public LinkedHashMap<String, VirtualDirectory> getDirectories(CheckRole in_role) {

        LinkedHashMap<String, VirtualDirectory> dirs = new LinkedHashMap<String, VirtualDirectory>();
        if(canAccess(in_role)) {
            for (Map.Entry<String, VirtualDirectory> entry : directories.entrySet()) {
                String name = entry.getKey();
                VirtualDirectory dir = entry.getValue();
                if(dir.canAccess(in_role)) {
                    dirs.put(name, dir);
                }
            }
        }
        return dirs;
    }

    public void print(String path, CheckRole in_role) {

	/* Print names of files in this directory */
        for (Map.Entry<String, VirtualFile> entry : getFiles(in_role).entrySet()) {
            String virtual_name = entry.getKey();
            String real_name    = entry.getValue().filesystem_path;
            System.out.printf("  File %s : %s\n", path+"/"+virtual_name, real_name);
        }

	/* Print names of subdirectories */
        for (Map.Entry<String, VirtualDirectory> entry : getDirectories(in_role).entrySet()) {
            String name = entry.getKey();
            VirtualDirectory subdir = entry.getValue();
            System.out.printf("Subdirectory %s\n", path+"/"+name);
            subdir.print(path+"/"+name, in_role);
	}
    }

    private void getAllFilesRecursive(String path, CheckRole in_role, LinkedHashMap<String, VirtualFile> result) {

	/* Add files from this directory */
        for(Map.Entry<String, VirtualFile> entry : getFiles(in_role).entrySet()) {
            String virtual_path = path + entry.getKey();
            result.put(virtual_path, entry.getValue());
        }

        /* Add files from any sub-directories */
        for(Map.Entry<String, VirtualDirectory> entry : getDirectories(in_role).entrySet()) {
            String dir_name = path + entry.getKey() + "/";
            VirtualDirectory dir = entry.getValue();
            dir.getAllFilesRecursive(dir_name, in_role, result);
        }
    }

    /*
      Return (virtual_name : real_name) pairs for all files below this directory
    */
    public LinkedHashMap<String, VirtualFile> getAllFiles(CheckRole in_role) {
	LinkedHashMap<String, VirtualFile> result = new LinkedHashMap<String, VirtualFile>();
	getAllFilesRecursive("", in_role, result);
	return result;
    }

    /*
      Given a virtual path, identify the specified directory and (possibly) file.
      Also stores the virtual base name if the path is a file.
    */
    public VirtualPathInfo resolvePath(String virtual_name, CheckRole in_role) throws VirtualDirectoryException{

        /* In case of directories, we ignore any trailing slash in the path */
        if(virtual_name.endsWith("/")) {
            virtual_name = virtual_name.substring(0, virtual_name.length() - 1);
        }

        /* Don't allow zero length paths */
        if(virtual_name.length()==0) {
	    throw new VirtualDirectoryException("Zero length virtual path");
        }

	/* Split into directory name and rest of the path */
        String components[] = splitPath(virtual_name);
        String subdir_name = components[0];
        String remainder = components[1];

	/* Check for the case of only one component: could be a file or directory */
	if(subdir_name == null) {
	    VirtualDirectory directory = directories.get(remainder);
            VirtualFile file = files.get(remainder);
            if((directory == null) && (file == null))throw new VirtualDirectoryException("Virtual path not found: "+virtual_name);
            if(directory != null) {
                if(!directory.canAccess(in_role))throw new VirtualDirectoryException("Virtual path not found: "+virtual_name);
            }
            String basename = null;
            if(file != null) basename = remainder;
            return new VirtualPathInfo(directory, file, basename);
	}

	/* Otherwise, the first component must be a subdirectory */
	VirtualDirectory subdir = directories.get(subdir_name);
	if(subdir==null)throw new VirtualDirectoryException("Virtual directory not found: "+virtual_name);
        if(!subdir.canAccess(in_role))throw new VirtualDirectoryException("Virtual directory not found: "+virtual_name);

        VirtualFile subdir_file = subdir.files.get(remainder);
        if(subdir_file != null) {
            /* Second component is a file, so we're done */
            return new VirtualPathInfo(subdir, subdir_file, remainder);
        } else {
            /* Second component should be a directory */
            return subdir.resolvePath(remainder, in_role);
        }
    }

    /*
      Return the total size of this directory
    */
    public long getTotalSize(CheckRole in_role) {

        long size = 0;
        if(canAccess(in_role)) {
            /* Add sizes of files in this directory */
            size += this.size_of_files;

            /* Add files in any subdirectories */
            for (Map.Entry<String, VirtualDirectory> entry : directories.entrySet()) {
                size += entry.getValue().getTotalSize(in_role);
            }
        }
        return size;
    }
}
