cmake_minimum_required(VERSION 3.19)
project(noren LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(noren
    WIN32 MACOSX_BUNDLE
    main.cpp
    norentray.cpp
    norentray.h
    dashboard.h dashboard.cpp
)

target_link_libraries(noren
    PRIVATE
        Qt::Core
        Qt::Widgets
)

install(TARGETS noren
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET noren
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
