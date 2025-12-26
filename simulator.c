#include <SDL3/SDL.h>
#include <stdbool.h>
#include <windows.h>
#include <process.h> 
#include <stdio.h> 
#include <string.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
#define ROAD_WIDTH 200
#define LANE_WIDTH 50

#define PIPE_NAME "\\\\.\\pipe\\VehicleQueuePipe"
#define MAX_TEXT 100
#define MAX_SIZE 100
#define STR_LEN 15

typedef struct {
    char id[STR_LEN];
    float progress; 
}Vehicle;

typedef struct {
    Vehicle data[MAX_SIZE];
    int front;
    int rear;
} Queue;

typedef struct {
    int nextLight;
    int priority;
    int lightPhase; 
    Queue traffic[4][3];
} SharedData;

bool initializeSDL(SDL_Window **window, SDL_Renderer **renderer);
void drawRoadsAndLanes(SDL_Renderer *renderer);
void drawLightForA(SDL_Renderer* renderer, bool isRed ,bool lightPhase);
void drawLightForB(SDL_Renderer* renderer, bool isRed,bool lightPhase);
void drawLightForC(SDL_Renderer* renderer, bool isRed,bool lightPhase);
void drawLightForD(SDL_Renderer* renderer, bool isRed,bool lightPhase);
void refreshLight(SDL_Renderer *renderer, SharedData* sharedData);
void drawVehicles(SDL_Renderer* renderer, SharedData* sharedData);

void initQueue(Queue* q);
void enqueue(Queue *q, char value[]);
void dequeue(Queue* q);
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
    sharedData.nextLight = 0;
    sharedData.priority = 4;
    sharedData.lightPhase = 0;
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 3; j++) {
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

        SDL_SetRenderDrawColor(renderer, 34, 139, 34, 255);
        SDL_RenderClear(renderer);
        
        drawRoadsAndLanes(renderer);
        drawVehicles(renderer, &sharedData);
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
                int lane;
                if(out + 1 == in || (out == 4 && in == 1)){
                    lane=0;
                }
                else if(out == in + 1 || (out == 1 && in == 4)){
                    lane=2;
                }
                else{
                    lane=1;
                }
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
        int total_cars=0,avg_cars=0,greenlight_time;
        for (int r = 0; r < 4; r++) {
            for (int l = 1; l < 3; l++) {  
                Queue* q = &data->traffic[r][l];
                if (!isEmpty(q)) {
                    total_cars += (q->rear - q->front) + 1;
                }
            }
        }
        avg_cars=(total_cars+2)/3;
        greenlight_time=avg_cars*2000;
        bool foundCongestion = false;
        if(data->priority==4){
            data->lightPhase = 0;
            Sleep(greenlight_time);
            data->lightPhase = 1;
            Sleep(2000);
            data->nextLight=(data->nextLight+1)%4;
        }
        else{
            data->nextLight=data->priority;
            data->priority=4;
            data->lightPhase = 0;
            Sleep(greenlight_time);
        }
        if (!foundCongestion) {
            data->priority = 4;
        }
        for (int r = 0; r < 4; r++) {
            int count = 0;
            for (int l = 1; l < 3; l++) {  
                Queue* q = &data->traffic[r][l];
                
                
                if (!isEmpty(q)) {
                    count += (q->rear - q->front) + 1;
                }

                if (count > 5) {
                    foundCongestion = true;
                    data->priority = r; 
                }
            }
        }
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
        strcpy(q->data[q->rear].id, value);
        q->data[q->rear].progress = 0.0f; 
    }
}

void dequeue(Queue* q) {
    if (!isEmpty(q)) {
        q->front++;
        if (q->front > q->rear) {
            q->front = -1;
            q->rear = -1;
        }
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

    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    for(int i = 0; i <= 3; i++){
        float offset = WINDOW_HEIGHT/2.0f - ROAD_WIDTH/2.0f + LANE_WIDTH*i;
        SDL_RenderLine(renderer, 0, offset, WINDOW_WIDTH/2.0f - ROAD_WIDTH/2.0f, offset);
        SDL_RenderLine(renderer, WINDOW_WIDTH/2.0f + ROAD_WIDTH/2.0f, offset, (float)WINDOW_WIDTH, offset);
        
        float vOffset = WINDOW_WIDTH/2.0f - ROAD_WIDTH/2.0f + LANE_WIDTH*i;
        SDL_RenderLine(renderer, vOffset, 0, vOffset, WINDOW_HEIGHT/2.0f - ROAD_WIDTH/2.0f);
        SDL_RenderLine(renderer, vOffset, WINDOW_HEIGHT/2.0f + ROAD_WIDTH/2.0f, vOffset, (float)WINDOW_HEIGHT);
    }
}

void drawLightForA(SDL_Renderer* renderer, bool isRed,bool lightPhase) {
    SDL_FRect box = {265, 140, 30, 60}; 
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderFillRect(renderer, &box);

    if (!isRed || lightPhase == 1) {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    }
    
    SDL_FRect bulb = {270, 150, 20, 40};
    SDL_RenderFillRect(renderer, &bulb);
}

void drawLightForB(SDL_Renderer* renderer, bool isRed,bool lightPhase) {
    SDL_FRect box = {140, 505, 60, 30}; 
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderFillRect(renderer, &box);

    if (!isRed || lightPhase == 1) {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    }
    
    SDL_FRect bulb = {150, 510, 40, 20};
    SDL_RenderFillRect(renderer, &bulb);
}

void drawLightForC(SDL_Renderer* renderer, bool isRed,bool lightPhase) {
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_FRect box = {505, 600, 30, 60};
    SDL_RenderFillRect(renderer, &box);

    if (!isRed || lightPhase == 1) {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    }
    
    SDL_FRect bulb = {510, 610, 20, 40};
    SDL_RenderFillRect(renderer, &bulb);
}

void drawLightForD(SDL_Renderer* renderer, bool isRed,bool lightPhase) {
    SDL_FRect box = {600, 265, 60, 30}; 
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderFillRect(renderer, &box);

    if (!isRed || lightPhase == 1) {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    }
    
    SDL_FRect bulb = {610, 270, 40, 20};
    SDL_RenderFillRect(renderer, &bulb);
}



void refreshLight(SDL_Renderer *renderer, SharedData* sharedData) {
    drawLightForA(renderer, (sharedData->nextLight == 0), sharedData->lightPhase);
    drawLightForB(renderer, (sharedData->nextLight == 1), sharedData->lightPhase);
    drawLightForC(renderer, (sharedData->nextLight == 2), sharedData->lightPhase);
    drawLightForD(renderer, (sharedData->nextLight == 3), sharedData->lightPhase);
}

void drawVehicles(SDL_Renderer* renderer, SharedData* sharedData) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    float speed = 2.0f;
    float stopLineDistance = 260.0f;
    float turnPoint0 = 310.0f;
    float turnPoint2 = 410.0f;

    for (int road = 0; road < 4; road++) {
        for (int lane = 0; lane < 3; lane++) {
            Queue* q = &sharedData->traffic[road][lane];
            if (isEmpty(q)) continue;

            for (int i = q->front; i <= q->rear; i++) {
                Vehicle* v = &q->data[i];
                int actualPosInLine = 0;
                float targetProgress = 1500.0f; 

                if (lane == 0) SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);  
                else if (lane == 1) SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);    
                else if (lane == 2) SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255); 
                if (lane != 0) {
                    for (int j = sharedData->traffic[road][1].front; j <= sharedData->traffic[road][1].rear; j++) {
                        if (sharedData->traffic[road][1].data[j].progress > v->progress) actualPosInLine++;
                    }
                    for (int j = sharedData->traffic[road][2].front; j <= sharedData->traffic[road][2].rear; j++) {
                        if (sharedData->traffic[road][2].data[j].progress > v->progress) actualPosInLine++;
                    }
                    targetProgress = stopLineDistance - (actualPosInLine * 40.0f);
                }
                if(lane==0||(sharedData->nextLight==road||v->progress>stopLineDistance)){
                    targetProgress=1500.0f;
                }
                if (v->progress < targetProgress) {
                    v->progress += speed;
                }
                SDL_FRect carRect;
                int displayLane = (lane != 0) ? 1 : 0; 

                if (lane == 0 && v->progress >= turnPoint0) {
                    float turnOffset = v->progress - turnPoint0;
                    if (road == 0)      carRect = (SDL_FRect){309 - turnOffset, turnPoint0, 30, 30};
                    else if (road == 1) carRect = (SDL_FRect){turnPoint0, 461 + turnOffset, 30, 30};
                    else if (road == 2) carRect = (SDL_FRect){461 + turnOffset, 800 - turnPoint0 - 30, 30, 30};
                    else if (road == 3) carRect = (SDL_FRect){800 - turnPoint0 - 30, 309 - turnOffset, 30, 30};
                }
                else if (lane == 2 && v->progress >= turnPoint2) {
                    float turnOffset = v->progress - turnPoint2;
                    if (road == 0)      carRect = (SDL_FRect){309 + 50 + turnOffset, turnPoint2, 30, 30};
                    else if (road == 1) carRect = (SDL_FRect){turnPoint2, 461 - 50 - turnOffset, 30, 30};
                    else if (road == 2) carRect = (SDL_FRect){461 - 50 - turnOffset, 800 - turnPoint2 - 30, 30, 30};
                    else if (road == 3) carRect = (SDL_FRect){800 - turnPoint2 - 30, 309 + 50 + turnOffset, 30, 30};
                }
                else {
                    if (road == 0)      carRect = (SDL_FRect){309 + (displayLane * 50), v->progress, 30, 30};
                    else if (road == 1) carRect = (SDL_FRect){v->progress, 461 - (displayLane * 50), 30, 30};
                    else if (road == 2) carRect = (SDL_FRect){461 - (displayLane * 50), 800 - v->progress - 30, 30, 30};
                    else if (road == 3) carRect = (SDL_FRect){800 - v->progress - 30, 309 + (displayLane * 50), 30, 30};
                }

                SDL_RenderFillRect(renderer, &carRect);
            }

            if (!isEmpty(q) && q->data[q->front].progress >= 1000.0f) {
                dequeue(q);
            }
        }
    }
}