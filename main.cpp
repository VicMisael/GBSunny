

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

#include <SDL2/SDL.h> // Include the main SDL header

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
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
		return 1;
	}

	SDL_Window* window = SDL_CreateWindow(
		"GBsunny Emulator",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		SCREEN_WIDTH * SCREEN_SCALE,
		SCREEN_HEIGHT * SCREEN_SCALE,
		SDL_WINDOW_SHOWN
	);
	if (window == nullptr) {
		std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return 1;
	}

	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (renderer == nullptr) {
		std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}

	// Create a texture that we can update each frame.
	// The format RGBA8888 matches our `rgba` struct (on little-endian systems).
	SDL_Texture* texture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_RGBA8888,
		SDL_TEXTUREACCESS_STREAMING,
		SCREEN_WIDTH,
		SCREEN_HEIGHT
	);
	if (texture == nullptr) {
		std::cerr << "Texture could not be created! SDL_Error: " << SDL_GetError() << std::endl;
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}

	try {
		// Create an instance of the emulator.
		// gb gameboy("D:\\Emulation\\bgb\\cpu_instrs\\individual\\09-op r,r.gb");
		 //gb gameboy("D:\\Emulation\\bgb\\cpu_instrs.gb");
		gb gameboy("/mnt/d/Emulation/bgb/cpu_instrs/individual/09-op r,r.gb");
		// The main application loop.
		bool is_running = true;
		SDL_Event e;
		while (is_running) {
			// Handle events on queue
			while (SDL_PollEvent(&e) != 0) {
				if (e.type == SDL_QUIT) {
					is_running = false;
				}
			}

			// 1. Run the emulator for one full frame's worth of cycles.
			gameboy.run_one_frame();

			// 2. Get the resulting framebuffer from the emulator.
			const auto& framebuffer = gameboy.get_framebuffer();

			// 3. Update the SDL Texture with the emulator's framebuffer.
			SDL_UpdateTexture(texture, nullptr, framebuffer.data(), SCREEN_WIDTH * sizeof(ppu_types::rgba));

			// --- Drawing ---
			SDL_RenderClear(renderer);
			SDL_RenderCopy(renderer, texture, nullptr, nullptr);
			SDL_RenderPresent(renderer);
		}
	}
	catch (const std::exception& e) {
		std::cerr << "An emulator error occurred: " << e.what() << std::endl;
	}

	// --- Cleanup ---
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}