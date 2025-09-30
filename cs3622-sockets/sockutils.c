#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include "sockutils.h"

#define RCVBUFSIZE 32

void DieWithError(char *errorMessage){
    perror(errorMessage);
    exit(1);
}

void HandleTCPClient(int clntSocket) {
    char echoBuffer[RCVBUFSIZE];
    int recvMsgSize;

    if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
        DieWithError("recv() failed");

    while (recvMsgSize > 0) {
        echoBuffer[recvMsgSize] = '\0';
        printf("> [CLIENT]: %s\n", echoBuffer);

        reverseString(echoBuffer, recvMsgSize);

        if (send(clntSocket, echoBuffer, recvMsgSize, 0) != recvMsgSize)
            DieWithError("send() failed");

        if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0) 
            DieWithError("recv() failed");
    }

    close(clntSocket);
}

void reverseString(char *str, int len) {
    int l = 0;
    int r = len-1;
    while (l < r) {
        char tmp = str[l];
        str[l] = str[r];
        str[r] = tmp;
        l++;
        r--;
    }
}