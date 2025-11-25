#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <jni.h>

#include "uk_ac_dur_cosma_libhdfstream_HDFStream.h" /* JNI generated header */
#include "hdfstream.h" /* C library header */

JNIEXPORT jlong JNICALL Java_uk_ac_dur_cosma_libhdfstream_HDFStream_c_1new(JNIEnv *env, jobject thisObj, jint nr_processes,
                                                                           jint max_open_files, jint max_open_datasets,
                                                                           jint file_cache_check_interval,
                                                                           jint file_cache_expiry_interval) {
  (void) env;
  (void) thisObj;
  assert(sizeof(intptr_t) <= sizeof(jlong));
  struct hdfstream *hs = hdfstream_new((int) nr_processes, (int) max_open_files, (int) max_open_datasets,
                                       (jint) file_cache_check_interval, (jint) file_cache_expiry_interval);
  return (jlong) ((intptr_t) hs);
}


JNIEXPORT jlong JNICALL Java_uk_ac_dur_cosma_libhdfstream_HDFStream_c_1new_1with_1executable(JNIEnv *env, jobject thisObj,
                                                                                             jint nr_processes,
                                                                                             jstring executable,
                                                                                             jint max_open_files, jint max_open_datasets,
                                                                                             jint file_cache_check_interval,
                                                                                             jint file_cache_expiry_interval) {
  (void) thisObj;
  const char *c_path = (*env)->GetStringUTFChars(env, executable, NULL);
  struct hdfstream *hs = hdfstream_new_with_executable((int) nr_processes, c_path, (int) max_open_files, (int) max_open_datasets,
                                                       (jint) file_cache_check_interval, (jint) file_cache_expiry_interval);
  (*env)->ReleaseStringUTFChars(env, executable, c_path);

  return (jlong) ((intptr_t) hs);
}


JNIEXPORT jint JNICALL Java_uk_ac_dur_cosma_libhdfstream_HDFStream_c_1get_1max_1slices(JNIEnv *env, jobject thisObj) {
  (void) env;
  (void) thisObj;
  return HDFSTREAM_MAX_SLICES;
}


JNIEXPORT jint JNICALL Java_uk_ac_dur_cosma_libhdfstream_HDFStream_c_1get_1max_1dims(JNIEnv *env, jobject thisObj) {
  (void) env;
  (void) thisObj;
  return HDFSTREAM_MAX_DIMS;
}


JNIEXPORT void JNICALL Java_uk_ac_dur_cosma_libhdfstream_HDFStream_c_1free(JNIEnv *env, jobject thisObj, jlong ptr) {
  (void) env;
  (void) thisObj;
  struct hdfstream *hs = (struct hdfstream *) ((intptr_t) ptr);
  hdfstream_free(hs);
}


JNIEXPORT jlong JNICALL Java_uk_ac_dur_cosma_libhdfstream_HDFStream_c_1open_1slices(JNIEnv *env, jobject thisObj, long ptr,
                                                                                    jstring file_name, jstring dataset_name,
                                                                                    jint nr_slices, jint rank, jlongArray start,
                                                                                    jlongArray count, jlong buffer_size) {
  (void) thisObj;
  jlong result = 0;
  hsize_t *c_start = NULL;
  hsize_t *c_count = NULL;

  /* Extract file and dataset names */
  const char *c_file_name = (*env)->GetStringUTFChars(env, file_name, NULL);
  const char *c_dataset_name = (*env)->GetStringUTFChars(env, dataset_name, NULL);

  /* Get pointer to the hdfstream */
  struct hdfstream *hs = (struct hdfstream *) ((intptr_t) ptr);

  /* Number of slices */
  if((nr_slices > HDFSTREAM_MAX_SLICES) || (nr_slices < 1))goto cleanup;
  int c_nr_slices = (int) nr_slices;
  if(c_nr_slices != nr_slices)goto cleanup; /* Just in case of overflow? */

  /* Number of dimensions */
  if((rank > HDFSTREAM_MAX_DIMS) || (rank < 0))goto cleanup;
  int c_rank = (int) rank;
  if(c_rank != rank)goto cleanup;

  /* Expected size of start and count arrays */
  jsize arr_len = nr_slices * rank;

  /* Offset in each dimension */
  jsize start_len = (*env)->GetArrayLength(env, start);
  if(start_len != arr_len)goto cleanup;
  jlong *start_data = (*env)->GetLongArrayElements(env, start, 0);
  c_start = malloc(sizeof(hsize_t)*arr_len);
  for(jsize i=0; i<arr_len; i+=1)
    c_start[i] = start_data[i];
  (*env)->ReleaseLongArrayElements(env, start, start_data, 0);

  /* Number of elements in each dimension */
  jsize count_len = (*env)->GetArrayLength(env, count);
  if(count_len != arr_len)goto cleanup;
  jlong *count_data = (*env)->GetLongArrayElements(env, count, 0);
  c_count = malloc(sizeof(hsize_t)*arr_len);
  for(jsize i=0; i<arr_len; i+=1)
    c_count[i] = count_data[i];
  (*env)->ReleaseLongArrayElements(env, count, count_data, 0);

  /* Buffer size */
  size_t c_buffer_size = (size_t) buffer_size;

  /* Call the C code */
  struct data_stream *stream = hdfstream_dataset_multi_slice_open(hs, c_file_name, c_dataset_name,
                                                                  c_nr_slices, c_rank, c_start, c_count,
                                                                  c_buffer_size);
  if(!stream)goto cleanup;

  /* Success. Return pointer to the stream as a long. */
  result = (jlong) ((intptr_t) stream);

 cleanup:
  (*env)->ReleaseStringUTFChars(env, file_name, c_file_name);
  (*env)->ReleaseStringUTFChars(env, dataset_name, c_dataset_name);
  if(c_start)free(c_start);
  if(c_count)free(c_count);
  return result;

}


JNIEXPORT jlong JNICALL Java_uk_ac_dur_cosma_libhdfstream_HDFStream_c_1open_1object(JNIEnv *env, jobject thisObj, jlong ptr,
                                                                                    jstring file_name, jstring object_name,
                                                                                    jint max_depth, jlong buffer_size, jlong data_size_limit) {
  (void) thisObj;
  jlong result = 0;

  /* Get pointer to the hdfstream */
  struct hdfstream *hs = (struct hdfstream *) ((intptr_t) ptr);

  /* Extract file and dataset names */
  const char *c_file_name = (*env)->GetStringUTFChars(env, file_name, NULL);
  const char *c_object_name = (*env)->GetStringUTFChars(env, object_name, NULL);

  /* Buffer size */
  size_t c_buffer_size = (size_t) buffer_size;
  size_t c_data_size_limit = (size_t) data_size_limit;

  /* Call the C code */
  struct data_stream *stream = hdfstream_object_open(hs, c_file_name, c_object_name,
						    max_depth, c_buffer_size, c_data_size_limit);
  if(!stream)goto cleanup;

  /* Success. Return pointer to the stream as a long. */
  result = (jlong) ((intptr_t) stream);

 cleanup:
  (*env)->ReleaseStringUTFChars(env, file_name, c_file_name);
  (*env)->ReleaseStringUTFChars(env, object_name, c_object_name);
  return result;
}


JNIEXPORT void JNICALL Java_uk_ac_dur_cosma_libhdfstream_HDFStream_c_1cache_1info(JNIEnv *env, jobject thisObj, jlong ptr,
                                                                                  jint worker_nr, jintArray fields) {
  (void) thisObj;

  /* Get pointer to the hdfstream */
  struct hdfstream *hs = (struct hdfstream *) ((intptr_t) ptr);

  /* Get cache stats */
  struct cache_info ci = hdfstream_cache_info(hs, (int) worker_nr);

  /* Copy to the output array */
  jint *fields_data = (*env)->GetIntArrayElements(env, fields, 0);
  fields_data[0] = ci.process_state;
  fields_data[1] = ci.nr_file_cache_hits;
  fields_data[2] = ci.nr_file_cache_misses;
  (*env)->ReleaseIntArrayElements(env, fields, fields_data, 0);
}
