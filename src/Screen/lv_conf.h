#ifndef LV_CONF_H
#define LV_CONF_H

#ifndef __ASSEMBLER__
    #include <stdint.h>
    #include <stddef.h>
    #include <stdbool.h>
#endif
/* ------------------------------------------ */

#if 1 

#define LV_COLOR_DEPTH     32
#define LV_MEM_SIZE        (128 * 1024U)

#define LV_USE_SDL              1
#if LV_USE_SDL
    #define LV_SDL_INCLUDE_PATH    <SDL2/SDL.h>
    #define LV_SDL_RENDER_MODE     LV_DISPLAY_RENDER_MODE_DIRECT
    #define LV_SDL_HOR_RES         800
    #define LV_SDL_VER_RES         480
#endif


#ifndef __ASSEMBLER__
    #define LV_USE_STDLIB_MALLOC    LV_STDLIB_CLIB
    #define LV_USE_STDLIB_STRING    LV_STDLIB_CLIB
    #define LV_USE_STDLIB_SPRINTF  LV_STDLIB_CLIB
#endif

#define LV_USE_THORVG_INTERNAL 0
#define LV_USE_VECTOR_GRAPHIC  0



#endif 
#endif 