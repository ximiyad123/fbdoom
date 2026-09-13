FROM debian:stretch

ENV DEPS=/opt/fbprinter-deps
ENV DEBIAN_FRONTEND=noninteractive

# ------------------------------------------------------------
# Debian Stretch archive repositories
# ------------------------------------------------------------

RUN printf '%s\n' \
        'deb http://archive.debian.org/debian stretch main' \
        'deb http://archive.debian.org/debian-security stretch/updates main' \
        > /etc/apt/sources.list && \
    printf '%s\n' \
        'Acquire::Check-Valid-Until "false";' \
        'Acquire::AllowInsecureRepositories "true";' \
        'Acquire::AllowDowngradeToInsecureRepositories "true";' \
        > /etc/apt/apt.conf.d/99archive

# ------------------------------------------------------------
# Build dependencies + AArch64 cross compiler
# ------------------------------------------------------------

RUN dpkg --add-architecture arm64 && \
    apt-get update && \
    apt-get install -y \
        build-essential \
        gcc-aarch64-linux-gnu \
        g++-aarch64-linux-gnu \
        libc6-dev-arm64-cross \
        binutils-aarch64-linux-gnu \
        autoconf \
        automake \
        libtool \
        cmake \
        pkg-config \
        ca-certificates \
        curl \
        wget \
        xz-utils \
        bzip2 \
        tar \
        file \
        nasm \
        python3 \
        python3-minimal \
    && rm -rf /var/lib/apt/lists/*

# ------------------------------------------------------------
# Dependency prefix
# ------------------------------------------------------------

RUN mkdir -p \
        ${DEPS}/include \
        ${DEPS}/lib \
        ${DEPS}/lib/pkgconfig

ENV PKG_CONFIG_PATH=${DEPS}/lib/pkgconfig
ENV PKG_CONFIG_LIBDIR=${DEPS}/lib/pkgconfig
ENV CPPFLAGS="-I${DEPS}/include"
ENV CFLAGS="-I${DEPS}/include"
ENV LDFLAGS="-L${DEPS}/lib"

# ------------------------------------------------------------
# Build zlib
# ------------------------------------------------------------

WORKDIR /tmp

RUN curl -L \
        https://zlib.net/fossils/zlib-1.3.1.tar.gz \
        -o zlib.tar.gz && \
    tar xf zlib.tar.gz && \
    cd zlib-1.3.1 && \
    CC=aarch64-linux-gnu-gcc \
    AR=aarch64-linux-gnu-ar \
    RANLIB=aarch64-linux-gnu-ranlib \
    ./configure \
        --prefix=${DEPS} \
        --static && \
    make -j"$(nproc)" && \
    make install && \
    cd /tmp && \
    rm -rf zlib-1.3.1 zlib.tar.gz

# ------------------------------------------------------------
# Build libpng
# ------------------------------------------------------------

RUN curl -L \
        https://download.sourceforge.net/libpng/libpng-1.6.50.tar.xz \
        -o libpng.tar.xz && \
    tar xf libpng.tar.xz && \
    cd libpng-1.6.50 && \
    ./configure \
        --host=aarch64-linux-gnu \
        --prefix=${DEPS} \
        --disable-shared \
        --enable-static \
        ZLIB_CFLAGS="-I${DEPS}/include" \
        ZLIB_LIBS="-L${DEPS}/lib -lz" && \
    make -j"$(nproc)" && \
    make install && \
    cd /tmp && \
    rm -rf libpng-1.6.50 libpng.tar.xz

# ------------------------------------------------------------
# Build giflib
# ------------------------------------------------------------

RUN curl -L \
        https://downloads.sourceforge.net/giflib/giflib-5.2.2.tar.gz \
        -o giflib.tar.gz && \
    tar xf giflib.tar.gz && \
    cd giflib-5.2.2 && \
    make \
        CC=aarch64-linux-gnu-gcc \
        AR=aarch64-linux-gnu-ar \
        RANLIB=aarch64-linux-gnu-ranlib \
        CFLAGS="-O2 -fPIC" \
        LDFLAGS="" \
        libgif.a && \
    mkdir -p ${DEPS}/include ${DEPS}/lib && \
    cp gif_lib.h ${DEPS}/include/ && \
    cp libgif.a ${DEPS}/lib/ && \
    cd /tmp && \
    rm -rf giflib-5.2.2 giflib.tar.gz

# ------------------------------------------------------------
# Build libjpeg-turbo
# ------------------------------------------------------------

RUN curl -L \
        https://github.com/libjpeg-turbo/libjpeg-turbo/releases/download/2.1.5.1/libjpeg-turbo-2.1.5.1.tar.gz \
        -o libjpeg.tar.gz && \
    tar xf libjpeg.tar.gz && \
    cd libjpeg-turbo-2.1.5.1 && \
    mkdir build && \
    cd build && \
    cmake .. \
        -DCMAKE_SYSTEM_NAME=Linux \
        -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
        -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
        -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++ \
        -DCMAKE_INSTALL_PREFIX=${DEPS} \
        -DENABLE_SHARED=OFF \
        -DENABLE_STATIC=ON \
        -DWITH_SIMD=OFF && \
    make -j"$(nproc)" && \
    make install && \
    cd /tmp && \
    rm -rf libjpeg-turbo-2.1.5.1 libjpeg.tar.gz

# ------------------------------------------------------------
# Build SDL2
#
# Stretch's SDL2 is only 2.0.5.
# Chocolate Doom requires >= 2.0.14.
#
# SDL2 is built statically for AArch64 into ${DEPS}.
# ------------------------------------------------------------

RUN curl -L \
        https://github.com/libsdl-org/SDL/releases/download/release-2.30.9/SDL2-2.30.9.tar.gz \
        -o sdl2.tar.gz && \
    tar xf sdl2.tar.gz && \
    cd SDL2-2.30.9 && \
    ./configure \
        --host=aarch64-linux-gnu \
        --prefix=${DEPS} \
        --disable-shared \
        --enable-static \
        --disable-video-wayland \
        --disable-video-x11 \
        --disable-video-opengl \
        --disable-video-opengles \
        --disable-video-vulkan \
        --disable-render \
        --disable-joystick \
        --disable-haptic \
        --disable-power \
        --disable-filesystem \
        --disable-libsamplerate \
        --disable-pulseaudio \
        --disable-alsa \
        --disable-esd \
        --disable-arts \
        --disable-nas \
        --disable-jack \
        --disable-diskaudio \
        --disable-dummyaudio \
        --disable-video-dummy \
        --disable-input-tslib \
        --disable-video-rpi \
        --disable-video-kmsdrm \
        --disable-video-fbcon && \
    make -j"$(nproc)" && \
    make install && \
    cd /tmp && \
    rm -rf SDL2-2.30.9 sdl2.tar.gz


# ------------------------------------------------------------
# Build SDL2_mixer
# ------------------------------------------------------------

RUN curl -L \
        https://github.com/libsdl-org/SDL_mixer/releases/download/release-2.8.0/SDL2_mixer-2.8.0.tar.gz \
        -o sdl2-mixer.tar.gz && \
    tar xf sdl2-mixer.tar.gz && \
    cd SDL2_mixer-2.8.0 && \
    ./configure \
        --host=aarch64-linux-gnu \
        --prefix=${DEPS} \
        --disable-shared \
        --enable-static \
        --disable-music-flac \
        --disable-music-mod \
        --disable-music-midi \
        --disable-music-opus \
        --disable-music-wavpack \
        --disable-music-gme \
        --disable-music-mp3 && \
    make -j"$(nproc)" && \
    make install && \
    cd /tmp && \
    rm -rf SDL2_mixer-2.8.0 sdl2-mixer.tar.gz

RUN echo "AArch64 SDL2_mixer version:" && \
    aarch64-linux-gnu-pkg-config --modversion SDL2_mixer

RUN echo "AArch64 SDL2_mixer flags:" && \
    aarch64-linux-gnu-pkg-config --cflags --libs SDL2_mixer


# ------------------------------------------------------------
# Build SDL2_net
# ------------------------------------------------------------

RUN curl -L \
        https://github.com/libsdl-org/SDL_net/releases/download/release-2.2.0/SDL2_net-2.2.0.tar.gz \
        -o sdl2-net.tar.gz && \
    tar xf sdl2-net.tar.gz && \
    cd SDL2_net-2.2.0 && \
    ./configure \
        --host=aarch64-linux-gnu \
        --prefix=${DEPS} \
        --disable-shared \
        --enable-static && \
    make -j"$(nproc)" && \
    make install && \
    cd /tmp && \
    rm -rf SDL2_net-2.2.0 sdl2-net.tar.gz

RUN echo "AArch64 SDL2_net version:" && \
    aarch64-linux-gnu-pkg-config --modversion SDL2_net

RUN echo "AArch64 SDL2_net flags:" && \
    aarch64-linux-gnu-pkg-config --cflags --libs SDL2_net

    

# ------------------------------------------------------------
# Make sure our SDL2 pkg-config file is the one being found.
# ------------------------------------------------------------

RUN echo "SDL2 version:" && \
    PKG_CONFIG_PATH=${DEPS}/lib/pkgconfig \
    pkg-config --modversion sdl2 || true

RUN echo "AArch64 SDL2 version:" && \
    aarch64-linux-gnu-pkg-config --modversion sdl2

RUN echo "AArch64 SDL2 flags:" && \
    aarch64-linux-gnu-pkg-config --cflags --libs sdl2

# ------------------------------------------------------------
# Toolchain sanity check
# ------------------------------------------------------------

RUN aarch64-linux-gnu-gcc --version && \
    aarch64-linux-gnu-ar --version && \
    aarch64-linux-gnu-ranlib --version

WORKDIR /src
