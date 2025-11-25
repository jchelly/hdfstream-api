#include <jni.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#include "uk_ac_dur_cosma_libhdfstream_DataStream.h" /* JNI generated header */
#include "hdfstream.h" /* C library header */


JNIEXPORT jlong JNICALL Java_uk_ac_dur_cosma_libhdfstream_DataStream_c_1read_1chunk(JNIEnv *env, jobject thisObj,
                                                                                    jlong ptr, jbyteArray buffer) {
  (void) thisObj;
  struct data_stream *stream = (struct data_stream *) ((intptr_t) ptr);

  jboolean isCopy;
  jbyte *jb = (*env)->GetByteArrayElements(env, buffer, &isCopy);
  int status;
  size_t nr_bytes_read = hdfstream_read_chunk(stream, (char *) jb, &status);
  (*env)->ReleaseByteArrayElements(env, buffer, jb, 0);
  if(nr_bytes_read == 0) {
    if(status==0)
      return (jlong) 0; /* Successful end of stream */
    else
      return (jlong) -1; /* Failed end of stream */
  } else {
    return (jlong) nr_bytes_read; /* More data may remain */
  }
}

JNIEXPORT void JNICALL Java_uk_ac_dur_cosma_libhdfstream_DataStream_c_1free(JNIEnv *env, jobject thisObj, jlong ptr) {
  (void) thisObj;
  (void) env;
  struct data_stream *stream = (struct data_stream *) ((intptr_t) ptr);
  hdfstream_close_stream(stream);
}
