
#ifndef errorFuncts
#define errorFuncts

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#define MAX_ERROR_MESSAGES 64
#define MAX_ERRMSG_LENGTH 128
typedef struct errorInfo {
    char* errorMessages[MAX_ERROR_MESSAGES];
    int errorCount;
    char msg[MAX_ERRMSG_LENGTH];
} errorInfo;

void* Malloc(int size);

void addErrorMsgFunc(errorInfo* errorData);

void printErrorMessages(errorInfo* errorData);

#define addErrorMsgFormat(errorPackage, message, ...) \
    do { \
        sprintf(errorPackage.msg, message, __VA_ARGS__); \
        addErrorMsgFunc(&errorPackage); \
        errorPackage.errorCount++; \
    } while (0)
#define addErrorMsg(errorPackage, message) \
    do { \
        sprintf(errorPackage.msg, message); \
        addErrorMsgFunc(&errorPackage); \
        errorPackage.errorCount++; \
    } while (0)

#endif