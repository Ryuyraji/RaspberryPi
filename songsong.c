#include <pthread.h>
#include <stdio.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <softTone.h>
#include <softPwm.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#define PORT 60000
#define BUF_SIZE 1024
#define LED_GPIO 18
#define BUZZER_GPIO 25
#define CDS_I2C_ADDR 0x48
#define CDS_CHANNEL 0
#define THRESHOLD 180
#define TOTAL 32

int notes[] = {
    391, 391, 440, 440, 391, 391, 329, 329,
    391, 391, 329, 329, 293, 293, 293, 0,
    391, 391, 440, 440, 391, 391, 329, 329,
    391, 329, 293, 329, 261, 261, 261, 0
};

int musicPlay() {
    softToneCreate(BUZZER_GPIO);
    for (int i = 0; i < TOTAL; ++i) {
        softToneWrite(BUZZER_GPIO, notes[i]);
        delay(280);
    }
    return 0;
}

void* buzzer_thread(void* arg) {
    musicPlay();
    pthread_exit(NULL);
}

void* led_thread(void* arg) {
    int state = *((int*)arg);
    pinMode(LED_GPIO, OUTPUT);
    digitalWrite(LED_GPIO, state ? HIGH : LOW);
    pthread_exit(NULL);
}

void* led_threadpwd(void* arg) {
    printf("led_threadpwd 들어옴");
    const char* level = (const char*)arg;
    int value=0;
    if (strcmp(level, "high") == 0) value = 100;
    else if (strcmp(level, "middle") == 0) value = 50;
    else if (strcmp(level, "low") == 0) value = 10;
    else {
        printf("잘못된 입력: %s\n", level);
        pthread_exit(NULL);
    }
    printf("%d",value);
    pinMode(LED_GPIO, OUTPUT);
    printf("pinmode 설정완료");
    softPwmWrite(LED_GPIO, value);
    printf("PWM 들어옴");
    pthread_exit(NULL);
}

void* cds_thread(void* arg) {
    int fd, a2dVal, cnt = 0;
    int ledState = LOW;
    if ((fd = wiringPiI2CSetupInterface("/dev/i2c-1", 0x48)) < 0) {
        perror("wiringPiI2CSetupInterface failed");
        pthread_exit(NULL);
    }
    pinMode(LED_GPIO, OUTPUT);
    digitalWrite(LED_GPIO, LOW);
    while (1) {
        wiringPiI2CWrite(fd, 0x00 | CDS_CHANNEL);
        wiringPiI2CRead(fd);
        a2dVal = wiringPiI2CRead(fd);
        printf("[CDS %d] a2dVal = %d - %s\n", cnt++, a2dVal, (a2dVal < THRESHOLD) ? "Bright" : "Dark");
        if (a2dVal < THRESHOLD && ledState == LOW) {
            digitalWrite(LED_GPIO, HIGH);
            ledState = HIGH;
        } else if (a2dVal >= THRESHOLD && ledState == HIGH) {
            digitalWrite(LED_GPIO, LOW);
            ledState = LOW;
        }
        delay(1000);
    }
    pthread_exit(NULL);
}

void* fnd_thread(void* arg) {
    int gpiopins[4] = {16, 20, 21, 12};
    int number[10][4] = {
        {0,0,0,0}, {0,0,0,1}, {0,0,1,0}, {0,0,1,1},
        {0,1,0,0}, {0,1,0,1}, {0,1,1,0}, {0,1,1,1},
        {1,0,0,0}, {1,0,0,1}
    };
    int num = *((int*)arg);
    for (int i = 0; i < 4; i++) pinMode(gpiopins[i], OUTPUT);
    while (num >= 0) {
        for (int i = 0; i < 4; i++)
            digitalWrite(gpiopins[i], number[num][i] ? HIGH : LOW);
        sleep(1);
        if (num == 0) musicPlay();
        num--;
    }
    for (int i = 0; i < 4; i++) digitalWrite(gpiopins[i], HIGH);
    pthread_exit(NULL);
}

int main() {
    if (wiringPiSetupGpio() == -1) {
        perror("wiringPiSetupGpio");
        return 1;
    }
    pinMode(LED_GPIO, OUTPUT);
    softPwmCreate(LED_GPIO, 0, 100);

    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_size = sizeof(clnt_addr);
    char buf[BUF_SIZE];

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT);
    bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    listen(serv_sock, 5);
    printf("서버 대기 중...\n");

    clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_size);
    printf("클라이언트 연결됨.\n");

    while (1) {
        memset(buf, 0, BUF_SIZE);
        int len = recv(clnt_sock, buf, BUF_SIZE - 1, 0);
        if (len <= 0) break;
        buf[len] = '\0';
        buf[strcspn(buf,"\r\n")]= 0;
        pthread_t t;
        if (strncmp(buf, "LED ON", 6) == 0) {
            printf("LED ON");
            int s = 1;
            pthread_create(&t, NULL, led_thread, &s);
            pthread_join(t, NULL);
        } else if (strncmp(buf, "LED OFF", 7) == 0) {
            printf("LED OFF");
            int s = 0;
            pthread_create(&t, NULL, led_thread, &s);
            pthread_join(t, NULL);
        } else if (strncmp(buf, "PWM", 3) == 0) {
            printf("밝기 조절");
            char* level = strdup(buf + 4);
            printf("[PWM level] → [%s]\n", level);
            pthread_create(&t, NULL, led_threadpwd, level);
            pthread_join(t, NULL);
            free(level);
        } else if (strncmp(buf, "BUZZER", 6) == 0) {
            printf("BUZZER ON");
            pthread_create(&t, NULL, buzzer_thread, NULL);
            pthread_join(t, NULL);
        } else if (strncmp(buf, "CDS", 3) == 0) {
            printf("CDS ON");
            pthread_create(&t, NULL, cds_thread, NULL);
            pthread_join(t, NULL);
        } else if (strncmp(buf, "FND", 3) == 0) {
            int num = atoi(buf + 4);  // "FND 5" → 5 추출
            int* p = malloc(sizeof(int));
            *p = num;

            printf("FND ON (%d부터 카운트다운)\n", num);
            pthread_create(&t, NULL, fnd_thread, p);
            pthread_join(t, NULL);
            free(p);  // 동적할당 해제
        } else if (strncmp(buf, "EXIT", 4) == 0) {
            printf("클라이언트 종료 요청\n");
            break;
        } else {
            printf("알 수 없는 명령: %s\n", buf);
        }
    }

    close(clnt_sock);
    close(serv_sock);
    return 0;
}
