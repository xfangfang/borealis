FROM ubuntu:22.04

MAINTAINER xfangfang <xfangfang@126.com>

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get upgrade -y && \
    apt-get install -y --no-install-recommends ca-certificates curl wget git patch cmake make pkg-config zstd libarchive-tools libgpgme11  && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

RUN wget http://archive.ubuntu.com/ubuntu/pool/main/o/openssl/libssl1.1_1.1.0g-2ubuntu4_amd64.deb && \
    wget https://github.com/PacBrew/pacbrew-pacman/releases/download/v1.1/pacbrew-pacman-1.1.deb && \
    dpkg -i *.deb && \
    rm *.deb

RUN ln -s /proc/mounts /etc/mtab && \
    pacbrew-pacman -Sy --noconfirm && \
    pacbrew-pacman -S --noconfirm ps4-openorbis ps4-openorbis-sdl2 ps4-openorbis-libsamplerate && \
    yes | pacbrew-pacman -Scc && \
    cp /opt/pacbrew/ps4/openorbis/ps4vars.sh /root/.bash_aliases

RUN mkdir /src/ &&\
    echo \#\!/bin/bash -i >> /entrypoint.sh &&\
    echo >> /entrypoint.sh &&\
    echo "set -e" >> /entrypoint.sh &&\
    echo "cd /src" >> /entrypoint.sh &&\
    echo "echo \"\$@\"" >> /entrypoint.sh &&\
    echo "bash -c \"\$@\"" >> /entrypoint.sh &&\
    chmod +x /entrypoint.sh

ENV LANG en_US.UTF-8
VOLUME /src/
WORKDIR /src/
SHELL ["/bin/bash", "-i", "-c"]
ENTRYPOINT ["/entrypoint.sh"]
