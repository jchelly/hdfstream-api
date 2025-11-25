#!/bin/sh
#
# Path settings used by tomcat in the docker container.
#
# Specify path to libhdfstream.so and libhdfstream_jni.so
export CATALINA_OPTS=-Djava.library.path=/usr/local/lib/

# Specify path to HDFStream.jar
export CLASSPATH=/usr/local/lib/HDFStream.jar
