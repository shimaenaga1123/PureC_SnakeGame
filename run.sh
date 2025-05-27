#!/bin/bash

# 뱀 게임 실행 스크립트

GAME_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$GAME_DIR/build"
EXECUTABLE="$BUILD_DIR/snake"

echo "🐍 크로스 플랫폼 뱀 게임 실행기 🐍"
echo "========================================"

# 게임이 빌드되어 있는지 확인
if [ ! -f "$EXECUTABLE" ]; then
    echo "게임을 찾을 수 없습니다. 게임 빌드 중..."
    cd "$GAME_DIR"
    ./build.sh

    if [ $? -ne 0 ]; then
        echo "빌드 실패. 빌드 로그를 확인해주세요."
        exit 1
    fi
fi

# 빌드 후 실행 파일 존재 여부 확인
if [ ! -f "$EXECUTABLE" ]; then
    echo "게임 실행 파일을 찾을 수 없습니다: $EXECUTABLE"
    echo "게임이 올바르게 빌드되었는지 확인해주세요."
    exit 1
fi

# 게임 실행
echo "뱀 게임 시작 중..."
cd "$GAME_DIR"
"$EXECUTABLE"

echo "플레이해주셔서 감사합니다!"
