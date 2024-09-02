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

Requires: procps-ng
BuildRequires: autoconf, autoconf-archive, automake, libtool, gcc-c++
BuildRequires: hicolor-icon-theme, zip, unzip, nasm, cmake, perl

%if 0%{?fedora_version} >= 40
    BuildRequires: wget2, wget2-wget
%else
    BuildRequires: wget
%endif

%if 0%{?suse_version} || 0%{?sle_version}
    %if 0%{?suse_version} > 1500
        BuildRequires: pkgconf-pkg-config
    %else
        BuildRequires: pkg-config
    %endif
%endif
%if 0%{?fedora}
    BuildRequires: pkgconf-pkg-config
%endif


#OpenSUSE
%if 0%{?suse_version} || 0%{?sle_version}
    # disabling post-build-checks that ocassionally prevent opensuse rpms from being generated
    # plus it speeds up building process
    #!BuildIgnore: post-build-checks

    # OpenSUSE leap features too old compiler and python 3.10 by default:
    %if 0%{?suse_version} && 0%{?suse_version} <= 1500
        BuildRequires: gcc13 gcc13-c++
        BuildRequires: python311
    %endif
%endif

#Fedora specific
%if 0%{?fedora}
    # allowing for rpaths (taken as invalid, as if they were not absolute paths when they are)
    %if 0%{?fedora_version} >= 35
        %define __brp_check_rpaths QA_RPATHS=0x0002 /usr/lib/rpm/check-rpaths
    %endif
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
sed -i -E "s/(^#define MEGACMD_BUILD_ID )[0-9]*/\1${mega_build_id}/g" src/megacmdversion.h.in

%define fullreqs -DREQUIRE_HAVE_PDFIUM -DREQUIRE_HAVE_FFMPEG -DREQUIRE_HAVE_LIBUV -DREQUIRE_USE_MEDIAINFO -DREQUIRE_USE_PCRE

%if ( 0%{?fedora_version} && 0%{?fedora_version}<=38 ) || ( 0%{?centos_version} == 600 ) || ( 0%{?centos_version} == 800 ) || ( 0%{?sle_version} && 0%{?sle_version} < 150500 )
    %define extradefines -DMEGACMD_DEPRECATED_OS
%else
    %define extradefines %{nil}
%endif

if [ -f /opt/vcpkg.tar.gz ]; then
    export VCPKG_DEFAULT_BINARY_CACHE=/opt/persistent/vcpkg_cache
    mkdir -p ${VCPKG_DEFAULT_BINARY_CACHE}
    tar xzf /opt/vcpkg.tar.gz
    vcpkg_root="-DVCPKG_ROOT=vcpkg"
fi

# use a custom cmake if required/available:
if [ -f /opt/cmake.tar.gz ]; then
    echo "8dc99be7ba94ad6e14256b049e396b40  /opt/cmake.tar.gz" | md5sum -c -
    tar xzf /opt/cmake.tar.gz
    ln -s cmake-*-Linux* cmake_inst
    export PATH="${PWD}/cmake_inst/bin:${PATH}"
fi

# OpenSuse Leap 15.x defaults to gcc7.
# Python>=10 needed for VCPKG pkgconf
%if 0%{?suse_version} && 0%{?suse_version} <= 1500
    export CC=gcc-13
    export CXX=g++-13
    mkdir python311
    ln -sf /usr/bin/python3.11 python311/python3
    export PATH=$PWD/python311:$PATH
%endif

if [ -n "%{extradefines}" ]; then
    export CXXFLAGS="%{extradefines} ${CXXFLAGS}"
fi
cmake --version
cmake ${vcpkg_root} -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo -S . -B %{_builddir}/build_dir

%build

if [ -f /opt/cmake.tar.gz ]; then
    export PATH="${PWD}/cmake_inst/bin:${PATH}"
fi

cmake --build %{_builddir}/build_dir %{?_smp_mflags}

%install


if [ -f /opt/cmake.tar.gz ]; then
    export PATH="${PWD}/cmake_inst/bin:${PATH}"
fi

cmake --install %{_builddir}/build_dir --prefix %{buildroot}

%post
#TODO: source bash_completion?

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
    %if 0%{?sle_version} == 150000 || 0%{?sle_version} == 150100 || 0%{?sle_version} == 150200
        %define reponame openSUSE_Leap_15.0
    %endif

    %if 0%{?sle_version} == 150300
        %define reponame openSUSE_Leap_15.3
    %endif

    %if 0%{?sle_version} == 150400
        %define reponame openSUSE_Leap_15.4
    %endif

    %if 0%{?sle_version} == 150500
        %define reponame openSUSE_Leap_15.5
    %endif

    %if 0%{?sle_version} == 150600
        %define reponame openSUSE_Leap_15.6
    %endif

    %if 0%{?sle_version} == 0 && 0%{?suse_version} >= 1550
        %define reponame openSUSE_Tumbleweed
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
[ "$1" == "1" ] && pkill --signal SIGUSR1 mega-cmd-server 2> /dev/null || true


%postun

# kill running MEGAcmd instance when uninstall (!upgrade)
[ "$1" == "0" ] && pkill mega-cmd 2> /dev/null || true
[ "$1" == "0" ] && pkill mega-cmd-server 2> /dev/null || true


%posttrans
# to restore dormant MEGAcmd upon updates
pkill --signal SIGUSR2 mega-cmd-server 2> /dev/null || true


%clean
%{?buildroot:%__rm -rf "%{buildroot}"}


%files
%defattr(-,root,root)
%{_bindir}/*
/opt/*
/etc/bash_completion.d/megacmd_completion.sh
/etc/sysctl.d/99-megacmd-inotify-limit.conf

%changelog
