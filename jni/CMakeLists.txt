include(UseJava)
include_directories(${CMAKE_BINARY_DIR}/src/lib ../src/util/ ../src/hdf5/ ${HDF5_INCLUDE_DIRS})

# Java class which loads the JNI library. Also generates C headers.
add_jar(HDFStream
  SOURCES
    uk/ac/dur/cosma/libhdfstream/DataStream.java
    uk/ac/dur/cosma/libhdfstream/HDFStream.java
    uk/ac/dur/cosma/libhdfstream/HDFStreamCacheInfo.java
  GENERATE_NATIVE_HEADERS hdfstream-headers
  OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
)
install_jar(HDFStream DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Build the native library which will be loaded from java code
add_library(hdfstream_jni SHARED HDFStream.c DataStream.c)
target_link_libraries(hdfstream_jni JNI::JNI hdfstream-headers hdfstream)
install(TARGETS hdfstream_jni DESTINATION ${CMAKE_INSTALL_LIBDIR})
set_target_properties(hdfstream_jni PROPERTIES VERSION ${CMAKE_PROJECT_VERSION} SOVERSION 1)

# The jni_install target will install the library to our local maven repo so we can build the web app
if(MVN_EXECUTABLE)
  add_custom_command(
    DEPENDS HDFStream.jar
    COMMAND ${MVN_EXECUTABLE} install:install-file ${MVN_REPO_DIR} -Dfile=${CMAKE_CURRENT_BINARY_DIR}/HDFStream.jar -DgroupId=uk.ac.dur.cosma -DartifactId=libhdfstream -Dversion=1.0-SNAPSHOT -Dpackaging=jar
    COMMAND ${CMAKE_COMMAND} -E touch install.stamp
    OUTPUT install.stamp
  )
  add_custom_target(jni_install DEPENDS install.stamp)
endif()
