# ---- Stage 1: Build dependencies ----
FROM tomcat:9 AS builder

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    curl \
    pkg-config \
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
    && cmake . -DCMAKE_INSTALL_PREFIX=/usr/local/ \
    && make -j$(nproc) \
    && make install \
    && rm -rf /tmp/hdf5


# ---- Stage 2: Runtime ----
FROM tomcat:9

# Copy only the built libraries from builder
COPY --from=builder /usr/local /usr/local

# Maintainer info (optional)
LABEL maintainer="j.c.helly@durham.ac.uk"

# Remove default webapps
RUN rm -rf /usr/local/tomcat/webapps/*

# Copy your WAR file into Tomcat’s webapps directory
#COPY myapp.war /usr/local/tomcat/webapps/

# Expose Tomcat’s default port
EXPOSE 8080

# Start Tomcat
CMD ["catalina.sh", "run"]
