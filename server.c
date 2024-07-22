
#define DEBUG_START_INFO false

#define MIN_WIDTH 6
#define MAX_WIDTH 30
#define MIN_HEIGHT 6
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

    printf("Connected to client. Client port: %d\n", clientPortAsInt);

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

    int randomX = rand() % (screen.width - 4);
    int randomY = rand() % ((screen.height - 4) / 2);
    randomX += 2;
    randomY += 2;
    int serverStartingPosition = randomY * screen.width + randomX;
    char charServerStartingPosition[8] = {0};
    snprintf(charServerStartingPosition, 8, "%d", serverStartingPosition);

    strcat(sendBuffer, "\nserverStartingPosition: ");
    strcat(sendBuffer, charServerStartingPosition);


    randomX = rand() % (screen.width - 4);
    randomY = rand() % ((screen.height - 4) / 2 + screen.height / 2);
    randomX += 1;
    randomY += 1;
    int clientStartingPosition = randomY * screen.width + randomX;
    char charClientStartingPosition[8] = {0};
    snprintf(charClientStartingPosition, 8, "%d", clientStartingPosition);
                          
    strcat(sendBuffer, "\nclientStartingPosition: ");
    strcat(sendBuffer, charClientStartingPosition);

    strcat(sendBuffer, "\nrandomSeed: ");
    char stringRandomSeed[64] = {0};
    snprintf(stringRandomSeed, 64, "%d", randomSeed);
    strcat(sendBuffer, stringRandomSeed);
    strcat(sendBuffer, "\n");

    printf("sent random seed = %d\n", randomSeed);

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

    #if DEBUG_START_INFO != true
    //printScreen(screen);
    //exit(1);
    disableEcho();
    #endif

    printf("Initializing server player\n");
    node* player = (node*) Malloc(sizeof(node));
    player->xMov = 0;
    player->yMov = 0;
    player->prevXMov = 0;
    player->prevYMov = 0;
    player->xPos = serverStartingPosition % screen.width;
    player->yPos = serverStartingPosition / screen.width;
    player->next = NULL;
    player->hasTail = false;
    player->score = 0;
    player->id = 0;

    printf("Initializing client player\n");
    node* other = (node*) Malloc(sizeof(node));
    other->xMov = 0;
    other->yMov = 0;
    other->prevXMov = 0;
    other->prevYMov = 0;
    other->xPos = clientStartingPosition % screen.width;
    other->yPos = clientStartingPosition / screen.width;
    other->next = NULL;
    other->hasTail = false;
    other->score = 0;
    other-> id = 1;

    errorInfo errorData = {0, 0, 0};


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

    bool appleOnMap = false;
    bool gameStart = false;
    char* pointer;
    char* quinter;
    printf("Sent start time: %s,%s\n", startTimeSeconds, startTimeNanoSeconds);
    printf("Starting game\n");
    #if DEBUG_START_INFO == true
    enableEcho();
    exit(1);
    #endif
    //////////////// GAME LOOP ////////////////////
    #define DEBUG false
    int bytesReceived = 0;
    while (true) {
        char readBuffer[128] = {0};
        char sendBuffer[128] = {0};
        clock_gettime(CLOCK_MONOTONIC, &currentTime);

        long long elapsedTime = diffMilli(&lastTime, &currentTime);

        bool shouldBuffer = true;
        if (elapsedTime > 400 && shouldBuffer) {
            getInput(player, shouldBuffer, player->hasTail);}

        if (bytesReceived <= 0) {
            bytesReceived = recvfrom(clientSocket, readBuffer, strlen(readBuffer), MSG_DONTWAIT, (struct sockaddr*) &clientAddressStorage, &clientLen);
        }

        if (elapsedTime < 1000) {
            continue;}

        lastTime = currentTime;

        shouldBuffer = false;      
        getInput(player, shouldBuffer, player->hasTail);

        snprintf(sendBuffer, 128, "%d,%d,", player->xMov, player->yMov);
        printf("sending %s\n", sendBuffer);
        int bytes = sendto(clientSocket,
                    sendBuffer, strlen(sendBuffer),
                    0,
                    clientAddress->ai_addr, clientAddress->ai_addrlen);
        // RECVFROM blocks btw
        printf("recvfrmo\n");
        if (bytesReceived <= 0) {
            bytesReceived = recvfrom(clientSocket, readBuffer, strlen(readBuffer), MSG_DONTWAIT, (struct sockaddr*) &clientAddressStorage, &clientLen);
        }
        printf("after\n");
        if (bytesReceived == 0) {
            printf("bruh\n");
            exit(1);
        }
        printf("received %d bytes: %s\n", bytesReceived, readBuffer);
        bytesReceived = 0;

        pointer = readBuffer;
        quinter = strstr(pointer, ",");
        char otherXMovString[8];
        snprintf(otherXMovString, quinter - pointer + 1, "%s", pointer);
        pointer = quinter + 1;
        quinter = strstr(pointer, ",");
        char otherYMovString[8];
        snprintf(otherYMovString, quinter - pointer + 1, "%s", pointer);

        other->prevXMov = other->xMov;
        other->prevYMov = other->yMov;

        other->xMov = atoi(otherXMovString);
        other->yMov = atoi(otherYMovString);

        if (other->hasTail) {
            if (other->xMov == -other->prevXMov) {
                other->xMov = other->prevXMov;
            }
            if (other->yMov == -other->prevYMov) {
                other->yMov = other->prevYMov;
            }
        }
     

        addErrorMsgFormat(errorData, "Player at (%d, %d). Velocity = (%d, %d). PrevMov = (%d, %d)\n", 
                player->xPos, player->yPos, player->xMov, player->yMov, player->prevXMov, player->prevYMov);

        deathCheck(player, screen);
        deathCheck(other, screen);

        // Update head
        player->xPos += player->xMov;
        player->yPos += player->yMov;
        other->xPos += other->xMov;
        other->yPos += other->yMov;

        updateTailMovement(player, &errorData, &screen);
        updateTailMovement(other, &errorData, &screen);
        
        if (appleOnMap) {
            appleOnMap = addTailPieceIfApple(player, &errorData, screen);
            appleOnMap = addTailPieceIfApple(other, &errorData, screen);
        }

        // Draw head
        screen.map[player->yPos * screen.width + player->xPos] = 'O';
        if (!player->hasTail && player->xMov != 0 && player->yMov != 0) {
            screen.map[(player->yPos - player->yMov) * screen.width + player->xPos - player->xMov] = ' ';
        }
        screen.map[other->yPos * screen.width + other->xPos] = 'O';
        if (!other->hasTail && other->xMov != 0 && other->yMov != 0) {
            screen.map[(other->yPos - other->yMov) * screen.width + other->xPos - other->xMov] = ' ';
        }

        drawTail(player, &errorData, &screen);
        drawTail(other, &errorData, &screen);

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