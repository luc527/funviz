#include <stdlib.h>
#include <SDL3/SDL.h>

typedef struct vec2_s {
    float x, y;
} vec2;

typedef struct line_s {
    vec2 v0;
    vec2 v1;
} line;

typedef struct plane_s {
    vec2 o; // offset from center
    vec2 s; // scale
} plane;


#define VEC2_SPREAD(var) var.x, var.y

#define LINE_SPREAD(var) var.v0.x, var.v0.y, var.v1.x, var.v1.y


vec2 vec2_win_xform(vec2 v, float w, float h) {
    vec2 r = {
        x: ( v.x + 1.0f) * w / 2.0f,
        y: (-v.y + 1.0f) * h / 2.0f,
    };
    return r;
}

vec2 vec2_win_xform_inv(vec2 v, float w, float h) {
    vec2 r = {
        x:   v.x / w * 2.0f - 1.0f,
        y: -(v.y / h * 2.0f - 1.0f),
    };
    return r;
}

line line_win_xform(line l, float w, float h) {
    line r = {
        v0: vec2_win_xform(l.v0, w, h),
        v1: vec2_win_xform(l.v1, w, h),
    };
    return r;
}

line line_win_xform_inv(line l, float w, float h) {
    line r = {
        v0: vec2_win_xform_inv(l.v0, w, h),
        v1: vec2_win_xform_inv(l.v1, w, h),
    };
    return r;
}

vec2 vec2_plane_xform(plane plane, vec2 v) {
    vec2 r = {
        x: v.x * plane.s.x - plane.o.x,
        y: v.y * plane.s.y - plane.o.y,
    };
    return r;
}

vec2 vec2_plane_xform_inv(plane plane, vec2 v) {
    vec2 r = {
        x: (v.x + plane.o.x) / plane.s.x,
        y: (v.y + plane.o.y) / plane.s.y,
    };
    return r;
}

line line_plane_xform(plane plane, line l) {
    line r = {
        v0: vec2_plane_xform(plane, l.v0),
        v1: vec2_plane_xform(plane, l.v1),
    };
    return r;
}

line line_plane_xform_inv(plane plane, line l) {
    line r = {
        v0: vec2_plane_xform_inv(plane, l.v0),
        v1: vec2_plane_xform_inv(plane, l.v1),
    };
    return r;
}


int main(int argc, char *argv[])
{
    putenv("SDL_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR=0"); 

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error initialising SDL: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    SDL_Log("SDL initialised\n");

    const int soften_term = 21;

    const int initial_win_width  = 1200;
    const int initial_win_height =  800;

    const vec2 origin = {0.0f, 0.0f};

    SDL_Window *window;
    SDL_Renderer *renderer;

    int win_width  = initial_win_width;
    int win_height = initial_win_height;

    const plane initial_viewport = {
        {0.0f, 0.0f},
        {1.0f, 1.0f},
    };

    plane viewport = initial_viewport;

    {
        const char *title = "hello world";
        int width = initial_win_width;
        int height = initial_win_height;
        SDL_WindowFlags flags = 0
            | SDL_WINDOW_RESIZABLE
            ;
        SDL_CreateWindowAndRenderer(title, width, height, flags, &window, &renderer);
    }

    bool shift_down = false;

    // [todo] add options to reflect the viewport on yaxis/xaxis (just multiply the scale by -1) 

    while (true) {
        plane old_viewport = viewport;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_EVENT_QUIT:
                goto quit;
                break;
            case SDL_EVENT_KEY_UP:
                switch (e.key.key) {
                case SDLK_LSHIFT: case SDLK_RSHIFT:
                    shift_down = false;
                    break;
                }
                break;
            case SDL_EVENT_KEY_DOWN:
                // [todo] reset viewport
                switch (e.key.key) {
                case SDLK_R:
                    viewport = initial_viewport;
                    break;
                case SDLK_ESCAPE:
                    goto quit;
                case SDLK_LSHIFT: case SDLK_RSHIFT:
                    shift_down = true;
                    break;
                case SDLK_UP:
                    if (shift_down) viewport.s.y += 0.025f;
                    else viewport.o.y += 0.1f;
                    break;
                case SDLK_DOWN:
                    if (shift_down) viewport.s.y = SDL_max(viewport.s.y-0.025f, 0.01f);
                    else viewport.o.y -= 0.1f;
                    break;
                case SDLK_RIGHT:
                    if (shift_down) viewport.s.x += 0.025f;
                    else viewport.o.x += 0.1f;
                    break;
                case SDLK_LEFT:
                    if (shift_down) viewport.s.x = SDL_max(viewport.s.x-0.025f, 0.01f);
                    else viewport.o.x -= 0.1f;
                    break;
                }
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                win_width  = e.window.data1;
                win_height = e.window.data2;
                break;
            // TODO: drag with cursor (complicated!)
            }
        }

        bool viewport_changed = 0 != memcmp((void *)&viewport, (void *)&old_viewport, sizeof(plane));
        if (viewport_changed) SDL_Log("viewport changed!");

        // long elapsed_ms = SDL_GetTicks();

        // white bg
        SDL_SetRenderDrawColor(renderer, 255-soften_term, 255-soften_term, 255-soften_term, 255);
        SDL_RenderClear(renderer);

        vec2 origin_vp  = vec2_plane_xform(viewport, origin);
        vec2 origin_win = vec2_win_xform(origin_vp, win_width, win_height);

        // Left-to-right.
        line xaxis_win  = {{0.0f, origin_win.y}, {win_width, origin_win.y}};

        // Top-to-bottom.
        line yaxis_win  = {{origin_win.x, 0.0f}, {origin_win.x, win_height}};

        // draw y-axis and x-axis
        SDL_SetRenderDrawColor(renderer, 0+soften_term, 0+soften_term, 0+soften_term, 255);
        SDL_RenderLine(renderer, LINE_SPREAD(xaxis_win));
        SDL_RenderLine(renderer, LINE_SPREAD(yaxis_win));

        // The function has to be plotted only over the x-values visible in the window.
        // To know which part of the x-axis is being displayed, we apply the inverses of the transforms:
        line xaxis_vp = line_win_xform_inv(xaxis_win, win_width, win_height);
        line xaxis_c  = line_plane_xform_inv(viewport, xaxis_vp);

        // For bounds-checking.
        line yaxis_vp = line_win_xform_inv(yaxis_win, win_width, win_height);
        line yaxis_c  = line_plane_xform_inv(viewport, yaxis_vp);

        if (viewport_changed){
            SDL_Log("---");
            SDL_Log("x-axis: (%.2f, %.2f) to (%.2f, %.2f)", LINE_SPREAD(xaxis_c));
            SDL_Log("y-axis: (%.2f, %.2f) to (%.2f, %.2f)", LINE_SPREAD(yaxis_c));
        }

        // draw function
        SDL_SetRenderDrawColor(renderer, 0+soften_term, 0+soften_term, 255-soften_term, 255);

        float resolution = 0.1f;

        vec2 v1 = {xaxis_c.v0.x, SDL_sin(xaxis_c.v0.x)};

        int line_count = 0;
        for (
            float c_x2 = xaxis_c.v0.x + resolution;
            c_x2 < xaxis_c.v1.x;
            c_x2 += resolution
        ) {
            vec2 v2 = {c_x2, SDL_sin(c_x2)};

            line l_c   = {v1, v2};
            line l_vp  = line_plane_xform(viewport, l_c);
            line l_win = line_win_xform(l_vp, win_width, win_height);
            SDL_RenderLine(renderer, LINE_SPREAD(l_win));

            v1 = v2;
            line_count++;
        }

        if (viewport_changed){
            SDL_Log("---");
            SDL_Log("x-axis: (%.2f, %.2f) to (%.2f, %.2f)", LINE_SPREAD(xaxis_c));
            SDL_Log("y-axis: (%.2f, %.2f) to (%.2f, %.2f)", LINE_SPREAD(yaxis_c));
            SDL_Log("lines: %d", line_count);
        }


        SDL_RenderPresent(renderer);
    }

quit:
    SDL_Log("Quitting\n");

    return EXIT_SUCCESS;
}
