include_directories(${HDF5_INCLUDE_DIRS} ${CMAKE_BINARY_DIR}/src/lib ../src/lib ../src/hdf5 ../src/util)
link_directories(${HDF5_LIBRARY_DIRS})

# Directory for tmp files
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tmp)

# Bundle up files needed for tests
add_library(testlib
  ../src/util/avltree.c
  ../src/util/linked_list.c
  ../src/util/ordered_map.c
  ../src/util/process_pool.c
  ../src/util/verify.c
  ../src/util/worker_process.c
  ../src/util/shared_memory.c
  ../src/hdf5/dataset_cache.c
  ../src/hdf5/pack_attributes.c
  ../src/hdf5/pack_group.c
  ../src/hdf5/type_mapping.c
  ../src/hdf5/file_cache.c
  ../src/hdf5/pack_object.c
  ../src/hdf5/pack_unknown.c
  ../src/hdf5/pack_dataset.c
  ../src/hdf5/pack_numpy_type.c
  ../src/hdf5/pack_contents.c
  ../src/hdf5/pack_soft_link.c
  ../src/hdf5/multislice.c
  ../src/hdf5/select.c
  ../src/hdf5/native_type.c
  pack_multiple_slices_1d.c
  pack_multiple_slices_2d.c
  decode_ndarray.c
  receive_ndarray.c
  receive_msgpack_object.c
  create_test_file.c
  decoder.c
  pack_and_decode.c)

target_link_libraries(testlib ${HDF5_C_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} msgpack-c rt)

# Unit tests of functions used to implement the reader processes
foreach(TEST_NAME
    file_cache
    file_cache_expiry
    dataset_cache
    worker
    worker_callback
    worker_callback_failure
    worker_failed_recv
    worker_failed_wait
    worker_fail
    type_mapping
    process_pool
    process_pool_dead_process
    process_pool_score
    ordered_map_add_remove
    ordered_map_iterate
    shared_memory
    pack_dataset
    pack_dataset_scalar
    pack_dataset_vlen_string
    pack_dataset_vlen_int
    pack_dataset_slice_scalar
    pack_dataset_slice_1d
    pack_dataset_slice_2d
    pack_dataset_buffer_too_small
    pack_dataset_multislice_1d
    pack_dataset_multislice_1d_random
    pack_dataset_multislice_2d
    pack_dataset_multislice_2d_random
    pack_compound_dataset
    pack_attributes
    pack_attribute_vlen_string
    pack_group
    pack_compound_type
    pack_array_type
    pack_soft_link
    pack_broken_link
    recursion
    max_links
    unknown_type
  )
  add_executable(test_${TEST_NAME} test_${TEST_NAME}.c)
  target_link_libraries(test_${TEST_NAME} testlib)
  add_test(NAME test_${TEST_NAME} COMMAND test_${TEST_NAME})
endforeach()

# Tests of the libhdfstream C API
foreach(TEST_NAME
    start_stop
    start_stop_repeat
    wrong_file
    wrong_dataset
    out_of_bounds
    wrong_rank
    dataset_1d_int
    dataset_1d_int_repeat
    dataset_2d_int
    dataset_1d_float
    dataset_1d_str dataset_2d_float
    dataset_1d_int_partial
    dataset_2d_int_partial
    dataset_1d_int_zero_size
    dataset_1d_int_partial_zero_size_slice
    dataset_scalar_int
    dataset_1d_int_threads
    dataset_1d_int_multifile_threads
    dataset_1d_int_corrupted
    stream_dataset_multislice_1d
    stream_dataset_multislice_1d_random
    stream_dataset_multislice_2d
    stream_dataset_multislice_2d_random
    multiple_instances
    multifile_threads_failures
    abort_read
    abort_immediately
    single_block)
  add_executable(test_${TEST_NAME} test_${TEST_NAME}.c)
  target_link_libraries(test_${TEST_NAME} hdfstream testlib)
  add_test(NAME test_${TEST_NAME} COMMAND test_${TEST_NAME} ./tmp/${TEST_NAME}.hdf5)
endforeach()

# Utility to serialize a full HDF5 file to msgpack by calling pack functions directly
add_executable(serialize_hdf5_to_msgpack serialize_hdf5_to_msgpack.c)
target_link_libraries(serialize_hdf5_to_msgpack testlib)

# Utility to serialize a full HDF5 file to msgpack via the thread safe libhdfstream interface
add_executable(serialize_hdf5_via_lib serialize_hdf5_via_lib.c)
target_link_libraries(serialize_hdf5_via_lib hdfstream)

# Test download rate for a single dataset
add_executable(test_download_rate test_download_rate.c receive_ndarray.c receive_msgpack_object.c decode_ndarray.c)
target_link_libraries(test_download_rate hdfstream msgpack-c)

# Test download rate for a single dataset without unpacking
add_executable(test_read_rate test_read_rate.c)
target_link_libraries(test_read_rate hdfstream)
