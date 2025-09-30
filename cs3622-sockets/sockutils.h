#ifndef SOCKUTILS_H
#define SOCKUTILS_H

void DieWithError(char *errorMessage);
void HandleTCPClient(int clntSocket);
void reverseString(char *str, int len);

#endif // sockutils
