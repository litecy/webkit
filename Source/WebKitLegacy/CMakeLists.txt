cmake_minimum_required(VERSION 3.10)
include(WebKitCommon)
include(target/TargetWTF)
include(target/TargetJavaScriptCore)
set_property(DIRECTORY . PROPERTY FOLDER "WebKitLegacy")

set(WebKitLegacy_SOURCES
    Storage/InprocessIDBServer.cpp
    Storage/StorageAreaImpl.cpp
    Storage/StorageAreaSync.cpp
    Storage/StorageNamespaceImpl.cpp
    Storage/StorageSyncManager.cpp
    Storage/StorageThread.cpp
    Storage/StorageTracker.cpp
    Storage/WebDatabaseProvider.cpp
    Storage/WebStorageNamespaceProvider.cpp

    WebCoreSupport/NetworkStorageSessionMap.cpp
    WebCoreSupport/WebResourceLoadScheduler.cpp
    WebCoreSupport/WebViewGroup.cpp
)

set(WebKitLegacy_PRIVATE_INCLUDE_DIRECTORIES
    "${CMAKE_BINARY_DIR}"
    "${WEBKITLEGACY_DIR}/Storage"
    "${WEBKITLEGACY_DIR}/WebCoreSupport"
    "${WebCore_PRIVATE_FRAMEWORK_HEADERS_DIR}"
    "${WebKitLegacy_DERIVED_SOURCES_DIR}"
)

set(WebKitLegacy_PRIVATE_LIBRARIES
    WebKit::JavaScriptCore
    WebCore${DEBUG_SUFFIX}
    PAL${DEBUG_SUFFIX}
)

WEBKIT_FRAMEWORK_DECLARE(WebKitLegacy)
WEBKIT_INCLUDE_CONFIG_FILES_IF_EXISTS()

if (MSVC)
    WEBKIT_ADD_PRECOMPILED_HEADER("WebKitPrefix.h" "win/WebKitPrefix.cpp" WebKitLegacy_SOURCES)
    list(APPEND WebKitLegacy_SOURCES win/WebKit.resources/WebKit.rc)
    add_definitions(/DWEBKIT_EXPORTS /DFRAMEWORK_NAME=WebKitLegacy)
endif ()

WEBKIT_FRAMEWORK(WebKitLegacy)

if (NOT INTERNAL_BUILD)
    add_dependencies(WebKitLegacy WebCore WebCorePrivateFrameworkHeaders)
endif ()
if (TARGET WebKitLegacyGUID)
    add_dependencies(WebKitLegacy WebKitLegacyGUID)
endif ()

if (APPLE)
    set_target_properties(WebKitLegacy PROPERTIES LINK_FLAGS "-umbrella WebKit")
endif ()

if (MSVC)
    add_custom_command(
        TARGET WebKitLegacy
        POST_BUILD
        COMMAND ${PERL_EXECUTABLE} ${WEBKIT_LIBRARIES_DIR}/tools/scripts/version-stamp.pl ${WebKitLegacy_DERIVED_SOURCES_DIR} $<TARGET_FILE:WebKitLegacy>
        VERBATIM)

    if (USE_CA)
        add_subdirectory(win/WebKitQuartzCoreAdditions)
    endif ()
endif ()
