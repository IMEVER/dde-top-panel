cmake_minimum_required(VERSION 3.7)

set(BIN_NAME dde-top-panel)
set(CMAKE_INSTALL_PREFIX /opt/apps/me.imever.dde-top-panel)

# 启用 qt moc 的支持
set(CMAKE_AUTOMOC ON)
# 启用 qrc 资源文件的支持
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Sources files
file(GLOB_RECURSE SRCS "*.h" "*.cpp" "../widgets/*.h" "../widgets/*.cpp")

# Find the library
find_package(PkgConfig REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Concurrent REQUIRED)
find_package(Qt5X11Extras REQUIRED)
find_package(Qt5DBus REQUIRED)
find_package(Qt5Svg REQUIRED)
find_package(DtkWidget REQUIRED)
find_package(DtkCMake REQUIRED)
find_package(KF5WindowSystem REQUIRED)

qt5_add_dbus_adaptor(SRCS dbus/me.imever.dde.toppanel.xml
    window/MainWindow.h MainWindow dbustoppaneladaptors DBusTopPanelAdaptors)

# set_source_files_properties(dbus/org.kde.StatusNotifierWatcher.xml CLASSNAME StatusNotifierWatcherInterface)
# set_source_file_property(dbus/sni/org.kde.StatusNotifierWatcher.xml )
qt5_add_dbus_interface(SRCS dbus/org.kde.StatusNotifierWatcher.xml statusnotifierwatcher_interface)

# qt5_add_dbus_interface(SRCS dbus/com.deepin.daemon.Display.xml dbusdisplay_interface)

pkg_check_modules(XCB_EWMH REQUIRED xcb-ewmh x11)
pkg_check_modules(DFrameworkDBus REQUIRED dframeworkdbus)
pkg_check_modules(QGSettings REQUIRED gsettings-qt)
pkg_check_modules(DtkGUI REQUIRED dtkgui)

link_directories(../appmenu)
link_libraries(appmenu)

link_directories(../globalmenu)
link_libraries(globalmenu)

# driver-manager
add_executable(${BIN_NAME} ${SRCS} ${INTERFACES} frame.qrc)
target_include_directories(${BIN_NAME} PUBLIC ${DtkWidget_INCLUDE_DIRS}
        ${XCB_EWMH_INCLUDE_DIRS}
        ${DFrameworkDBus_INCLUDE_DIRS}
        ${Qt5Gui_PRIVATE_INCLUDE_DIRS}
        ${Qt5Widgets_PRIVATE_INCLUDE_DIRS}
        ${PROJECT_BINARY_DIR}
        ${QGSettings_INCLUDE_DIRS}
        ${DtkGUI_INCLUDE_DIRS}
        ${KF5WindowSystem_INCLUDE_DIRS}
        ../interfaces
        )

target_link_libraries(${BIN_NAME} PRIVATE
        ${XCB_EWMH_LIBRARIES}
        ${DFrameworkDBus_LIBRARIES}
        ${DtkWidget_LIBRARIES}
        ${Qt5Widgets_LIBRARIES}
        ${Qt5Concurrent_LIBRARIES}
        ${Qt5X11Extras_LIBRARIES}
        ${Qt5DBus_LIBRARIES}
        ${QGSettings_LIBRARIES}
        ${DtkGUI_LIBRARIES}
        ${Qt5Svg_LIBRARIES}
        KF5::WindowSystem
        xdo
        X11
        )

if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "sw_64")
    target_compile_definitions(${BIN_NAME} PUBLIC DISABLE_SHOW_ANIMATION)
endif()

if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "mips64")
    target_compile_definitions(${BIN_NAME} PUBLIC DISABLE_SHOW_ANIMATION)
endif()

if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")
    target_compile_definitions(${BIN_NAME} PUBLIC DISABLE_SHOW_ANIMATION)
endif()

install(TARGETS ${BIN_NAME}
	RUNTIME DESTINATION files/bin/
	)
