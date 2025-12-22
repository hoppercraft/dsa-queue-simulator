#include <SDL3/SDL.h>
#include <stdbool.h>
#include <windows.h>
#include <process.h> 
#include <stdio.h> 
#include <string.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
#define ROAD_WIDTH 150
#define LANE_WIDTH 50

#define PIPE_NAME "\\\\.\\pipe\\VehicleQueuePipe"
#define MAX_TEXT 100
#define MAX_SIZE 100
#define STR_LEN 15

typedef struct {
    char data[MAX_SIZE][STR_LEN];
    int front;
    int rear;
} Queue;

typedef struct {
    int currentLight; 
    int nextLight;
    Queue traffic[4][2];
} SharedData;

bool initializeSDL(SDL_Window **window, SDL_Renderer **renderer);
void drawRoadsAndLanes(SDL_Renderer *renderer);
void drawLightForB(SDL_Renderer* renderer, bool isRed);
void refreshLight(SDL_Renderer *renderer, SharedData* sharedData);

void initQueue(Queue* q);
void enqueue(Queue *q, char value[]);
int isFull(Queue* q);
int isEmpty(Queue* q);

unsigned __stdcall chequeQueue(void* arg);
unsigned __stdcall pipeListenerThread(void* arg);

int main() {
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;    
    SDL_Event event;    

    if (!initializeSDL(&window, &renderer)) {
        return -1;
    }

    SharedData sharedData;
    sharedData.currentLight = 0;
    sharedData.nextLight = 0;
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 2; j++) {
            initQueue(&sharedData.traffic[i][j]);
        }
    }
    
    _beginthreadex(NULL, 0, chequeQueue, &sharedData, 0, NULL);
    _beginthreadex(NULL, 0, pipeListenerThread, &sharedData, 0, NULL);

    bool running = true;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        
        drawRoadsAndLanes(renderer);
        refreshLight(renderer, &sharedData);
        
        
        SDL_RenderPresent(renderer);
        SDL_Delay(16); 
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

unsigned __stdcall pipeListenerThread(void* arg) {
    SharedData* sharedData = (SharedData*)arg;
    HANDLE pipe;
    char buffer[MAX_TEXT];
    DWORD bytesRead;

    pipe = CreateNamedPipeA(
        PIPE_NAME, PIPE_ACCESS_INBOUND,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1, MAX_TEXT, MAX_TEXT, 0, NULL
    );

    if (pipe == INVALID_HANDLE_VALUE) return 1;

    printf("Traffic Simulator: Waiting for sender on pipe...\n");
    if (ConnectNamedPipe(pipe, NULL)) {
        while (1) {
            BOOL success = ReadFile(pipe, buffer, sizeof(buffer)-1, &bytesRead, NULL);
            if (!success || bytesRead == 0) break;

            buffer[bytesRead] = '\0';
            
            int out = (buffer[9] - '0'); 
            int in  = (buffer[10] - '0');
            
            if(out >= 0 && out < 4) {
                int lane = (out + 1 == in || (out == 4 && in == 1)) ? 0 : 1;
                enqueue(&sharedData->traffic[out][lane], buffer);
                printf("%s Added vehicle to Road %d, Lane %d\n",buffer, out, lane);
            }
        }
    }
    CloseHandle(pipe);
    return 0;
}

unsigned __stdcall chequeQueue(void* arg) {
    SharedData* data = (SharedData*)arg;
    while (1) {
        data->nextLight = 0; 
        Sleep(3000);         
        data->nextLight = 2; 
        Sleep(3000);
    }
    return 0;
}


void initQueue(Queue* q) { q->front = -1; q->rear = -1; }
int isFull(Queue* q) { return (q->rear == MAX_SIZE - 1); }
int isEmpty(Queue* q) { return (q->front == -1 || q->front > q->rear); }

void enqueue(Queue *q, char value[]) {
    if (!isFull(q)) {
        if (isEmpty(q)) q->front = 0;
        q->rear++;
        strcpy(q->data[q->rear], value);
    }
}


bool initializeSDL(SDL_Window **window, SDL_Renderer **renderer) {
    if (!SDL_Init(SDL_INIT_VIDEO)) return false;
    *window = SDL_CreateWindow("Traffic Simulator", WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (!*window) return false;
    *renderer = SDL_CreateRenderer(*window, NULL);
    return (*renderer != NULL);
}

void drawRoadsAndLanes(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
    SDL_FRect vRoad = {WINDOW_WIDTH/2.0f - ROAD_WIDTH/2.0f, 0, ROAD_WIDTH, (float)WINDOW_HEIGHT};
    SDL_RenderFillRect(renderer, &vRoad);
    SDL_FRect hRoad = {0, WINDOW_HEIGHT/2.0f - ROAD_WIDTH/2.0f, (float)WINDOW_WIDTH, ROAD_WIDTH};
    SDL_RenderFillRect(renderer, &hRoad);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for(int i = 0; i <= 3; i++){
        float offset = WINDOW_HEIGHT/2.0f - ROAD_WIDTH/2.0f + LANE_WIDTH*i;
        SDL_RenderLine(renderer, 0, offset, WINDOW_WIDTH/2.0f - ROAD_WIDTH/2.0f, offset);
        SDL_RenderLine(renderer, WINDOW_WIDTH/2.0f + ROAD_WIDTH/2.0f, offset, (float)WINDOW_WIDTH, offset);
        
        float vOffset = WINDOW_WIDTH/2.0f - ROAD_WIDTH/2.0f + LANE_WIDTH*i;
        SDL_RenderLine(renderer, vOffset, 0, vOffset, WINDOW_HEIGHT/2.0f - ROAD_WIDTH/2.0f);
        SDL_RenderLine(renderer, vOffset, WINDOW_HEIGHT/2.0f + ROAD_WIDTH/2.0f, vOffset, (float)WINDOW_HEIGHT);
    }
}

void drawLightForB(SDL_Renderer* renderer, bool isRed) {
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_FRect box = {480, 600, 30, 60};
    SDL_RenderFillRect(renderer, &box);

    if(isRed) SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    else SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    
    SDL_FRect bulb = {485, 610, 20, 40};
    SDL_RenderFillRect(renderer, &bulb);
}

void refreshLight(SDL_Renderer *renderer, SharedData* sharedData) {
    drawLightForB(renderer, (sharedData->nextLight == 0));
}