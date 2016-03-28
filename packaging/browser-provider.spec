
Name:       browser-provider
Summary:    sync in background.
Version:    1.8.0
Release:    1
Group:      Development/Libraries
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Requires(post): /usr/bin/sqlite3
#Requires(post): sys-assert
#Requires(post): libdevice-node
BuildRequires:  cmake
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(sqlite3)
#BuildRequires:  pkgconfig(glib-2.0)
#BuildRequires:  pkgconfig(gobject-2.0)
#BuildRequires:  pkgconfig(ecore)
#BuildRequires:  pkgconfig(edbus)
BuildRequires:  pkgconfig(libsystemd-daemon)
BuildRequires:  pkgconfig(tapi)
BuildRequires:  pkgconfig(libpng)
BuildRequires:  pkgconfig(libsmack)
BuildRequires:  pkgconfig(cynara-client)
BuildRequires:  pkgconfig(cynara-creds-socket)
BuildRequires:  pkgconfig(cynara-session)
BuildRequires:  pkgconfig(openssl)
BuildRequires:  pkgconfig(libtzplatform-config)

%define _data_install_path %{TZ_SYS_DATA}/%{name}

%define _databasedir %{_data_install_path}/database
%define _notifydir %{_data_install_path}/notify
%define _ipc_socket /tmp/.browser-provider.sock
%define _license_path /usr/share/license
%define _cloud_pdm_server /usr/bin/cloud-pdm-server

%description
Description: sync in background

%package devel
Summary:    %{name}
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
Description: sync in background (developement files)

%prep
%setup -q

%define cmake \
	CFLAGS="${CFLAGS:-%optflags} -fPIC -D_REENTRANT -fvisibility=hidden"; export CFLAGS \
	FFLAGS="${FFLAGS:-%optflags} -fPIC -fvisibility=hidden"; export FFLAGS \
	LDFLAGS+=" -Wl,--as-needed -Wl,--hash-style=both"; export LDFLAGS \
	%__cmake \\\
		-DCMAKE_INSTALL_PREFIX:PATH=%{_prefix} \\\
		-DBIN_INSTALL_DIR:PATH=%{_bindir} \\\
		-DLIB_INSTALL_DIR:PATH=%{_libdir} \\\
		-DINCLUDE_INSTALL_DIR:PATH=%{_includedir} \\\
		-DPKG_NAME=%{name} \\\
		-DPKG_VERSION=%{version} \\\
		-DPKG_RELEASE=%{release} \\\
		-DPKG_LICENSE_PATH:PATH=%{_license_path} \\\
		-DCLOUD_PDM_SERVER:PATH=%{_cloud_pdm_server} \\\
		-DPROVIDER_DIR:PATH=%{_data_install_path} \\\
		-DDATABASE_DIR:PATH=%{_databasedir} \\\
		-DNOTIFY_DIR:PATH=%{_notifydir} \\\
		-DIPC_SOCKET:PATH=%{_ipc_socket} \\\
		-DSUPPORT_CLOUD_SYSTEM:BOOL=ON \\\
		-DSUPPORT_BOOTING_DONE:BOOL=OFF \\\
		-DSUPPORT_LOG_MESSAGE:BOOL=ON \\\
		-DSUPPORT_FILE_LOGGING:BOOL=OFF \\\
		-DSUPPORT_SERVER_PRIVILEGE:BOOL=ON \\\
		%if "%{?_lib}" == "lib64" \
		%{?_cmake_lib_suffix64} \\\
		%endif \
		%{?_cmake_skip_rpath} \\\
		-DBUILD_SHARED_LIBS:BOOL=ON

%build
%if 0%{?sec_build_binary_debug_enable}
export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"
%endif

%cmake .
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

%install_service multi-user.target.wants browser-provider.service
%install_service sockets.target.wants browser-provider.socket

%post
/sbin/ldconfig
mkdir -p %{_databasedir}
mkdir -p %{_notifydir}
chsmack -a 'System::Shared' %{_notifydir}
chsmack -t %{_notifydir}

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_libdir}/libcapi-web-bookmark-csc.so.%{version}
%{_libdir}/libcapi-web-bookmark-csc.so.0
%{_libdir}/libcapi-web-bookmark.so.%{version}
%{_libdir}/libcapi-web-bookmark.so.0
%{_libdir}/libcapi-web-tab.so.%{version}
%{_libdir}/libcapi-web-tab.so.0
%{_libdir}/libcapi-web-history.so.%{version}
%{_libdir}/libcapi-web-history.so.0
%{_license_path}/%{name}
%manifest %{name}.manifest
%{_unitdir}/browser-provider.service
%{_unitdir}/multi-user.target.wants/browser-provider.service
%{_unitdir}/browser-provider.socket
%{_unitdir}/sockets.target.wants/browser-provider.socket

%files devel
%defattr(-,root,root,-)
%{_includedir}/web/web_bookmark_csc.h
#%{_includedir}/web/web_bookmark_csc_doc.h
%{_includedir}/web/web_bookmark.h
%{_includedir}/web/web_tab.h
#%{_includedir}/web/web_tab_doc.h
%{_includedir}/web/web_history.h
#%{_includedir}/web/web_history_doc.h
%{_libdir}/pkgconfig/capi-web-bookmark-csc.pc
%{_libdir}/pkgconfig/capi-web-bookmark.pc
%{_libdir}/pkgconfig/capi-web-tab.pc
%{_libdir}/pkgconfig/capi-web-history.pc
%{_libdir}/libcapi-web-bookmark-csc.so
%{_libdir}/libcapi-web-bookmark.so
%{_libdir}/libcapi-web-tab.so
%{_libdir}/libcapi-web-history.so
#deprecated below
%{_includedir}/web/bookmark-csc-adaptor.h
%{_includedir}/web/bookmark-adaptor.h
%{_includedir}/web/tab-adaptor.h
%{_includedir}/web/history-adaptor.h
%{_libdir}/pkgconfig/bookmark-csc-adaptor.pc
%{_libdir}/pkgconfig/bookmark-adaptor.pc
%{_libdir}/pkgconfig/tab-adaptor.pc
%{_libdir}/pkgconfig/history-adaptor.pc

%changelog
* Tue Jun 11 2013 Kwangmin Bang <justine.bang@samsung.com>
- enable systemd and prevent to be launched repeatedly
- divide commands by permission for speed up
- apply secure log for pkgname
- prevent the lockup of client when provider deny call without closing socket
- move sql installing to post section

* Tue Jun 4 2013 Kwangmin Bang <justine.bang@samsung.com>
- search keyword in url except www. prefix

* Wed May 29 2013 Kwangmin Bang <justine.bang@samsung.com>
- migration systemd boot script

* Tue May 28 2013 Kwangmin Bang <justine.bang@samsung.com>
- return wrong error-code to client

* Thu May 23 2013 Kwangmin Bang <justine.bang@samsung.com>
- security server privilege
- the limitation of string length 2k -> 8k
- [P130520-4283] browser-provider has closed unexpectedly

* Wed May 15 2013 Kwangmin Bang <justine.bang@samsung.com>
- enhance the speed of query for BLOB
- exclude http protocol string in predictive searching

* Thu May 09 2013 Kwangmin Bang <justine.bang@samsung.com>
- finalize prepared stmt for creating a tab

* Wed May 08 2013 Kwangmin Bang <justine.bang@samsung.com>
- enhance the speed of tab create API

* Mon Apr 29 2013 Kwangmin Bang <justine.bang@samsung.com>
- get id list created by user
- apply sql DATETIME for date_modified and date_visited column

* Tue Apr 23 2013 Kwangmin Bang <justine.bang@samsung.com>
- search csc bookmark with operator condition
- apply sql DATETIME function to date_created
- remove warning message in build time

* Fri Apr 19 2013 Kwangmin Bang <justine.bang@samsung.com>
- New API for bookmark can search keyword in title or url
- apply SECURE_LOG

* Wed Apr 10 2013 Kwangmin Bang <justine.bang@samsung.com>
- missing library linking
- PRAGMA synchronous=FULL
- recognize to a adaptor if failed to read cmdline

* Mon Apr 8 2013 Kwangmin Bang <justine.bang@samsung.com>
- prevent defect : remove sample code

* Fri Apr 05 2013 Kwangmin Bang <justine.bang@samsung.com>
- add the macro for SECLOG
- guarantree the operation although provider was terminated
- recognize sync-adaptor exactly by reading cmdline in /proc
- crash if not installed app call any API
- No matter that childs, bp_bookmark_adaptor_remove can delete a folder

* Wed Mar 27 2013 Kwangmin Bang <justine.bang@samsung.com>
- prevent defects
- add checking the params for csc-bookmark API
- get the value which bookmark entry structure can store
- smack for dbus service

* Fri Mar 22 2013 Kwangmin Bang <justine.bang@samsung.com>
- does not update dirty if csc-adaptor call easy_set api

* Thu Mar 21 2013 Kwangmin Bang <justine.bang@samsung.com>
- prevent defects
- thumbnail and favicon

* Wed Mar 20 2013 Kwangmin Bang <justine.bang@samsung.com>
- fix the bug failed to get bookmark info by mask offset API
- security coding : check all of parameters
- security & privacy : remove private info from log message

* Mon Mar 18 2013 Kwangmin Bang <justine.bang@samsung.com>
- just one sql call including int, text and blob
- fix lockup if set the bit mask for TAG
- new API : can choose the values want to get from a bookmar
- support thumbnail by easy get API
- negative cid will be regarded as peer crashin
- change the type of variable for mask offset
- smack bookmark-csc-adaptor
- Disconnect bad client by checking cid
- return IO_ERROR if failed to send the error code
- new API : can choose the values want to get from a tab info

* Fri Mar 15 2013 Kwangmin Bang <justine.bang@samsung.com>
- change prefix of bookmark APIs
- enhance the speed to get a rows from DB
- default editable is true
- set is_deleted flag by bookmark_reset API

* Thu Mar 14 2013 Kwangmin Bang <justine.bang@samsung.com>
- new APIs for bookmark CSC module
- replace all APIs of libbookmark-service
- support thumbnail for bookmark
- setting the property of sqlite3

* Fri Mar 8 2013 Kwangmin Bang <justine.bang@samsung.com>
- fix incomplete sending noti event
- new tab api : bp_tab_adaptor_activate
- new tab api : bp_tab_adaptor_get_sequence_ids_p
- divide sync-adaptor and other client
- use blocking socket for writing

* Tue Mar 5 2013 Kwangmin Bang <justine.bang@samsung.com>
- add Requires(post): /usr/bin/sqlite3

* Mon Feb 25 2013 Kwangmin Bang <justine.bang@samsung.com>
- [prevent defect] Untrusted value as argument (TAINTED_SCALAR)
- ignore SIGPIPE
- check System Signal in all read call

* Fri Feb 22 2013 Kwangmin Bang <justine.bang@samsung.com>
- new APIs for scraps

* Wed Feb 20 2013 Kwangmin Bang <justine.bang@samsung.com>
- support adding max sequence value(lastindex) for bookmark
- exclude a property if set negative
- deleting a item instead of setting deleted flag in no cloud system
- move the define for DBUS used in common to main CMakeList.txt

* Tue Feb 19 2013 Kwangmin Bang <justine.bang@samsung.com>
- check the length of string before sending it
- prevent defects
- New interface for 1 text(mandatory) + 2 integer(optional) conditions
- DB API can support multiple conditions
- support LIKE condition for string query
- add duplicated functions in adaptor code as common API

* Fri Feb 15 2013 Kwangmin Bang <justine.bang@samsung.com>
- match the filename with dbus service name
- const keyword for the parameter of set API
- fix wrong length of favicon
- [smack] add default rules into manifest and remove rule file
- allow id is zero

* Wed Feb 14 2013 Kwangmin Bang <justine.bang@samsung.com>
- [prevent defects]
- remove unnecessary mutex
- API getting ids after ordering, rows limitation with 2 conditions for bookmark
- ordering ids
- APIs for backup and restore bookmarks
- divide database file for each module (bookmark, scrap and tab)
- support 64bit architecture using the macro in spec file
- add easy_free API which can free the structure allocated by adaptor_get_easy_all
- set compile option in spec file
- sample for scrap APIs
- support APIs for expandable tag
- commonize the declaration of column for TAGs
- fix wrong identification of scrap adaptor
- add bookmark_get_tag_ids

* Tue Feb 05 2013 Kwangmin Bang <justine.bang@samsung.com>
- [SMACK] add rule, add scrap library to manifest
- compile option fvisibility=hidden
- [bookmarks] use switch instead of if-else
- [scraps] use switch instead of if-else
- remove the code called twice and unused parameter
- support expandable tag list for bookmark
- fix the lockup when client send unsupport command
- functionize duplicated codes for binding value to sqlite3
- fix wrong error handling
- use switch instead of if-else
- declare global variable as static
- remove unnecessary log message
- create id using time() in sample
- include samples(bookmark,tab) to build script
- fix wrong overwriting id in set_dirty()

* Wed Jan 30 2013 Bang Kwangmin <justine.bang@samsung.com>
- get ids API return success with zero count in no rows
- support the requests for scrap
- change function name as common name
- impletement the library for scrap
- concentrate the header files for adaptors at one directory
- smack labeling for libraries
- the schema of DB table for scraps
- [prevent defect] leak handle
- change the role of bp_XX_delete() to delete a row really for sync-adaptor
- cmake follow the variable declared in spec file
- remove post
- apply soversion

* Fri Jan 25 2013 Bang Kwangmin <justine.bang@samsung.com>
- support Bookmark APIs

* Tue Jan 22 2013 Bang Kwangmin <justine.bang@samsung.com>
- completed all APIs
- enhance the speed which connecting to provider
- APIs for Tab sync and browser-provider

* Thu Jan 17 2013 Bang Kwangmin <justine.bang@samsung.com>
- first release

* Tue Jan 08 2013 Bang Kwangmin <justine.bang@samsung.com>
- default files for packaging
