#sbs-git:slp/pkgs/s/remote-key-framework remote-key-framework 0.2.5 f585f766aa864c3857e93c776846771899a4fa41
Name:       remote-key-framework
Summary:    Remote key framework
Version: 0.2.31
Release:    1
Group:      Framework/system
License:    Apache License, Version 2.0
Source0:    %{name}-%{version}.tar.gz
Source1:    remote-key-framework.service

BuildRequires:  cmake
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(capi-network-bluetooth)
BuildRequires:  pkgconfig(capi-system-info)
BuildRequires:  pkgconfig(dlog)

%description
Remote key framework

%prep
%setup -q

%build
cmake . -DCMAKE_INSTALL_PREFIX=/usr -DPLATFORM_ARCH=%{_archtype}

make %{?jobs:-j%jobs}

%install
%make_install

mkdir -p %{buildroot}/usr/share/license
cp LICENSE %{buildroot}/usr/share/license/%{name}

mkdir -p %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants
install -m 0644 %SOURCE1 %{buildroot}%{_libdir}/systemd/system/
ln -s ../remote-key-framework.service %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants/remote-key-framework.service

%post 

%files
%manifest remote-key-framework.manifest
%attr(0755,root,root) %{_sysconfdir}/rc.d/init.d/rkfsvc
%{_bindir}/rkf_server
%{_libdir}/systemd/system/remote-key-framework.service
%{_libdir}/systemd/system/multi-user.target.wants/remote-key-framework.service
/usr/share/license/%{name}

