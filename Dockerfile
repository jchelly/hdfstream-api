#
# Docker commands if not using github actions
#
# Build:   docker build --tag "hdfstream-api" .
# Run:     docker run -d -p 8080:8080 --name hdfstream-api hdfstream-api
# Log in:  docker exec -it hdfstream-api /bin/bash
# Stop:    docker stop hdfstream-api
# Remove:  docker remove hdfstream-api
#
# Build dependencies:
#
# Here we use an Ubuntu Noble image with the java JDK installed.
#
FROM eclipse-temurin:25-jdk-noble AS builder

# Install various build tools
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    curl \
    pkg-config \
    zlib1g \
    zlib1g-dev \
    maven \
    && rm -rf /var/lib/apt/lists/*

# Build msgpack-c
RUN git clone https://github.com/msgpack/msgpack-c.git /tmp/msgpack-c \
    && cd /tmp/msgpack-c \
    && git checkout c-6.1.0 \
    && cmake . -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local/ \
    && make -j$(nproc) \
    && make install \
    && rm -rf /tmp/msgpack-c

# Build HDF5 (needs to be >=1.12, so apt package is too old)
RUN git clone https://github.com/HDFGroup/hdf5.git /tmp/hdf5 \
    && cd /tmp/hdf5 \
    && git checkout hdf5_1.14.6 \
    && mkdir build \
    && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local/ \
    && make -j$(nproc) \
    && make install \
    && rm -rf /tmp/hdf5

# Copy over and build the hdfstream-api source code
COPY . /hdfstream-api
RUN cd /hdfstream-api \
    && mkdir build \
    && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/usr/local/ -DCMAKE_INSTALL_PREFIX=/usr/local -DPYNBODY_DATA_DIR=/usr/local/data \
    && make \
    && make test \
    && make install

#
# Set up the final image
#
# This starts from the same Ubuntu Noble image but with tomcat installed.
# Need to copy over the libraries and data set up in the builder, and
# configure tomcat to find the libraries.
#
FROM tomcat:9-jdk25-temurin-noble

# Remove default webapps
RUN rm -rf /usr/local/tomcat/webapps/*

# Copy the built libraries from the builder
COPY --from=builder /usr/local /usr/local

# Copy the web app package over
COPY --from=builder /hdfstream-api/build/webapp/target/hdfstream.war /usr/local/tomcat/webapps/

# Copy file with path to libhdfstream for tomcat
COPY setenv.sh /usr/local/tomcat/bin/

# Expose Tomcat’s default port
EXPOSE 8080

# Start Tomcat
CMD ["catalina.sh", "run"]
