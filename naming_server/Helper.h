#ifndef __HELPER_H__
#define __HELPER_H__

bool IsSocketConnectedW(int socket);

bool IsSocketConnectedR(int socket);

void CheckError(int nRet, char *msg);

#endif