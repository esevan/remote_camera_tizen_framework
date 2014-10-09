## Basic Descriptions of this package
Name:       remote-key-framework
Summary:    Remote key framework
Version:		1.2
Release:    1
Group:      Framework/system
License:    Apache License, Version 2.0
Source0:    %{name}-%{version}.tar.gz
Source1:    remote-key-framework.service

# Required packages
# Pkgconfig tool helps to find libraries that have already been installed
BuildRequires:  cmake
BuildRequires:  libattr-devel
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(capi-network-bluetooth)
BuildRequires:  pkgconfig(capi-system-info)
BuildRequires:  pkgconfig(dlog)

## Description string that this package's human users can understand
%description
Remote key framework


## Preprocess script
%prep
# setup: to unpack the original sources / -q: quiet
# patch: to apply patches to the original sources
%setup -q

## Build script
%build
# 'cmake' does all of setting the Makefile, then 'make' builds the binary.
cmake . -DCMAKE_INSTALL_PREFIX=/usr
make %{?jobs:-j%jobs}

## Install script
%install
# make_install: equivalent to... make install DESTDIR="%(?buildroot)"
%make_install

# install license file
mkdir -p %{buildroot}/usr/share/license
cp LICENSE %{buildroot}/usr/share/license/%{name}

# install systemd service
mkdir -p %{buildroot}%{_libdir}/systemd/system/graphical.target.wants
install -m 0644 %SOURCE1 %{buildroot}%{_libdir}/systemd/system/
ln -s ../remote-key-framework.service %{buildroot}%{_libdir}/systemd/system/graphical.target.wants/remote-key-framework.service

## Postprocess script
%post 

## Binary Package: File list
%files
%manifest remote-key-framework.manifest
%{_bindir}/rkf_server
%{_libdir}/systemd/system/remote-key-framework.service
%{_libdir}/systemd/system/graphical.target.wants/remote-key-framework.service
/usr/share/license/%{name}
