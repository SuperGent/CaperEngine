#include "Engine/Engine.h"

#include <fstream>
#include <iostream>
#include <SDL.h>
#include <SDL_image.h>
#include <glm/glm.hpp>

#include "AssetStore/AssetStore.h"
#include "Components/RigidBodyComponent.h"
#include "Components/SpriteComponent.h"
#include "Components/TransformComponent.h"
#include "Log/Logger.h"
#include "Systems/BounceSystem.h"
#include "Systems/MovementSystem.h"
#include "Systems/RenderSystem.h"

Engine::Engine()
{
	registry_ = std::make_unique <Registry>();
	asset_store_ = std::make_unique<AssetStore>();
}

Engine::~Engine()
{
}

void Engine::Initialise()
{
	Logger::Initialise();

	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		std::cerr << "Error initializing SDL. Error:" << SDL_GetError() << std::endl;
		return;
	}

	SDL_DisplayMode display_mode;
	SDL_GetCurrentDisplayMode(0, &display_mode);
	window_width_ = 800; // display_mode.w;
	window_height_ = 600; // display_mode.h

	window_ = SDL_CreateWindow(
		nullptr,
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		window_width_,
		window_height_,
		SDL_WINDOW_BORDERLESS);
	if (!window_)
	{
		std::cerr << "Error initializing window. Error:" << SDL_GetError() << std::endl;
		return;
	}

	renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!renderer_)
	{
		std::cerr << "Error SDL renderer. Error:" << SDL_GetError() << std::endl;
		return;
	}
	//SDL_SetWindowFullscreen(window_, SDL_WINDOW_FULLSCREEN);

	is_running_ = true;
}

void Engine::LoadLevel(int level)
{
	//Add Systems
	registry_->AddSystem<MovementSystem>();
	registry_->AddSystem<RenderSystem>();
	//registry_->AddSystem<BounceSystem>();

	asset_store_->AddTexture(renderer_, "tank-image", "./assets/images/kenney/Tank-right.png");
	asset_store_->AddTexture(renderer_, "truck-image", "./assets/images/kenney/Truck_Right.png");
	asset_store_->AddTexture(renderer_, "tilemap-image", "./assets/tilemaps/jungle.png");

	// Load the tilemap
	int tileSize = 32;
	double tileScale = 1.0;
	int mapNumCols = 25;
	int mapNumRows = 20;

	std::fstream map_file;
	map_file.open("./assets/tilemaps/jungle.map");

	for (int y = 0; y < mapNumRows; y++) {
		for (int x = 0; x < mapNumCols; x++) {
			char ch;
			map_file.get(ch);
			int srcRectY = std::atoi(&ch) * tileSize;
			map_file.get(ch);
			int srcRectX = std::atoi(&ch) * tileSize;
			map_file.ignore();

			Entity tile = registry_->CreateEntity();
			tile.AddComponent<TransformComponent>(glm::vec2(x * (tileScale * tileSize), y * (tileScale * tileSize)), glm::vec2(tileScale, tileScale), 0.0);
			tile.AddComponent<SpriteComponent>("tilemap-image", tileSize, tileSize, srcRectX, srcRectY);
		}
	}
	map_file.close();

	Entity tank = registry_->CreateEntity();
	Entity truck = registry_->CreateEntity();

	tank.AddComponent<TransformComponent>(glm::vec2(32.0, 32.0), glm::vec2(2.0, 2.0), 0.0);
	tank.AddComponent<RigidBodyComponent>(glm::vec2(10.0, 20.0));
	tank.AddComponent<SpriteComponent>("tank-image", 64, 64);

	truck.AddComponent<TransformComponent>(glm::vec2(100.0, 130.0), glm::vec2(1.0, 1.0), 0.0);
	truck.AddComponent<RigidBodyComponent>(glm::vec2(-10.0, 25.0));
	truck.AddComponent<SpriteComponent>("truck-image", 64, 64);
}

void Engine::Setup()
{
	LoadLevel(1);
}

void Engine::Run()
{
	Setup();
	while (is_running_)
	{
		ProcessInput();
		Update();
		Render();
	}
}

void Engine::ProcessInput()
{
	SDL_Event sdl_event;
	while (SDL_PollEvent(&sdl_event))
	{
		switch (sdl_event.type)
		{
		case SDL_QUIT:
			is_running_ = false;
			break;
		case SDL_KEYDOWN:
			if (sdl_event.key.keysym.sym == SDLK_ESCAPE)
			{
				is_running_ = false;
			}
			break;
		default:
			break;
		}
	}
}

void Engine::Update()
{
	int timeToWait = MILLISECS_PER_FRAME - (SDL_GetTicks() - millisecs_prev_frame_);
	if (timeToWait > 0 && timeToWait <= millisecs_prev_frame_)
	{
		SDL_Delay(timeToWait);
	}

	double delta_time = (SDL_GetTicks() - millisecs_prev_frame_) / 1000.0f;

	millisecs_prev_frame_ = SDL_GetTicks();

	//Update Systems
	registry_->GetSystem<MovementSystem>().Update(delta_time);
	//registry_->GetSystem<BounceSystem>().Update();

	//Update Registry
	registry_->Update();
}

void Engine::Render()
{
	SDL_SetRenderDrawColor(renderer_, 0x15, 0x15, 0x15, 0xFF);
	SDL_RenderClear(renderer_);
	registry_->GetSystem<RenderSystem>().Update(renderer_, asset_store_);
	SDL_RenderPresent(renderer_);
}

void Engine::Shutdown()
{
	Logger::Log("Shutting down");

	SDL_DestroyRenderer(renderer_);
	SDL_DestroyWindow(window_);
	SDL_Quit();

	Logger::ShutDown();
}