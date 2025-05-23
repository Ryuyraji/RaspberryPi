# Raspberry Pi GPIO TCP Control Server

이 프로젝트는 **Raspberry Pi**를 기반으로 한 **TCP 서버**를 통해 원격에서 **LED, Buzzer, 조도 센서(CDS), 7세그먼트 디스플레이**를 제어할 수 있는 시스템입니다. 클라이언트는 명령어를 전송하여 각 기능을 제어할 수 있습니다.

## 주요 기능

- LED ON/OFF 제어
- LED 밝기(PWM) 조절
- Buzzer를 통한 음악 재생
- 조도센서를 활용한 자동 LED 제어
- 7세그먼트 숫자 카운트다운 후 알림음
- TCP 통신 기반 명령 수신 및 스레드 처리

---

## 회로도

아래 이미지는 본 프로젝트의 전체 회로 구성입니다.

> 파일명: `pi.png` (README와 같은 폴더에 위치해야 함)

![회로도](./pi.png)

---

## 주요 코드 예시

### LED ON/OFF 제어

```c
void* led_thread(void* arg) {
    int state = *((int*)arg);
    pinMode(LED_GPIO, OUTPUT);
    digitalWrite(LED_GPIO, state ? HIGH : LOW);
    pthread_exit(NULL);
}
```

### LED 밝기(PWM) 조절

```c
void* led_threadpwd(void* arg) {
    const char* level = (const char*)arg;
    int value = 0;
    if (strcmp(level, "high") == 0) value = 100;
    else if (strcmp(level, "middle") == 0) value = 50;
    else if (strcmp(level, "low") == 0) value = 10;
    softPwmWrite(LED_GPIO, value);
    pthread_exit(NULL);
}
```

### Buzzer 음악 재생

```c
int musicPlay() {
    softToneCreate(BUZZER_GPIO);
    for (int i = 0; i < TOTAL; ++i) {
        softToneWrite(BUZZER_GPIO, notes[i]);
        delay(280);
    }
    return 0;
}
```

### 조도센서 → LED 자동 제어

```c
void* cds_thread(void* arg) {
    int a2dVal;
    while (1) {
        wiringPiI2CWrite(fd, 0x00 | CDS_CHANNEL);
        wiringPiI2CRead(fd);  // dummy read
        a2dVal = wiringPiI2CRead(fd);
        digitalWrite(LED_GPIO, (a2dVal < THRESHOLD) ? HIGH : LOW);
        delay(1000);
    }
}
```

### 7세그먼트 카운트다운

```c
void* fnd_thread(void* arg) {
    int num = *((int*)arg);
    while (num >= 0) {
        // 4비트 출력 설정
        ...
        sleep(1);
        if (num == 0) musicPlay();
        num--;
    }
}
```

---

## 📡 명령어 목록 (클라이언트 → 서버)

| 명령어       | 설명                                      |
|--------------|-------------------------------------------|
| `LED ON`     | LED 켜기                                  |
| `LED OFF`    | LED 끄기                                  |
| `PWM high`   | LED 밝기 100% 설정                         |
| `PWM middle` | LED 밝기 50% 설정                          |
| `PWM low`    | LED 밝기 10% 설정                          |
| `BUZZER`     | 버저로 음악 재생                          |
| `CDS`        | 조도 감지하여 밝을 때만 LED ON            |
| `FND N`      | 7세그먼트로 N부터 0까지 카운트 후 BUZZER  |
| `EXIT`       | 서버 종료                                 |

---

## ⚙️ 컴파일 및 실행

```bash
gcc main.c -o gpio_server -lwiringPi -lpthread -lcrypt
sudo ./gpio_server
```

> `sudo`는 GPIO 제어에 필요합니다.

---

## 프로젝트 구조

```bash
.
├── main.c         # 전체 TCP 서버 및 장치 제어 코드
├── pi.png         # 회로도 이미지
└── README.md      # 설명 문서
```

---

## ❗ 참고사항

- CDS는 무한 루프 구조이므로 테스트 용도로 사용하거나 이후 종료 로직 추가 필요
- `softTone`과 `softPwm`은 서로 다른 GPIO 핀에서 사용해야 충돌 없음
- FND 출력은 SN74LS32N IC를 통해 제어됨

---

## 📜 라이선스

MIT License - 자유롭게 사용 및 수정 가능합니다.
