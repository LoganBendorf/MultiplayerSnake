
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
#include "host_and_connect.h"

int main(int argc, char* argv[]) {

    signal(SIGINT , catchSigThenExit);
    signal(SIGABRT , catchSigThenExit);
    signal(SIGILL , catchSigThenExit);
    signal(SIGFPE , catchSigThenExit);
    signal(SIGSEGV, catchSigThenExit); // <-- this one is for segmentation fault
    signal(SIGTERM , catchSigThenExit);

    while (true) {
        printf("    |OPTIONS|\n");
        printf("1. Connect\n");
        printf("2. Host\n");
        printf("3. Exit\n");
        printf(" ");
        char input[64] = {0};
        if (fgets(input, 64, stdin) == NULL) {
            continue;
        }

        if (input[0] == '1') {
            client_connect();
        } else if (input[0] == '2') {
            server_connect();
        } else if (input[0] == '3') {
            break;
        }
    }
    printf("Bye nerd\n");

}