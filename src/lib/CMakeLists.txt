include_directories(${HDF5_INCLUDE_DIRS})
include_directories(../util ../hdf5 .)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Generate header with executable install location and max buffer size
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/hdfstream.h.in ${CMAKE_CURRENT_BINARY_DIR}/hdfstream.h)

# Generate header with number of buffers in the shared ring buffer
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/shared_data.h.in ${CMAKE_CURRENT_BINARY_DIR}/shared_data.h)

# HDF5 streaming reader library
add_library(hdfstream SHARED hdfstream.c shared_data.c reader_data.c ../util/process_pool.c ../util/worker_process.c ../util/verify.c ../hdf5/file_cache.c ../hdf5/dataset_cache.c ../util/ordered_map.c ../util/linked_list.c ../util/avltree.c ../util/shared_memory.c)
set_target_properties(hdfstream PROPERTIES VERSION ${CMAKE_PROJECT_VERSION} SOVERSION 1)

# The library maintains a shadow copy of the reader process caches to help select the best process
# to carry out each request. This is implemented by using the same code with the HDF5 calls disabled.
target_compile_definitions(hdfstream PRIVATE SHADOW_CACHE)

target_link_libraries(hdfstream ${CMAKE_THREAD_LIBS_INIT} rt)
install(TARGETS hdfstream DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/hdfstream.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
