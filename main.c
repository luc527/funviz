#include <stdlib.h>
#include <stdio.h>
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

float lerp(float a, float b, float amount) {
    return a + (b - a) * amount;
}

vec2 vec2_lerp(vec2 a, vec2 b, float amount) {
    vec2 r = {
        x: lerp(a.x, b.x, amount),
        y: lerp(a.y, b.y, amount),
    };
    return r;
}

plane plane_lerp(plane a, plane b, float amount) {
    plane r = {
        o: vec2_lerp(a.o, b.o, amount),
        s: vec2_lerp(a.s, b.s, amount),
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

    const plane initial_viewport = {
        {0.0f, 0.0f},
        {1.0f, 1.0f},
    };

    const Uint64 anim_duration_ms = 80;

    const float scale_step = 0.04f;
    const float offset_step = 0.16f;


    SDL_Window *window;
    SDL_Renderer *renderer;

    plane viewport = initial_viewport;
    plane anim_prev_viewport = viewport;
    plane anim_curr_viewport = viewport;

    int win_width  = initial_win_width;
    int win_height = initial_win_height;

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


    // [todo] anim is nice, but keeping the key pressed down is jittery

    Uint64 anim_start_ms = 0;

    while (true) {
        // [todo] framelimit to 60fps

        Uint64 anim_elapsed_ms = SDL_GetTicks() - anim_start_ms;

        bool in_anim = anim_elapsed_ms <= anim_duration_ms;
        if (in_anim) {
            float z = (float) anim_elapsed_ms / anim_duration_ms;
            z = 1-SDL_pow(1-z, 3); // ease out cubic
            anim_curr_viewport = plane_lerp(anim_prev_viewport, viewport, z);
        } else {
            anim_curr_viewport = viewport;
        }

        plane viewport_before_pollevent = viewport;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (in_anim) continue;

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
                    if (shift_down) viewport.s.y += scale_step;
                    else viewport.o.y += offset_step;
                    break;
                case SDLK_DOWN:
                    if (shift_down) viewport.s.y = SDL_max(viewport.s.y-scale_step, 0.01f);
                    else viewport.o.y -= offset_step;
                    break;
                case SDLK_RIGHT:
                    if (shift_down) viewport.s.x += scale_step;
                    else viewport.o.x += offset_step;
                    break;
                case SDLK_LEFT:
                    if (shift_down) viewport.s.x = SDL_max(viewport.s.x-scale_step, 0.01f);
                    else viewport.o.x -= offset_step;
                    break;
                }
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                win_width  = e.window.data1;
                win_height = e.window.data2;
                break;
            // [todo] drag with cursor
            }
        }

        bool viewport_changed = 0 != memcmp((void *)&viewport, (void *)&viewport_before_pollevent, sizeof(plane));
        if (viewport_changed) {
            // SDL_Log("viewport changed!");
            anim_start_ms = SDL_GetTicks();
            anim_prev_viewport = viewport_before_pollevent;
            anim_curr_viewport = viewport_before_pollevent;
        }

        // change to just `viewport` to disable animations
        plane viewport_render = anim_curr_viewport;

        // white bg
        SDL_SetRenderDrawColor(renderer, 255-soften_term, 255-soften_term, 255-soften_term, 255);
        SDL_RenderClear(renderer);

        vec2 origin_vp  = vec2_plane_xform(viewport_render, origin);
        vec2 origin_win = vec2_win_xform(origin_vp, win_width, win_height);

        // Left-to-right.
        line xaxis_win = {{0.0f, origin_win.y}, {win_width, origin_win.y}};

        // Top-to-bottom.
        line yaxis_win = {{origin_win.x, 0.0f}, {origin_win.x, win_height}};

        // draw y-axis and x-axis
        SDL_SetRenderDrawColor(renderer, 0+soften_term, 0+soften_term, 0+soften_term, 255);
        SDL_RenderLine(renderer, LINE_SPREAD(xaxis_win));
        SDL_RenderLine(renderer, LINE_SPREAD(yaxis_win));

        // The function has to be plotted only over the x-values visible in the window.
        // To know which part of the x-axis is being displayed, we apply the inverses of the transforms:
        line xaxis_vp = line_win_xform_inv(xaxis_win, win_width, win_height);
        line xaxis_c  = line_plane_xform_inv(viewport_render, xaxis_vp);

        // For bounds-checking.
        line yaxis_vp = line_win_xform_inv(yaxis_win, win_width, win_height);
        line yaxis_c  = line_plane_xform_inv(viewport_render, yaxis_vp);

        // draw rulers
        {
            // for now hardcode 0.1 step size
            float step = 0.1f;
            // [todo] change dynamically based on x and y scale

            float ruler_height = (float) win_height / 150.0f;
            float ruler_width  = (float) win_width  / 150.0f;

            // origin rightwards
            for (float c_x = 0.0f; c_x < xaxis_c.v1.x; c_x += step) {
                vec2 point_c   = {c_x, xaxis_c.v0.y};
                vec2 point_vp  = vec2_plane_xform(viewport_render, point_c);
                vec2 point_win = vec2_win_xform(point_vp, win_width, win_height);
                line ruler_win = {
                    {point_win.x, point_win.y - ruler_height},
                    {point_win.x, point_win.y + ruler_height}
                };
                SDL_RenderLine(renderer, LINE_SPREAD(ruler_win));
            }

            // origin leftwards
            for (float c_x = 0.0f; c_x > xaxis_c.v0.x; c_x -= step) {
                vec2 point_c   = {c_x, xaxis_c.v0.y};
                vec2 point_vp  = vec2_plane_xform(viewport_render, point_c);
                vec2 point_win = vec2_win_xform(point_vp, win_width, win_height);
                line ruler_win = {
                    {point_win.x, point_win.y - ruler_height},
                    {point_win.x, point_win.y + ruler_height}
                };
                SDL_RenderLine(renderer, LINE_SPREAD(ruler_win));
            }

            // origin downwards
            for (float c_y = 0.0f; c_y < yaxis_c.v0.y; c_y += step) {
                vec2 point_c   = {yaxis_c.v0.x, c_y};
                vec2 point_vp  = vec2_plane_xform(viewport_render, point_c);
                vec2 point_win = vec2_win_xform(point_vp, win_width, win_height);
                line ruler_win = {
                    {point_win.x - ruler_width, point_win.y},
                    {point_win.x + ruler_width, point_win.y}
                };
                SDL_RenderLine(renderer, LINE_SPREAD(ruler_win));
            }

            // origin upwards
            for (float c_y = 0.0f; c_y > yaxis_c.v1.y; c_y -= step) {
                vec2 point_c   = {yaxis_c.v0.x, c_y};
                vec2 point_vp  = vec2_plane_xform(viewport_render, point_c);
                vec2 point_win = vec2_win_xform(point_vp, win_width, win_height);
                line ruler_win = {
                    {point_win.x - ruler_width, point_win.y},
                    {point_win.x + ruler_width, point_win.y}
                };
                SDL_RenderLine(renderer, LINE_SPREAD(ruler_win));
            }

        }

        // draw function
        int line_count = 0;
        {
            SDL_SetRenderDrawColor(renderer, 0+soften_term, 0+soften_term, 255-soften_term, 255);
    
            float resolution = 0.1f;
    
            vec2 v1 = {xaxis_c.v0.x, SDL_sin(xaxis_c.v0.x)};
    
            for (
                float c_x2 = xaxis_c.v0.x + resolution;
                c_x2 < xaxis_c.v1.x;
                c_x2 += resolution
            ) {
                vec2 v2 = {c_x2, SDL_sin(c_x2)};
    
                line l_c   = {v1, v2};
                line l_vp  = line_plane_xform(viewport_render, l_c);
                line l_win = line_win_xform(l_vp, win_width, win_height);
                SDL_RenderLine(renderer, LINE_SPREAD(l_win));
    
                v1 = v2;
                line_count++;
            }
        }

        // if (viewport_changed) {
        //     SDL_Log("---");
        //     SDL_Log("x-axis: (%.2f, %.2f) to (%.2f, %.2f)", LINE_SPREAD(xaxis_c));
        //     SDL_Log("y-axis: (%.2f, %.2f) to (%.2f, %.2f)", LINE_SPREAD(yaxis_c));
        //     SDL_Log("lines: %d", line_count);
        // }

        SDL_RenderPresent(renderer);
    }

quit:
    SDL_Log("Quitting\n");

    return EXIT_SUCCESS;
}
