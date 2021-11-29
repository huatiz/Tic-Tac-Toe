#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT "80"
#define BUF_SIZE 1024
#define FD_SIZE 10

char username[FD_SIZE][8] = {};
char sendBuf[BUF_SIZE], recvBuf[BUF_SIZE];
int clients[FD_SIZE], status[FD_SIZE], isLogin[FD_SIZE];
fd_set master;

int createSocket();
int checkLogin(char *login_str);
int checkOpponent(int fd, int oppo_fd);
void closeConnection(int fd, int user_i);
void setUsername(int user_i, char *login_str);
void sendClientList(int fd);
void handleMessage(int fd, int user_i);

int main() {
    int listen_fd, conn_fd, max_fd, sock_fd;
    int max_i, nready, i;
    struct sockaddr_storage address;
    socklen_t addrlen;
    fd_set reads;
    char client_ip_str[INET_ADDRSTRLEN];

    FD_ZERO(&reads);
    FD_ZERO(&master);

    listen_fd = createSocket();

    for (int i = 0; i < FD_SIZE; i++) {
        clients[i] = -1;
        status[i] = -1;
        isLogin[i] = 0;
    }

    FD_SET(listen_fd, &master);
    max_fd = listen_fd;
    max_i = -1;

    printf("Waiting for connections...\n");

    // Accept conncetions
    while(1) {
        reads = master;

        if ((nready = select(max_fd + 1, &reads, NULL, NULL, NULL)) < 0) {
            perror("select() failed.\n");
            return 1;
        }

        // handle new connection
        if (FD_ISSET(listen_fd, &reads)) {
            addrlen = sizeof(address);
            if ((conn_fd = accept(listen_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
                perror("accept() failed.\n");
                return 1;
            }

            printf("New connection on socket %d\n", conn_fd);

            for (i = 0; i < FD_SIZE; i++) 
                if (clients[i] < 0) {
                    clients[i] = conn_fd;
                    break;
                }
            
            if (FD_SIZE == i) {
                perror("too many connection.\n");
                exit(1);
            }

            FD_SET(conn_fd, &master);
            max_fd = (conn_fd > max_fd) ? conn_fd : max_fd;
            max_i = (i > max_i) ? i : max_i;
            
            if (--nready < 0)
                continue;
        }

        // handle data from client
        for (i = 0; i <= max_i; i++) {
            if ((sock_fd = clients[i]) < 0)
                continue;

            if (FD_ISSET(sock_fd, &reads)) {
                memset(recvBuf, 0, BUF_SIZE);
                if (recv(sock_fd, recvBuf, BUF_SIZE, 0) <= 0)
                    closeConnection(sock_fd, i);
                else {
                    printf("fd = %d send message: %s\n", sock_fd, recvBuf);
                    handleMessage(sock_fd, i);
                }

                if (--nready <= 0)
                    break;
            }
        }
    }

    return 0;
}

int createSocket() {
    struct addrinfo hints, *bind_address;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;  // IPv4
    hints.ai_socktype = SOCK_STREAM;    // TCP
    hints.ai_flags = AI_PASSIVE;    // Use my IP

    getaddrinfo(0, PORT, &hints, &bind_address);

    int listen_fd;
    if ((listen_fd = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol)) < 0) {
        perror("socket() failed.\n");
        exit(1);
    }

    if (bind(listen_fd, bind_address->ai_addr, bind_address->ai_addrlen) < 0) {
        perror("bind() failed.\n");
        exit(1);
    }
    freeaddrinfo(bind_address);

    if(listen(listen_fd, 5) < 0) {
        perror("listen() failed.\n");
        exit(1);
    }

    return listen_fd;
}

int checkLogin(char *login_str) {
    FILE *fp = fopen("account.txt", "r");
    char str[32];

    while (fscanf(fp, "%s", str) != EOF)
        if (!strcmp(str, login_str+1)) 
            return 1;

    return 0;
}

int checkOpponent(int fd, int oppo_fd) {
    if (fd == oppo_fd) return 0;

    for (int i = 0; i < FD_SIZE; i++)
        if (clients[i] == oppo_fd)
            if (!status[i]) return 1;

    return 0;
}

void closeConnection(int fd, int user_i) {
    close(fd);
    FD_CLR(fd, &master);
    clients[user_i] = -1;
    status[user_i] = -1;
    isLogin[user_i] = 0;
    printf("[Quit] fd = %d\n", fd);

    return;
}

void setUsername(int user_i, char *login_str) {
    char *p = login_str + 1;
    char *end = strstr(p, ":");
    *end = 0;
    memset(username[user_i], 0, sizeof(username));
    strcat(username[user_i], p);
    status[user_i] = 0;

    return;
}

void sendClientList(int fd) {
    int flag = 0;
    strcat(sendBuf, "\n== Online Client List ==\n");
    send(fd, sendBuf, strlen(sendBuf), 0);
    for (int i = 0; i < FD_SIZE; i++) {
        if (clients[i] < 0)
            continue;
        if (clients[i] != fd && status[i] != -1) {
            flag = 1;
            char *p = status[i] ? "in-game" : "free";
            memset(sendBuf, 0, BUF_SIZE);
            sprintf(sendBuf, "[%s] fd: %2d | %s\n", username[i], clients[i], p);
            send(fd, sendBuf, strlen(sendBuf), 0);
        }
    }

    memset(sendBuf, 0, BUF_SIZE);
    if (flag)
        strcat(sendBuf, "\nChoose your opponent: (enter @fd)");
    else
        strcat(sendBuf, "No other users online.\n");
    send(fd, sendBuf, strlen(sendBuf), 0);

    return;
}

void handleMessage(int fd, int user_i) {
    char op = recvBuf[0];
    int oppo_fd;
    char pos;

    memset(sendBuf, 0, BUF_SIZE);
    switch (op) {
        case '!':
            if (isLogin[user_i])    return;
            if (checkLogin(recvBuf)) {
                isLogin[user_i] = 1;
                setUsername(user_i, recvBuf);
                send(fd, "Login successfully.", BUF_SIZE, 0);
            }
            else {
                send(fd, "Login failed.", BUF_SIZE, 0);
                closeConnection(fd, user_i);
            }
            break;
        case 'l':
            if (!strcmp(recvBuf, "ls"))
                sendClientList(fd);
            break;
        case 'q':
            if (!strncmp(recvBuf, "quit", 4)) {
                closeConnection(fd, user_i);
                oppo_fd = atoi(&recvBuf[5]);
                if (oppo_fd)
                    send(oppo_fd, "Opponent quit.", BUF_SIZE, 0);
            }
            break;
        case '@':
            oppo_fd = atoi(&recvBuf[1]);
            if (checkOpponent(fd, oppo_fd)) {
                sprintf(sendBuf, "Battle %d %s", fd, username[user_i]);
                send(oppo_fd, sendBuf, BUF_SIZE, 0);
            }
            else
                send(fd, "Invalid opponent.", BUF_SIZE, 0);
            break;
        case 'A':
            if (!strncmp(recvBuf, "Agree", 5)) {
                oppo_fd = atoi(&recvBuf[6]);

                status[user_i] = 1;
                for (int i = 0; i < FD_SIZE; i++)
                    if (clients[i] == oppo_fd) {
                        status[i] = 1;
                        break;
                    }

                sprintf(sendBuf, "Agree %s", username[user_i]);
                send(oppo_fd, sendBuf, BUF_SIZE, 0);
            }
            break;
        case 'D':
            if (!strncmp(recvBuf, "Disagree", 8)) {
                oppo_fd = atoi(&recvBuf[9]);
                sprintf(sendBuf, "Disagree %s", username[user_i]);
                send(oppo_fd, sendBuf, BUF_SIZE, 0);
            }
            break;
        case '#':
            pos = recvBuf[1];
            oppo_fd = atoi(&recvBuf[3]);
            sprintf(sendBuf, "#%c", pos);
            send(oppo_fd, sendBuf, BUF_SIZE, 0);
            break;
        case 'O':
            if (!strcmp(recvBuf, "Over"))
                status[user_i] = 0;
            break;
        default:    
            for (int i = 0; i < FD_SIZE; i++) {
                if (clients[i] == -1)   continue;
                if (clients[i] != fd) {
                    char *p = recvBuf;
                    sprintf(sendBuf, "[%s] %s\n", username[user_i], p);
                    send(clients[i], sendBuf, BUF_SIZE, 0);
                }
            }
            break;
    }

    return;
}