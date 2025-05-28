# 크로스 플랫폼 뱀 게임 CMake 설정 파일
cmake_minimum_required(VERSION 3.10)
project(CrossPlatformSnake VERSION 1.0 LANGUAGES C)

# C 표준 설정
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 플랫폼 감지 및 정의
if(WIN32)
    set(PLATFORM_NAME "windows")
    add_definitions(-DPLATFORM_WINDOWS)
    message(STATUS "Windows 플랫폼으로 빌드합니다")
elseif(APPLE)
    set(PLATFORM_NAME "macos")
    add_definitions(-DPLATFORM_MACOS)
    message(STATUS "macOS 플랫폼으로 빌드합니다")
elseif(EMSCRIPTEN)
    set(PLATFORM_NAME "web")
    add_definitions(-DPLATFORM_WEB)
    message(STATUS "WebAssembly 플랫폼으로 빌드합니다")
else()
    set(PLATFORM_NAME "unix")
    add_definitions(-DPLATFORM_UNIX)
    message(STATUS "Unix/Linux 플랫폼으로 빌드합니다")
endif()

# 컴파일러별 최적화 옵션
if(MSVC)
    # MSVC 컴파일러: 경고 레벨 4
    add_compile_options(/W4)
    # UTF-8 인코딩 설정 (한국어 주석 지원)
    add_compile_options(/utf-8)
    # 수학 라이브러리 상수 정의
    add_definitions(-D_USE_MATH_DEFINES)
else()
    # GCC/Clang: 모든 경고 활성화
    add_compile_options(-Wall -Wextra -pedantic)
    # 한국어 주석 지원을 위한 인코딩 설정
    add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
    # 수학 라이브러리 링크
endif()

# 인클루드 디렉터리 설정
include_directories(src)

# 플랫폼별 소스 파일 정의
set(PLATFORM_SOURCES
    src/platform/platform.h
)

# 플랫폼에 따른 구현 파일 선택
if(WIN32)
    list(APPEND PLATFORM_SOURCES src/platform/platform_windows.c)
elseif(EMSCRIPTEN)
    list(APPEND PLATFORM_SOURCES src/platform/platform_web.c)
else()
    list(APPEND PLATFORM_SOURCES src/platform/platform_unix.c)
endif()

# 게임 로직 소스 파일들
set(GAME_SOURCES
    src/game/game.h
    src/game/game.c
    src/game/ai.h
    src/game/ai.c
)

# UI 시스템 소스 파일들
set(UI_SOURCES
    src/ui/ui.h
    src/ui/ui.c
)

# 전체 소스 파일 목록
set(ALL_SOURCES
    src/main.c
    ${PLATFORM_SOURCES}
    ${GAME_SOURCES}
    ${UI_SOURCES}
)

# 실행 파일 생성
add_executable(snake ${ALL_SOURCES})

# 수학 라이브러리 링크 (sin, cos 등의 함수를 위해 필요)
if(NOT WIN32)
    target_link_libraries(snake m)
endif()

# 플랫폼별 라이브러리 링크
if(WIN32)
    # Windows: 네트워킹용 winsock, 오디오용 winmm
    target_link_libraries(snake ws2_32 winmm)
elseif(EMSCRIPTEN)
    # Emscripten 웹 빌드 설정
    set_target_properties(snake PROPERTIES
        SUFFIX ".html"
    )
    
    # 컴파일 옵션 추가
    target_compile_options(snake PRIVATE
        "-sASYNCIFY=1"
        "-finput-charset=UTF-8"
        "-fexec-charset=UTF-8"
    )
    
    # 링크 옵션 추가
    target_link_options(snake PRIVATE
        "-sASYNCIFY=1"
        "-sASYNCIFY_STACK_SIZE=32768"
        "--shell-file=${CMAKE_SOURCE_DIR}/web/shell.html"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sINITIAL_MEMORY=33554432"
        "-sSTACK_SIZE=2097152"
        "-sENVIRONMENT=web"
        "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','UTF8ToString']"
        "-sEXPORTED_FUNCTIONS=['_main']"
        "-sMODULARIZE=0"
        "-sINVOKE_RUN=1"
        "-sNO_EXIT_RUNTIME=1"
        "-sSAFE_HEAP=1"
        "-sASSERTIONS=1"
        "-sSTACK_OVERFLOW_CHECK=2"
        "-sABORTING_MALLOC=0"
        "-sUSE_CLOSURE_COMPILER=0"
        "-sWASM_BIGINT=0"
    )
elseif(UNIX AND NOT APPLE)
    # Linux: pthread 라이브러리와 동적 링크 라이브러리
    target_link_libraries(snake pthread dl)
elseif(APPLE)
    # macOS: pthread 라이브러리
    target_link_libraries(snake pthread)
    # macOS 프레임워크 링크 (오디오용)
    find_library(COREAUDIO_FRAMEWORK CoreAudio)
    find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation)
    if(COREAUDIO_FRAMEWORK AND AUDIOTOOLBOX_FRAMEWORK AND COREFOUNDATION_FRAMEWORK)
        target_link_libraries(snake 
            ${COREAUDIO_FRAMEWORK} 
            ${AUDIOTOOLBOX_FRAMEWORK}
            ${COREFOUNDATION_FRAMEWORK}
        )
    endif()
endif()

# 설치 설정
install(TARGETS snake DESTINATION bin)

# 설정 파일도 함께 설치
install(DIRECTORY config/ DESTINATION share/snake/config)

# 디버그 빌드 설정
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
    message(STATUS "디버그 모드로 빌드합니다")
    if(NOT MSVC)
        add_compile_options(-g -O0 -DDEBUG_MODE)
    endif()
endif()

# 릴리즈 빌드 설정
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_definitions(-DNDEBUG)
    message(STATUS "릴리즈 모드로 빌드합니다")
    if(NOT MSVC)
        add_compile_options(-O3 -DNDEBUG)
    endif()
endif()

# 빌드 정보 출력
message(STATUS "================================")
message(STATUS "크로스 플랫폼 뱀 게임 빌드 설정")
message(STATUS "================================")
message(STATUS "대상 플랫폼: ${PLATFORM_NAME}")
message(STATUS "빌드 타입: ${CMAKE_BUILD_TYPE}")
message(STATUS "C 컴파일러: ${CMAKE_C_COMPILER}")
message(STATUS "CMake 버전: ${CMAKE_VERSION}")
message(STATUS "포함된 기능:")
message(STATUS "  - 게임 엔진: ✓")
message(STATUS "  - AI 시스템: ✓")
message(STATUS "  - UI 시스템: ✓")
message(STATUS "  - 오디오 시스템: ✓")
message(STATUS "  - 크로스 플랫폼 지원: ✓")
message(STATUS "================================")

# 사용자 정의 빌드 타겟 추가
if(NOT EMSCRIPTEN)
    # 실행 타겟 (웹 빌드에서는 제외)
    add_custom_target(run
        COMMAND ${CMAKE_BINARY_DIR}/snake
        DEPENDS snake
        COMMENT "게임을 실행합니다..."
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    
    # 디버그 실행 타겟
    add_custom_target(debug
        COMMAND gdb ${CMAKE_BINARY_DIR}/snake
        DEPENDS snake
        COMMENT "디버거로 게임을 실행합니다..."
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif()

# 정리 타겟
add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/CMakeFiles
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/CMakeCache.txt
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/cmake_install.cmake
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/Makefile
    COMMENT "모든 빌드 파일을 정리합니다..."
)

# 패키징 지원
set(CPACK_PACKAGE_NAME "CrossPlatformSnake")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "한국어 지원 크로스 플랫폼 뱀 게임")
set(CPACK_PACKAGE_VENDOR "OpenSource")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
include(CPack)