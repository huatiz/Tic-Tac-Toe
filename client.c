#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>

#define PORT "80"
#define IP "192.168.207.129"
#define BUF_SIZE 1024

int oppo_fd = 0;
bool mark = false;
bool myTurn = false;
bool isStart = false;
bool invite = false;
char win = ' ';
char mySymbol, oppoSymbol;
char sendBuf[BUF_SIZE], recvBuf[BUF_SIZE];
char userName[16] = {};
char oppoName[16] = {};
char square[9] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};

void printDivision();
void printBoard();
void gameInit();
void setBoard(int fd, int pos, char symbol, int turn);
void resetBoard();
int isLegalMove(int pos);
int isWin();
void checkWin(int fd, int turn);
int createSocket();
char* login();
void handleSend(int fd);
void handleRecv(int fd);

int main() {
    int sockfd = createSocket();
    char *login_str = login();
    int max_fd;
    fd_set reads;

    FD_ZERO(&reads);

    send(sockfd, login_str, BUF_SIZE, 0);

    while (1) {
        FD_SET(fileno(stdin), &reads);
        FD_SET(sockfd, &reads);
        max_fd = ((fileno(stdin) > sockfd) ? fileno(stdin) : sockfd) + 1;
        select(max_fd, &reads, NULL, NULL, NULL);
        // handle send data
        if (FD_ISSET(fileno(stdin), &reads)) {
            if (fgets(sendBuf, BUF_SIZE, stdin) == NULL) {
                close(sockfd);
                exit(0);
            }
            char *end = strstr(sendBuf, "\n");
            *end = '\0';

            if (strlen(sendBuf) > 0)
                handleSend(sockfd);
            memset(sendBuf, 0, BUF_SIZE);
        }
        // handle recv data
        if (FD_ISSET(sockfd, &reads)) {
            memset(recvBuf, 0, BUF_SIZE);
            if (recv(sockfd, recvBuf, BUF_SIZE, 0) <= 0) {
                perror("server terminated.\n");
                exit(1);
            }
            handleRecv(sockfd);
        }
    }

    return 0;
}

void printDivision() {
    printf("\n---------------------------------------------\n");
    return;
}

void printBoard() {
    printf("\nYou (%c)  vs  %s (%c)\n\n", mySymbol, oppoName, oppoSymbol);
    printf("     |     |     \n");
    printf("  %c  |  %c  |  %c \n", square[0], square[1], square[2]);
    printf("_____|_____|_____\n");
    printf("     |     |     \n");
    printf("  %c  |  %c  |  %c \n", square[3], square[4], square[5]);
    printf("_____|_____|_____\n");
    printf("     |     |     \n");
    printf("  %c  |  %c  |  %c \n", square[6], square[7], square[8]);
    printf("     |     |     \n\n");

    return;
}

void gameInit() {
    mySymbol = (mark) ? 'O' : 'X';
    oppoSymbol = (mark) ? 'X' : 'O';
    isStart = true;

    printDivision();
    printf("\nGame Start!\n\n");
    printf("== Tic Tac Toe ==\n");
    printBoard();

    return;
}

void setBoard(int fd, int pos, char symbol, int turn) {
    square[pos-1] = symbol;
    printDivision();
    printBoard();
    checkWin(fd, turn);

    return;
}

void resetBoard() {
    oppo_fd = 0;
    memset(oppoName, 0, sizeof(oppoName));
    isStart = false;
    mark = false;
    myTurn = false;
    invite = false;
    for (int i = 0; i < 9; i++)
        square[i] = (i+1) + '0';
    
    return;
}

int isLegalMove(int pos) {
    if (!isStart)   return 0;

    if (pos < 1 || pos > 9) {
        printf("Invalid move.\n");
        return 0;
    }
    if (square[pos-1] == 'O' || square[pos-1] == 'X') {
        printf("This square has been occupied.\n");
        return 0;
    }

    return 1;
}

int isWin() {
    if (square[0] == square[1] && square[1] == square[2])   { win = square[0]; return 1; }
    if (square[3] == square[4] && square[4] == square[5])   { win = square[3]; return 1; }
    if (square[6] == square[7] && square[7] == square[8])   { win = square[6]; return 1; }
    if (square[0] == square[3] && square[3] == square[6])   { win = square[0]; return 1; }
    if (square[1] == square[4] && square[4] == square[7])   { win = square[1]; return 1; }
    if (square[2] == square[5] && square[5] == square[8])   { win = square[2]; return 1; }
    if (square[0] == square[4] && square[4] == square[8])   { win = square[0]; return 1; }
    if (square[2] == square[4] && square[4] == square[6])   { win = square[2]; return 1; }

    if (square[0] != '1' && square[1] != '2' && square[2] != '3' &&
        square[3] != '4' && square[4] != '5' && square[5] != '6' && 
        square[6] != '7' && square[7] != '8' && square[8] != '9')
        return 0;

    return -1;
}
void checkWin(int fd, int turn) {
    switch (isWin()) {
        case 1:
            if (win == mySymbol)
                printf("==> You Win! :)\n");
            else
                printf("==> You Lose :(\n");
            break;
        case 0:
            printf("\n==> Game Draw!\n");
            break;
        default:
            if (turn)
                printf("Please enter #(1-9)\n");
            else
                printf("Waiting for your opponent...\n");
            return;
            break;
    }
    send(fd, "Over", BUF_SIZE, 0);
    resetBoard();
    printDivision();
    printf("Please enter 'ls' to view online users.\n");
    printf("Also you can enter 'quit' to leave the room.\n\n");

    return;
}

int createSocket() {   
    int sockfd;
    struct addrinfo hints, *servinfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;  // IPv4
    hints.ai_socktype = SOCK_STREAM;    // TCP

    getaddrinfo(IP, PORT, &hints, &servinfo);

    if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) < 0) {
        perror("socket() failed.\n");
        exit(1);
    }

    if (connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) < 0) {
        perror("connect() failed.\n");
        exit(1);
    }

    printf("Client connect successfully.\n");

    return sockfd;
}

char* login() {
    char username[16], password[16], str[34];

    printDivision();
    printf("== Login ==\n");
    printf("Username: ");
    scanf("%s", username);
    printf("Password: ");
    scanf("%s", password);
    printDivision();

    strcat(userName, username);
    sprintf(str, "!%s:%s", username, password);
    char *p = str;

    return p;
}

void handleSend(int fd) {
    // ban request
    if (isStart && strcmp(sendBuf, "quit")) {
        if (!myTurn) {
            printf("\nWaiting for your opponent...\n");
            return;
        }
        if (sendBuf[0] != '#') {
            printf("\nPlease enter #(1-9)\n");
            return;
        }
        
    }
    
    char tmp[4] = {};
    bool flag = false;

    switch (sendBuf[0]) {
        case 'q':
            if (!strcmp(sendBuf, "quit")) {
                if (oppo_fd) 
                    sprintf(sendBuf, "quit %d", oppo_fd);
                send(fd, sendBuf, BUF_SIZE, 0);
                exit(0);
            }
            break;
        case '@':
            oppo_fd = atoi(&sendBuf[1]);
            break;
        case 'o':
            myTurn = true;
            if (invite) {
                sprintf(sendBuf, "Agree %d", oppo_fd);
                gameInit();
                printf("Please enter #(1-9)\n");
            }
            break;
        case 'x':
            if (invite) {
                invite = false;
                memset(oppoName, 0, sizeof(oppoName));
                sprintf(sendBuf, "Disagree %d", oppo_fd);
            }
            break;
        case '#':
            if (isLegalMove(atoi(&sendBuf[1]))) {
                flag = false;
                myTurn = false;
                strcat(tmp, sendBuf);
                sprintf(sendBuf, "%s %d", tmp, oppo_fd);
                setBoard(fd, atoi(&sendBuf[1]), mySymbol, 0);
            }
            else {
                flag = true;
                if (isStart)
                    printf("\nPlease enter #(1-9)\n");
            }
            break;
    }
    if (!flag)
        send(fd, sendBuf, BUF_SIZE, 0);

    return;
}

void handleRecv(int fd) {
    switch (recvBuf[0]) {
        case 'L':
            if (!strcmp(recvBuf, "Login successfully.")) {
                printf("Hello [%s]!\nPlease enter 'ls' to view online users.\n", userName);
                printf("Also you can enter 'quit' to leave the room.\n");
                printDivision();
            }
            if (!strcmp(recvBuf, "Login failed.")) {
                printf("%s\n", recvBuf);
                exit(0);
            }
            break;
        case 'O':
            if (!strcmp(recvBuf, "Opponent quit.")) {
                printf("\n%s\n", recvBuf);
                printf("You can start an new game with other users.\n\n");
                resetBoard();
            }
            break;
        case 'I':
            if (!strcmp(recvBuf, "Invalid opponent.")) {
                printf("%s\n\n", recvBuf);
                printf("Choose your opponent: (enter @fd)\n");
            }
        case 'B':
            if (!strncmp(recvBuf, "Battle", 6)) {
                invite = true;
                oppo_fd = atoi(&recvBuf[7]);
                char *p = recvBuf + 9;
                memset(oppoName, 0, sizeof(oppoName));
                strcat(oppoName, p);
                printf("Do you agree to battle with [%s]? (enter o/x)\n", oppoName);
            }
            break;
        case 'A':
            if (!strncmp(recvBuf, "Agree", 5)) {
                char *p = recvBuf + 6;
                memset(oppoName, 0, sizeof(oppoName));
                strcat(oppoName, p);
                mark = true;
                gameInit();
                printf("Waiting for your opponent...\n");
            }
            break;
        case 'D':
            if (!strncmp(recvBuf, "Disagree", 8)) {
                char *p = recvBuf + 9;
                memset(oppoName, 0, sizeof(oppoName));
                strcat(oppoName, p);
                printf("\n[%s] rejects you.\n\n", oppoName);
            }
            break;
        case '#':
            myTurn = true;
            setBoard(fd, atoi(&recvBuf[1]), oppoSymbol, 1);
            break;
        default:
            printf("%s\n", recvBuf);
            break;
    }

    return;
}