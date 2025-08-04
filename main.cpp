#include "raylib.h"
#include "gb.h"
#include <iostream>
#include <chrono>
#include <thread>

// Define screen dimensions
const int SCREEN_WIDTH = 160;
const int SCREEN_HEIGHT = 144;
const int SCREEN_SCALE = 4; // Scale up the window

int main(int argc, char* argv[]) {


    //if (argc < 2) {
    //    std::cerr << "Error: Please provide a path to the ROM file." << std::endl;
    //    std::cerr << "Usage: " << argv[0] << " <path_to_rom>" << std::endl;
    //    
    //    return 1; // Return an error code
    //}
    // std::string path_arg(argv[1]);
    // std::cout << path_arg << std::endl;;
    // Pass the first command-line argument (the path) to the constructor
    //gb gameboy(path_arg);

    gb gameboy(R"(D:\Emulation\bgb\F-1 Race (World).gb)",true);
	//
	 while (false) {
	 	gameboy.run_one_frame();
	 }

     InitWindow(SCREEN_WIDTH * SCREEN_SCALE, SCREEN_HEIGHT * SCREEN_SCALE, "GBsunny Emulator");
     //SetTargetFPS(60);  // Target frame rate
    
     // Criar textura da Raylib
     Image image = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
     Texture2D texture = LoadTextureFromImage(image);
     UnloadImage(image);
    
     while (!WindowShouldClose()) {
         gameboy.run_one_frame();
         const auto& framebuffer = gameboy.get_framebuffer();
    
         // Atualizar textura com o framebuffer
         UpdateTexture(texture, framebuffer.data());
    
         BeginDrawing();
         ClearBackground(BLACK);
    
         // Desenha a textura em escala
         DrawTextureEx(texture, { 0, 0 }, 0.0f, SCREEN_SCALE, WHITE);
    
         EndDrawing();  
     }
    
     // Cleanup
     UnloadTexture(texture);
     CloseWindow();

    return 0;
}