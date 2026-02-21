Name:           izprime
Version:        1.0.0
Release:        1%{?dist}
Summary:        High-performance prime sieving library and CLI

License:        MIT
URL:            https://github.com/Zprime137/iZprime
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc
BuildRequires:  make
BuildRequires:  pkgconf-pkg-config
BuildRequires:  gmp-devel
BuildRequires:  openssl-devel

%description
izprime provides optimized prime sieving, range counting/streaming, and
prime-generation routines, plus a CLI interface.

%package -n libizprime
Summary: Runtime shared library for izprime

%description -n libizprime
Runtime shared library for izprime.

%package devel
Summary: Development files for izprime
Requires: libizprime = %{version}-%{release}

%description devel
Header files, static library, and pkg-config metadata for izprime.

%prep
%autosetup -n %{name}-%{version}

%build
make lib cli ARCH_NATIVE=0 TUNE_NATIVE=0

%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot} PREFIX=%{_prefix} ARCH_NATIVE=0 TUNE_NATIVE=0

%files
%license LICENSE
%doc README.md
%{_bindir}/izprime

%files -n libizprime
%{_libdir}/libizprime.so.*

%files devel
%{_includedir}/*.h
%{_libdir}/libizprime.a
%{_libdir}/libizprime.so
%{_libdir}/pkgconfig/izprime.pc

%changelog
* Sat Feb 21 2026 Hazem Mounir <hazemmounir@users.noreply.github.com> - 1.0.0-1
- Initial RPM packaging metadata.
