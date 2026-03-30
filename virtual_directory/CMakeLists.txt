# List of all source files
set(VIRTUAL_DIRECTORY_SOURCES
  ./pom.xml
  ./src/main/java/uk/ac/dur/cosma/virtual_directory/CheckRole.java
  ./src/main/java/uk/ac/dur/cosma/virtual_directory/TarFile.java
  ./src/main/java/uk/ac/dur/cosma/virtual_directory/VirtualDirectory.java
  ./src/main/java/uk/ac/dur/cosma/virtual_directory/VirtualDirectoryException.java
  ./src/main/java/uk/ac/dur/cosma/virtual_directory/VirtualFile.java
  ./src/main/java/uk/ac/dur/cosma/virtual_directory/VirtualPathInfo.java
  ./src/main/java/uk/ac/dur/cosma/virtual_directory/DirectoryMetadata.java
  ./src/test/java/TestAddFromReader.java
  ./src/test/java/TestFileProperties.java
  ./src/test/java/TestGetAllFiles.java
  ./src/test/java/TestImplicitDirectory.java
  ./src/test/java/TestInit.java
  ./src/test/java/TestMultiRole.java
  ./src/test/java/TestMultiRoleDirList.java
  ./src/test/java/TestSingleRole.java
  ./src/test/java/TestTarFile.java
  ./src/test/java/TestTarFileLarge.java
  ./src/test/java/TestTarFileNoAccess.java
  ./src/test/java/TestTarFileRole.java
  ./src/test/java/TestTotalSize.java
  ./src/test/java/TestDirectoryMetadata.java
  ./src/test/java/TestGetMetadata.java
)

# Build and install the library
add_custom_command(
  DEPENDS ${VIRTUAL_DIRECTORY_SOURCES}
  COMMAND ${MVN_EXECUTABLE} -f ${CMAKE_CURRENT_SOURCE_DIR} ${MVN_REPO_DIR} -Dcmake.build.dir=${CMAKE_CURRENT_BINARY_DIR}/target clean package install
  COMMAND ${CMAKE_COMMAND} -E touch install.stamp
  OUTPUT install.stamp
)
add_custom_target(virtual_directory_install DEPENDS install.stamp)

# Make sure we run "mvn test" as part of CMake's "make test" target
add_test(NAME virtual_directory_tests COMMAND ${MVN_EXECUTABLE} -f ${CMAKE_CURRENT_SOURCE_DIR} ${MVN_REPO_DIR} -Dcmake.build.dir=${CMAKE_CURRENT_BINARY_DIR}/target test)
