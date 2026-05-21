# Vendored from plasma-workspace/libdbusmenuqt (GPL-2.0+)

set(libdbusmenuqt_SRCS
    dbusmenuimporter.cpp
    dbusmenushortcut_p.cpp
    dbusmenutypes_p.cpp
    utils.cpp
    debug.cpp
)

set_source_files_properties(com.canonical.dbusmenu.xml PROPERTIES
    NO_NAMESPACE TRUE
    INCLUDE "dbusmenutypes_p.h"
    CLASSNAME DBusMenuInterface
)
qt_add_dbus_interface(libdbusmenuqt_SRCS com.canonical.dbusmenu.xml dbusmenu_interface)

add_library(dbusmenuqt STATIC ${libdbusmenuqt_SRCS})
target_include_directories(dbusmenuqt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(dbusmenuqt PUBLIC Qt::DBus Qt::Widgets)
set_property(TARGET dbusmenuqt PROPERTY POSITION_INDEPENDENT_CODE ON)
