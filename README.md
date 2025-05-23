# Raspberry Pi GPIO TCP Control Server
## 개요
이 프로젝트는 **Raspberry Pi**를 기반으로 한 **TCP 서버**를 통해 원격에서 **LED, Buzzer, 조도 센서(CDS), 7세그먼트 디스플레이**를 제어할 수 있는 시스템입니다. 클라이언트는 명령어를 전송하여 각 기능을 제어할 수 있습니다.

## 개발 일정(5.21~5.23)

| 날짜       | 내용                                                                    
| ---------- | ------------------------------------------------------------            
| 2025.05.21 | Raspberry Pi 장치 제어 기능 테스트 (LED, Buzzer, CDS, 7세그먼트 동작 확인) 
| 2025.05.22 | 장치 제어 시 발생한 오류를 수정하고, 각각의 장치 제어 함수들을 스레드 형태로 구조화하여 재구성함                       
| 2025.05.23 | TCP 서버 구현 및 클라이언트 명령어 처리 로직 완성/README 문서화 및 회로도 추가, GitHub 업로드                              

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

![image](https://github.com/user-attachments/assets/0533a3cb-8132-426d-b0ee-6712169405cc)


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

## 명령어 목록 (클라이언트 → 서버)

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

### Makefile을 이용한 컴파일 

TARGET = songsong
CC = gcc
CFLAGS = -Wall -pthread
LIBS = -lwiringPi -lpthread -lrt

SRC = songsong.c
OBJ = $(SRC:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)


---


## 문제점 및 보안사항 

led 밝기 조절하는 부분 → 공유 라이브러리(.so)로 분리하여 유연한 업데이트 가능하게 보안 필요

cds_thread()가 무한 루프 → 종료 불가함, pthread_cancel() 등으로 스레드 종료 보안 필요

너무 많은 스레드를 detach 방식으로 실행 → 스레드 풀 또는 join, 종료조건 명확히 설정하여 보안 필요

장치 기능이 한 파일에 몰려 있음 → 기능별로 동적 라이브러리로 분리하여 유지보수성 향상 필요

클라이언트가 SIGINT 외 시그널 처리 안됨 → 비정상 종료 방지 위해 시그널 핸들링 보안 필요

서버가 데몬 프로세스 형식 아님 → fork/setsid 등으로 백그라운드 실행 구조로 보안 필요

## 실행 영상  
프로젝트 작동 모습을 영상으로 확인하실 수 있습니다.  
👉 [YouTube에서 보기](https://youtu.be/ggEWkoVlXVg)

---

## 🖼️ 실행 화면  
아래 이미지는 프로젝트 실행 시의 실제 화면을 캡처한 것입니다.  
![image](https://github.com/user-attachments/assets/b271ac63-7537-4dad-a5fb-1a41f7a59b54)
