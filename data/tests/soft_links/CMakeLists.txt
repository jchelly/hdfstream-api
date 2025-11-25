file(SIZE ${CMAKE_CURRENT_SOURCE_DIR}/test_data.hdf5 TEST_DATA_SIZE)
file(SIZE ${CMAKE_CURRENT_SOURCE_DIR}/broken_link.hdf5 BROKEN_LINK_SIZE)

configure_file(config.csv.in config.csv @ONLY)
