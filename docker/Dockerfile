# Switch build environment: devkitA64 + libnx + the portlibs we need, plus
# libopenmpt (not packaged by devkitPro) for the game's MO3 music.
# Pinned base; -Syu below pulls the current devkitA64 (r30, gcc 16.1 as of 2026-11).
FROM devkitpro/devkita64:20260219

RUN dkp-pacman -Syu --noconfirm --needed \
        switch-dev switch-sdl2 switch-mesa switch-glad \
        switch-libvorbis switch-libogg switch-mpg123 \
        switch-libpng switch-libjpeg-turbo switch-zlib \
    && yes | dkp-pacman -Scc

ARG OPENMPT_VERSION=0.8.10
RUN set -eux; \
    . $DEVKITPRO/switchvars.sh; \
    cd /tmp; \
    curl -fsSL "https://lib.openmpt.org/files/libopenmpt/src/libopenmpt-${OPENMPT_VERSION}+release.autotools.tar.gz" | tar xz; \
    cd libopenmpt-${OPENMPT_VERSION}+release.autotools; \
    ./configure --host=aarch64-none-elf --prefix="$PORTLIBS_PREFIX" \
        --disable-shared --enable-static \
        --disable-openmpt123 --disable-examples --disable-tests --disable-doxygen-doc \
        --without-portaudio --without-portaudiocpp --without-pulseaudio \
        --without-sndfile --without-flac --without-sdl2 \
        --with-mpg123 --with-ogg --with-vorbis --with-vorbisfile --with-zlib; \
    make -j"$(nproc)"; \
    make install; \
    cd /; rm -rf /tmp/libopenmpt-*

WORKDIR /src
