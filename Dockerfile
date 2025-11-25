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

# Build HDF5 >= 1.12
RUN curl -L https://github.com/HDFGroup/hdf5/releases/download/hdf5-1.12.3/hdf5-1.12.3.tar.gz \
    | tar xz -C /tmp \
    && cd /tmp/hdf5-1.12.3 \
    && ./configure --prefix=/usr/local \
    && make -j$(nproc) \
    && make install \
    && rm -rf /tmp/hdf5-1.12.3


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
