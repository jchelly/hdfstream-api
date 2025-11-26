# Real filesystem location where the data files will be stored
set(PYNBODY_DATA_DIR "${CMAKE_CURRENT_BINARY_DIR}/" CACHE FILEPATH "Where to put the pynbody example data")

# Location to write the CSV config file
set(PYNBODY_CONFIG_CSV "${CMAKE_BINARY_DIR}/webapp/src/main/example/pynbody.csv")

if(NOT EXISTS "${PYNBODY_CONFIG_CSV}")

  #
  # Download the pynbody swift test data
  #
  set(PYNBODY_DATA_URL "https://zenodo.org/records/17084976/files/swift.tar.gz?download=1")
  set(PYNBODY_DATA_TARBALL "${PYNBODY_DATA_DIR}/swift.tar.gz")
  file(MAKE_DIRECTORY "${PYNBODY_DATA_DIR}")
  file(DOWNLOAD "${PYNBODY_DATA_URL}" "${PYNBODY_DATA_TARBALL}" SHOW_PROGRESS)
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar xzf "${PYNBODY_DATA_TARBALL}"
    WORKING_DIRECTORY "${PYNBODY_DATA_DIR}"
  )

  #
  # Generate the CSV configuration file describing the unpacked files
  #
  file(WRITE "${PYNBODY_CONFIG_CSV}" "")
  file(GLOB_RECURSE ALL_FILES RELATIVE "${PYNBODY_DATA_DIR}/" "${PYNBODY_DATA_DIR}/*.hdf5")
  foreach(data_file ${ALL_FILES})
    file(SIZE "${PYNBODY_DATA_DIR}/${data_file}" file_size)
    file(TIMESTAMP "${PYNBODY_DATA_DIR}/${data_file}" file_timestamp "%s")
    file(APPEND "${PYNBODY_CONFIG_CSV}" "Tests/pynbody/testdata/${data_file}, ${PYNBODY_DATA_DIR}/${data_file}, ${file_size}, ${file_timestamp}, application/x-hdf5\n")
  endforeach()

endif()
