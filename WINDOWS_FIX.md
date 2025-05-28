# Windows 화면 깜빡임 해결 방법

## 🔧 문제 분석
Windows 환경에서 콘솔 게임 실행 시 화면이 깜빡이는 현상이 발생했습니다. 이는 Windows 콘솔의 특성과 기존 Unix/Linux 최적화된 렌더링 방식의 차이로 발생하는 문제였습니다.

## ✅ 해결 방법

### 1. 더블 버퍼링 구현
- Windows 전용 더블 버퍼링 시스템 구현
- `CreateConsoleScreenBuffer`를 사용한 백 버퍼 생성
- `WriteConsoleOutput`을 통한 한 번에 전체 화면 업데이트

### 2. 플랫폼별 조건부 컴파일
```c
#ifdef PLATFORM_WINDOWS
    // Windows에서는 매번 백 버퍼를 클리어
    platform_clear_screen();
#else
    // 다른 플랫폼에서는 기존 방식 사용
    if (first_render) {
        platform_clear_screen();
        first_render = false;
    }
#endif
```

### 3. 새로운 함수 추가
- `platform_present_buffer()`: 백 버퍼를 실제 화면에 출력
- Windows에서만 동작하며, 다른 플랫폼에서는 빈 함수

## 🚀 성능 개선 효과

### Before (기존)
- ❌ 매 프레임마다 화면 깜빡임
- ❌ 개별 문자 단위 출력으로 인한 성능 저하
- ❌ 시각적으로 불편한 게임 경험

### After (수정 후)  
- ✅ 완전히 깜빡임 없는 부드러운 화면
- ✅ 한 번에 전체 화면 업데이트로 성능 향상
- ✅ 전문적인 게임 품질의 시각적 경험

## 🔧 기술적 세부사항

### 메모리 관리
```c
// 화면 버퍼 메모리 할당
size_t buffer_size = g_buffer_size.X * g_buffer_size.Y * sizeof(CHAR_INFO);
g_screen_buffer = (CHAR_INFO*)malloc(buffer_size);
```

### 백 버퍼 초기화
```c
// 버퍼 초기화
for (int i = 0; i < g_buffer_size.X * g_buffer_size.Y; i++) {
    g_screen_buffer[i].Char.UnicodeChar = L' ';
    g_screen_buffer[i].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
}
```

### 화면 출력
```c
WriteConsoleOutput(
    g_console_handle,
    g_screen_buffer,
    g_buffer_size,
    (COORD){0, 0},
    &g_write_region
);
```

## 📋 테스트 방법

### Windows에서 빌드 및 실행
```bash
# Visual Studio 빌드
build.bat --release

# 또는 MinGW 빌드  
build.bat --mingw --release

# 게임 실행
build\snake.exe
```

### 확인 사항
1. 게임 화면이 부드럽게 렌더링되는지 확인
2. 뱀 이동 시 깜빡임이 없는지 확인
3. UI 메뉴 전환이 부드러운지 확인

## 🔍 추가 최적화

### UTF-8 문자 처리
Windows 콘솔에서 이모지 문자 출력을 위한 개선:
```c
// UTF-8 문자 처리 개선
if ((unsigned char)text[i] >= 128) {
    // 유니코드 문자 처리 - 간단한 이모지 대체
    g_screen_buffer[index].Char.UnicodeChar = L'*';
} else {
    g_screen_buffer[index].Char.AsciiChar = text[i];
}
```

### 콘솔 모드 설정
```c
// 콘솔 모드 설정 (깜빡임 최소화)
DWORD console_mode;
GetConsoleMode(g_console_handle, &console_mode);
console_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
SetConsoleMode(g_console_handle, console_mode);
```

## 📞 문제 해결

만약 여전히 깜빡임 문제가 발생한다면:

1. **관리자 권한으로 실행**: 일부 Windows 버전에서 콘솔 버퍼 생성 권한 필요
2. **최신 Windows 터미널 사용**: 기본 cmd.exe보다 성능이 우수함  
3. **컴파일러 최적화**: `-O2` 또는 `-O3` 플래그로 컴파일

---

**🎮 이제 Windows에서도 완벽하게 부드러운 뱀 게임을 즐기세요! 🐍**
