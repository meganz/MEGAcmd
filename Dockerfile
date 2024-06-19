FROM ubuntu:20.04

ENV DEBIAN_FRONTEND noninteractive

WORKDIR /

RUN \
    apt-get update &&\
    apt-get install -y autoconf libtool g++ libcrypto++-dev libz-dev libsqlite3-dev libssl-dev \
                   libcurl4-gnutls-dev libreadline-dev libpcre++-dev libsodium-dev libc-ares-dev \
                   libfreeimage-dev libavcodec-dev libavutil-dev libavformat-dev libswscale-dev \
                   libmediainfo-dev libzen-dev libuv1-dev git make &&\
    apt-get clean &&\
    rm -rf /var/lib/apt/lists/*

COPY . /MEGAcmd/

RUN \
    cd MEGAcmd &&\
    git submodule update --init --recursive &&\
    sh autogen.sh &&\
    ./configure --disable-dependency-tracking &&\
    make &&\
    make install &&\
    ldconfig


