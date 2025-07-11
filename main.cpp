

//#include <iostream>
//#include <chrono>
//#include <thread>
//#include "gb.h"

//const int SCREEN_WIDTH = 160;
//const int SCREEN_HEIGHT = 144;

//int main(int argc, char* argv[]) {
//    // --- SDL Initialization ---
//         gb gameboy("D:\\Emulation\\bgb\\cpu_instrs.gb");
//         int i = 0;
//   while (i<6) {

//        gameboy.run_one_frame();
//        i++;
//    }
//    return 0;
//}




#include <iostream>
#include <chrono>
#include <thread>
#include "gb.h"
#include "raylib.h" // Include the main SDL header

// Define screen dimensions for clarity
const int SCREEN_WIDTH = 160;
const int SCREEN_HEIGHT = 144;
const int SCREEN_SCALE = 4; // Scale up the window for better visibility

int main(int argc, char* argv[]) {


    // gb gameboy("D:\\Emulation\\bgb\\cpu_instrs\\individual\\09-op r,r.gb");
    ////gb gameboy("/home/visael/gp/tetris.gb");
    //int i = 0;
    //int frames = 0;
    //while (i < 13196) {

    //	gameboy.run_one_frame();
    //	frames++;
    //	i++;
    //}


    //return 0;
    // --- SDL Initialization ---

    InitWindow(SCREEN_WIDTH * SCREEN_SCALE, SCREEN_HEIGHT * SCREEN_SCALE, "GBsunny Emulator");
    //SetTargetFPS(60);

    // Create a Raylib Image to hold our pixel data.
    // This only needs to be created once.
    // The format RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 matches our rgba struct.
    Image gb_screen = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, BLANK);
    // Create a texture that we can update each frame.
    Texture2D gb_texture = LoadTextureFromImage(gb_screen);
    UnloadImage(gb_screen); // The image data is now in the texture, so we can unload it

    try {
        // Create an instance of the emulator, providing a path to a ROM.
        // You must replace "path/to/your/rom.gb" with a valid path.
        gb gameboy("D:\\Emulation\\bgb\\bgbtest.gb");
        // The main application loop.
        while (!WindowShouldClose()) {
            // 1. Run the emulator for one full frame's worth of cycles.
            gameboy.run_one_frame();

            // 2. Get the resulting framebuffer from the emulator.
            const auto& framebuffer = gameboy.get_framebuffer();

            // 3. Update the Raylib Texture data with the emulator's framebuffer.
            //    The UpdateTexture function expects a `const void*`, so we can pass a pointer
            //    to the start of our rgba array.
            UpdateTexture(gb_texture, framebuffer.data());

            // --- Drawing ---
            BeginDrawing();
            ClearBackground(BLACK);

            // Draw the texture, scaled up to fill the window.
            DrawTextureEx(
                gb_texture,
                { 0, 0 },      // Position (top-left corner)
                0.0f,        // Rotation
                SCREEN_SCALE, // Scale
                WHITE        // Tint
            );

            EndDrawing();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "An emulator error occurred: " << e.what() << std::endl;
        // The cleanup below will still run.
    }

    // --- Cleanup ---
    UnloadTexture(gb_texture);
    CloseWindow();

    return 0;
}