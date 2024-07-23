
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

#include "gameLogic.h"
#include "errorFunctions.h"
#include "timeHelpers.h"

void gameLoop   (
                struct timespec* lastTimePtr, struct timespec* currentTimePtr, int listenSocket, int otherSocket, struct addrinfo** otherAddressPtr,
                struct sockaddr* otherAddressStoragePtr, socklen_t otherLen, node** playerPtr, node** otherPtr, screenData screen, CLIENT_OR_SERVER cOs
                ) {
    struct timespec lastTime = *lastTimePtr;
    struct timespec currentTime = *currentTimePtr;

    struct addrinfo* otherAddress = *otherAddressPtr;
    //struct sockaddr otherAddressStorage = *otherAddressStoragePtr;

    node* player = *playerPtr;
    node* other = *otherPtr;
    
    errorInfo errorData = {0, 0, 0};
    bool appleOnMap = false;
    while (true) {
        #define BUFFER_SIZE 128
        char readBuffer[BUFFER_SIZE] = {0};
        char sendBuffer[BUFFER_SIZE] = {0};
        clock_gettime(CLOCK_MONOTONIC, &currentTime);

        long long elapsedTime = diffMilli(&lastTime, &currentTime);

        bool shouldBuffer = true;
        if (elapsedTime > 400 && shouldBuffer) {
            getInput(player, shouldBuffer, player->hasTail);}

        if (elapsedTime < 1000) {
            continue;}

        lastTime = currentTime;

        shouldBuffer = false;      
        getInput(player, shouldBuffer, player->hasTail);

        snprintf(sendBuffer, BUFFER_SIZE, "%d,%d,", player->xMov, player->yMov);
        if (strlen(sendBuffer) == 0) printf("sendbuffer empty\n"), exit(1);
        int bytesSent = 0;
        int bytesReceived = 0;
        if (cOs == CLIENT) {
            bytesSent = sendto(otherSocket, sendBuffer, strlen(sendBuffer), 0, otherAddress->ai_addr, otherAddress->ai_addrlen);
            if (bytesSent == 0) printf("bytesSent 0\n"), exit(1);
            printf("bytes were sent\n");
            bytesReceived = recvfrom(otherSocket, readBuffer, BUFFER_SIZE, 0, otherAddressStoragePtr, &otherLen);

        } else if (cOs == SERVER) {
            bytesReceived = recvfrom(listenSocket, readBuffer, BUFFER_SIZE, 0, otherAddressStoragePtr, &otherLen);
            bytesSent = sendto(otherSocket, sendBuffer, strlen(sendBuffer), 0, otherAddress->ai_addr, otherAddress->ai_addrlen);
            if (bytesSent == 0) printf("bytesSent 0\n"), exit(1);
            printf("bytes were sent\n");
        }

        printf("read buffer = (%s)\n", readBuffer);
        if (bytesReceived == 0) printf("bytesReceived 0\n"), exit(1);
        
        

        char* pointer = readBuffer;
        char* quinter = strstr(pointer, ",");
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

        deathCheck(player, screen, cOs);
        deathCheck(other, screen, !cOs);

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
}