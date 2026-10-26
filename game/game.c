#include <stdio.h>
#include <string.h>
#include <time.h>
#include <raylib.h>
#include "network.h"
#include <math.h>

#define PORT 55555

#define MAX_MOLES 5
#define BORDER_BUFFER 50
#define PLAYER_SIZE 16
#define MOLE_SIZE 48
#define GRASS_SIZE 32
#define PLAYER_BUFFER (BORDER_BUFFER + PLAYER_SIZE)
#define MOLE_BUFFER (BORDER_BUFFER + MOLE_SIZE)
#define TIMEOUT 3.0f
#define TIMERESPAWN 3.0f
#define ALERTTIME 2.0f
#define CONVERT_SCALE_X 8.68f
#define CONVERT_SCALE_Y 12.4f

int screenWidth= 960;
int screenHeight= 960;

// Anchor sensor's physical locations for calculations (in cm) 
// Center point of the sensors are 11mm from the center of the box
const float SensPos[BOX_COUNT * SENSOR_COUNT][2]= {
{36.4f, 0.0f},
{38.6f, 0.0f}, // Box 1 centred at 37.5cm
{73.9f, 0.0f},
{76.1f, 0.0f}, // Box 2 centred at 75cm
{111.4f, 0.0f},
{113.6f, 0.0f} // Box 3 centred at 112.5cm
};

typedef enum GameScreen { MENU, GAMEPLAY, SETTINGS } GameScreen;

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

typedef struct {
    int width;
    int height;
    const char *label;
} ResolutionOption;

DataPayload dp;
PlayerLoc pl;

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

void drawAlert(const GameAlert *alert, int width, int height) {
    if (!alert->active) { return; }
    int fontSize= 150;
    int textWidth= MeasureText(alert->text, fontSize);
    int textX= (width - textWidth) / 2;
    int textY= (height - fontSize) / 2;
    Color alertColor= alert->color;
    alertColor.a= alert->time / ALERTTIME * 255.0f;
    DrawText(alert->text, textX, textY, fontSize, alertColor);
}

 // Trilateration algorithm supporting 3 to 6 sensors with Least Squares & Circle Intersection
void calculatePlayerLoc() {
  int valid_count= 0;
  for (int i= 0; i < BOX_COUNT; i++) {
    for (int j= 0; j < SENSOR_COUNT; j++) {
        if (dp.data[i].distance[j] > 0.0f) {
            valid_count++;
        }
      }
    }

  if (valid_count < 3) {
    return; // Need at least 3 sensors for a calculation
  }

  float best_x= 0.0f;
  float best_y= 0.0f;
  float min_err= 1e9f; // Start with a very high err

  // Playfield boundaries (in cm): X from 0 to 150, Y from (0) 60 to 200
  // Step size of 2.0 cm: 75 * 70= 5250 comparisons
  for (float x= 0.0f; x <= 150.0f; x+= 2.0f) {
    for (float y= 60.0f; y <= 200.0f; y+= 2.0f) { // 60cm deadzone
      float total_err= 0.0f;
      int active_comparisons= 0;

      for (int i= 0; i < BOX_COUNT; i++) {
        for (int j= 0; j < SENSOR_COUNT; j++) {
            if (dp.data[i].distance[j] > 0.0f) {
              int sens_idx= (i * SENSOR_COUNT) + j;
              float dx= x - SensPos[sens_idx][0];
              float dy= y - SensPos[sens_idx][1];

              float pred_dist_sq= (dx * dx) + (dy * dy);
              float target_dist= dp.data[i].distance[j];
              float target_dist_sq= target_dist * target_dist;
              float diff= pred_dist_sq - target_dist_sq;

              total_err+= diff * diff;
              active_comparisons++;
              }
            }
          }

      if (active_comparisons > 0) {
        float avg_err= total_err / active_comparisons;
        if (avg_err < min_err) {
          min_err= avg_err;
          best_x= x;
          best_y= y;
        }
      }
    }
  }
  // Update player location struct with the best grid match
  pl.loc[0]= best_x;
  pl.loc[1]= best_y;
  printf("Calculated grid coordinates: [%0.f, %0.f]\n", pl.loc[0], pl.loc[1]);
}

int main(void)
{
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(screenWidth, screenHeight, "Whack-a-Mole");
    initUDP(PORT);

    GameScreen screen= MENU;

    ResolutionOption resOptions[]= {
        { 960, 960, "960 x 960"},
        { 1280, 1280, "1280 x 1280" },
        { 1920, 1920, "1920 x 1920" }
    };
    int numResolutions= 3;
    int selectedResIndex= 0;

    Vector2 playerCursor= {screenWidth / 2, screenHeight / 2};
    Vector2 targetCursor= {screenWidth / 2, screenHeight / 2};
    const float CURSOR_MAX_SPEED= 1000.0f;
    const float CURSOR_LERP_SPEED= 10.0f;
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
                if (IsKeyPressed(KEY_S)) { screen= SETTINGS; }
            } break;

            case SETTINGS:
            {
                int oldWidth= GetScreenWidth();
                int oldHeight= GetScreenHeight();

                if (IsKeyPressed(KEY_DOWN)) {
                    selectedResIndex= (selectedResIndex + 1) % numResolutions;
                }
                if (IsKeyPressed(KEY_UP)) {
                    selectedResIndex= (selectedResIndex - 1 + numResolutions) % numResolutions;
                }
                if (IsKeyPressed(KEY_ENTER)) {
                    screenWidth= resOptions[selectedResIndex].width;
                    screenHeight= resOptions[selectedResIndex].height;

                    Vector2 currentPos= GetWindowPosition();

                    // 2. Apply the new size
                    SetWindowSize(screenWidth, screenHeight);

                    // 3. Shift the position so the center point remains identical
                    int shiftX= currentPos.x - (screenWidth - oldWidth) / 2;
                    int shiftY= currentPos.y - (screenHeight - oldHeight) / 2;
                    
                    SetWindowPosition(shiftX, shiftY);
                }
                if (IsKeyPressed(KEY_BACKSPACE)) {
                    screen= MENU;
                }
            } break;

            case GAMEPLAY:
            {
                if(receiveUDPDataPayload(&dp)) {
                    calculatePlayerLoc();
                    targetCursor.x= (pl.loc[0] * CONVERT_SCALE_X) + 132;
                    targetCursor.y= (pl.loc[1] * CONVERT_SCALE_Y) - 612;

                    float dt= GetFrameTime();
                    float smoothing= 1.0f - expf(-CURSOR_LERP_SPEED * dt);
                    playerCursor.x+= (targetCursor.x - playerCursor.x) * smoothing;
                    playerCursor.y+= (targetCursor.y - playerCursor.y) * smoothing;
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
                        if(moles[i].time<= 0 && moles[i].active== true) {
                        despawnMole(&moles[i]);
                        } else if (moles[i].time <= 0 && moles[i].active== false) {
                        spawnMole(&moles[i]);
                        }

                        if(CheckCollisionCircles(playerCursor, PLAYER_SIZE, moles[i].pos, moleRadius) && moles[i].active) {
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
                    // Scale font sizes relative to the virtual resolution
                    int fontSize= (int)(screenWidth / 10.0f); // Title

                    const char *titleText= "WHACK-A-MOLE";
                    const char *startText= "Press ENTER to start";
                    const char *settingsText= "Press S for Settings";

                    DrawText(titleText, 
                            screenWidth / 2 - MeasureText(titleText, fontSize) / 2, 
                            screenHeight * 0.30f, 
                            fontSize, 
                            DARKGRAY);

                    fontSize/= 2; // Start prompt

                    DrawText(startText, 
                            screenWidth / 2 - MeasureText(startText, fontSize) / 2, 
                            screenHeight * 0.48f, 
                            fontSize, 
                            GRAY);

                    fontSize /= 1.5f; // Settings

                    DrawText(settingsText, 
                            screenWidth / 1.33f - MeasureText(settingsText, fontSize) / 2, 
                            screenHeight * 0.88f, 
                            fontSize, 
                            GRAY);
                } break;

                case SETTINGS:
                {
                    int fontSize= (int)(screenWidth / 10.0f);   
                    DrawText("SETTINGS", screenWidth / 2 - MeasureText("SETTINGS", fontSize) / 2, screenHeight * 0.15f, fontSize, DARKGRAY);
                    
                    int startY= screenHeight * 0.35f;
                    int spacing= screenHeight * 0.1f;
                    
                    fontSize= (int)(screenWidth / 20.0f);
                    for (int i= 0; i < numResolutions; i++) {
                        Color col= (i== selectedResIndex) ? BLUE : DARKGRAY;
                        int itemWidth= MeasureText(resOptions[i].label, fontSize);
                        DrawText(resOptions[i].label, screenWidth / 2 - itemWidth / 2, startY + (i * spacing), fontSize, col);
                        if (i== selectedResIndex) {
                            DrawText(">", screenWidth / 2 - itemWidth / 2 - 40, startY + (i * spacing), fontSize, BLUE);
                        }
                    }

                    fontSize= (int)(screenWidth / 25.0f);   
                    DrawText("Use UP/DOWN to select, ENTER to apply", screenWidth / 2 - MeasureText("Use UP/DOWN to select, ENTER to apply", fontSize) / 2, screenHeight * 0.80f, fontSize, GRAY);
                    DrawText("Press BACKSPACE to return", screenWidth / 2 - MeasureText("Press BACKSPACE to return", fontSize) / 2, screenHeight * 0.85f, fontSize, GRAY);
                } break;

                case GAMEPLAY:
                {
                    ClearBackground(BLACK);

                    BeginScissorMode(BORDER_BUFFER, BORDER_BUFFER, screenWidth - (BORDER_BUFFER * 2), screenHeight - (BORDER_BUFFER * 2));
                    {
                        for (int x= BORDER_BUFFER; x < screenWidth - BORDER_BUFFER; x += GRASS_SIZE) {
                            for (int y= BORDER_BUFFER; y < screenHeight - BORDER_BUFFER; y += GRASS_SIZE) {
                                DrawTexture(grassSprite.texture, x, y, WHITE);
                            }
                        }
                    }
                    EndScissorMode();

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
                    buffer+= MeasureText((TextFormat("Level: %i", level)), fontSize);
                    DrawText(TextFormat("Level: %i", level), screenWidth - buffer, 20, fontSize, WHITE);
                    buffer+= MeasureText((TextFormat("Moles: %i", count)), fontSize) + fontSize;
                    DrawText(TextFormat("Moles: %i", count), screenWidth - buffer, 20, fontSize, WHITE);
                    buffer+= MeasureText((TextFormat("Time: %.0f", roundtimer)), fontSize) + fontSize;
                    DrawText(TextFormat("Time: %.0f", roundtimer), screenWidth - buffer, 20, fontSize, WHITE);
                    DrawText(TextFormat("[%.0f, %.0f]", playerCursor.x, playerCursor.y), screenWidth - buffer, screenHeight - fontSize, fontSize, WHITE);

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