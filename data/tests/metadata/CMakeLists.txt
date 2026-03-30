file(SIZE ${CMAKE_CURRENT_SOURCE_DIR}/test_data.hdf5 TEST_DATA_SIZE)
file(SIZE ${CMAKE_CURRENT_SOURCE_DIR}/test_data_2d.hdf5 TEST_DATA_2D_SIZE)

configure_file(config.csv.in config.csv @ONLY)
