
#define DEBUG_START_INFO false

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <stdarg.h>

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
#include "gameLoop.h"

int main(int argc, char* argv[]) {

    if (argc > 2) {
        printf("Userage: ./snake port\n"); exit(1); }
    char localPort[16] = "9002";
    if (argc == 2) {
        if (strlen(argv[1]) > 6) {
            fprintf(stderr, "Port too long\n");
            exit(1);
        }
        strncpy(localPort, argv[1], 6);
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
    ////////// CONNECT TO SERVER /////////////
    // 1. Connect
    // 2. Set up map size (2 apples per map?)
    // 3. Decide player spawn
    // 4. receive info (map, player location, random seed)
    // 5. Set up and parse data
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
    printf("Client is running on %s:%d\n", readableIP, ntohs(sin.sin_port));

    // Connect
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_family = AF_INET;
    struct addrinfo *serverAddress;
    int maxLoops = 10;
    int numLoops = 0;
    while (true) {
        if (numLoops++ > maxLoops) {
            fprintf(stderr, "Too many loops getting server address. Line: %d\n", __LINE__);
            exit(1);
        }
        char ip[64] = {0};
        while (true) {
            printf("SERVER IP?\n");
            if (fgets(ip, 64, stdin) != NULL) {
                ip[strcspn(ip, "\n")] = 0;
                break;}
        }
        if (strncmp(ip, "localhost", 9 || strlen(ip) == 0)) {
            strcpy(ip, "0.0.0.0");
        }
        char port[16] = {0};
        while (true) {
            printf("SERVER PORT?\n");
            if (fgets(port, 16, stdin) != NULL) {
                port[strcspn(port, "\n")] = 0;
                break;}
        }
        if (strlen(port) == 0) {
            strcpy(port, "9001");
        }
        if (addrRC = getaddrinfo(ip, port, &hints, &serverAddress) != 0) {
            printf("Failed to connect. (rc: %d) Line: %d\n", addrRC, __LINE__);
            continue;
        }
        break;
    }
    printf("Connecting to server\n");

    SOCKET serverSocket;
    serverSocket = socket(serverAddress->ai_family, 
                        serverAddress->ai_socktype, serverAddress->ai_protocol);
    if (!IS_VALID_SOCKET(serverSocket)) {
        fprintf(stderr, "socket() failed. (%d, %s). Line: %d\n", SOCKET_ERRNO, GET_ERROR_STRING, __LINE__);
        exit(1);
    }

    int sum = 0;
    const char* createGame = "Create game";
    maxLoops = 100000000;
    numLoops = 0;
    while (sum < strlen(createGame)) {
        int bytes = sendto(serverSocket,
                    createGame, strlen(createGame),
                    0,
                    serverAddress->ai_addr, serverAddress->ai_addrlen);
        sum += bytes;
        if (numLoops++ >= maxLoops) {
            fprintf(stderr, "massive failure while sending 'Create game'. Line: %d\n", __LINE__);
            exit(1);
        }
    }
    printf("Acknowledgement sent, waiting for game info response...\n");

    struct sockaddr_storage serverAddressStorage;
    socklen_t serverLen = sizeof(serverAddressStorage);
    char read[1024] = {0};
    int totalBytesRead = 0;
    maxLoops = 100;
    numLoops = 0;
    while (true) {
        int bytes = recvfrom(serverSocket,
                    read + totalBytesRead, 1024 - totalBytesRead,
                    0,
                    (struct sockaddr*) &serverAddressStorage, &serverLen);
        totalBytesRead += bytes;
        if (totalBytesRead > 1024) {
            fprintf(stderr, "massive failure while receiving game state from server. Line: %d\n", __LINE__);
            exit(1);
        }
        if (numLoops > maxLoops) {
            fprintf(stderr, "massive failure while receiving game state from server. Line: %d\n", __LINE__);
            exit(1);
        }
        if (totalBytesRead > 80) {
            break;}
    }
    printf("Received:\n%s\n", read);
    printf("Received game state data from server, processing input...\n");

    screenData screen = {0, 0, 0};
    // Has no bounds checking with pointer but I don't care
    const char* screenSizeString = "screenSize: ";
    char* pointer = strstr(read, screenSizeString);
    if (pointer == 0) {
        fprintf(stderr, "Failed to find screen size. Line: %d\n", __LINE__);
        exit(1);
    }
    pointer += strlen(screenSizeString);

    char* quinter = strstr(pointer, "x");
    char temp[128] = {0};
    snprintf(temp, quinter - pointer + 1, "%s", pointer);
    screen.width = atoi(temp);
    printf("width = %d\n", screen.width);
    pointer = quinter + 1;
    quinter = strstr(read, "\n");
    snprintf(temp, quinter - pointer + 1, "%s", pointer);
    screen.height = atoi(temp);
    printf("height = %d\n", screen.height);

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

    const char* serverStartingPositionString = "serverStartingPosition: ";
    pointer = strstr(pointer, serverStartingPositionString);
    if (pointer == 0) {
        fprintf(stderr, "Failed to find sever starting position. Line: %d\n", __LINE__);
        exit(1);
    }
    pointer += strlen(serverStartingPositionString);
    quinter = strstr(pointer, "\n");
    snprintf(temp, quinter - pointer + 1, "%s", pointer);
    int serverStartingPosition = atoi(temp);
    printf("server starting position = %d\n", serverStartingPosition);

    screen.map[serverStartingPosition] = 'O';

    const char* clientStartingPositionString = "clientStartingPosition: ";
    pointer = strstr(pointer, clientStartingPositionString);
    if (pointer == 0) {
        fprintf(stderr, "Failed to find client starting position. Line: %d\n", __LINE__);
        exit(1);
    }
    pointer += strlen(clientStartingPositionString);
    quinter = strstr(pointer, "\n");
    snprintf(temp, pointer - quinter + 1, "%s", pointer);
    int clientStartingPosition = atoi(temp);
    printf("client starting position = %d\n", clientStartingPosition);

    screen.map[clientStartingPosition] = 'O';

    const char* randomSeedString = "randomSeed: ";
    pointer = strstr(pointer, randomSeedString);
    if (pointer == 0) {
        fprintf(stderr, "Failed to find random seed. Line: %d\n", __LINE__);
        exit(1);
    }
    pointer += strlen(randomSeedString);
    quinter = strstr(pointer, "\n");
    snprintf(temp, quinter - pointer + 1, "%s", pointer);
    int randomSeed = atoi(temp);
    printf("random seed = %d\n", randomSeed);
    my_srand(randomSeed);

    #if DEBUG_START_INFO != true
    //printScreen(screen);
    disableEcho();
    #endif

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

    node* other = (node*) Malloc(sizeof(node));
    other->xMov = 0;
    other->yMov = 0;
    other->prevXMov = 0;
    other->prevYMov = 0;
    other->xPos = serverStartingPosition % screen.width;
    other->yPos = serverStartingPosition / screen.width;
    other->next = NULL;
    other->hasTail = false;
    other->score = 0;

    errorInfo errorData = {0, 0, 0};

    bool appleOnMap = false;
    bool gameStart = false;

    // SYNC //
    printf("Processing complete, attempting to sync with server...\n");
    const char* sync = "Sync";
    sum = 0;
    maxLoops = 100;
    numLoops = 0;
    while (sum < strlen(sync)) {
        int bytes = sendto(serverSocket,
                    sync, strlen(sync),
                    0,
                    serverAddress->ai_addr, serverAddress->ai_addrlen);
        sum += bytes;
        if (numLoops++ >= maxLoops) {
            fprintf(stderr, "massive failure while sending 'Sync'. Line: %d\n", __LINE__);
            enableEcho();
            exit(1);
        }
    }
    printf("'Sync' sent to server, waiting for ack\n");
    memset(read, 0, strlen(read) - 1);
    totalBytesRead = 0;
    while (true) {
        int bytes = recvfrom(serverSocket,
                    read + totalBytesRead, 1024 - totalBytesRead,
                    0,
                    (struct sockaddr*) &serverAddressStorage, &serverLen);
        totalBytesRead += bytes;
        if (totalBytesRead > 1024) {
            fprintf(stderr, "massive failure while syncing. Line: %d\n", __LINE__);
            enableEcho();
            exit(1);
        }
        if (numLoops > maxLoops) {
            fprintf(stderr, "massive failure while syncing. Line: %d\n", __LINE__);
            enableEcho();
            exit(1);
        }
        if (totalBytesRead != 0) {
            break;
        }
    }

    printf("Received %s\n", read);

    const char* startTimeString = "startTime: ";
    pointer = strstr(read, startTimeString);
    if (pointer == 0) {
        fprintf(stderr, "Failed to start time. Line: %d\n", __LINE__);
        enableEcho();
        exit(1);
    }
    pointer += strlen(startTimeString);
    quinter = strstr(pointer, ",");
    snprintf(temp, quinter - pointer + 1, "%s", pointer);
    struct timespec lastTime, currentTime;
    lastTime.tv_sec = atoi(temp);
    //DO NSEC

    pointer = quinter + 1;
    quinter = strstr(pointer, "\n");
    snprintf(temp, quinter - pointer + 1, "%s", pointer);
    lastTime.tv_nsec = atoi(temp);

    printf("Receieved start time, starting game\n");

    #if DEBUG_START_INFO == true
    enableEcho();
    exit(1);
    #endif
    //////////////// GAME LOOP ////////////////////
    #define DEBUG false
    gameLoop    (
                &lastTime, &currentTime, listenSocket, serverSocket, &serverAddress,
                (struct sockaddr*) &serverAddressStorage, serverLen, &player, &other, screen, CLIENT
                );



    freeaddrinfo(listenAddress);
    freeaddrinfo(serverAddress);
    CLOSE_SOCKET(listenSocket);
    CLOSE_SOCKET(serverSocket);
    #if defined(_WIN32)
    WSACleanup();
    #endif
    enableEcho();
    printf("Finished.\n");
}