// vehicle_sender.c
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define PIPE_NAME "\\\\.\\pipe\\VehicleQueuePipe"
#define MAX_TEXT 100

// Generate a random vehicle number
void generateVehicleNumber(char* buffer) {
    buffer[0] = 'A' + rand() % 26;
    buffer[1] = 'A' + rand() % 26;
    buffer[2] = '0' + rand() % 10;
    buffer[3] = 'A' + rand() % 26;
    buffer[4] = 'A' + rand() % 26;
    buffer[5] = '0' + rand() % 10;
    buffer[6] = '0' + rand() % 10;
    buffer[7] = '0' + rand() % 10;
    buffer[9] = '\0';
}

// Generate a random lane
char generateOutgoingDirection() {
    char lanes[] = {'0', '1', '2', '3'};
    return lanes[rand() % 4];
}

char generateIncomingDirection(char incoming) {
    char lanes[] = {'0', '1', '2', '3'};
    char outgoing;

    do {
        outgoing = lanes[rand() % 4];
    } while (outgoing == incoming); 

    return outgoing;
}

int main() {
    HANDLE pipe;
    DWORD bytesWritten;
    char message[MAX_TEXT];

    srand((unsigned int)time(NULL));

    printf("Connecting to receiver...\n");

    // Connect to the named pipe
    while (1) {
        pipe = CreateFileA(
            PIPE_NAME,
            GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
        );

        if (pipe != INVALID_HANDLE_VALUE) break;

        Sleep(500); // Retry every 0.5 seconds
    }

    
    printf("Connected to receiver!\n");

    while (1) {
        char vehicle[9];
        generateVehicleNumber(vehicle);
        char inlane = generateOutgoingDirection();
        char outlane = generateIncomingDirection(inlane);
        snprintf(message, MAX_TEXT, "%s:%c%c", vehicle, inlane,outlane);

        // Send message
        WriteFile(pipe, message, (DWORD)strlen(message), &bytesWritten, NULL);
        printf("Sent: %s\n", message);

        Sleep(1000); // wait 1 second
    }

    CloseHandle(pipe);
    return 0;
}
