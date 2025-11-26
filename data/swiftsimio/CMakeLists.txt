# Real filesystem location where the data files will be stored
set(SWIFTSIMIO_DATA_DIR "${CMAKE_CURRENT_BINARY_DIR}/" CACHE FILEPATH "Where to put the swiftsimio example data")

# Location to write the CSV config file
set(SWIFTSIMIO_CONFIG_CSV "${CMAKE_BINARY_DIR}/webapp/src/main/example/swiftsimio.csv")

# Names of the files to download and configure
set(SWIFT_FILES LegacyCosmologicalVolume.hdf5 LegacyCosmologicalVolumeDithered.hdf5 eagle_0025.0.hdf5 eagle_0025.1.hdf5 EagleSingle.hdf5 EagleDistributed.hdf5 SoapExample.hdf5)

if(NOT EXISTS "${SWIFTSIMIO_CONFIG_CSV}")

  #
  # Download the swiftsimio swift test data
  #
  set(SWIFTSIMIO_DATA_URL "https://virgodb.cosma.dur.ac.uk/swift-webstorage/IOExamples/ssio_ci_04_2025/")
  file(MAKE_DIRECTORY "${SWIFTSIMIO_DATA_DIR}")
  foreach(FILENAME ${SWIFT_FILES})
    file(DOWNLOAD "${SWIFTSIMIO_DATA_URL}${FILENAME}" "${SWIFTSIMIO_DATA_DIR}/${FILENAME}")
  endforeach()

  #
  # Generate the CSV configuration file describing the unpacked files
  #
  file(WRITE "${SWIFTSIMIO_CONFIG_CSV}" "")
  file(GLOB_RECURSE ALL_FILES RELATIVE "${SWIFTSIMIO_DATA_DIR}/" "${SWIFTSIMIO_DATA_DIR}/*.hdf5")
  foreach(data_file ${ALL_FILES})
    file(SIZE "${SWIFTSIMIO_DATA_DIR}/${data_file}" file_size)
    file(TIMESTAMP "${SWIFTSIMIO_DATA_DIR}/${data_file}" file_timestamp "%s")
    file(APPEND "${SWIFTSIMIO_CONFIG_CSV}" "Tests/SWIFT/IOExamples/ssio_ci_04_2025/${data_file}, ${SWIFTSIMIO_DATA_DIR}/${data_file}, ${file_size}, ${file_timestamp}, application/x-hdf5\n")
  endforeach()

endif()
