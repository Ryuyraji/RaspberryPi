#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 60000
#define SERVER_IP "192.168.0.85"
#define BUF_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buf[BUF_SIZE];
    int option, state;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
    }

    while (1) {
        printf("--menu--\n");
        printf("1: LED\n");
        printf("2: LED 밝기 조절 (high/middle/low)\n");
        printf("3: buzzer\n");
        printf("4: cds\n");
        printf("5: 7segment\n");
        printf("6: 종료\n");
        printf("------\n");
        printf("Enter number: ");
        scanf("%d", &option);
        getchar();

        memset(buf, 0, BUF_SIZE);

        switch(option) {
            case 1:
                printf("Enter LED State (1=ON, 0=OFF): ");
                scanf("%d", &state);
                getchar();
                if (state == 1) strcpy(buf, "LED ON");
                else if (state == 0) strcpy(buf, "LED OFF");
                else {
                    printf("잘못된 상태입니다.\n");
                    continue;
                }
                break;

            case 2: {
			char input[BUF_SIZE];  

   			 printf("밝기 입력 (high/middle/low): ");
   			 fgets(input, sizeof(input), stdin);       
   			 input[strcspn(input, "\n")] = 0;           

    			 snprintf(buf, BUF_SIZE, "PWM %s", input);
		       	 printf("클라이언트 전송: [%s]\n", buf); 	 
 		 break;		
		    }


            case 3:
                strcpy(buf, "BUZZER");
                break;

            case 4:
                strcpy(buf, "CDS");
                break;

            case 5: 
		int num;
		printf("0~9 숫자 입력: ");
		scanf("%d", &num);
		getchar();
		
		snprintf(buf, BUF_SIZE, "FND %d", num);
		printf("클라이언트 전송 : %s\n",buf);
		break;
		


            case 6:
                strcpy(buf, "EXIT");
                send(sock, buf, strlen(buf), 0);
                close(sock);
                return 0;

            default:
                printf("잘못된 입력입니다.\n");
                continue;
        }

        send(sock, buf, strlen(buf), 0);
    }

    return 0;
}

