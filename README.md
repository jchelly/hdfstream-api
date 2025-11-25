# HDF5 parallel streaming library and web application

This repository contains a C library capable of reading and serialization of
HDF5 data and a web application which uses this to allow remote access to
collections of HDF5 files.

## Dependencies

Requirements for the C API:

  * C compiler
  * msgpack-c library (4.0 or later)
  * HDF5 C library (1.12.0 or later)
  * CMake

Requirements for the web application:

  * Java JDK (requires Java 11 or later)
  * Apache Tomcat web server (needs to be version 8 or 9 for now due to package name changes)
  * Apache Maven build tool

## Configuration

### Messagepack library

The location of the messagepack C library is specified using the cmake variable
`msgpack-c_DIR`. This should be set to the location of the msgpack-c*.cmake
files. E.g.
```
cmake -Dmsgpack-c_DIR=<msgpack_install_prefix>/lib64/cmake/msgpack-c/
```
Note that version 3.1.0 of the msgpack C library contains a bug which causes
several unit test failures.

### HDF5 library

HDF5 is located by searching for the `h5cc` wrapper compiler, which should be
in your $PATH.

### Java JDK version

Java is located using the $JAVA_HOME environment variable. The java interface
to the C API and the web application are only built if a java JDK installation
is found.

Note that if JAVA_HOME is set then it must be set during the cmake configuration
AND build steps.

## Example compilation

```
mkdir -p hdfstream/build
cd hdfstream/build
cmake .. \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_C_FLAGS="-Wall -Werror" \
      -DCMAKE_INSTALL_PREFIX=<hdfstream_install_prefix> \
      -Dmsgpack-c_DIR=<msgpack_install_prefix>/lib64/cmake/msgpack-c
make
make test
make install
```
Here the install step installs the C library and its Java interface.

The test target runs a set of unit tests on the C library. There are
python scripts to test the web application in `python/tests` but these
are not run by `make test` because they require a running tomcat
instance and access to the EAGLE simulation data.

The virtual directory library and the web application also contain
unit tests but these are run automatically by maven as part of the
build process. If any of these tests fail the build will not complete.

## HDF5 streaming API

The libhdfstream library provides a C API which can be called simultaneously
on multiple threads to read and serialize HDF5 groups and datasets. This is
implemented by maintaining a pool of processes to deal with requests for data.
The reader processes serialize HDF5 objects into a fixed size shared memory
buffer where the data can be read out by the calling process.

Each reader process maintains a cache of recently used HDF5 file and dataset
handles. Requests are routed to a process which has the requested file and
dataset in its cache wherever possible.

The main operations implemented by the library are:

  * Open a data stream for a serialized HDF5 group, possibly recursively serializing member groups and datasets
  * Open a data stream for a serialized HDF5 dataset slice
  * Read the next data chunk from a data stream
  * Close a datastream, possibly before all data has been read

## Project layout

  * src/util: various data structures and utilities
  * src/hdf5: HDF5 to msgpack serialization and file/dataset caching code
  * src/reader: reader process source code
  * src/lib: C API for streaming of HDF5 objects
  * src/tests: unit tests for the C API
  * jni: Java native interface for the C API
  * virtual_directory: Java code for translating between real and virtual paths
  * webapp: Tomcat web application which uses the virtual directory and HDF5 streaming code to handle requests for data
  * python/client: python module which provides a simple interface for requesting HDF5 objects
  * python/tests: various integration tests for the web application
  * python/config: python scripts to generate configuration files for EAGLE and FLAMINGO

## Tomcat web application notes

### Java native library interface

The files libhdfstream.so, libhdfstream_jni.so and HDFStream.jar must be
installed to a location where the server can load them on startup. Native
libraries cannot be bundled in the .war file.

The java library must be built with a java version which is not newer than
the version used to run tomcat.

### Tomcat downloaded from apache.org

Tomcat's library search path can be set by creating a file `setenv.sh` in
its bin directory. This is automatically sourced when tomcat starts, so if we
put
```
#!/bin/sh

# Specify path to libhdfstream.so and libhdfstream_jni.so
export CATALINA_OPTS=-Djava.library.path=<install_prefix>/lib

# Specify path to HDFStream.jar
export CLASSPATH=<install_prefix>/lib/HDFStream.jar
```
in the file then it will search the specified path for libraries. Here,
`<install_prefix>` is the prefix passed to the configure command.

### Tomcat installed from rpm package (tested on Rocky linux)

In this case setenv.sh is not used because tomcat is started by systemd. The
search path for the C library can be set by modifying JAVA_OPTS in
`/etc/tomcat/tomcat.conf`. Note that variables are not expanded in this file
and that only the last JAVA_OPTS= line will take effect. It might also be
possible to put the setting in a new file in `/etc/tomcat/conf.d`. Variables
are expanded in that case.

The full path to HDFStream.jar can be added to the shared.loader entry in
`/usr/share/tomcat/conf/catalina.properties`.

Note that tomcat must have read permission on the files and execute permission
for all directories in their paths. On Rocky linux the tomcat user does not
have execute permission for other user directories, which causes potentially
misleading "not found" errors when tomcat tries to load the libraries.

### Starting the application

Once Tomcat is running, the web application is deployed by copying the .war
file to the webapps directory. The web interface can be accessed at
`https://localhost:8443/hdfstream` (if SSL is configured) or
`http://localhost:8080/hdfstream`.

Various service parameters can be adjusted in webapp/web.xml. The application
will need to be rebuilt and redeployed for changes to take effect.

## Authentication

The web application is set up to use http basic authentication with user
information stored in a database accessed via JDBC. Database connection
parameters are set in `webapp/context.xml`.

In order to avoid storing database credentials in the git repository the
`context.xml` file references variables which are assumed to be set in Tomcat's
`conf/catalina.properties` configuration file:

  * virgodb.user - username to connect to the database
  * virgodb.password - password used to connect to the database
  * virgodb.url - database connection URL
  * virgodb.driver - name of the JDBC driver. The corresponding .jar file needs to be placed in Tomcat's lib directory.

For testing purposes the application can be compiled with
-DTOMCAT_DUMMY_USERS=ON to use user accounts specified in the file
`dummy_users.xml` in Tomcat's conf directory. There's an example of this file
in `webapp/dummy_users.xml`.

## Virtual directories

### Directory structure

The java code in ./virtual_directory allows the construction of a virtual
directory structure defined by a configuration file. The code sets up
a tree structure in memory where each node is a virtual directory which
contains a mapping between virtual file names within the directory and full
paths to the corresponding files on disk.

This can be used to define the set of valid filenames which can be specified
by the user, so that an extensive set of files can be made available for
access without allowing users to browse the real file system.

The configuration file is a CSV file with 6 columns and one line for each
file to be served. The columns are:
  * The virtual path which will be presented to users
  * The path to the file on the real file system
  * The size of the file
  * The last modification time of the file (as a unix timestamp)
  * The file's media type (aka MIME type)
  * An optional security role (only applies to directories)

This directory also contains code to stream a tar file with the contents
of any directory in the hierarchy.

The name of the configuration file used by the web app is set in
`webapp/src/main/webapp/WEB-INF/web.xml`.

### Access permissions

Zero or more tomcat security roles can be associated with each virtual directory.
A user can only access a directory and any files and subdirectories in it if
they belong to all roles associated with the directory. Directories inherit all
roles associated with their parents, so subdirectories cannot have less
restricted access than their parents.

Files and directories which a user does not have permission to access will not
appear in the web interface, directory listings, or full directory downloads.
Attempting to access files or directories without permission will result in a
"not found" error.

## Web application structure

### Full file and directory downloads

Files and directories can be downloaded via the `download` endpoint:

`https://localhost:8443/hdfstream/download/<virtual path>`

If `virtual_path` is a file then the file is downloaded.

If `virtual_path` is a directory, then the response is a tar file containing
the directory contents including all subdirectories.

### Browsing files and directories

The virtual directory structure is shown in html form by the `viewer`:

`https://localhost:8443/hdfstream/viewer/<virtual path>`

For directories this shows a list of all files and subdirectories and a
download link for the directory. For HDF5 files it shows the groups and datasets
within the file and a link to download the file.

The viewer is now implemented as a javascript application which downloads
msgpack data and interprets it to generate documentation pages on the client.
This allows lazy loading of HDF5 metadata, which is important for files with
many groups and datasets.

### Messagepack encoded directory listings

Directory listings can be returned in a machine readable form using the `msgpack`
endpoint:

`https://localhost:8443/hdfstream/msgpack/<virtual path>?max_depth=...`

This returns msgpack map with keys "files" and "directories". The files entry
contains an array with the names of files in this directory. The directories
entry contains a map of msgpack encoded sub-directories. The optional
max_depth parameter limits the recursion depth to allow lazy loading of
subdirectories.

### HDF5 messagepack streaming

HDF5 groups and datasets are requested from the `msgpack` endpoint
using a http GET request to a URL of the form:

`https://localhost:8443/hdfstream/msgpack/<virtual_path>?object=...`

Here the virtual path must be the path of a HDF5 file and the `object`
parameter specifies the name of the HDF5 object within the file.

For groups, there are two optional parameters:
  * `max_depth` limits the recursion depth
  * `max_data_size` specifies the maximum size in bytes above which the contents of datasets will not be included

For datasets the parameter `slice` may be specified, which will cause
the service to return one or more slices of the dataset. The elements
to read in each dimension are specified using numpy style slicing
syntax. Multiple slices are separated using semicolons. E.g. to read
the coordinates of particles 0-9 and 20-29 inclusive from a snapshot
the request URL would be:

`https://localhost:8443/hdfstream/msgpack/<virtual_path>?object=/PartType1/Coordinates&slice=0:10,0:3;20:30,0:3`

If multiple slices are requested they can only differ in the first
dimension, they must not overlap and they must be in ascending order
of starting index in the first dimension. The result is a single array
containing the slices concatenated along the first dimension.

### POST requests for HDF5 data

In order to support requests for large numbers of slices in a more
efficient way, the server also supports http POST requests with
msgpack encoded parameters stored in the request body. In this case
the URL is of the form:

`https://localhost:8443/hdfstream/msgpack/<virtual_path>`

The body of the request should be a msgpack map where the keys are
parameter names as msgpack strings and the values are msgpack encoded
parameter values. The supported parameters are:

  * `object` : HDF5 object name (string)
  * `slice` : dataset slice specifier (see below)
  * `max_depth` : maximum recursion depth in HDF5 groups (integer)
  * `data_size_limit` : maximum dataset size to download with metadata in bytes (integer)

The slice specifier can be a string in the same format as used for GET
requests. Alternatively, it can be specified using nested arrays of
integers.

This type of slice specifier consists of a msgpack array with one
element for each dimension in the datase. For each dimension we have a
two element array containing a `[start, count]` pair. If `start` and
`count` are integers then the server will read elements
`start:start+count` in that dimension.

For example, to read the x, y, z coordinates of particles [10:15]
(inclusive) from a 2D coordinates dataset with dimensions `[N,3]`, the
slice specifier would be

```
[[10, 5], [0, 3]]
```

where the `[]` notation indicates a msgpack array. If `start` in the
first dimension is an array instead of a scalar, then its size
determines the number of slices to read. In this case the `count` in
the first dimension may be an array of the same size or it may be a
scalar, in which case the same count is used for every slice.

To read particles [10:15] and [20:30] in the example above the slice
specifier would be:


```
[[[10, 20], [5, 10]], [0, 3]
```

Note that these msgpack slice specifiers specify the length of a slice
and not it's ending index. This is for efficiency in the case where
all slices are the same length: the count can then be a scalar instead
of an array. We can then efficiently express requests for arbitrary,
non-contiguous indexes in the first dimension by setting `count=1`.

## Documentation

Documentation can be stored in the virtual directory structure along with the
simulation data. The server does not distinguish between documentation files
and data files.

The client-side web interface identifies documentation to be displayed
inline using a naming convention. If a virtual directory contains a
file `description.md` then the contents are rendered as markdown on
the directory listing page.  If a virtual directory contains a file
`labels.msgpack` then the file is interpreted as a msgpack map which
contains brief descriptions to be displayed alongside the file and
directory names.

These special files are not displayed in directory listings in the web
interface but will be included in tar file downloads and can be read using
the python module.
