
FROM debian:11-slim


ADD ./builder.sh /opt/builder.sh
RUN chmod +x /opt/builder.sh

# install all build dependencies
RUN apt-get update; \
apt-get install -y libzen-dev libmediainfo-dev pkg-config debhelper libcrypto++-dev libpcre3-dev libc-ares-dev libssl-dev \
libsqlite3-dev zlib1g-dev wget dh-autoreconf libtool-bin libreadline-dev libfreeimage-dev \
libsodium-dev libuv1-dev libavcodec-dev libavutil-dev libavformat-dev libswscale-dev libmediainfo-dev libzen-dev zip libicu-dev \
fuse libfuse-dev

#Install pdfium
RUN wget https://artifactory.developers.mega.co.nz/artifactory/debian-dev/private/Debian_11/amd64/pdfium-mega_5247.0-4.1_amd64.deb
RUN apt-get install -y $PWD/pdfium-mega_5247.0-4.1_amd64.deb

#Install gtest
RUN apt-get install -y libgtest-dev libgmock-dev

RUN apt-get clean && rm -rf /var/lib/apt/lists/*

ENTRYPOINT /opt/builder.sh
