
#include "errorFunctions.h"

// Technically a helper function
void* Malloc(int size) {
    void* ptr = malloc(size);
    if (ptr == NULL) {
        printf("Malloc fail\n");
        exit(1);
    }
    return ptr;
}
void* Calloc(int size) {
    void* ptr = calloc(1, size);
    if (ptr == NULL) {
        printf("Calloc fail\n");
        exit(1);
    }
    return ptr;
}

void addErrorMsgFunc(errorInfo* errorData) {

    if (errorData->errorCount < MAX_ERROR_MESSAGES) {
        errorData->errorMessages[errorData->errorCount] = (char*) Calloc (MAX_ERRMSG_LENGTH);
        strncpy(errorData->errorMessages[errorData->errorCount], errorData->msg, MAX_ERRMSG_LENGTH - 1);
    } else {
        errorData->errorMessages[0] = "TOO MANY ERRORS\n";
    }
}

void printErrorMessages(errorInfo* errorData) {
    for (int i = 0; i < errorData->errorCount; i++) {
        if (errorData->errorMessages[i] != NULL) {
            printf("%s", errorData->errorMessages[i]);
            free(errorData->errorMessages[i]);
            errorData->errorMessages[i] = NULL;
        }
    }
    errorData->errorCount = 0;
}
