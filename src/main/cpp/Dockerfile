# Using this docker file, you can build the JNI library that exposes Chimera.
# In fact, the generated docker image that is created from this docker file
# will contain the JNI static library in this path: /tmp/build-files/build/jni/libchimera_jni.so

FROM ubuntu:18.04

# Install required apt packages
RUN apt-get update && apt-get install -y --no-install-recommends \
    autoconf \
    make \
    ragel \
    libtool \
    apt-utils \
    python-dev \
    pkg-config \
    wget \
    openjdk-8-jdk \
    && rm -rf /var/lib/apt/list/*

# Install appropriate version of G++/GCC based on libstd++.so file library
# More information: https://gcc.gnu.org/onlinedocs/libstdc++/manual/abi.html
RUN apt-get install -y --no-install-recommends g++-4.8 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-4.8 1 \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.8 1 \
    && update-alternatives --set g++ /usr/bin/g++-4.8 \
    && update-alternatives --set gcc /usr/bin/gcc-4.8 \
    && rm -rf /var/lib/apt/list/*

# Create the temp build directory
ENV JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64/jre \
    BUILD_PATH=/tmp/build-files
RUN mkdir -p ${BUILD_PATH}

# Install Boost
ENV BOOST_FILE_NAME boost_1_63_0
ENV BOOST_DOWNLOAD_URL https://netix.dl.sourceforge.net/project/boost/boost/1.63.0/${BOOST_FILE_NAME}.tar.gz
RUN wget -qO- ${BOOST_DOWNLOAD_URL} | tar -xz -C ${BUILD_PATH}
RUN cd ${BUILD_PATH}/${BOOST_FILE_NAME} \
    && ./bootstrap.sh \
    && ./b2 install

# Install CMake
ENV CMAKE_FILE_NAME cmake-3.11.4
ENV CMAKE_DOWNLOAD_URL https://cmake.org/files/v3.11/${CMAKE_FILE_NAME}.tar.gz
RUN wget -qO- ${CMAKE_DOWNLOAD_URL} | tar -xz -C ${BUILD_PATH}
RUN cd ${BUILD_PATH}/${CMAKE_FILE_NAME} \
    && ./bootstrap \
    && make -j4 \
    && make install

# Install PCRE
ENV PCRE_VERSION 8.44
ENV PCRE_DOWNLOAD_URL https://freefr.dl.sourceforge.net/project/pcre/pcre/${PCRE_VERSION}/pcre-${PCRE_VERSION}.tar.gz
# We should use these variables to override the choices made by `configure'
# PIC (Position Independent Code) flag is necessary for shared libraries.
ENV CFLAGS "-fPIC"
RUN wget -qO- ${PCRE_DOWNLOAD_URL} | tar -xz -C ${BUILD_PATH}
RUN cd ${BUILD_PATH}/pcre-${PCRE_VERSION} \
    # It is not possible to use PCRE JIT with Chimera, which can cause memory leaks.
    # Do not use config '--enable-jit' to activate it until the problem is resolved.
    # issue: https://github.com/intel/hyperscan/issues/143
    && ./configure \
        --disable-shared \
        --enable-utf8 \
        --enable-unicode-properties \
    && make -j4 \
    && make install

# Install Hyperscan and Chimera
ENV HYPERSCAN_VERSION 5.4.0
ENV HYPERSCAN_DOWNLOAD_URL https://github.com/intel/hyperscan/archive/refs/tags/v${HYPERSCAN_VERSION}.tar.gz
RUN wget -qO- ${HYPERSCAN_DOWNLOAD_URL} | tar -xz -C ${BUILD_PATH}
# PCRE source directory should be under Hyperscan root directory in order to build Chimera. (symlink doesn't work!)
RUN cp -r ${BUILD_PATH}/pcre-${PCRE_VERSION} ${BUILD_PATH}/hyperscan-${HYPERSCAN_VERSION}/pcre \
    && cd ${BUILD_PATH}/hyperscan-${HYPERSCAN_VERSION} \
    && cmake \
        -DFAT_RUNTIME=true \
        -DCMAKE_C_FLAGS="-fPIC" \
        -DCMAKE_CXX_FLAGS="-fPIC" \
    && make -j4 \
    && make install

# Run Hyperscan and Chimera unit tests
RUN cd ${BUILD_PATH}/hyperscan-${HYPERSCAN_VERSION} \
    && bin/unit-hyperscan \
    && bin/unit-chimera

# Build JNI library
COPY ./ ${BUILD_PATH}/cpp
RUN cd ${BUILD_PATH} && mkdir -p build
RUN cd ${BUILD_PATH}/build \
    && cmake -DCMAKE_BUILD_TYPE=Debug ../cpp \
    && make -j4