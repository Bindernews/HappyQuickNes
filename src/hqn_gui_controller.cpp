#include "hqn_gui_controller.h"
#include "hqn_main.h"
#include "hqn_util.h"
#include <SDL.h>

#define DEFAULT_WINDOW_TITLE "HappyQuickNES"

namespace hqn
{

const SDL_Rect NES_BLIT_RECT = {0, 0, 256, 240};


// Function to initalize the video palette
int32_t *_initF_VideoPalette()
{
    static int32_t VideoPalette[512];
    const Nes_Emu::rgb_t *palette = Nes_Emu::nes_colors;
    for (int i = 0; i < 512; i++)
    {
        VideoPalette[i] = palette[i].red << 16 | palette[i].green << 8
            | palette[i].blue | 0xff000000;
    }
    return VideoPalette;
}

// Initialize the video palette
const int32_t *VideoPalette = _initF_VideoPalette();


GUIController::GUIController(HQNState *state)
{
    m_tex = nullptr;
    m_renderer = nullptr;
    m_window = nullptr;
    m_state = state;
}

GUIController::~GUIController()
{
    if (m_tex)
        SDL_DestroyTexture(m_tex);
    if (m_texOverlay)
        SDL_DestroyTexture(m_texOverlay);
    if (m_renderer)
        SDL_DestroyRenderer(m_renderer);
    if (m_window)
        SDL_DestroyWindow(m_window);
}

bool GUIController::init()
{
    const size_t width = 256,
                 height = 240;

    // create the window
    if (!(m_window = SDL_CreateWindow(DEFAULT_WINDOW_TITLE,
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, 0)))
        return false;
    if (!(m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED)))
        return false;
    if (!(m_tex = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING, 256, 256)))
        return false;
    if (!(m_texOverlay = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING, 256, 256)))
        return false;

    SDL_SetTextureBlendMode(m_texOverlay, SDL_BLENDMODE_BLEND);
    // Set the clear color now rather than later
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
    m_overlay = new Surface(width, height);

    return true;
}

void GUIController::onAdvanceFrame(HQNState *state)
{
    void *nesPixels = nullptr;
    void *overlayPixels = nullptr;
    int pitch = 0;
    // lock both textures
	if (SDL_LockTexture(m_tex, nullptr, &nesPixels, &pitch) < 0)
		return;
    if (SDL_LockTexture(m_texOverlay, nullptr, &overlayPixels, &pitch) < 0)
        return;
    // update them
    blit(state->emu(), (int32_t*)nesPixels, VideoPalette, 0, 0, 0, 0);
    memcpy(overlayPixels, m_overlay->getPixels(), m_overlay->getDataSize());
    // unlock the textures
    SDL_UnlockTexture(m_tex);
    SDL_UnlockTexture(m_texOverlay);

    // render to screen
    SDL_RenderClear(m_renderer);
    SDL_RenderCopy(m_renderer, m_tex, &NES_BLIT_RECT, &NES_BLIT_RECT);
    SDL_RenderCopy(m_renderer, m_texOverlay, &NES_BLIT_RECT, &NES_BLIT_RECT);
    SDL_RenderPresent(m_renderer);
    // Process any outstanding events
    processEvents();
}

// Copied directly from bizinterface.cpp in BizHawk/quicknes
void GUIController::blit(Nes_Emu *e, int32_t *dest, const int32_t *colors, int cropleft, int croptop, int cropright, int cropbottom)
{
    // what is the point of the 256 color bitmap and the dynamic color allocation to it?
    // why not just render directly to a 512 color bitmap with static palette positions?

    const int srcpitch = e->frame().pitch;
    const unsigned char *src = e->frame().pixels;
    const unsigned char *const srcend = src + (e->image_height - cropbottom) * srcpitch;

    const short *lut = e->frame().palette;

    const int rowlen = 256 - cropleft - cropright;

    src += cropleft;
    src += croptop * srcpitch;

    for (; src < srcend; src += srcpitch)
    {
        for (int i = 0; i < rowlen; i++)
        {
            *dest++ = colors[lut[src[i]]];
        }
    }
}

void GUIController::processEvents()
{
    bool quit = false;
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch(event.type)
        {
        case SDL_QUIT:
            quit = true;
            break;
        }
    }
    if (quit)
    {
        endItAll();
    }
}

void GUIController::onLoadROM(HQNState *state, const char *filename) {} // unimportant
void GUIController::onLoadState(HQNState *state) {} // also unimportant

}
