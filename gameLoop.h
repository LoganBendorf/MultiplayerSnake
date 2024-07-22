#ifndef gameLoopFunc
#define gameLoopFunc

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
                struct timespec lastTime, struct timespec currentTime, int listenSocket, int otherSocket, struct addrinfo* otherAddress,
                struct sockaddr* otherAddressStorage, socklen_t otherLen, node* player, node* other, screenData screen
                );

#endif