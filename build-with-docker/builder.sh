#!/bin/bash
set -e
volume_dest="/opt/megacmd"
owner_uid=`stat -c "%u" $volume_dest`
owner_gid=`stat -c "%g" $volume_dest`


groupadd -g $owner_gid jenkins
echo "Adding \"jenkins\" user..."
useradd -r -M -u $owner_uid -g $owner_gid -d $volume_dest -s /bin/bash jenkins
chown -R jenkins:jenkins $volume_dest

echo "Configuring core dumpings on docker"
ulimit -c unlimited
rm -rf installdir || :

exec su - jenkins -c "
./autogen.sh
./sdk/contrib/build_sdk.sh -b -g -f -I -i -s -n -z -o ./3rd_pkgs -p ./3rd_deps
rm ./3rd_deps/include/sqlite* ./3rd_deps/lib/libsqlite* || :
./configure --disable-silent-rules --disable-examples --prefix=$volume_dest/installdir --with-curl=$volume_dest/3rd_deps --disable-curl-checks --enable-megacmd-tests --with-gtest

make clean
make
make install

#now run the unit tests
$volume_dest/installdir/mega-cmd-unit-tests
"
