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

    %if 0%{?suse_version} > 1500
        BuildRequires: pkgconf-pkg-config
    %else
        BuildRequires: pkg-config
    %endif

    %if 0%{?suse_version} > 1500 || 0%{?sle_version} >= 150300 || (0%{?is_opensuse} && 0%{?sle_version} >= 150000)
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
    BuildRequires: openssl-devel, sqlite-devel, c-ares-devel

    %if 0%{?fedora_version} < 33
        BuildRequires: cryptopp-devel

        %if 0%{?fedora_version} >= 26
            Requires: cryptopp >= 5.6.5
        %endif
    %endif

    %if 0%{?fedora_version} >= 31
        BuildRequires: bzip2-devel
    %endif

    # allowing for rpaths (taken as invalid, as if they were not absolute paths when they are)
    %if 0%{?fedora_version} >= 35
        %define __brp_check_rpaths QA_RPATHS=0x0002 /usr/lib/rpm/check-rpaths
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

%if 0%{?centos_version} || 0%{?scientificlinux_version} || 0%{?rhel_version} ||  0%{?suse_version} > 1320 || 0%{?fedora_version} >= 33
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
%if 0%{?fedora_version} < 33
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

CPPFLAGS="$CPPFLAGS %{fullreqs}" LDFLAGS="$LDFLAGS -Wl,-rpath,/opt/megacmd/lib" ./configure --disable-shared --enable-static \
  --disable-silent-rules --disable-curl-checks %{with_cryptopp} %{with_libraw} --with-sodium=$PWD/deps --with-pcre \
  %{with_zlib} --with-sqlite=$PWD/deps --with-cares=$PWD/deps --with-libuv=$PWD/deps --with-pdfium \
  --with-curl=$PWD/deps --with-freeimage=$PWD/deps --with-readline=$PWD/deps \
  --with-termcap=$PWD/deps --prefix=$PWD/deps --disable-examples %{with_mediainfo} || export CONFFAILED=1

if [ "x$CONFFAILED" == "x1" ]; then
    sed -i "s#.*CONFLICTIVEOLDAUTOTOOLS##g" sdk/configure.ac
    ./autogen.sh
    CPPFLAGS="$CPPFLAGS %{fullreqs}" LDFLAGS="$LDFLAGS -Wl,-rpath,/opt/megacmd/lib" ./configure --disable-shared --enable-static \
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
echo "fs.inotify.max_user_watches = 524288" > %{buildroot}/etc/sysctl.d/99-megacmd-inotify-limit.conf

mkdir -p  %{buildroot}/opt/megacmd/lib
install -D deps/lib/libfreeimage.so.* %{buildroot}/opt/megacmd/lib

%post
#source bash_completion?

## Configure repository ##

%if 0%{?fedora}

    YUM_FILE="/etc/yum.repos.d/megasync.repo"
    cat > "$YUM_FILE" << DATA
[MEGAsync]
name=MEGAsync
baseurl=https://mega.nz/linux/repo/Fedora_\$releasever/
gpgkey=https://mega.nz/linux/repo/Fedora_\$releasever/repodata/repomd.xml.key
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
baseurl=https://mega.nz/linux/repo/%{reponame}/
gpgkey=https://mega.nz/linux/repo/%{reponame}/repodata/repomd.xml.key
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

    %if 0%{?sle_version} == 150000 || 0%{?sle_version} == 150100 || 0%{?sle_version} == 150200
        %define reponame openSUSE_Leap_15.0
    %endif

    %if 0%{?sle_version} == 150300
        %define reponame openSUSE_Leap_15.3
    %endif

    %if 0%{?sle_version} == 150400
        %define reponame openSUSE_Leap_15.4
    %endif

    %if 0%{?sle_version} == 0 && 0%{?suse_version} >= 1550
        %define reponame openSUSE_Tumbleweed
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
baseurl=https://mega.nz/linux/repo/%{reponame}/
gpgcheck=1
autorefresh=1
gpgkey=https://mega.nz/linux/repo/%{reponame}/repodata/repomd.xml.key
enabled=1
DATA
    fi

%endif

### include public signing key #####
# Install new key if it's not present
# Notice, for openSuse, postinst is checked (and therefore executed) when creating the rpm
# we need to ensure no command results in fail (returns !=0)

# Remove old key if present.
if (rpm -q gpg-pubkey-7f068e5d-563dc081 &> /dev/null); then
    mv /var/lib/rpm/.rpm.lock /var/lib/rpm/.rpm.lock_moved || : #to allow key management.
    %if 0%{?suse_version}
        #Key management would fail due to lock in /var/lib/rpm/Packages. We create a copy
        cp /var/lib/rpm/Packages{,_moved}
        mv /var/lib/rpm/Packages{_moved,}
    %endif
    rpm -e gpg-pubkey-7f068e5d-563dc081
    mv /var/lib/rpm/.rpm.lock_moved /var/lib/rpm/.rpm.lock || : #take it back
fi

rpm -q gpg-pubkey-7094a482-61ded129 > /dev/null 2>&1 || KEY_NOT_FOUND=1

if [ ! -z "$KEY_NOT_FOUND" ]; then

    KEYFILE=$(mktemp /tmp/megasync.XXXXXX || :)
    if [ -n "$KEYFILE" ]; then

            cat > "$KEYFILE" <<KEY || :
-----BEGIN PGP PUBLIC KEY BLOCK-----

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
sAdVa6xer/c=
=F8S0
-----END PGP PUBLIC KEY BLOCK-----
KEY

        mv /var/lib/rpm/.rpm.lock /var/lib/rpm/.rpm.lock_moved || : #to allow rpm import within postinst

        %if 0%{?suse_version}
            #Key import would hang and fail due to lock in /var/lib/rpm/Packages. We create a copy
            cp /var/lib/rpm/Packages{,_moved}
            mv /var/lib/rpm/Packages{_moved,}
        %endif

        rpm --import "$KEYFILE" 2>&1 || FAILED_IMPORT=1

        mv /var/lib/rpm/.rpm.lock_moved /var/lib/rpm/.rpm.lock || : #take it back

        rm $KEYFILE || :
    fi
fi

sysctl -p /etc/sysctl.d/99-megacmd-inotify-limit.conf

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
/etc/sysctl.d/99-megacmd-inotify-limit.conf
/opt/megacmd/lib/libfreeimage.so.3

%changelog
