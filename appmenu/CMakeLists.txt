cmake_minimum_required(VERSION 3.13)

set(PROJECT appmenu)

project(${PROJECT})

remove_definitions(-DQT_NO_CAST_FROM_ASCII -DQT_STRICT_ITERATORS -DQT_NO_CAST_FROM_BYTEARRAY -DQT_NO_KEYWORDS)

set(CMAKE_CXX_STANDARD 14)

# 启用 qt moc 的支持
set(CMAKE_AUTOMOC ON)
# 启用 qrc 资源文件的支持
set(CMAKE_AUTORCC ON)

set(CMAKE_AUTOUIC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5Widgets REQUIRED)
find_package(Qt5DBus REQUIRED)
find_package(Qt5X11Extras REQUIRED)
# find_package(dbusmenu-qt5 REQUIRED)
find_package(KF5WindowSystem REQUIRED)

set(SRCS
        dbusmenutypes_p.h dbusmenutypes_p.cpp
        dbusmenushortcut_p.h dbusmenushortcut_p.cpp
        appmenumodel.h appmenumodel.cpp
        dbusmenuimporter.h dbusmenuimporter.cpp
        dbusmenu_interface.h dbusmenu_interface.cpp
        menuimporter.h menuimporter.cpp
        utils_p.h utils.cpp
        dbus_registrar.h dbus_registrar.cpp
        # registrar_proxy.h registrar_proxy.cpp
        )

qt5_add_dbus_adaptor(SRCS com.canonical.AppMenu.Registrar.xml
    menuimporter.h MenuImporter menuimporteradaptor MenuImporterAdaptor)


# add_library(globalmenu STATIC IMPORTED)
# set_target_properties(globalmenu
#   PROPERTIES IMPORTED_LOCATION
#   ${CMAKE_CURRENT_SOURCE_DIR}/libglobalmenu.a)

add_library(${PROJECT} ${SRCS})

target_include_directories(${PROJECT} PUBLIC
        ${Qt5Widgets_INCLUDE_DIRS}
        ${Qt5DBus_INCLUDE_DIRS}
        ${Qt5X11Extras_INCLUDE_DIRS}
        # ${DFrameworkDBus_INCLUDE_DIRS}
        # ${dbusmenu-qt5_INCLUDE_DIRS}
        )

target_link_libraries(${PROJECT}
        Qt5::Widgets
        Qt5::DBus
        Qt5::X11Extras
        KF5::WindowSystem
        xcb
        # ${Qt5DBus_LIBRARIES}
        # ${DFrameworkDBus_LIBRARIES}
        # globalmenu
        # dbusmenu-qt5
        )