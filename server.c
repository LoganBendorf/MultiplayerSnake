
#define MIN_WIDTH 5
#define MAX_WIDTH 30
#define MIN_HEIGHT 5
#define MAX_HEIGHT 20

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <stdarg.h>
#include <ctype.h>

#if defined(_WIN32)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#endif

#if defined(_WIN32)
#define IS_VALID_SOCKET(s) ((s) != INVALID_SOCKET)
#define CLOSE_SOCKET(s) closesocket(s)
#define SOCKET_ERRNO WSAGetLastError()
// susssy
#define GET_ERROR_STRING wchar_t *s = NULL; \
                    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, \
                        NULL, WSAGetLastError(), \
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), \
                        (LPWSTR)&s, 0, NULL); \
                    fprintf(stderr, "%S\n", s); \
                    {LocalFree(s)}
#else
#define IS_VALID_SOCKET(s) ((s) >= 0)
#define CLOSE_SOCKET(s) close(s)
#define SOCKET int
#define SOCKET_ERRNO errno
#define GET_ERROR_STRING strerror(errno)
#endif

#include "gameLogic.h"
#include "errorFunctions.h"
#include "timeHelpers.h"

int main(int argc, char* argv[]) {

    if (argc > 2) {
        printf("Userage: ./snake port\n"); exit(1); }
    char localPort[16] = "9001";
    if (argc == 2) {
        if (strlen(argv[1]) > 6) {
            fprintf(stderr, "Port too long\n");
            exit(1);
        }
        strncpy(localPort, argv[2], 6);
    }


    signal(SIGINT , catchSigThenExit);
    signal(SIGABRT , catchSigThenExit);
    signal(SIGILL , catchSigThenExit);
    signal(SIGFPE , catchSigThenExit);
    signal(SIGSEGV, catchSigThenExit); // <-- this one is for segmentation fault
    signal(SIGTERM , catchSigThenExit);

    #if defined(_WIN32)
    WASDATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "Failed to initialize\n"); exit (1);
    }
    #endif
    ////////// SETUP /////////////
    // 1. Wait for client to connect
    // 2. Set up map size (2 apples per map?)
    // 3. Decide player spawn
    // 4. Send info (map, player location, random seed) to client
    // 5. Set up    
    // 6. Sync
    // 7. Start game
    #define TIMEOUT 5.0
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    struct addrinfo *listenAddress;
    int addrRC;
    if (addrRC = getaddrinfo(NULL, localPort, &hints, &listenAddress) != 0) {
        printf("Failed to set up listen address. (rc: %d) Line: %d\n", addrRC, __LINE__);
        exit(1);
    }

    SOCKET listenSocket;
    listenSocket = socket(listenAddress->ai_family,
                listenAddress->ai_socktype, listenAddress->ai_protocol);
                
    if (!IS_VALID_SOCKET(listenSocket)) {
        fprintf(stderr, "socket() failed. (%d, %s). Line: %d\n", SOCKET_ERRNO, GET_ERROR_STRING, __LINE__);
        exit(1);
    }

    if (bind(listenSocket, listenAddress->ai_addr, listenAddress->ai_addrlen) != 0) {
        fprintf(stderr, "bind() failed. (%d, %s). Line: %d\n", SOCKET_ERRNO, GET_ERROR_STRING, __LINE__);
        exit(1);
    }
    freeaddrinfo(listenAddress);

    // Print address
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(listenSocket, (struct sockaddr *)&sin, &len) == -1) {
        fprintf(stderr, "getsockname() failed. Line: %d\n", __LINE__);
    }
    char readableIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &sin.sin_addr, readableIP, sizeof(readableIP));
    printf("Server is running on %s:%d\n", readableIP, ntohs(sin.sin_port));

    // Connect
    struct sockaddr_storage clientAddressStorage;
    socklen_t clientLen = sizeof(clientAddressStorage);
    char read[1024] = {0};
    int totalBytesRead = 0;
    printf("Waiting for client connection...\n");
    while (true) {
        int bytes = recvfrom(listenSocket,
                    read + totalBytesRead, 1024 - totalBytesRead,
                    0,
                    (struct sockaddr*) &clientAddressStorage, &clientLen);
        totalBytesRead += bytes;
        if (totalBytesRead > 1024) {
            fprintf(stderr, "massive failure while receiving game start from client. Line: %d\n", __LINE__);
            exit(1);
        }
        if (strncmp(read, "Create game", 11) == 0) {
            break;
        }
    }
    printf("Received %s: Client connected\n", read);

    char clientIP[INET_ADDRSTRLEN];
    int clientPortAsInt;

    struct sockaddr_in *s = (struct sockaddr_in *)&clientAddressStorage;
    inet_ntop(AF_INET, &s->sin_addr, clientIP, sizeof(clientIP));
    clientPortAsInt = ntohs(s->sin_port);
    char clientPort[8];
    snprintf(clientPort, 8, "%d", clientPortAsInt);

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_family = AF_INET;
    struct addrinfo *clientAddress;

    if (addrRC = getaddrinfo(clientIP, clientPort, &hints, &clientAddress) != 0) {
        printf("Failed to set up client address. (rc: %d) Line: %d\n", addrRC, __LINE__);
        exit(1);
    }

    SOCKET clientSocket;
    clientSocket = socket(clientAddress->ai_family,
                clientAddress->ai_socktype, clientAddress->ai_protocol);
    if (!IS_VALID_SOCKET(clientSocket)) {
        fprintf(stderr, "socket() failed. (%d, %s). Line: %d\n", SOCKET_ERRNO, GET_ERROR_STRING, __LINE__);
        exit(1);
    }
    // Not sure if I should call bind for clientSocket
    printf("Connected to client\n");

    // Set up game state and send info to client
    char stringWidth[8];
    char stringHeight[8];
    int WIDTH = 0;
    int HEIGHT = 0;
    while (true) {
        printf("Width? ");
        if (fgets(stringWidth, 8, stdin) != NULL) {
            stringWidth[strcspn(stringWidth, "\n")] = 0;
            bool invalid = false;
            for (int i = 0; i < strlen(stringWidth); i++) {
                if (isdigit(stringWidth[i]) == 0) {
                    printf("Invalid width, bad characters\n");
                    invalid = true;}}
            if (invalid) {
                continue;}
            WIDTH = atoi(stringWidth);
            if (WIDTH < MIN_WIDTH || WIDTH > MAX_WIDTH) {
                printf("Invalid width, out of bounds\n");
                printf("width = %d\n", WIDTH);
                continue;
            }
        }
        printf("Height? ");
        if (fgets(stringHeight, 8, stdin) != NULL) {
            stringHeight[strcspn(stringHeight, "\n")] = 0;
            bool invalid = false;
            for (int i = 0; i < strlen(stringHeight); i++) {
                if (isdigit(stringHeight[i]) == 0) {
                    printf("Invalid height, bad characters\n");
                    invalid = true;}}
            if (invalid) {
                continue;}
            HEIGHT = atoi(stringHeight);
            if (HEIGHT < MIN_HEIGHT || HEIGHT > MAX_HEIGHT) {
                printf("Invalid height, out of bounds\n");
                printf("height = %d\n", HEIGHT);
                continue;
            }
            break;
        }
    }
    printf("width = %d\n", WIDTH);
    printf("height = %d\n", HEIGHT);

    printf("Creating game state and sending info to client...\n");
    char sendBuffer[1024] = {0};
    strncpy(sendBuffer, "screenSize: ", 12);
    strcat(sendBuffer, stringWidth);
    strcat(sendBuffer, "x");
    strcat(sendBuffer, stringHeight);


    screenData screen = {0, WIDTH, HEIGHT};

    int randomSeed = time(NULL);
    srand(randomSeed);

    int randomX = rand() % (screen.width - 2);
    int randomY = rand() % (screen.height - 2);
    randomX += 1;
    randomY += 1;
    int serverStartingPosition = randomY * screen.width + randomX;
    char charServerStartingPosition[8] = {0};
    snprintf(charServerStartingPosition, 8, "%d", serverStartingPosition);

    strcat(sendBuffer, "\nserverStartingPosition: ");
    strcat(sendBuffer, charServerStartingPosition);


    randomX = rand() % (screen.width - 2);
    randomY = rand() % (screen.height - 2);
    randomX += 1;
    randomY += 1;
    int clientStartingPosition = randomY * screen.width + randomX;
    char charClientStartingPosition[8] = {0};
    snprintf(charClientStartingPosition, 8, "%d", clientStartingPosition);
                          
    strcat(sendBuffer, "\nclientStartingPosition: ");
    strcat(sendBuffer, charClientStartingPosition);

    strcat(sendBuffer, "\nrandomSeed: ");
    char stringRandomSeed[64] = {0};
    snprintf(stringRandomSeed, 32, "%d", randomSeed);
    strcat(sendBuffer, stringRandomSeed);
    strcat(sendBuffer, "\n");

    int bytesSent = sendto(clientSocket,
                    sendBuffer, strlen(sendBuffer),
                    0,
                    clientAddress->ai_addr, clientAddress->ai_addrlen);
    if (bytesSent < strlen(sendBuffer) ) {
        fprintf(stderr, "bruh. Line: %d\n", __LINE__);
        exit(1);
    }

    // Set up game
    printf("Setting up game locally\n");
    
    
    printf("Creating map\n");
    char* map = (char*) Malloc(screen.width * screen.height);
    for (int i = 0; i < screen.height; i++) {
        for (int j = 0; j < screen.width; j++) {
            if (i == 0 || i == screen.height - 1 || j == 0 || j == screen.width - 1) {
                map[i * screen.width + j] = '#';
            } else {
                map[i * screen.width + j] = ' '; }
        }
    }
    screen.map = map;

    screen.map[serverStartingPosition] = 'O';

    screen.map[clientStartingPosition] = 'O';

    printScreen(screen);
    disableEcho();

    printf("Initializing client player\n");
    node* player = (node*) Malloc(sizeof(node));
    player->xMov = 0;
    player->yMov = 0;
    player->prevXMov = 0;
    player->prevYMov = 0;
    player->xPos = clientStartingPosition % screen.width;
    player->yPos = clientStartingPosition / screen.width;
    player->next = NULL;
    player->hasTail = false;
    player->score = 0;

    printf("Initializing server player\n");
    node* server = (node*) Malloc(sizeof(node));
    server->xMov = 0;
    server->yMov = 0;
    server->prevXMov = 0;
    server->prevYMov = 0;
    server->xPos = serverStartingPosition % screen.width;
    server->yPos = serverStartingPosition / screen.width;
    server->next = NULL;
    server->hasTail = false;
    server->score = 0;

    errorInfo errorData = {0, 0, 0};

    bool appleOnMap = false;
    bool gameStart = false;


    // Wait for sync
    printf("Waiting for client sync message...\n");
    memset(read, 0, strlen(read) - 1);
    totalBytesRead = 0;
    while (true) {
        int bytes = recvfrom(listenSocket,
                    read + totalBytesRead, 1024 - totalBytesRead,
                    0,
                    (struct sockaddr*) &clientAddressStorage, &clientLen);
        totalBytesRead += bytes;
        if (totalBytesRead > 1024) {
            fprintf(stderr, "massive failure while receiving game start from client. Line: %d\n", __LINE__);
            enableEcho();
            exit(1);
        }
        if (strncmp(read, "Sync", 4) == 0) {
            break;
        }
        if (bytes != 0) {
            printf("got something\n");
            printf("%s\n", read);
        }
    }

    printf("Sending sync time to client\n");
    memset(sendBuffer, 0, strlen(sendBuffer) - 1);
    char startTimeSeconds[128];
    char startTimeNanoSeconds[128];
    const char* startTimeString = "startTime: ";
    strcat(sendBuffer, startTimeString);
    struct timespec lastTime, currentTime;
    clock_gettime(CLOCK_MONOTONIC, &lastTime);
    snprintf(startTimeSeconds, 64, "%ld", lastTime.tv_sec);
    strcat(sendBuffer, startTimeSeconds);
    strcat(sendBuffer, ",");
    snprintf(startTimeNanoSeconds, 64, "%ld", lastTime.tv_nsec);
    strcat(sendBuffer, startTimeNanoSeconds);
    strcat(sendBuffer, "\n");

    bytesSent = sendto(clientSocket,
                    sendBuffer, strlen(sendBuffer),
                    0,
                    clientAddress->ai_addr, clientAddress->ai_addrlen);
    if (bytesSent < strlen(sendBuffer) ) {
        fprintf(stderr, "bruh. Line: %d\n", __LINE__);
        enableEcho();
        exit(1);
    }

    printf("Sent start time: %s,%s\n", startTimeSeconds, startTimeNanoSeconds);
    printf("Starting game\n");
    //////////////// GAME LOOP ////////////////////
    #define DEBUG false
    while (true) {
        clock_gettime(CLOCK_MONOTONIC, &currentTime);

        long long elapsedTime = diffMilli(&lastTime, &currentTime);
        //printf("elapsedTime = %lld\n", elapsedTime);

        bool shouldBuffer = true;
        if (elapsedTime > 400 && shouldBuffer) {
            getInput(player, shouldBuffer, player->hasTail);}

        if (elapsedTime < 1000) {
            continue;}

        lastTime = currentTime;

        shouldBuffer = false;
        getInput(player, shouldBuffer, player->hasTail);
        /*if (!gameStart) {
            if (player->xMov == 0 && player->yMov == 0) {
                continue;}
            gameStart = true;
        }*/

        addErrorMsgFormat(errorData, "Player at (%d, %d). Velocity = (%d, %d). PrevMov = (%d, %d)\n", 
                player->xPos, player->yPos, player->xMov, player->yMov, player->prevXMov, player->prevYMov);

        deathCheck(player, screen);

        // Update head
        player->xPos += player->xMov;
        player->yPos += player->yMov;

        updateTailMovement(player, &errorData, &screen);
        
        if (appleOnMap) {
            appleOnMap = addTailPieceIfApple(player, &errorData, screen);
        }

        // Draw head
        screen.map[player->yPos * screen.width + player->xPos] = 'O';
        if (!player->hasTail) {
            screen.map[(player->yPos - player->yMov) * screen.width + player->xPos - player->xMov] = ' ';
        }

        drawTail(player, &errorData, &screen);

        while (!appleOnMap) {
            appleOnMap = addApples(&screen);
        }

        printScreen(screen);

        #if DEBUG_GAME == true
        printErrorMessages(&errorData);
        #endif
    }
    freeaddrinfo(listenAddress);
    freeaddrinfo(clientAddress);
    CLOSE_SOCKET(listenSocket);
    CLOSE_SOCKET(clientSocket);
    #if defined(_WIN32)
    WSACleanup();
    #endif
    enableEcho();
    printf("Finished.\n");
}