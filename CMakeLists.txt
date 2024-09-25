cmake_minimum_required(VERSION 3.16)
project(QPeltierUI)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(APP_ICON_RESOURCE_WINDOWS "QPeltierUI.rc")

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        SerialPort
        Charts
        REQUIRED)

if (MSVC)
    add_definitions("-D__WIN32__ -DUNICODE")
    add_definitions("-DSPDLOG_WCHAR_FILENAMES")
    set(WINAPP "WIN32")
elseif(UNIX)
    include_directories(".")
    set(WINAPP "")
endif()

if (CMAKE_BUILD_TYPE MATCHES "Debug")
    set(DEBUG_SUFFIX "d")
endif ()

set(QPELTIERUI_SRC
        main.cpp
        mainwindow.cpp
        mainwindow.ui
        serialportworker.cpp
        xyseriesiodevice.cpp
        )

set(APP_VERSION "1.0.0.0")

include_directories(qcustomplot)

add_executable(QPeltierUI ${WINAPP} ${QPELTIERUI_SRC} ${PLATFORM_SRC} ${APP_ICON_RESOURCE_WINDOWS})
target_compile_definitions(QPeltierUI PUBLIC "-D_APP_VERSION=\"${APP_VERSION}\"")
target_include_directories(QPeltierUI PUBLIC inc)
target_link_libraries(QPeltierUI PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::SerialPort
        Qt6::Charts
        )

if(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(QPeltierUI PRIVATE Threads::Threads)
endif()

# if (WIN32)
#     set(DEBUG_SUFFIX)
#     if (CMAKE_BUILD_TYPE MATCHES "Debug")
#         set(DEBUG_SUFFIX "d")
#     endif ()
#     set(QT_INSTALL_PATH "${CMAKE_PREFIX_PATH}")
#     if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
#         set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
#         if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
#             set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
#         endif ()
#     endif ()
#     if (EXISTS "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll")
#         add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#                 COMMAND ${CMAKE_COMMAND} -E make_directory
#                 "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
#         add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#                 COMMAND ${CMAKE_COMMAND} -E copy
#                 "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll"
#                 "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
#     endif ()
#     foreach (QT_LIB Core Gui Widgets SerialPort PrintSupport)
#         add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#                 COMMAND ${CMAKE_COMMAND} -E copy
#                 "${QT_INSTALL_PATH}/bin/Qt6${QT_LIB}${DEBUG_SUFFIX}.dll"
#                 "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
#     endforeach (QT_LIB)
# endif ()
