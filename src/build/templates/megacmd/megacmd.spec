Name:		megacmd
Version:	megacmd_VERSION
Release:	1%{?dist}
Summary:	MEGA Command Line Interactive and Scriptable Application
License:	https://github.com/meganz/sdk/blob/megacmd/LICENSE
Group:		Applications/Others
Url:		https://mega.nz
Source0:	megacmd_%{version}.tar.gz
Vendor:		MEGA Limited
Packager:	MEGA Linux Team <linux@mega.co.nz>

BuildRequires: openssl-devel, sqlite-devel, zlib-devel, autoconf, automake, libtool, gcc-c++, pcre-devel
BuildRequires: hicolor-icon-theme, unzip, wget

%if 0%{?suse_version}
BuildRequires: libcares-devel
 
# disabling post-build-checks that ocassionally prevent opensuse rpms from being generated
# plus it speeds up building process
BuildRequires: -post-build-checks

%if 0%{?suse_version} <= 1320
BuildRequires: libcryptopp-devel
%endif

%endif

%if 0%{?fedora}
BuildRequires: c-ares-devel, cryptopp-devel
%endif

%if 0%{?centos_version} || 0%{?scientificlinux_version}
BuildRequires: c-ares-devel,
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

%build

%define flag_cryptopp %{nil}
%define with_cryptopp %{nil}

%if 0%{?centos_version} || 0%{?scientificlinux_version}
%define flag_cryptopp -q
%define with_cryptopp --with-cryptopp=deps
%endif

%if 0%{?rhel_version}
%define flag_cryptopp -q
%define with_cryptopp --with-cryptopp=deps
%endif

%if 0%{?suse_version} > 1320
%define flag_cryptopp -q
%define with_cryptopp --with-cryptopp=deps
%endif


%define flag_disablezlib %{nil}
%define with_zlib --with-zlib=deps

%if 0%{?fedora_version} == 23
%define flag_disablezlib -z
%define with_zlib %{nil}
%endif

# Fedora uses system Crypto++ header files
%if 0%{?fedora}
rm -fr bindings/qt/3rdparty/include/cryptopp
%endif

./autogen.sh

#build dependencies into folder deps
mkdir deps
bash -x ./contrib/build_sdk.sh %{flag_cryptopp} -o archives \
  -g %{flag_disablezlib} -b -l -c -s -u -a -p deps/

./configure --disable-shared --enable-static --disable-silent-rules \
  --disable-curl-checks %{with_cryptopp} --with-sodium=deps \
  %{with_zlib} --with-sqlite=deps --with-cares=deps \
  --with-curl=deps --with-freeimage=deps --with-readline=deps \
  --with-termcap=deps --prefix=$PWD/deps --disable-examples

make

%install

for i in examples/megacmd/client/mega-*; do install -D $i %{buildroot}%{_bindir}/${i/examples\/megacmd\/client\//}; done
install -D examples/megacmd/client/megacmd_completion.sh %{buildroot}%{_sysconfdir}/bash_completion.d/megacmd_completion.sh
install -D examples/mega-cmd %{buildroot}%{_bindir}/mega-cmd
install -D examples/mega-cmd-server %{buildroot}%{_bindir}/mega-cmd-server
install -D examples/mega-exec %{buildroot}%{_bindir}/mega-exec


%post
#source bash_completion?

%if 0%{?rhel_version} == 700
# RHEL 7
YUM_FILE="/etc/yum.repos.d/megasync.repo"
cat > "$YUM_FILE" << DATA
[MEGAsync]
name=MEGAsync
baseurl=https://mega.nz/linux/MEGAsync/RHEL_7/
gpgkey=https://mega.nz/linux/MEGAsync/RHEL_7/repodata/repomd.xml.key
gpgcheck=1
enabled=1
DATA
%endif

%if 0%{?scientificlinux_version} == 700
# Scientific Linux 7
YUM_FILE="/etc/yum.repos.d/megasync.repo"
cat > "$YUM_FILE" << DATA
[MEGAsync]
name=MEGAsync
baseurl=https://mega.nz/linux/MEGAsync/ScientificLinux_7/
gpgkey=https://mega.nz/linux/MEGAsync/ScientificLinux_7/repodata/repomd.xml.key
gpgcheck=1
enabled=1
DATA
%endif

%if 0%{?sc} == 700
# CentOS 7
YUM_FILE="/etc/yum.repos.d/megasync.repo"
cat > "$YUM_FILE" << DATA
[MEGAsync]
name=MEGAsync
baseurl=https://mega.nz/linux/MEGAsync/CentOS_7/
gpgkey=https://mega.nz/linux/MEGAsync/CentOS_7/repodata/repomd.xml.key
gpgcheck=1
enabled=1
DATA
%endif

%if 0%{?fedora_version} == 26
# Fedora 26
YUM_FILE="/etc/yum.repos.d/megasync.repo"
cat > "$YUM_FILE" << DATA
[MEGAsync]
name=MEGAsync
baseurl=http://mega.nz/linux/MEGAsync/Fedora_26/
gpgkey=https://mega.nz/linux/MEGAsync/Fedora_26/repodata/repomd.xml.key
gpgcheck=1
enabled=1
DATA
%endif


%if 0%{?fedora_version} == 25
# Fedora 25
YUM_FILE="/etc/yum.repos.d/megasync.repo"
cat > "$YUM_FILE" << DATA
[MEGAsync]
name=MEGAsync
baseurl=https://mega.nz/linux/MEGAsync/Fedora_25/
gpgkey=https://mega.nz/linux/MEGAsync/Fedora_25/repodata/repomd.xml.key
gpgcheck=1
enabled=1
DATA
%endif

%if 0%{?fedora_version} == 24
# Fedora 24
YUM_FILE="/etc/yum.repos.d/megasync.repo"
cat > "$YUM_FILE" << DATA
[MEGAsync]
name=MEGAsync
baseurl=https://mega.nz/linux/MEGAsync/Fedora_24/
gpgkey=https://mega.nz/linux/MEGAsync/Fedora_24/repodata/repomd.xml.key
gpgcheck=1
enabled=1
DATA
%endif

%if 0%{?fedora_version} == 23
# Fedora 23
YUM_FILE="/etc/yum.repos.d/megasync.repo"
cat > "$YUM_FILE" << DATA
[MEGAsync]
name=MEGAsync
baseurl=https://mega.nz/linux/MEGAsync/Fedora_23/
gpgkey=https://mega.nz/linux/MEGAsync/Fedora_23/repodata/repomd.xml.key
gpgcheck=1
enabled=1
DATA
%endif

%if 0%{?fedora_version} == 22
# Fedora 22
YUM_FILE="/etc/yum.repos.d/megasync.repo"
cat > "$YUM_FILE" << DATA
[MEGAsync]
name=MEGAsync
baseurl=https://mega.nz/linux/MEGAsync/Fedora_22/
gpgkey=https://mega.nz/linux/MEGAsync/Fedora_22/repodata/repomd.xml.key
gpgcheck=1
enabled=1
DATA
%endif

%if 0%{?fedora_version} == 21
# Fedora 21
YUM_FILE="/etc/yum.repos.d/megasync.repo"
cat > "$YUM_FILE" << DATA
[MEGAsync]
name=MEGAsync
baseurl=https://mega.nz/linux/MEGAsync/Fedora_21/
gpgkey=https://mega.nz/linux/MEGAsync/Fedora_21/repodata/repomd.xml.key
gpgcheck=1
enabled=1
DATA
%endif

%if 0%{?fedora_version} == 20
# Fedora 20
YUM_FILE="/etc/yum.repos.d/megasync.repo"
cat > "$YUM_FILE" << DATA
[MEGAsync]
name=MEGAsync
baseurl=https://mega.nz/linux/MEGAsync/Fedora_20/
gpgkey=https://mega.nz/linux/MEGAsync/Fedora_20/repodata/repomd.xml.key
gpgcheck=1
enabled=1
DATA
%endif

%if 0%{?fedora_version} == 19
# Fedora 19
YUM_FILE="/etc/yum.repos.d/megasync.repo"
cat > "$YUM_FILE" << DATA
[MEGAsync]
name=MEGAsync
baseurl=https://mega.nz/linux/MEGAsync/Fedora_19
gpgkey=https://mega.nz/linux/MEGAsync/Fedora_19/repodata/repomd.xml.key
gpgcheck=1
enabled=1
DATA
%endif

%if 0%{?sle_version} == 120300
# openSUSE Leap 42.3
if [ -d "/etc/zypp/repos.d/" ]; then
ZYPP_FILE="/etc/zypp/repos.d/megasync.repo"
cat > "$ZYPP_FILE" << DATA
[MEGAsync]
name=MEGAsync
type=rpm-md
baseurl=http://mega.nz/linux/MEGAsync/openSUSE_Leap_42.3/
gpgcheck=1
autorefresh=1
gpgkey=http://mega.nz/linux/MEGAsync/openSUSE_Leap_42.3/repodata/repomd.xml.key
enabled=1
DATA
fi
%endif

%if 0%{?sle_version} == 120200
# openSUSE Leap 42.2
if [ -d "/etc/zypp/repos.d/" ]; then
ZYPP_FILE="/etc/zypp/repos.d/megasync.repo"
cat > "$ZYPP_FILE" << DATA
[MEGAsync]
name=MEGAsync
type=rpm-md
baseurl=https://mega.nz/linux/MEGAsync/openSUSE_Leap_42.2/
gpgcheck=1
autorefresh=1
gpgkey=https://mega.nz/linux/MEGAsync/openSUSE_Leap_42.2/repodata/repomd.xml.key
enabled=1
DATA
fi
%endif


%if 0%{?sle_version} == 120100
# openSUSE Leap 42.1
if [ -d "/etc/zypp/repos.d/" ]; then
ZYPP_FILE="/etc/zypp/repos.d/megasync.repo"
cat > "$ZYPP_FILE" << DATA
[MEGAsync]
name=MEGAsync
type=rpm-md
baseurl=https://mega.nz/linux/MEGAsync/openSUSE_Leap_42.1/
gpgcheck=1
autorefresh=1
gpgkey=https://mega.nz/linux/MEGAsync/openSUSE_Leap_42.1/repodata/repomd.xml.key
enabled=1
DATA
fi
%endif
 

%if 0%{?suse_version} > 1320
# openSUSE Tumbleweed (rolling release)
if [ -d "/etc/zypp/repos.d/" ]; then
ZYPP_FILE="/etc/zypp/repos.d/megasync.repo"
cat > "$ZYPP_FILE" << DATA
[MEGAsync]
name=MEGAsync
type=rpm-md
baseurl=https://mega.nz/linux/MEGAsync/openSUSE_Tumbleweed/
gpgcheck=1
autorefresh=1
gpgkey=https://mega.nz/linux/MEGAsync/openSUSE_Tumbleweed/repodata/repomd.xml.key
enabled=1
DATA
fi
%endif

%if 0%{?suse_version} == 1320
# openSUSE 13.2
if [ -d "/etc/zypp/repos.d/" ]; then
ZYPP_FILE="/etc/zypp/repos.d/megasync.repo"
cat > "$ZYPP_FILE" << DATA
[MEGAsync]
name=MEGAsync
type=rpm-md
baseurl=https://mega.nz/linux/MEGAsync/openSUSE_13.2/
gpgcheck=1
autorefresh=1
gpgkey=https://mega.nz/linux/MEGAsync/openSUSE_13.2/repodata/repomd.xml.key
enabled=1
DATA
fi
%endif

%if 0%{?suse_version} == 1310
# openSUSE 13.1
if [ -d "/etc/zypp/repos.d/" ]; then
ZYPP_FILE="/etc/zypp/repos.d/megasync.repo"
cat > "$ZYPP_FILE" << DATA
[MEGAsync]
name=MEGAsync
type=rpm-md
baseurl=https://mega.nz/linux/MEGAsync/openSUSE_13.1/
gpgcheck=1
autorefresh=1
gpgkey=https://mega.nz/linux/MEGAsync/openSUSE_13.1/repodata/repomd.xml.key
enabled=1
DATA
fi
%endif

%if 0%{?suse_version} == 1230
# openSUSE 12.3
if [ -d "/etc/zypp/repos.d/" ]; then
ZYPP_FILE="/etc/zypp/repos.d/megasync.repo"
cat > "$ZYPP_FILE" << DATA
[MEGAsync]
name=MEGAsync
type=rpm-md
baseurl=https://mega.nz/linux/MEGAsync/openSUSE_12.3/
gpgcheck=1
autorefresh=1
gpgkey=https://mega.nz/linux/MEGAsync/openSUSE_12.3/repodata/repomd.xml.key
enabled=1
DATA
fi
%endif

%if 0%{?suse_version} == 1220
# openSUSE 12.2
if [ -d "/etc/zypp/repos.d/" ]; then
ZYPP_FILE="/etc/zypp/repos.d/megasync.repo"
cat > "$ZYPP_FILE" << DATA
[MEGAsync]
name=MEGAsync
type=rpm-md
baseurl=https://mega.nz/linux/MEGAsync/openSUSE_12.2/
gpgcheck=1
autorefresh=1
gpgkey=https://mega.nz/linux/MEGAsync/openSUSE_12.2/repodata/repomd.xml.key
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

### END of POSTINST


%postun
killall mega-cmd 2> /dev/null || true
killall mega-cmd-server 2> /dev/null || true

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
%{_bindir}/mega-exec
%{_bindir}/mega-export
%{_bindir}/mega-find
%{_bindir}/mega-get
%{_bindir}/mega-help
%{_bindir}/mega-https
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
%{_bindir}/mega-thumbnail
%{_bindir}/mega-userattr
%{_bindir}/mega-users
%{_bindir}/mega-version
%{_bindir}/mega-whoami
%{_bindir}/mega-cmd
%{_bindir}/mega-cmd-server
%{_sysconfdir}/bash_completion.d/megacmd_completion.sh

%changelog
