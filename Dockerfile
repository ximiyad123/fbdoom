FROM debian:stretch

ENV DEPS=/opt/fbprinter-deps
ENV DEBIAN_FRONTEND=noninteractive

RUN printf '%s\n' \
        'deb http://archive.debian.org/debian stretch main' \
        'deb http://archive.debian.org/debian-security stretch/updates main' \
        > /etc/apt/sources.list && \
    printf '%s\n' \
        'Acquire::Check-Valid-Until "false";' \
        'Acquire::AllowInsecureRepositories "true";' \
        'Acquire::AllowDowngradeToInsecureRepositories "true";' \
        > /etc/apt/apt.conf.d/99archive

RUN dpkg --add-architecture arm64 && \
    apt-get update && \
    apt-get install -y \
        build-essential \
        gcc-aarch64-linux-gnu \
        g++-aarch64-linux-gnu \
        libc6-dev-arm64-cross \
        binutils-aarch64-linux-gnu \
        cmake \
        pkg-config \
        ca-certificates \
        curl \
        xz-utils \
        bzip2 \
        tar \
        file \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir -p \
        ${DEPS}/include \
        ${DEPS}/lib \
        ${DEPS}/lib/pkgconfig

ENV PKG_CONFIG_PATH=${DEPS}/lib/pkgconfig
ENV CPPFLAGS="-I${DEPS}/include"
ENV CFLAGS="-I${DEPS}/include"
ENV LDFLAGS="-L${DEPS}/lib"

# ------------------------------------------------------------
# zlib
# ------------------------------------------------------------

RUN curl -L \
        https://zlib.net/fossils/zlib-1.3.1.tar.gz \
        -o /tmp/zlib.tar.gz && \
    tar -xf /tmp/zlib.tar.gz -C /tmp && \
    cd /tmp/zlib-1.3.1 && \
    CC=aarch64-linux-gnu-gcc \
    AR=aarch64-linux-gnu-ar \
    RANLIB=aarch64-linux-gnu-ranlib \
    ./configure \
        --static \
        --prefix=${DEPS} && \
    make -j"$(nproc)" && \
    make install && \
    rm -rf /tmp/zlib*

# ------------------------------------------------------------
# libpng
# ------------------------------------------------------------

RUN curl -L \
        https://downloads.sourceforge.net/libpng/libpng-1.6.50.tar.xz \
        -o /tmp/libpng.tar.xz && \
    tar -xf /tmp/libpng.tar.xz -C /tmp && \
    cd /tmp/libpng-1.6.50 && \
    CC=aarch64-linux-gnu-gcc \
    AR=aarch64-linux-gnu-ar \
    RANLIB=aarch64-linux-gnu-ranlib \
    ./configure \
        --host=aarch64-linux-gnu \
        --prefix=${DEPS} \
        --disable-shared \
        --enable-static \
        --with-zlib-prefix=${DEPS} && \
    make -j"$(nproc)" && \
    make install && \
    rm -rf /tmp/libpng*

# ------------------------------------------------------------
# giflib
# ------------------------------------------------------------

RUN curl -L \
        https://downloads.sourceforge.net/giflib/giflib-5.2.2.tar.gz \
        -o /tmp/giflib.tar.gz && \
    tar -xf /tmp/giflib.tar.gz -C /tmp && \
    cd /tmp/giflib-5.2.2 && \
    make \
        CC=aarch64-linux-gnu-gcc \
        AR=aarch64-linux-gnu-ar \
        RANLIB=aarch64-linux-gnu-ranlib \
        CFLAGS="-I${DEPS}/include" \
        LDFLAGS="-L${DEPS}/lib" \
        libgif.a && \
    cp libgif.a ${DEPS}/lib/ && \
    cp gif_lib.h ${DEPS}/include/ && \
    rm -rf /tmp/giflib*

# ------------------------------------------------------------
# libjpeg-turbo
# ------------------------------------------------------------

RUN curl -L \
        https://github.com/libjpeg-turbo/libjpeg-turbo/releases/download/2.1.5.1/libjpeg-turbo-2.1.5.1.tar.gz \
        -o /tmp/jpeg.tar.gz && \
    tar -xf /tmp/jpeg.tar.gz -C /tmp && \
    cd /tmp/libjpeg-turbo-2.1.5.1 && \
    mkdir build && \
    cd build && \
    cmake .. \
        -DCMAKE_SYSTEM_NAME=Linux \
        -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
        -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
        -DCMAKE_INSTALL_PREFIX=${DEPS} \
        -DENABLE_SHARED=FALSE \
        -DWITH_TURBOJPEG=FALSE \
        -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE && \
    make -j"$(nproc)" && \
    make install && \
    rm -rf /tmp/libjpeg-turbo*
    
# ------------------------------------------------------------
# Verify dependencies
# ------------------------------------------------------------

RUN ls -lh \
        ${DEPS}/lib/libz.a \
        ${DEPS}/lib/libpng.a \
        ${DEPS}/lib/libgif.a \
        ${DEPS}/lib/libjpeg.a

RUN aarch64-linux-gnu-gcc --version
