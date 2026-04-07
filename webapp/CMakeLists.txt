set(WEBAPP_SOURCES
  ./pom.xml
  ./src/main/example/context.xml
  ./src/main/example/web.xml
  ./src/main/java/uk/ac/dur/cosma/hdfstream/ConcurrentRequestCount.java
  ./src/main/java/uk/ac/dur/cosma/hdfstream/DimensionInfo.java
  ./src/main/java/uk/ac/dur/cosma/hdfstream/DimensionArray.java
  ./src/main/java/uk/ac/dur/cosma/hdfstream/HDFStreamContextListener.java
  ./src/main/java/uk/ac/dur/cosma/hdfstream/HDFStreamRequest.java
  ./src/main/java/uk/ac/dur/cosma/hdfstream/HDFStreamRequestException.java
  ./src/main/java/uk/ac/dur/cosma/hdfstream/HDFStreamServlet.java
  ./src/main/java/uk/ac/dur/cosma/hdfstream/InvalidSliceException.java
  ./src/main/java/uk/ac/dur/cosma/hdfstream/MsgpackUtils.java
  ./src/main/java/uk/ac/dur/cosma/hdfstream/ServletUtils.java
  ./src/main/java/uk/ac/dur/cosma/hdfstream/SliceInfo.java
  ./src/main/java/uk/ac/dur/cosma/hdfstream/StatusServlet.java
  ./src/main/java/uk/ac/dur/cosma/hdfstream/StringSanitizer.java
  ./src/main/java/uk/ac/dur/cosma/hdfstream/TarFileServlet.java
  ./src/main/java/uk/ac/dur/cosma/hdfstream/ConfigManager.java
  ./src/main/java/uk/ac/dur/cosma/hdfstream/InternalConfigManager.java
  ./src/main/java/uk/ac/dur/cosma/hdfstream/ExternalConfigManager.java
  ./src/main/java/uk/ac/dur/cosma/hdfstream/CacheInfo.java
  ./src/main/java/uk/ac/dur/cosma/hdfstream/StreamCopier.java
  ./src/main/webapp/RefL0012N0188_stars.png
  ./src/main/webapp/WEB-INF/error.jsp
  ./src/main/webapp/WEB-INF/status.jsp
  ./src/main/webapp/WEB-INF/viewer.jsp
  ./src/main/webapp/common.css
  ./src/main/webapp/static/api_description.html
  ./src/main/webapp/static/msgpack_encoding.html
  ./src/main/webapp/static/overview.html
  ./src/main/webapp/static/performance.html
  ./src/main/webapp/static/python_examples.html
  ./src/main/webapp/static/python_module.html
  ./src/main/webapp/static/web_interface.html
  ./src/main/webapp/viewer.js
  ./src/test/java/BasicUnitTest.java
  ./src/test/java/DirectoryEntry.java
  ./src/test/java/HDF5Object.java
  ./src/test/java/NDArray.java
  ./src/test/java/PermissionsUnitTest.java
  ./src/test/java/TestDataset.java
  ./src/test/java/TestReload.java
  ./src/test/java/TestDimensionInfo.java
  ./src/test/java/TestDirectoryEntry.java
  ./src/test/java/TestFileDownload.java
  ./src/test/java/TestGroup.java
  ./src/test/java/TestHDFStreamJNI.java
  ./src/test/java/TestMsgpackSliceInfo.java
  ./src/test/java/TestMultiSlice.java
  ./src/test/java/TestMultiSlice2D.java
  ./src/test/java/TestRestrictedFileDownload.java
  ./src/test/java/TestSingleSlice.java
  ./src/test/java/TestSoftLink.java
  ./src/test/java/TestStringSliceInfo.java
  ./src/test/java/TestTarFileDownload.java
  ./src/test/java/TestDirectoryMetadata.java
  ./src/test/java/UnitTestClient.java
  ./src/test/java/UnitTestContextListener.java
  ./src/test/java/UnitTestRealm.java
  ./src/test/java/UnitTestServer.java
)

# Build the web application with maven
add_custom_command(
  DEPENDS ${WEBAPP_SOURCES}
  COMMAND ${MVN_EXECUTABLE} -f ${CMAKE_CURRENT_SOURCE_DIR} ${MVN_REPO_DIR} -Dcmake.build.dir=${CMAKE_CURRENT_BINARY_DIR}/target -Pconfig -Dhdfstream.config.dir="${HDFSTREAM_CONFIG}" clean package
  OUTPUT target/hdfstream.war
)
add_custom_target(webapp ALL DEPENDS target/hdfstream.war)
add_dependencies(webapp jni_install virtual_directory_install)

# Make sure we run "mvn test" as part of CMake's "make test" target
add_test(NAME web_app_tests COMMAND ${MVN_EXECUTABLE} -f ${CMAKE_CURRENT_SOURCE_DIR} ${MVN_REPO_DIR} -Dcmake.build.dir=${CMAKE_CURRENT_BINARY_DIR}/target -Pconfig -Dhdfstream.config.dir="${HDFSTREAM_CONFIG}" test)

# Make a script to run a specified unit test separately
configure_file(run_test.sh.in run_test.sh @ONLY)

# Make a script to run a specified unit test in the java debugger
configure_file(debug_test.sh.in debug_test.sh @ONLY)

# Make script to set library paths
configure_file(setenv.sh.in setenv.sh @ONLY)

# Copy example config to the build dir
add_subdirectory(src/main/example)
