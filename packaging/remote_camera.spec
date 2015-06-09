## Basic Descriptions of this package
Name:       remote_camera
Summary:    Receiveing from android and sending to the camera application the frame buffer
Version:		1.2
Release:    1
Group:      Framework/multimedia
License:    Apache License, Version 2.0
Source0:    %{name}-%{version}.tar.gz
Source1:    remote_camera.service

# Required packages
# Pkgconfig tool helps to find libraries that have already been installed
BuildRequires:  cmake
BuildRequires:  libattr-devel
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(capi-system-info)
BuildRequires:  pkgconfig(dlog)
BuildRequires:	pkgconfig(dbus-1)
BuildRequires:	pkgconfig(dbus-glib-1)

## Description string that this package's human users can understand
%description
Tizen remote camera 


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
ln -s ../remote_camera.service %{buildroot}%{_libdir}/systemd/system/graphical.target.wants/remote_camera.service

## Postprocess script
%post 

## Binary Package: File list
%files
%manifest remote_camera.manifest
%{_bindir}/remote_camera
%{_libdir}/systemd/system/remote_camera.service
%{_libdir}/systemd/system/graphical.target.wants/remote_camera.service
/usr/share/license/%{name}
