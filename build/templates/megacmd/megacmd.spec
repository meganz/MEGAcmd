Name:		megacmd
Version:	megacmd_VERSION
Release:	%(cat MEGA_BUILD_ID || echo "1").1
Summary:	MEGA Command Line Interactive and Scriptable Application
License:	https://github.com/meganz/megacmd/blob/master/LICENSE
Group:		Applications/Others
Url:		https://mega.nz
Source0:	megacmd_%{version}.tar.gz
Vendor:		MEGA Limited
Packager:	MEGA Linux Team <linux@mega.co.nz>

BuildRequires: zlib-devel, autoconf, automake, libtool, gcc-c++, pcre-devel
BuildRequires: hicolor-icon-theme, unzip, wget
BuildRequires: ffmpeg-mega pdfium-mega

#OpenSUSE
%if 0%{?suse_version} || 0%{?sle_version}

    BuildRequires: libopenssl-devel, sqlite3-devel
    BuildRequires: libbz2-devel

    # disabling post-build-checks that ocassionally prevent opensuse rpms from being generated
    # plus it speeds up building process
    #!BuildIgnore: post-build-checks

    %if 0%{?sle_version} >= 150000
        BuildRequires: libcurl4
    %endif

    %if 0%{?suse_version}>=1550
        BuildRequires: pkgconf-pkg-config
    %else
        BuildRequires: pkg-config
    %endif

    %if 0%{?suse_version}>=1550 || (0%{?is_opensuse} && 0%{?sle_version} > 120300 )
        BuildRequires: c-ares-devel
    %else
        BuildRequires: libcares-devel
    %endif

    %if 0%{?suse_version} <= 1320
        BuildRequires: libcryptopp-devel
    %endif

%endif

#Fedora specific
%if 0%{?fedora}
    BuildRequires: openssl-devel, sqlite-devel, c-ares-devel, cryptopp-devel

    %if 0%{?fedora_version} >= 31
        BuildRequires: bzip2-devel
    %endif

    %if 0%{?fedora_version} >= 26
        Requires: cryptopp >= 5.6.5
    %endif

%endif

#centos/scientific linux
%if 0%{?centos_version} || 0%{?scientificlinux_version}
    BuildRequires: openssl-devel, sqlite-devel, c-ares-devel, bzip2-devel

    %if 0%{?centos_version} >= 800
        BuildRequires: bzip2-devel
    %endif
%endif

#red hat
%if 0%{?rhel_version}
    BuildRequires: openssl-devel, sqlite-devel
    %if 0%{?rhel_version} >= 800
        BuildRequires: bzip2-devel
    %endif
%endif

### Specific buildable dependencies ###

#Media info
%define flag_disablemediainfo -i
%define with_mediainfo %{nil}

%if 0%{?fedora_version}==21 || 0%{?fedora_version}==22 || 0%{?fedora_version}>=25 || !(0%{?sle_version} < 120300)
    BuildRequires: libzen-devel, libmediainfo-devel
%endif

%if 0%{?fedora_version}==19 || 0%{?fedora_version}==20 || 0%{?fedora_version}==23 || 0%{?fedora_version}==24 || 0%{?centos_version} || 0%{?scientificlinux_version} || 0%{?rhel_version} || ( 0%{?suse_version} && 0%{?sle_version} < 120300)
    %define flag_disablemediainfo %{nil}
    %define with_mediainfo --with-libmediainfo=$PWD/deps --with-libzen=$PWD/deps
%endif

#Build cryptopp?
%define flag_cryptopp %{nil}
%define with_cryptopp %{nil}

%if 0%{?centos_version} || 0%{?scientificlinux_version} || 0%{?rhel_version} ||  0%{?suse_version} > 1320
    %define flag_cryptopp -q
    %define with_cryptopp --with-cryptopp=$PWD/deps
%endif

#Build cares?
%define flag_cares %{nil}

%if 0%{?rhel_version}
    %define flag_cares -e
    %define with_cares --with-cares=$PWD/deps
%endif

#Build libraw?
%define flag_libraw %{nil}
%define with_libraw --without-libraw

#Build zlib?
%define flag_disablezlib %{nil}
%define with_zlib --with-zlib=$PWD/deps

%if 0%{?fedora_version} == 23
    %define flag_disablezlib -z
    %define with_zlib %{nil}
%endif

%description
MEGAcmd provides non UI access to MEGA services. 
It intends to offer all the functionality 
with your MEGA account via shell interaction. 
It features 2 modes of interaction:

  - interactive. A shell to query your actions
  - scriptable. A way to execute commands from a shell/a script/another program.

%prep
%setup -q

mega_build_id=`echo %{release} | cut -d'.' -f 1`
sed -i -E "s/(^#define MEGACMD_BUILD_ID )[0-9]*/\1${mega_build_id}/g" src/megacmdversion.h

%build

# Fedora uses system Crypto++ header files
%if 0%{?fedora}
    rm -fr bindings/qt/3rdparty/include/cryptopp
%endif

%if 0%{?centos_version} == 600
    sed -i "s#AC_INIT#m4_pattern_allow(AC_PROG_OBJCXX)\nAC_INIT#g" configure.ac
    sed -i "s#AC_INIT#m4_pattern_allow(AC_PROG_OBJCXX)\nAC_INIT#g" sdk/configure.ac
%endif

%define fullreqs -DREQUIRE_HAVE_PDFIUM -DREQUIRE_HAVE_FFMPEG -DREQUIRE_HAVE_LIBUV -DREQUIRE_USE_MEDIAINFO -DREQUIRE_USE_PCRE

./autogen.sh

#build dependencies into folder deps
mkdir deps || :
bash -x ./contrib/build_sdk.sh %{flag_cryptopp} %{flag_libraw} %{flag_cares} -o archives \
  -g %{flag_disablezlib} %{flag_disablemediainfo} -b -l -c -s -u -v -a -I -p deps/

ln -sfr $PWD/deps/lib/libfreeimage*.so $PWD/deps/lib/libfreeimage.so.3
ln -sfn libfreeimage.so.3 $PWD/deps/lib/libfreeimage.so

%if ( 0%{?fedora_version} && 0%{?fedora_version}<=31 ) || ( 0%{?centos_version} == 600 ) || ( 0%{?sle_version} && 0%{?sle_version} < 150000 )
    export CPPFLAGS="$CPPFLAGS -DMEGACMD_DEPRECATED_OS"
%endif

CPPFLAGS="$CPPFLAGS %{fullreqs}" LDFLAGS="$LDFLAGS -Wl,-rpath,/opt/mega/lib" ./configure --disable-shared --enable-static \
  --disable-silent-rules --disable-curl-checks %{with_cryptopp} %{with_libraw} --with-sodium=$PWD/deps --with-pcre \
  %{with_zlib} --with-sqlite=$PWD/deps --with-cares=$PWD/deps --with-libuv=$PWD/deps --with-pdfium \
  --with-curl=$PWD/deps --with-freeimage=$PWD/deps --with-readline=$PWD/deps \
  --with-termcap=$PWD/deps --prefix=$PWD/deps --disable-examples %{with_mediainfo} || export CONFFAILED=1

if [ "x$CONFFAILED" == "x1" ]; then
    sed -i "s#.*CONFLICTIVEOLDAUTOTOOLS##g" sdk/configure.ac
    ./autogen.sh
    CPPFLAGS="$CPPFLAGS %{fullreqs}" LDFLAGS="$LDFLAGS -Wl,-rpath,/opt/mega/lib" ./configure --disable-shared --enable-static \
      --disable-silent-rules --disable-curl-checks %{with_cryptopp} %{with_libraw} --with-sodium=$PWD/deps --with-pcre \
      %{with_zlib} --with-sqlite=$PWD/deps --with-cares=$PWD/deps --with-libuv=$PWD/deps --with-pdfium \
      --with-curl=$PWD/deps --with-freeimage=$PWD/deps --with-readline=$PWD/deps \
      --with-termcap=$PWD/deps --prefix=$PWD/deps --disable-examples %{with_mediainfo} || cat sdk/configure
fi

make


%install

for i in src/client/mega-*; do install -D $i %{buildroot}%{_bindir}/${i/src\/client\//}; done
install -D src/client/megacmd_completion.sh %{buildroot}%{_sysconfdir}/bash_completion.d/megacmd_completion.sh
install -D mega-cmd %{buildroot}%{_bindir}/mega-cmd
install -D mega-cmd-server %{buildroot}%{_bindir}/mega-cmd-server
install -D mega-exec %{buildroot}%{_bindir}/mega-exec

mkdir -p  %{buildroot}/etc/sysctl.d/
echo "fs.inotify.max_user_watches = 524288" > %{buildroot}/etc/sysctl.d/100-megacmd-inotify-limit.conf

mkdir -p  %{buildroot}/opt/mega/lib
install -D deps/lib/libfreeimage.so.* %{buildroot}/opt/mega/lib

%post
#source bash_completion?

## Configure repository ##

%if 0%{?fedora}

    YUM_FILE="/etc/yum.repos.d/megasync.repo"
    cat > "$YUM_FILE" << DATA
[MEGAsync]
name=MEGAsync
baseurl=https://mega.nz/linux/MEGAsync/Fedora_\$releasever/
gpgkey=https://mega.nz/linux/MEGAsync/Fedora_\$releasever/repodata/repomd.xml.key
gpgcheck=1
enabled=1
DATA

%endif

%if 0%{?rhel_version} || 0%{?centos_version} || 0%{?scientificlinux_version}

    %if 0%{?rhel_version} == 800
        %define reponame RHEL_8
    %endif

    %if 0%{?rhel_version} == 700
        %define reponame RHEL_7
    %endif

    %if 0%{?scientificlinux_version} == 700
        %define reponame ScientificLinux_7
    %endif

    %if 0%{?centos_version} == 700
        %define reponame CentOS_7
    %endif

    %if 0%{?centos_version} == 800
        %define reponame CentOS_8
    %endif

    YUM_FILE="/etc/yum.repos.d/megasync.repo"
    cat > "$YUM_FILE" << DATA
[MEGAsync]
name=MEGAsync
baseurl=https://mega.nz/linux/MEGAsync/%{reponame}/
gpgkey=https://mega.nz/linux/MEGAsync/%{reponame}/repodata/repomd.xml.key
gpgcheck=1
enabled=1
DATA

%endif

%if 0%{?sle_version} || 0%{?suse_version}
    %if 0%{?sle_version} == 120300
        %define reponame openSUSE_Leap_42.3
    %endif

    %if 0%{?sle_version} == 120200
        %define reponame openSUSE_Leap_42.2
    %endif

    %if 0%{?sle_version} == 120100
        %define reponame openSUSE_Leap_42.1
    %endif

    %if 0%{?sle_version} == 150000
        %define reponame openSUSE_Leap_15.0
    %else
        %if 0%{?suse_version} > 1320
            %define reponame openSUSE_Tumbleweed
        %endif
    %endif
    %if 0%{?suse_version} == 1320
        %define reponame openSUSE_13.2
    %endif

    %if 0%{?suse_version} == 1310
        %define reponame openSUSE_13.1
    %endif

    %if 0%{?suse_version} == 1230
        %define reponame openSUSE_12.3
    %endif
    %if 0%{?suse_version} == 1220
        %define reponame openSUSE_12.2
    %endif

    if [ -d "/etc/zypp/repos.d/" ]; then
        ZYPP_FILE="/etc/zypp/repos.d/megasync.repo"
        cat > "$ZYPP_FILE" << DATA
[MEGAsync]
name=MEGAsync
type=rpm-md
baseurl=https://mega.nz/linux/MEGAsync/%{reponame}/
gpgcheck=1
autorefresh=1
gpgkey=https://mega.nz/linux/MEGAsync/%{reponame}/repodata/repomd.xml.key
enabled=1
DATA
    fi

%endif

### include public signing key #####
# Install new key if it's not present
# Notice, for openSuse, postinst is checked (and therefore executed) when creating the rpm
# we need to ensure no command results in fail (returns !=0)
rpm -q gpg-pubkey-7f068e5d-563dc081 > /dev/null 2>&1 || KEY_NOT_FOUND=1

if [ ! -z "$KEY_NOT_FOUND" ]; then

    KEYFILE=$(mktemp /tmp/megasync.XXXXXX || :)
    if [ -n "$KEYFILE" ]; then

            cat > "$KEYFILE" <<KEY || :
-----BEGIN PGP PUBLIC KEY BLOCK-----
Version: GnuPG v2

mI0EVj3AgQEEAO2XyJgpvE5HDRVsggcrMhf5+KpQepl7m7OyrPSgxLi72Wuy5GWp
hO64BX1UzmdUirIEOc13YxdeuhwJ3YP0wnKUyUrdWA0r2HjOz555vN6ldrPlSCBI
RxKBWRMQaR4cwNKQ8V4xV9tVdPGgrQ9L/4H3fM9fYqCwEMKBxxLZsF3PABEBAAG0
IE1lZ2FMaW1pdGVkIDxzdXBwb3J0QG1lZ2EuY28ubno+iL8EEwECACkFAlY9wIEC
GwMFCRLMAwAHCwkIBwMCAQYVCAIJCgsEFgIDAQIeAQIXgAAKCRADw606fwaOXfOS
A/998rh6f0wsrHmX2LTw2qmrWzwPj4m+vp0m3w5swPZw1x4qSNsmNsIXX8J0ZcSE
qymOwHZ0B9imBS3iz+U496NSfPNWABbIBnUAu8zq0IR28Q9pUcLe5MWFsw9NO+w2
5dByoN9JKeUftZt1x76NHn5wmxB9fv7WVlCnZJ+T16+nh7iNBFY9wIEBBADHpopM
oXNkrGZLI6Ok1F5N7+bSgiyZwkvBMAqCkPawUgwJztFKGf8F/sSbydsKRC2aQcuJ
eOj0ZPUtJ80+o3w8MsHRtZDSxDIxqqiHeupoDRI3Be9S544vI5/UmiiygTuhmNTT
NWgStoZz7hEK4IsELAG1EFodIMtBSkptDL92HwARAQABiKUEGAECAA8FAlY9wIEC
GwwFCRLMAwAACgkQA8OtOn8Gjl3HlAQAoOckF5JBJWekmlX+k2RYwtgfszk31Gq+
Jjiho4rUEW8c1EUPvK8v1jRGwjYED3ihJ6510eblYFPl+6k91OWlScnxuVVAmSn4
35RW3vR+nYUvf3s8rctbw97gJJZAA7p5oAowTux3oHotKSYhhxKcz15goMXzSb5G
/h7fJRhMnw4=
=fp/e
-----END PGP PUBLIC KEY BLOCK-----
KEY

        mv /var/lib/rpm/.rpm.lock /var/lib/rpm/.rpm.lock_moved || : #to allow rpm import within postinst

        %if 0%{?suse_version}
            #Key import would hang and fail due to lock in /var/lib/rpm/Packages. We create a copy
            mv /var/lib/rpm/Packages{,_moved}
            cp /var/lib/rpm/Packages{_moved,}
        %endif

        rpm --import "$KEYFILE" 2>&1 || FAILED_IMPORT=1
        %if 0%{?suse_version}
            rm /var/lib/rpm/Packages_moved  #remove the old one
        %endif

        mv /var/lib/rpm/.rpm.lock_moved /var/lib/rpm/.rpm.lock || : #take it back

        rm $KEYFILE || :
    fi
fi

sysctl -p /etc/sysctl.d/100-megacmd-inotify-limit.conf

### END of POSTINST


%preun
[ "$1" == "1" ] && killall -s SIGUSR1 mega-cmd-server 2> /dev/null || true


%postun

# kill running MEGAcmd instance when uninstall (!upgrade)
[ "$1" == "0" ] && killall mega-cmd 2> /dev/null || true
[ "$1" == "0" ] && killall mega-cmd-server 2> /dev/null || true


%posttrans
# to restore dormant MEGAcmd upon updates
killall -s SIGUSR2 mega-cmd-server 2> /dev/null || true


%clean
%{?buildroot:%__rm -rf "%{buildroot}"}


%files
%defattr(-,root,root)
%{_bindir}/mega-attr
%{_bindir}/mega-cd
%{_bindir}/mega-confirm
%{_bindir}/mega-cp
%{_bindir}/mega-debug
%{_bindir}/mega-du
%{_bindir}/mega-df
%{_bindir}/mega-proxy
%{_bindir}/mega-exec
%{_bindir}/mega-export
%{_bindir}/mega-find
%{_bindir}/mega-get
%{_bindir}/mega-help
%{_bindir}/mega-https
%{_bindir}/mega-webdav
%{_bindir}/mega-permissions
%{_bindir}/mega-deleteversions
%{_bindir}/mega-transfers
%{_bindir}/mega-import
%{_bindir}/mega-invite
%{_bindir}/mega-ipc
%{_bindir}/mega-killsession
%{_bindir}/mega-lcd
%{_bindir}/mega-log
%{_bindir}/mega-login
%{_bindir}/mega-logout
%{_bindir}/mega-lpwd
%{_bindir}/mega-ls
%{_bindir}/mega-backup
%{_bindir}/mega-mkdir
%{_bindir}/mega-mount
%{_bindir}/mega-mv
%{_bindir}/mega-passwd
%{_bindir}/mega-preview
%{_bindir}/mega-put
%{_bindir}/mega-pwd
%{_bindir}/mega-quit
%{_bindir}/mega-reload
%{_bindir}/mega-rm
%{_bindir}/mega-session
%{_bindir}/mega-share
%{_bindir}/mega-showpcr
%{_bindir}/mega-signup
%{_bindir}/mega-speedlimit
%{_bindir}/mega-sync
%{_bindir}/mega-exclude
%{_bindir}/mega-thumbnail
%{_bindir}/mega-userattr
%{_bindir}/mega-users
%{_bindir}/mega-version
%{_bindir}/mega-whoami
%{_bindir}/mega-cat
%{_bindir}/mega-tree
%{_bindir}/mega-mediainfo
%{_bindir}/mega-graphics
%{_bindir}/mega-ftp
%{_bindir}/mega-cancel
%{_bindir}/mega-confirmcancel
%{_bindir}/mega-errorcode
%{_bindir}/mega-cmd
%{_bindir}/mega-cmd-server
%{_sysconfdir}/bash_completion.d/megacmd_completion.sh
/etc/sysctl.d/100-megacmd-inotify-limit.conf
/opt/mega/lib/libfreeimage.so.3

%changelog
