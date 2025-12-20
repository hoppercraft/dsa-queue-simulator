// vehicle_receiver.c
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <SDL3/SDL.h>
#define PIPE_NAME "\\\\.\\pipe\\VehicleQueuePipe"
#define MAX_TEXT 100
#define MAX_SIZE 100
#define STR_LEN 15

typedef struct {
    char data[MAX_SIZE][STR_LEN];
    int front;
    int rear;
} Queue;

void initQueue(Queue* q) {
    q->front = -1;
    q->rear = -1;
}

int isFull(Queue* q) {
    return (q->rear == MAX_SIZE - 1);
}

int isEmpty(Queue* q) {
    return (q->front == -1 || q->front > q->rear);
}

void enqueue(Queue *q, char value[]) {
    if (isFull(q)) {
        printf("Queue is full.\n");
        return;
    }
    if (isEmpty(q)) {
        q->front = 0;
    }
    q->rear++;
    strcpy(q->data[q->rear], value);
    printf("Enqueued: %s\n", value);
}

void display(Queue *q) {
    if (isEmpty(q)) {
        printf("Queue is empty.\n");
        return;
    }
    printf("Queue elements: ");
    for (int i = q->front; i <= q->rear; i++) {
        printf("[%s] ", q->data[i]);
    }
    printf("\n");
}

int main() {
    HANDLE pipe;
    char buffer[MAX_TEXT];
    DWORD bytesRead;
    Queue traffic[4][2];

    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 2; j++) {
            initQueue(&traffic[i][j]);
        }
    }

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

        buffer[bytesRead] = '\0';
        int out = (buffer[9] - '0'); 
        int in  = (buffer[10] - '0');
        if(out+1 == in || (out == 4 && in == 1)){
            enqueue(&traffic[out][0],buffer);
            printf("%d,0\n",out);
            display(&traffic[out][0]);
        }
        else{
            enqueue(&traffic[out][1],buffer);
            printf("%d,1\n",out);
            display(&traffic[out][1]);
        }
    }

    CloseHandle(pipe);
    return 0;
}
