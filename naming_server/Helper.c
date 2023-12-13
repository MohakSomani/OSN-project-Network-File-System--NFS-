#include "NS_header.h"

// Function to check if a socket is still open for writing
bool IsSocketConnectedW(int socket) {
    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(socket, &writeSet);

    struct timeval timeout;
    timeout.tv_sec = 0;  // seconds
    timeout.tv_usec = 0; // microseconds

    // Using select to check if the socket is open for writing
    int result = select(socket + 1, NULL, &writeSet, NULL, &timeout);

    if (result < 0) {
        // Handle error, e.g., by returning false
        return false;
    } else if (result == 0) {
        // Timeout occurred, the socket is not open for writing
        return false;
    }

    // If we reach here, the socket is open for writing
    return true;
}

// Function to check if a socket is still open for reading
bool IsSocketConnectedR(int socket) {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(socket, &readSet);

    struct timeval timeout;
    timeout.tv_sec = 0;  // seconds
    timeout.tv_usec = 0; // microseconds

    // Using select to check if the socket is open for reading
    int result = select(socket + 1, &readSet, NULL, NULL, &timeout);

    if (result < 0) {
        // Handle error, e.g., by returning false
        return false;
    } else if (result == 0) {
        // Timeout occurred, the socket is not open for reading
        return false;
    }

    // If we reach here, the socket is open for reading
    return true;
}

// Wrapper for catching errors
void CheckError(int nRet, char* msg)
{
    if (nRet < 0)
    {
        fprintf(stderr, "%s failed with error %d: ", msg, nRet);
        perror("");
        exit(EXIT_FAILURE);  // Use EXIT_FAILURE instead of -1 for consistency
    }
}
