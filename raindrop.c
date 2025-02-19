#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_timer.h>
#include <stdbool.h>
#include <stdio.h>

#define FRAME_DELAY 3
#define WINDOWWIDTH 800
#define WINDOWHEIGHT 600

typedef struct {
    int dropDensityHorizontal;
    int dropDensityVertical;
    int dropLength;
    int dropSpacing;
    int dropDistance;
    double raindropProbability;
    double goldDropProbability;
    int bestScore;
} GameConfig;

void EventHandler(SDL_Event event, bool* running, bool* dragging, int* mouseOffsetX, int* mouseOffsetY, SDL_Rect* rect, int rectSpeed){
    while(SDL_PollEvent(&event)){
        if(event.type == SDL_QUIT){
            *running = false;
        }
        if(event.type == SDL_MOUSEBUTTONDOWN){
            if(event.button.x <= rect->x + rect->w && event.button.x >= rect->x && event.button.y <= rect->y + rect->h && event.button.y >= rect->y){
                *dragging = true;
                *mouseOffsetX = event.button.x - rect->x;
                *mouseOffsetY = event.button.y - rect->y;
            }
        }
        if(event.type == SDL_MOUSEBUTTONUP){
            *dragging = false;
        }
        if(event.type == SDL_MOUSEMOTION && *dragging){
            rect->x = event.motion.x - *mouseOffsetX;
            rect->y = event.motion.y - *mouseOffsetY;
        }
    }

    const Uint8* keys = SDL_GetKeyboardState(NULL);
    if(keys[SDL_SCANCODE_LEFT]) {
        if(rect->x > 0) {
            rect->x -= rectSpeed;
        }
    }
    if(keys[SDL_SCANCODE_RIGHT]) {
        if(rect->x + rect->w < WINDOWWIDTH) {
            rect->x += rectSpeed;
        }
    }
}

int main(int argc, char* argv[]){
    GameConfig config = {80, 6, 20, 100, 3, 0.15, 0.005, 10};
    if(SDL_Init(SDL_INIT_EVERYTHING) != 0){
        printf("error initializing SDL: %s\n", SDL_GetError());
        return 1;
    }
    
    SDL_Window* win = SDL_CreateWindow("Raindrop", 0, 0, WINDOWWIDTH, WINDOWHEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* rend = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    bool running = true;
    SDL_Event event;

    // generate a 2d array of all the drops
    // horizontaldropdesntiy is the amount of lateral drops seen in the image
    // config.dropDensityVertical is the amount of vertical drops in a single column at a time
    int horizontalDrops[config.dropDensityHorizontal][config.dropDensityVertical];
    for(int i = 0; i < config.dropDensityHorizontal; i++){
        for(int j = 0; j < config.dropDensityVertical; j++){
            horizontalDrops[i][j] = WINDOWHEIGHT + 1;
        }
    }

    bool dragging = false;
    int mouseOffsetX = 0;
    int mouseOffsetY = 0;
    SDL_Rect rect = {WINDOWWIDTH / 2 - 50, WINDOWHEIGHT * 4 / 5, 100, 20};
    const int rectSpeed = 3;
    int goldCoords[2] = {-1, -1};
    int goldCount = 0;
    int currentScore = 0;
    bool gameOver = false;
    while(running){
        EventHandler(event, &running, &dragging, &mouseOffsetX, &mouseOffsetY, &rect, rectSpeed);

        SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
        SDL_RenderClear(rend);
        
        SDL_SetRenderDrawColor(rend, 255, 255, 255, 255);
        SDL_RenderDrawRect(rend, &rect);
        SDL_SetRenderDrawColor(rend, 0, 0, 255, 255);

        //horizontal drop spacing calculation
        // hds = horizontal drop spacing
        int hds = (WINDOWWIDTH)/(config.dropDensityHorizontal);      
        gameOver = (goldCount == config.bestScore && -1 == goldCoords[0] && -1 == goldCoords[1]);
        if(gameOver) {
            running = false;
        }
        // setup vertical and horizontal drops
        for(int i = 0; i < config.dropDensityHorizontal; i++){
            
            bool dropInit = true;
            // ensure spacing between each drop
            for(int k = 0; k < config.dropDensityVertical; k++){
                if(horizontalDrops[i][k] < config.dropSpacing){
                    dropInit = false;
                }
            }
            
            // generates a new drop in the column if to start form the top if, a. drop doesn't already exist and b. no other drop is occupying starting location
            for(int j = 0; j < config.dropDensityVertical; j++){
                if(horizontalDrops[i][j] >= WINDOWHEIGHT && dropInit){
                    if(i == goldCoords[0] && j == goldCoords[1]){
                        goldCoords[0] = -1;
                        goldCoords[1] = -1;
                    }
                    if(rand() < (RAND_MAX * config.raindropProbability) && i != 0){
                        if(rand() < (RAND_MAX * config.goldDropProbability) && goldCoords[0] < 0 && goldCoords[1] < 0){
                            goldCoords[0] = i;
                            goldCoords[1] = j;
                            goldCount += 1;
                        }
                        horizontalDrops[i][j] = 0 - config.dropLength;
                        dropInit = false;
                    }  
                }
                // collision check with rect (player object)
                bool collision = (horizontalDrops[i][j] <= rect.y && horizontalDrops[i][j] + config.dropLength >= rect.y && hds*i <= rect.x + rect.w && hds*i >= rect.x);
                if(collision){
                    if(goldCoords[0] == i && goldCoords[1] == j){
                        currentScore += 1;
                    }           
                    horizontalDrops[i][j] = WINDOWHEIGHT + 1;
                }
                // checks if the drop for that index already exits within the window and then moves it down if it does
                if(horizontalDrops[i][j] < WINDOWHEIGHT){
                    if(i == goldCoords[0] && j == goldCoords[1]){
                        SDL_SetRenderDrawColor(rend, 255, 0, 0, 255);
                        SDL_RenderDrawLine(rend, hds*i, horizontalDrops[i][j], hds*i, horizontalDrops[i][j] + config.dropLength);
                        SDL_SetRenderDrawColor(rend, 0, 0, 255, 255);
                    }
                    else{
                        SDL_RenderDrawLine(rend, hds*i, horizontalDrops[i][j], hds*i, horizontalDrops[i][j] + config.dropLength);
                    }
                    horizontalDrops[i][j] = horizontalDrops[i][j] + config.dropDistance;
                }  
            }
        }
        printf("Your score is: %d/%d\n", currentScore, goldCount);

        SDL_RenderPresent(rend);
        SDL_Delay(FRAME_DELAY); 
    }
    printf("Your Final Score is: %d/%d\n", currentScore, goldCount);
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}