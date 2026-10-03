# build container for Ubuntu, adjust for your distro, or ask QM.
FROM ubuntu:25.04

LABEL maintainer="qm210"

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
            # for add-apt-repository (see below)docker r
            software-properties-common gnupg \
            # general building
            build-essential cmake ninja-build git ca-certificates pkg-config \
            # stuff for graphics
            libgl1-mesa-dev libglu1-mesa-dev libxkbcommon-dev libwayland-dev \
            libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libx11-dev \
			# and debugging =P
			vim && \
    # need the Toolchain PPA for GCC 13 for C++20
    add-apt-repository -y ppa:ubuntu-toolchain-r/test && \
    apt-get update && \
    apt-get install -y --no-install-recommends gcc-13 g++-13 && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100 && \
    rm -rf /var/lib/apt/lists/*

RUN useradd -m team210
USER team210
WORKDIR /home/team210

COPY --chown=team210:team210 . .

# this is meant to be used as (when the image is named simulator-build)
# docker run --rm -v $PWD/build:/mnt simulator-build

CMD cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build && \
    cp -rv build/* /mnt
