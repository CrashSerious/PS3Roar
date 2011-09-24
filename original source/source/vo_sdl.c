/*  XRoar - a Dragon/Tandy Coco emulator
 *  Copyright (C) 2003-2010  Ciaran Anscomb
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdlib.h>
#include <string.h>
#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <io/pad.h>

#include "rsxutil.h"
#include "ps3gui.h"
#include "types.h"
#include "logging.h"
#include "module.h"
#include "vdg_bitmaps.h"
#include "xroar.h"
#ifdef WINDOWS32
#include "common_windows32.h"
#endif

static int init(void);
static void shutdown(void);
static int set_fullscreen(int fullscreen);
static void vsync(void);
static void set_mode(unsigned int mode);
static void render_border(void);
static void alloc_colours(void);
static void hsync(void);

VideoModule video_sdl_module = {
	{ "sdl", "Standard SDL surface",
	  init, 0, shutdown },
	NULL, set_fullscreen, 0,
	vsync, set_mode,
	render_border, NULL, hsync
};

typedef Uint32 Pixel;
#define MAPCOLOUR(r,g,b) SDL_MapRGBA(screen->format, r, g, b,0)
#define VIDEO_SCREENBASE ((Pixel *)screen->pixels)
#define XSTEP 1
#define NEXTLINE 400
#define VIDEO_TOPLEFT (VIDEO_SCREENBASE + (720*70))
#define VIDEO_VIEWPORT_YOFFSET (136)
#define LOCK_SURFACE SDL_LockSurface(screen)
#define UNLOCK_SURFACE SDL_UnlockSurface(screen)
#define VIDEO_MODULE_NAME video_sdl_module

SDL_Surface *screen;
SDL_Surface *backgroundScreen;
SDL_Surface *egg;
#include "vo_generic_ops.h"

static int init(void) {

	ps3InitDisplay();

	LOG_DEBUG(2,"Initialising SDL video driver\n");
#ifdef WINDOWS32
	if (!getenv("SDL_VIDEODRIVER"))
		putenv("SDL_VIDEODRIVER=windib");
#endif
	/* init SDL TTF */
	TTF_Init();
	text_fontSmall = TTF_OpenFont("/dev_hdd0/game/PS3Roar00/USRDIR/fonts/CenturySchoolBold.ttf", 10);
	text_fontMedium = TTF_OpenFont("/dev_hdd0/game/PS3Roar00/USRDIR/fonts/CenturySchoolBold.ttf", 12);
	text_fontLarge = TTF_OpenFont("/dev_hdd0/game/PS3Roar00/USRDIR/fonts/CenturySchoolBold.ttf", 18);
	text_fontTiny = TTF_OpenFont("/dev_hdd0/game/PS3Roar00/USRDIR/fonts/CenturySchoolBold.ttf", 6);
	
	backgroundScreen = IMG_Load("/dev_hdd0/game/PS3Roar00/USRDIR/images/oldtv_480phires.png");
	egg = IMG_Load("/dev_hdd0/game/PS3Roar00/USRDIR/images/SUX.png");

	if (!backgroundScreen) 
	{
		printf("*********************************\n");
		printf("**** Error Loading Image! *******\n");
		printf("*********************************\n");
	}
	if (!SDL_WasInit(SDL_INIT_NOPARACHUTE)) {
		if (SDL_Init(SDL_INIT_NOPARACHUTE) < 0) {
			LOG_ERROR("Failed to initialise SDL: %s\n", SDL_GetError());
			return 1;
		}
	}
	if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
		LOG_ERROR("Failed to initialise SDL video driver: %s\n", SDL_GetError());
		return 1;
	}
	if (set_fullscreen(xroar_fullscreen))
		return 1;
#ifdef WINDOWS32
	{
		SDL_version sdlver;
		SDL_SysWMinfo sdlinfo;
		SDL_VERSION(&sdlver);
		sdlinfo.version = sdlver;
		SDL_GetWMInfo(&sdlinfo);
		windows32_main_hwnd = sdlinfo.window;
	}
#endif
	set_mode(0);
	/* Update screen with backround and display static */
	SDL_UpdateRect(screen, 0, 0, 720, 480);
	displayStatic(0);
	return 0;
}

static void shutdown(void) {
	LOG_DEBUG(2,"Shutting down SDL video driver\n");
	set_fullscreen(0);
	/* Should not be freed by caller: SDL_FreeSurface(screen); */
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

static int set_fullscreen(int fullscreen) {
	screen = SDL_SetVideoMode(720, 480, 32, SDL_SWSURFACE|(fullscreen?SDL_FULLSCREEN:0));
	SDL_BlitSurface(backgroundScreen, 0/*SDL_Rect *srcrect*/, screen, 0/*SDL_Rect *dstrect*/);
	if (screen == NULL) {
		LOG_ERROR("Failed to allocate SDL surface for display\n");
		return 1;
	}
	pixel = VIDEO_TOPLEFT + VIDEO_VIEWPORT_YOFFSET;
	alloc_colours();
	if (fullscreen)
		SDL_ShowCursor(SDL_DISABLE);
	else
		SDL_ShowCursor(SDL_ENABLE);
		video_sdl_module.is_fullscreen = fullscreen;
	return 0;
}

static void vsync(void) {
	SDL_UpdateRect(screen, 0, 0, 720, 480);
	pixel = VIDEO_TOPLEFT + VIDEO_VIEWPORT_YOFFSET;
	subline = 0;
	beam_pos = 0;
}
