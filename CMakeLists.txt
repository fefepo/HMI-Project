cmake_minimum_required(VERSION 3.19)
project(RealtimeHMI LANGUAGES CXX)

# 자동 설정 활성화 (UI 파일 및 시그널/슬롯 컴파일에 필수)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt 6 구성 요소 찾기 (버전 제약을 제거하여 호환성 높임)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts Network)

qt_standard_project_setup()

# 실행 파일 생성 및 소스 파일 등록
qt_add_executable(RealtimeHMI
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    server.cpp
    server.h
    client.cpp
    client.h
)

# 라이브러리 연결 (Qt6::Charts 가 핵심)
target_link_libraries(RealtimeHMI
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Charts
        Qt6::Network
)

# 설치 및 배포 설정 (기본값 유지)
install(TARGETS RealtimeHMI
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET RealtimeHMI
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)

install(SCRIPT ${deploy_script})