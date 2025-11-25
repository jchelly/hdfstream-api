file(SIZE ${CMAKE_CURRENT_SOURCE_DIR}/restricted/data.txt  RESTRICTED_DATA_TXT_SIZE)
file(SIZE ${CMAKE_CURRENT_SOURCE_DIR}/restricted/data.hdf5 RESTRICTED_DATA_HDF5_SIZE)
file(SIZE ${CMAKE_CURRENT_SOURCE_DIR}/unrestricted/data.txt  UNRESTRICTED_DATA_TXT_SIZE)
file(SIZE ${CMAKE_CURRENT_SOURCE_DIR}/unrestricted/data.hdf5 UNRESTRICTED_DATA_HDF5_SIZE)

configure_file(config.csv.in config.csv @ONLY)
