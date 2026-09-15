include_directories(${HDF5_INCLUDE_DIRS})
link_directories(${HDF5_LIBRARY_DIRS})
include_directories(../hdf5 ../util ../lib ${CMAKE_CURRENT_BINARY_DIR}/../lib)

# Reader executable
add_executable(hdfstream_reader hdfstream_reader.c ../hdf5/file_cache.c ../hdf5/dataset_cache.c ../hdf5/pack_object.c ../hdf5/pack_unknown.c ../hdf5/pack_group.c ../hdf5/pack_dataset.c ../hdf5/pack_attributes.c ../hdf5/pack_contents.c ../hdf5/pack_soft_link.c ../hdf5/multislice.c ../hdf5/select.c ../hdf5/type_mapping.c ../util/linked_list.c ../util/avltree.c ../util/ordered_map.c ../util/worker_process.c ../util/verify.c ../hdf5/pack_numpy_type.c ../hdf5/msgpack_chunkbuffer.c ../hdf5/native_type.c ../util/shared_memory.c)
target_link_libraries(hdfstream_reader ${HDF5_C_LIBRARIES} msgpack-c rt ${CMAKE_THREAD_LIBS_INIT})
install(TARGETS hdfstream_reader DESTINATION ${CMAKE_INSTALL_BINDIR})
