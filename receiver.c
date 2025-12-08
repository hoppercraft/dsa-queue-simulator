// vehicle_receiver.c
#include <windows.h>
#include <stdio.h>

#define PIPE_NAME "\\\\.\\pipe\\VehicleQueuePipe"
#define MAX_TEXT 100

int main() {
    HANDLE pipe;
    char buffer[MAX_TEXT];
    DWORD bytesRead;

    // Create named pipe
    pipe = CreateNamedPipeA(
        PIPE_NAME,
        PIPE_ACCESS_INBOUND, // read-only
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,          // max instances
        MAX_TEXT,   // out buffer
        MAX_TEXT,   // in buffer
        0,
        NULL
    );

    if (pipe == INVALID_HANDLE_VALUE) {
        printf("CreateNamedPipe failed. Error %d\n", GetLastError());
        return 1;
    }

    printf("Waiting for sender to connect...\n");

    if (!ConnectNamedPipe(pipe, NULL)) {
        printf("ConnectNamedPipe failed. Error %d\n", GetLastError());
        CloseHandle(pipe);
        return 1;
    }

    printf("Sender connected!\n");

    while (1) {
        BOOL success = ReadFile(pipe, buffer, sizeof(buffer)-1, &bytesRead, NULL);
        if (!success || bytesRead == 0) {
            printf("ReadFile failed or sender disconnected. Error %d\n", GetLastError());
            break;
        }

        buffer[bytesRead] = '\0'; // Null-terminate
        printf("Received: %s\n", buffer);
    }

    CloseHandle(pipe);
    return 0;
}
