cmake_minimum_required(VERSION 3.19)
project(noren LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Sql Network)

qt_standard_project_setup()

qt_add_executable(noren
    WIN32 MACOSX_BUNDLE
    main.cpp
    database.h database.cpp
    eventtracker.h eventtracker.cpp
    noren.cpp
    noren.h
    dashboard.h dashboard.cpp
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/noren.jpg")
    qt_add_resources(noren "app_resources"
        PREFIX "/"
        FILES noren.jpg
    )
endif()

target_link_libraries(noren
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Sql
        Qt::Network
)

if(WIN32)
    target_link_libraries(noren PRIVATE User32)
endif()

qt_add_executable(noren-cli
    cli_main.cpp
    database.h database.cpp
)

set_target_properties(noren-cli PROPERTIES
    WIN32_EXECUTABLE FALSE
)

if(MINGW)
    target_link_options(noren-cli PRIVATE -mconsole)
elseif(MSVC)
    target_link_options(noren-cli PRIVATE /SUBSYSTEM:CONSOLE)
endif()

target_link_libraries(noren-cli
    PRIVATE
        Qt::Core
        Qt::Sql
        Qt::Network
)

install(TARGETS noren noren-cli
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
