# ---- Stage 1: Build dependencies ----
FROM tomcat:9 AS builder

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
    && cmake . -DCMAKE_INSTALL_PREFIX=/usr/local/ \
    && make -j$(nproc) \
    && make install \
    && rm -rf /tmp/msgpack-c

# Build HDF5 (needs to be >=1.12, so apt package is too old)
RUN git clone https://github.com/HDFGroup/hdf5.git /tmp/hdf5 \
    && cd /tmp/hdf5 \
    && git checkout hdf5_1.14.6 \
    && mkdir build \
    && cd build \
    && cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local/ \
    && make -j$(nproc) \
    && make install \
    && rm -rf /tmp/hdf5

# Copy over and build the hdfstream-api source code
COPY . /hdfstream-api
RUN cd /hdfstream-api \
    && mkdir build \
    && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/usr/local/ -DCMAKE_INSTALL_PREFIX=/usr/local \
    && make \
    && make test \
    && make install

# ---- Stage 2: Runtime ----
FROM tomcat:9

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
