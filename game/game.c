#include <stdio.h>
#include <string.h>
#include <time.h>
#include <raylib.h>
#include "network.h"

#define PORT 55555

#define MAX_MOLES 5
#define BORDER_BUFFER 100
#define PLAYER_SIZE 32
#define MOLE_SIZE 96
#define GRASS_SIZE 64
#define PLAYER_BUFFER (BORDER_BUFFER + PLAYER_SIZE)
#define MOLE_BUFFER (BORDER_BUFFER + MOLE_SIZE)
#define TIMEOUT 3.0f
#define TIMERESPAWN 3.0f
#define ALERTTIME 2.0f
#define CONVERT_SCALE_X 8.68f
#define CONVERT_SCALE_Y 12.4f

int screenWidth= 2000;
int screenHeight= 2000;

typedef enum GameScreen { MENU, GAMEPLAY } GameScreen;

typedef struct {
    int active;
    Vector2 pos;
    float time;
} Mole;

typedef struct {
    int active;
    char text[64];
    Color color;
    float time;
} GameAlert;

GameAlert currentAlert= { 0 };

void pauseScreen() {
    DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, 100});

    int barWidth= 100;
    int barHeight= 400;
    int gap= 50;
    int totalWidth= (barWidth * 2) + gap;
    int startX= (screenWidth - totalWidth) / 2;
    int startY= (screenHeight - barHeight) / 2;
    int fontSize= 100;
    char* text= "PAUSED";
    int textWidth= MeasureText(text, fontSize);
    int textX= (screenWidth - textWidth) / 2;
    int textY= (screenHeight - fontSize) / 2;

    Color barColor= { 255, 255, 255, 100 };

    DrawRectangle(startX, startY, barWidth, barHeight, barColor);
    DrawRectangle(startX + barWidth + gap, startY, barWidth, barHeight, barColor);
    DrawText(text, textX, textY, fontSize, WHITE);
}

void gameoverScreen() {
    int fontSize= 100;
    char* text= "GAME OVER";
    DrawRectangle(0, 0, screenWidth, screenHeight, (Color){255, 0, 0, 100});

    int textWidth= MeasureText(text, fontSize);
    int textX= (screenWidth - textWidth) / 2;
    int textY= (screenHeight - fontSize) / 2;

    DrawText(text, textX, textY, fontSize, RED);
}

void spawnMole(Mole *mole) {
    mole->active= true;
    mole->pos.x= GetRandomValue(MOLE_BUFFER, screenWidth - MOLE_BUFFER);
    mole->pos.y= GetRandomValue(MOLE_BUFFER, screenHeight - MOLE_BUFFER);
    mole->time= TIMEOUT;
}

void despawnMole(Mole *mole) {
    mole->active= false;
    mole->time= TIMERESPAWN;
}

void triggerAlert(const char* text, Color color) {
    snprintf(currentAlert.text, sizeof(currentAlert.text), "%s", text);
    currentAlert.color= color;
    currentAlert.time= ALERTTIME;
    currentAlert.active= true;
}

void updateAlert(GameAlert *alert) {
    if (!alert->active) return;
    alert->time -= GetFrameTime();
    if (alert->time <= 0.0f) {
        alert->active= false;
    }
}

void drawAlert(const GameAlert *alert, int screenWidth, int screenHeight) {
    if (!alert->active) { return; }
    int fontSize= 150;
    int textWidth= MeasureText(alert->text, fontSize);
    int textX= (screenWidth - textWidth) / 2;
    int textY= (screenHeight - fontSize) / 2;
    Color alertColor= alert->color;
    alertColor.a= alert->time / ALERTTIME * 255.0f;
    DrawText(alert->text, textX, textY, fontSize, alertColor);
}

int main(void)
{
    InitWindow(screenWidth, screenHeight, "Whack-a-Mole");
    initUDP(PORT);

    GameScreen screen= MENU;
    Vector2 playerCursor= {screenWidth / 2, screenHeight / 2};
    PlayerLoc pl= { 0 };
    Mole moles[MAX_MOLES];
    int scorereq[]= {1, 1, 1, 1, 1, 1, 1, 1, 1, 1000};
    float moleRadius= MOLE_SIZE;
    int score= 0;
    int level= 1;
    int count= 1;
    int pause= false;
    int gameover= false;
    float roundtimer= 30.0f;

    RenderTexture2D moleSprite= LoadRenderTexture(MOLE_SIZE, MOLE_SIZE);

    BeginTextureMode(moleSprite);
        ClearBackground(BLANK);

        DrawEllipse(moleRadius / 2.0f, moleRadius / 1.28f, moleRadius / 2.0f, moleRadius / 4.0f, DARKGRAY);
        DrawRectangle(moleRadius / 4.0f, moleRadius / 1.75f, moleRadius / 2.0f, moleRadius / 2.28f, BROWN);
        DrawCircle(moleRadius / 2.0f, moleRadius / 2.0f, moleRadius / 4.0f, BROWN);

        DrawCircle(moleRadius / 2.0f - (moleRadius / 7.53f), moleRadius / 2.0f, moleRadius / 16.0f, BLACK);
        DrawCircle(moleRadius / 2.0f + (moleRadius / 7.53f), moleRadius / 2.0f, moleRadius / 16.0f, BLACK);
        DrawCircle(moleRadius / 2.0f - (moleRadius / 6.4f), moleRadius / 2.0f + moleRadius / 32.0f, moleRadius / 32.0f, WHITE);
        DrawCircle(moleRadius / 2.0f + (moleRadius / 9.14f), moleRadius / 2.0f + moleRadius / 32.0f, moleRadius / 32.0f, WHITE);

        DrawTriangle(
        (Vector2){ moleRadius / 2.0f, moleRadius / 1.3f },
        (Vector2){ moleRadius / 2.0f + (moleRadius / 12.8f), moleRadius / 1.75f },
        (Vector2){ moleRadius / 2.0f - (moleRadius / 12.8f), moleRadius / 1.75f },
        DARKBROWN
        );

        DrawLine(moleRadius / 2.0f - (moleRadius / 12.8f), moleRadius / 1.5f, moleRadius / 2.0f - (moleRadius / 5.33f), moleRadius / 1.6f, BLACK);
        DrawLine(moleRadius / 2.0f - (moleRadius / 12.8f), moleRadius / 1.4f, moleRadius / 2.0f - (moleRadius / 5.33f), moleRadius / 1.4f, BLACK);
        DrawLine(moleRadius / 2.0f - (moleRadius / 12.8f), moleRadius / 1.3f, moleRadius / 2.0f - (moleRadius / 5.33f), moleRadius / 1.25f, BLACK);

        DrawLine(moleRadius / 2.0f + (moleRadius / 12.8f), moleRadius / 1.5f, moleRadius / 2.0f + (moleRadius / 5.33f), moleRadius / 1.6f, BLACK);
        DrawLine(moleRadius / 2.0f + (moleRadius / 12.8f), moleRadius / 1.4f, moleRadius / 2.0f + (moleRadius / 5.33f), moleRadius / 1.4f, BLACK);
        DrawLine(moleRadius / 2.0f + (moleRadius / 12.8f), moleRadius / 1.3f, moleRadius / 2.0f + (moleRadius / 5.33f), moleRadius / 1.25f, BLACK);
    EndTextureMode();

    RenderTexture2D grassSprite= LoadRenderTexture(GRASS_SIZE, GRASS_SIZE);

    BeginTextureMode(grassSprite);
        ClearBackground((Color){40, 150, 40, 255});

        DrawRectangle(0, 0, GRASS_SIZE / 4, GRASS_SIZE / 4, (Color){40, 154, 40, 255});
        DrawRectangle(GRASS_SIZE / 4, GRASS_SIZE / 4, GRASS_SIZE / 4, GRASS_SIZE / 4, (Color){40, 146, 40, 255});
    EndTextureMode();

    SetTargetFPS(60);

    while(!WindowShouldClose())
    {
        switch(screen) 
        {
            case MENU:
            {
                score= 0;
                level= 1;
                count= 1;
                pause= false;
                gameover= false;
                roundtimer= 30.0f;
                for (int i= 0; i < MAX_MOLES; i++) {
                    spawnMole(&moles[i]);
                }
                if (IsKeyPressed(KEY_ENTER)) { screen= GAMEPLAY; }
            } break;

            case GAMEPLAY:
            {
                if(receivePlayerLoc(&pl)) {
                    playerCursor.x= (pl.loc[0] * CONVERT_SCALE_X) + 132;
                    playerCursor.y= (pl.loc[1] * CONVERT_SCALE_Y) - 612;
                }

                if(!pause && !gameover) {
                        updateAlert(&currentAlert);
                        for(int i= 0; i < count; i++) { moles[i].time-= GetFrameTime(); }
                        roundtimer-= GetFrameTime();
                    }

                    if(roundtimer <= 0.0f) {
                        gameover= true;
                    }

                    if(score >= scorereq[level - 1]) {
                        roundtimer= 30.0f;
                        score= 0;
                        level++;

                        if(level % 5== 0) {
                            triggerAlert("MOLE +1", GOLD);
                            } else {
                            triggerAlert("LEVEL +1", GOLD);
                            }
                        count= 1 + (level / 5);
                        if (count > MAX_MOLES) { count= MAX_MOLES; }
                    }

                if(IsKeyPressed(KEY_BACKSPACE)) pause= !pause;
                if(!pause && !gameover) {
                    if(IsKeyDown(KEY_UP))   playerCursor.y-= 10.0f;
                    if(IsKeyDown(KEY_DOWN)) playerCursor.y+= 10.0f;
                    if(IsKeyDown(KEY_LEFT)) playerCursor.x-= 10.0f;
                    if(IsKeyDown(KEY_RIGHT))playerCursor.x+= 10.0f;  

                    if (playerCursor.x < PLAYER_BUFFER) playerCursor.x= PLAYER_BUFFER;
                    if (playerCursor.x > screenWidth - PLAYER_BUFFER) playerCursor.x= screenWidth - PLAYER_BUFFER;
                    if (playerCursor.y < PLAYER_BUFFER) playerCursor.y= PLAYER_BUFFER;
                    if (playerCursor.y > screenHeight - PLAYER_BUFFER) playerCursor.y= screenHeight - PLAYER_BUFFER;

                    for(int i= 0; i< count; i++) {
                        if(moles[i].time<= 0 && moles[i].active== true) 
                        {
                        despawnMole(&moles[i]);
                        } else if (moles[i].time <= 0 && moles[i].active== false){
                        spawnMole(&moles[i]);
                        }

                        if(CheckCollisionCircles(playerCursor, PLAYER_SIZE, moles[i].pos, moleRadius) && moles[i].active)
                        {
                            score++;
                            despawnMole(&moles[i]);
                        }
                    }
                }
            } break;
        }

        BeginDrawing();
            ClearBackground(RAYWHITE);
            switch(screen) {
                case MENU:
                {
                    int fontSize= 100;
                    DrawText("WHACK-A-MOLE", screenWidth / 2 - MeasureText("WHACK-A-MOLE", fontSize) / 2, screenHeight / 2.5f, fontSize, DARKGRAY);
                    DrawText("Press ENTER to start", screenWidth / 2 - MeasureText("Press ENTER to Start", (fontSize / 2)) / 2, screenHeight / 2, (fontSize / 2), GRAY);
                } break;

                case GAMEPLAY:
                {
                    ClearBackground(BLACK);

                    for (int x= BORDER_BUFFER; x < screenWidth - BORDER_BUFFER - GRASS_SIZE; x += grassSprite.texture.width) {
                        for (int y= BORDER_BUFFER; y < screenHeight - BORDER_BUFFER - GRASS_SIZE; y += grassSprite.texture.height) {
                            DrawTexture(grassSprite.texture, x, y, WHITE);
                        }
                    }

                    Rectangle fence= { BORDER_BUFFER, BORDER_BUFFER, screenWidth - (BORDER_BUFFER * 2), screenHeight - (BORDER_BUFFER * 2) };
                    DrawRectangleLinesEx(fence, 10, BROWN);

                    for(int i= 0; i < count; i++) {
                        Rectangle sourceRec= { 0, 0, moleSprite.texture.width, -moleSprite.texture.height };
                        Rectangle destRec= { moles[i].pos.x, moles[i].pos.y, MOLE_SIZE * 2, MOLE_SIZE * 2 };
                        Vector2 origin= { MOLE_SIZE, MOLE_SIZE };
                        Color c= DARKGRAY;
                        if(moles[i].active) {
                            DrawTexturePro(moleSprite.texture, sourceRec, destRec, origin, 0, WHITE);
                            c.r= (1 - moles[i].time / TIMEOUT) * 255.0f;
                            c.g= moles[i].time / TIMEOUT * 80.0f;
                            c.b= moles[i].time / TIMEOUT * 80.0f;
                            DrawText(TextFormat("%.1f", moles[i].time), moles[i].pos.x - MeasureText(TextFormat("%.1f", moles[i].time), MOLE_SIZE / 2) / 2, moles[i].pos.y - MOLE_SIZE, MOLE_SIZE / 2, c);
                            // DrawCircleV(moles[i].pos, moleRadius, (Color){255, 0, 0, 50});
                        }
                    }

                    drawAlert(&currentAlert, screenWidth, screenHeight);

                    DrawCircleV(playerCursor, PLAYER_SIZE, BLUE);
                    DrawText("PLAYER", playerCursor.x - (MeasureText("PLAYER", PLAYER_SIZE) / 2), playerCursor.y - PLAYER_SIZE * 2, PLAYER_SIZE, DARKBLUE);

                    int fontSize= 30;
                    int buffer= fontSize;
                    DrawText(TextFormat("Score: %i", score), fontSize, 20, fontSize, WHITE);
                    buffer= MeasureText((TextFormat("[%.0f, %.0f]", playerCursor.x, playerCursor.y)), fontSize) + 20;
                    DrawText(TextFormat("[%.0f, %.0f]", playerCursor.x, playerCursor.y), screenWidth - buffer, screenHeight - fontSize, fontSize, WHITE);
                    buffer= fontSize;
                    buffer+= MeasureText((TextFormat("Level: %i", level)), fontSize);
                    DrawText(TextFormat("Level: %i", level), screenWidth - buffer, 20, fontSize, WHITE);
                    buffer+= MeasureText((TextFormat("Moles: %i", count)), fontSize) + fontSize;
                    DrawText(TextFormat("Moles: %i", count), screenWidth - buffer, 20, fontSize, WHITE);
                    buffer+= MeasureText((TextFormat("Time: %.0f", roundtimer)), fontSize) + fontSize;
                    DrawText(TextFormat("Time: %.0f", roundtimer), screenWidth - buffer, 20, fontSize, WHITE);

                    if(gameover) { 
                        gameoverScreen();
                        if (IsKeyPressed(KEY_ENTER)) { screen= MENU; }
                    }
                    if(pause && !gameover) { pauseScreen(); }
                } break;
            }
        EndDrawing();
    }
    UnloadRenderTexture(moleSprite);
    UnloadRenderTexture(grassSprite);
    closeUDP();
    CloseWindow();
    return false;
}

// gcc game.c network.c -o ./game.exe -IC:/raylib/raylib/src -LC:/raylib/raylib/src -lraylib -lopengl32 -lgdi32 -lwinmm -lws2_32