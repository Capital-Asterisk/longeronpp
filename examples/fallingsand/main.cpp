

#include <raylib.h>

int main(int argc, char** argv)
{
    const int screenWidth = 256;
    const int screenHeight = 256;
    const int screenScale = 2;

    SetAudioStreamBufferSizeDefault(14400 / 60 * 6);
    InitWindow(screenWidth * screenScale, screenHeight * screenScale, "Nice");

    SetTargetFPS(60);

    unsigned int frameCount = 0;
    
    while (!WindowShouldClose())
    {

        BeginDrawing();

        ClearBackground(CLITERAL(Color){ 0, 0, 0, 255 });

        EndDrawing();

        frameCount ++;
    }

    return 0;
}
