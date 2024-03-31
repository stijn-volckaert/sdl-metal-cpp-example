#include "triangle_types.h"

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <Metal/shared_ptr.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include <SDL.h>

#include <iostream>
#include <chrono>

namespace {

#include "triangle_metallib.h"

}

namespace {

const AAPLVertex triangleVertices[] = {
    // 2D positions,    RGBA colors
    { {  250,  -250 }, { 1, 0, 0, 1 } },
    { { -250,  -250 }, { 0, 1, 0, 1 } },
    { {    0,   250 }, { 0, 0, 1, 1 } },
};

const vector_uint2 viewport = {
    640, 480
};

}

int
main(int argc, char **argv) {
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "metal");
    SDL_InitSubSystem(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("SDL Metal", viewport[0], viewport[1], SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE | SDL_WINDOW_METAL);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, "metal", SDL_RENDERER_ACCELERATED);

	if (!renderer) {
		std::cerr << "Failed to create metal renderer: " << SDL_GetError() << "\n";

		std::cerr << "Available renderers:\n";

		for (int i = 0; i < SDL_GetNumRenderDrivers(); ++i) {
			std::cerr << "- " << SDL_GetRenderDriver(i) << "\n";
		}
		return -1;
	}
	
	SDL_SetWindowKeyboardGrab(window, SDL_TRUE);

	SDL_Version compiled, linked;
	SDL_VERSION(&compiled);
	SDL_GetVersion(&linked);

	std::cerr << "SDL metal demo app - compiled with SDL " << static_cast<int>(compiled.major) << "." << static_cast<int>(compiled.minor) << "." << static_cast<int>(compiled.patch)
			  << " - linked with SDL " << static_cast<int>(linked.major) << "." << static_cast<int>(linked.minor) << "." << static_cast<int>(linked.patch) << "\n"; 

	std::cerr << "Controls:\n"
			  << "- F: toggle fullscreen mode\n"
			  << "- C: toggle mouse capture (uses relative mouse mode)\n"
			  << "- Q: quit\n";
	
    NS::Error *err;

    auto swapchain = (CA::MetalLayer*)SDL_GetRenderMetalLayer(renderer);
    auto device = swapchain->device();

	if (!device) {
		std::cerr << "failed to create Metal device\n";
		return -1;
	}

    auto name = device->name();
    std::cerr << "device name: " << name->utf8String() << std::endl;

    auto library_data = dispatch_data_create(
        &triangle_metallib[0], triangle_metallib_len,
        dispatch_get_main_queue(),
        ^{ });

    auto library = MTL::make_owned(device->newLibrary(library_data, &err));

    if (!library) {
        std::cerr << "Failed to create library" << std::endl;
        std::exit(-1);
    }

    auto vertex_function_name = NS::String::string("vertexShader", NS::ASCIIStringEncoding);
    auto vertex_function = MTL::make_owned(library->newFunction(vertex_function_name));

    auto fragment_function_name = NS::String::string("fragmentShader", NS::ASCIIStringEncoding);
    auto fragment_function = MTL::make_owned(library->newFunction(fragment_function_name));

    auto pipeline_descriptor = MTL::make_owned(MTL::RenderPipelineDescriptor::alloc()->init());
    pipeline_descriptor->setVertexFunction(vertex_function.get());
    pipeline_descriptor->setFragmentFunction(fragment_function.get());

    auto color_attachment_descriptor = pipeline_descriptor->colorAttachments()->object(0);
    color_attachment_descriptor->setPixelFormat(swapchain->pixelFormat());

    auto pipeline = MTL::make_owned(device->newRenderPipelineState(pipeline_descriptor.get(), &err));

    if (!pipeline) {
        std::cerr << "Failed to create pipeline" << std::endl;
        std::exit(-1);
    }

    auto queue = MTL::make_owned(device->newCommandQueue());

    bool quit = false;
	bool fullscreen = false;
	bool inResize = false;
	bool capturingMouse = false;
    SDL_Event e;

	auto start = std::chrono::high_resolution_clock::now();
	long frameCount = 0;

    while (!quit) {

		auto now = std::chrono::high_resolution_clock::now();
		auto diff = std::chrono::duration_cast<std::chrono::microseconds>(now-start).count();

		if (diff > 1000000) {
			std::cerr << (frameCount * 1000000.0 / diff) << " FPS\n"; 
			frameCount = 0;
			start = now;
		}		

		while (SDL_PollEvent(&e) != 0) {
            switch (e.type) {
                case SDL_EVENT_QUIT: {
                    quit = true;
                } break;
				case SDL_EVENT_KEY_DOWN: {
					if (e.key.keysym.sym == SDLK_q) {
						quit = true;
					} else if (e.key.keysym.sym == SDLK_f) {
						if (inResize)
							break;
						std::cerr << "Switching to " << (fullscreen?"Windowed":"Fullscreen") << "\n";
						if (SDL_SetWindowFullscreen(window, fullscreen?SDL_FALSE:SDL_TRUE)) {
						   std::cerr << "SDL_SetWindowFullscreen failed: " << SDL_GetError() << "\n";
						} else {
							inResize = true;
							fullscreen = !fullscreen;
						}						
					} else if (e.key.keysym.sym == SDLK_c) {
						SDL_bool shouldCapture = capturingMouse ? SDL_FALSE : SDL_TRUE;
						SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, 0);
						SDL_SetWindowMouseGrab(window, shouldCapture);
						if (SDL_SetRelativeMouseMode(shouldCapture) == 0 &&
							SDL_GetRelativeMouseMode() == shouldCapture) {
							std::cerr << (shouldCapture?"Captured":"Released") << " mouse\n";
							capturingMouse = !capturingMouse;
						} else {
							std::cerr << "SDL_SetRelativeMouseMode failed: " << SDL_GetError() << "\n";
						}						
					}
				} break;
				case SDL_EVENT_WINDOW_RESIZED: {
					int drawableSizeX = swapchain->drawableSize().width;
					int drawableSizeY = swapchain->drawableSize().height;
		
					inResize = false;
					std::cerr << "Window resized => " << drawableSizeX << "x" << drawableSizeY << "\n";
					
				} break;
					
            }
        }

        auto drawable = swapchain->nextDrawable();

        auto pass = MTL::make_owned(MTL::RenderPassDescriptor::renderPassDescriptor());

        auto color_attachment = pass->colorAttachments()->object(0);
        color_attachment->setLoadAction(MTL::LoadAction::LoadActionClear);
        color_attachment->setStoreAction(MTL::StoreAction::StoreActionStore);
        color_attachment->setTexture(drawable->texture());

        //
        auto buffer = MTL::make_owned(queue->commandBuffer());

        //
        auto encoder = MTL::make_owned(buffer->renderCommandEncoder(pass.get()));

        encoder->setViewport(MTL::Viewport {
            0.0f, 0.0f,
            (double)viewport[0], (double)viewport[1],
            0.0f, 1.0f
         });

        encoder->setRenderPipelineState(pipeline.get());

        encoder->setVertexBytes(&triangleVertices[0], sizeof(triangleVertices), AAPLVertexInputIndexVertices);
        encoder->setVertexBytes(&viewport, sizeof(viewport), AAPLVertexInputIndexViewportSize);

        NS::UInteger vertex_start = 0, vertex_count = 3;
        encoder->drawPrimitives(MTL::PrimitiveType::PrimitiveTypeTriangle, vertex_start, vertex_count);

        encoder->endEncoding();

        buffer->presentDrawable(drawable);
        buffer->commit();

        drawable->release();

		frameCount++;
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
