#!/bin/bash

# 크로스 플랫폼 뱀 게임 빌드 스크립트

set -e

# 출력용 색상
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # 색상 없음

echo -e "${GREEN}크로스 플랫폼 뱀 게임 빌드 스크립트${NC}"
echo "========================================"

# 상태 출력 함수
print_status() {
    echo -e "${YELLOW}[정보]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[성공]${NC} $1"
}

print_error() {
    echo -e "${RED}[오류]${NC} $1"
}

# CMake 설치 여부 확인
if ! command -v cmake &> /dev/null; then
    print_error "CMake가 설치되어 있지 않습니다. 먼저 CMake를 설치해주세요."
    exit 1
fi

# 기본 빌드 타입
BUILD_TYPE="Release"
BUILD_DIR="build"
PLATFORM=""

# 명령줄 인수 파싱
while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -r|--release)
            BUILD_TYPE="Release"
            shift
            ;;
        -c|--clean)
            print_status "빌드 디렉토리 정리 중..."
            rm -rf ${BUILD_DIR}
            shift
            ;;
        -w|--web)
            PLATFORM="web"
            BUILD_DIR="build-web"
            shift
            ;;
        -h|--help)
            echo "사용법: $0 [옵션]"
            echo "옵션:"
            echo "  -d, --debug     디버그 모드로 빌드"
            echo "  -r, --release   릴리즈 모드로 빌드 (기본값)"
            echo "  -c, --clean     빌드 전 빌드 디렉토리 정리"
            echo "  -w, --web       WebAssembly용 빌드"
            echo "  -h, --help      이 도움말 메시지 표시"
            exit 0
            ;;
        *)
            print_error "알 수 없는 옵션: $1"
            exit 1
            ;;
    esac
done

print_status "빌드 타입: ${BUILD_TYPE}"
print_status "빌드 디렉토리: ${BUILD_DIR}"

# 빌드 디렉토리 생성
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

if [[ "${PLATFORM}" == "web" ]]; then
    # WebAssembly 빌드
    print_status "WebAssembly용 빌드 중..."

    if ! command -v emcc &> /dev/null; then
        print_error "Emscripten이 설치되어 있지 않거나 PATH에 없습니다."
        print_error "Emscripten을 설치하고 emsdk_env.sh를 소스해주세요"
        exit 1
    fi

    print_status "Emscripten 버전: $(emcc --version | head -n1)"
    print_status "ASYNCIFY 및 pthread 지원으로 빌드합니다..."

    emcmake cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} ..
    
    print_status "빌드 진행 중... (시간이 오래 걸릴 수 있습니다)"
    emmake make -j$(nproc) VERBOSE=1

    print_success "WebAssembly 빌드 완료!"
    print_status "${BUILD_DIR}/snake.html을 웹 브라우저에서 열어 플레이하세요"
    
else
    # 네이티브 빌드
    print_status "네이티브 플랫폼용 빌드 중..."

    cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} ..

    # 적절한 작업 수로 빌드
    if command -v nproc &> /dev/null; then
        JOBS=$(nproc)
    else
        JOBS=4
    fi

    make -j${JOBS}

    print_success "네이티브 빌드 완료!"
    print_status "./${BUILD_DIR}/snake를 실행하여 플레이하세요"
fi

cd ..

print_success "빌드 스크립트가 성공적으로 완료되었습니다!"
