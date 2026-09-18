#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include <minos/sysstd.h>
#include <minos/status.h>
#include <assert.h>
#define GT_IMPLEMENTATION
#include "gt.h"
#include <assert.h>
#include <minos/status.h>
#include <minos/fb/fb.h>
#include <minos/mouse.h>
#include "darray.h"
#include <unistd.h>
#include <fcntl.h>
#include "list_head.h"

#include <libwm.h>
#include <libwm/tags.h>
#include <libwm/events.h>
#include <libwm/key.h>

#define gtaccept(fd, ...) (gtblockfd(fd, GTBLOCKIN), accept(fd, __VA_ARGS__))
#define gtread(fd, ...)   (gtblockfd(fd, GTBLOCKIN), recv(fd, __VA_ARGS__, 0))
static ssize_t gtwrite(uintptr_t fd, const void* buf, size_t size) {
    gtblockfd(fd, GTBLOCKOUT);
    return send(fd, buf, size, 0);
}

static intptr_t gtread_exact(uintptr_t fd, void* buf, size_t size) {
    while(size) {
        intptr_t e = gtread(fd, buf, size);
        if(e < 0) return e;
        if(e == 0) return -PREMATURE_EOF; 
        buf = ((char*)buf) + (size_t)e;
size -= (size_t)e;
    }
    return 0;
}
static intptr_t gtwrite_exact(uintptr_t fd, const void* buf, size_t size) {
    while(size) {
        intptr_t e = gtwrite(fd, buf, size);
        if(e < 0) return e;
        if(e == 0) return -PREMATURE_EOF; 
        buf = ((char*)buf) + (size_t)e;
        size -= (size_t)e;
    }
    return 0;
}
// Other helpers
static intptr_t gtread_u32(uintptr_t fd, uint32_t* data) {
    return gtread_exact(fd, data, sizeof(*data));
}
static intptr_t gtread_u16(uintptr_t fd, uint16_t* data) {
    return gtread_exact(fd, data, sizeof(*data));
}
static intptr_t gtread_u8(uintptr_t fd, uint8_t* data) {
    return gtread_exact(fd, data, sizeof(*data));
}

static intptr_t gtwrite_u32(uintptr_t fd, uint32_t data) {
    return gtwrite_exact(fd, &data, sizeof(data));
}
static intptr_t gtwrite_u16(uintptr_t fd, uint16_t data) {
    return gtwrite_exact(fd, &data, sizeof(data));
}
static intptr_t gtwrite_u8(uintptr_t fd, uint8_t data) {
    return gtwrite_exact(fd, &data, sizeof(data));
}
static intptr_t gtwrite_i32(uintptr_t fd,  int32_t data) {
    return gtwrite_u32(fd, (uint32_t)data);
}


#define error(...) (fprintf(stderr, "ERROR:" __VA_ARGS__), fprintf(stderr, "\n"))
#define info(...)  (fprintf(stderr, "INFO:"  __VA_ARGS__), fprintf(stderr, "\n"))
#if 1
#   define trace(...)
#else
#   define trace(...) (fprintf(stderr, "TRACE:" __VA_ARGS__), fprintf(stderr, "\n"))
#endif

#define STRINGIFY1(x) # x
#define STRINGIFY2(x) STRINGIFY1(x)
#define unreachable(...) (error(__FILE__ ":" STRINGIFY2(__LINE__) " unreachable " __VA_ARGS__), abort())
// Shapes
typedef struct {
    size_t l, t, r, b;
} Rectangle;
typedef struct {
    size_t x, y;
} Vec2size;
static bool rect_collide_point(const Rectangle* a, const Vec2size* p) {
    return p->x >= a->l && p->x < a->r && 
           p->y >= a->t && p->y < a->b;
}
static bool rect_collides(const Rectangle* a, const Rectangle* b) {
    return a->l < b->r && a->r > b->l && 
           a->t < b->b && a->b > b->t;
}
static Rectangle rect_collision_rect(const Rectangle* a, const Rectangle* b) {
    #define max(a, b) (a) > (b) ? (a) : (b)
    #define min(a, b) (a) < (b) ? (a) : (b)
    return (Rectangle) {
        max(a->l, b->l),
        max(a->t, b->t),
        min(a->r, b->r),
        min(a->b, b->b),
    };
}
#define WINDOW_NAME_MAX 64
typedef struct {
    uint32_t* pixels;
    size_t width, height, pitch_bytes;
} Image;
typedef struct Client Client;
typedef struct {
    struct list_head list;
    char name[WINDOW_NAME_MAX];
    Rectangle rect;
    uint32_t* content;
    // The client this window belongs to
    Client* parent_client;
    // Index within the child list;
    size_t child_index;
    // Border:
    uint32_t border_color;
    uint32_t border_thick;
    // Menu:
    uint32_t menu_color;
    uint32_t menu_height;
    // Compositing:
    uint8_t alpha;
    bool has_transparency;
    bool blur_background;
    // Cached blurred background for content area
    uint32_t* cached_blur;
    size_t cached_blur_w, cached_blur_h;
    int cached_blur_x, cached_blur_y;  // Screen position of cached blur
    bool cached_blur_valid;

    int running;
} Window;
typedef struct {
    Window **items;
    size_t len, cap;
} Windows;
// TODO: only map when necessary
typedef struct {
    long key;
    void* addr;
    size_t size;
} SHMRegion;
typedef struct {
    SHMRegion* items;
    size_t len, cap;
} SHMRegions;
typedef struct Client {
    struct list_head list;
    int fd;
    Windows child_windows;
    SHMRegions shm_regions;
} Client;


typedef struct {
    uint32_t* pixels;
    size_t width, height, pitch_bytes;
    // Front of the double buffering
    uint32_t* hw_pixels;
    size_t hw_width, hw_height, hw_pitch_bytes;
} Framebuffer;

// Drawing
static void fill_rect(const Framebuffer* fb, const Rectangle* rect, uint32_t color) {
    uint32_t* head = (uint32_t*)(((uint8_t*)fb->pixels) + fb->pitch_bytes*rect->t);
    for(size_t y = rect->t; y < rect->b && y < fb->height; ++y) {
        for(size_t x = rect->l; x < rect->r && x < fb->width; ++x) {
            head[x] = color;
        }
        head = (uint32_t*)(((uint8_t*)head) + fb->pitch_bytes);
    }
}

// Fast alpha blending: dst = src * alpha + dst * (1 - alpha)
// Using integer arithmetic for performance
static inline uint32_t blend_pixel(uint32_t src, uint32_t dst) {
    uint8_t src_a = (src >> 24) & 0xFF;
    if (src_a == 0) return dst;
    if (src_a == 255) return src;
    
    uint8_t dst_a = (dst >> 24) & 0xFF;
    uint8_t inv_src_a = 255 - src_a;
    
    uint32_t src_r = (src >> 16) & 0xFF;
    uint32_t src_g = (src >> 8) & 0xFF;
    uint32_t src_b = src & 0xFF;
    
    uint32_t dst_r = (dst >> 16) & 0xFF;
    uint32_t dst_g = (dst >> 8) & 0xFF;
    uint32_t dst_b = dst & 0xFF;
    
    uint32_t r = (src_r * src_a + dst_r * inv_src_a) / 255;
    uint32_t g = (src_g * src_a + dst_g * inv_src_a) / 255;
    uint32_t b = (src_b * src_a + dst_b * inv_src_a) / 255;
    uint32_t a = src_a + (dst_a * inv_src_a) / 255;
    
    return (a << 24) | (r << 16) | (g << 8) | b;
}

static void draw_image_alpha(const Framebuffer* fb, const Image* image, size_t x, size_t y) {
    uint32_t* image_head = image->pixels;
    uint32_t* head = (uint32_t*)(((uint8_t*)fb->pixels) + fb->pitch_bytes*y);
    for(size_t dy = 0; dy < image->height && y + dy < fb->height; ++dy) {
        for(size_t dx = 0; dx < image->width && x + dx < fb->width; ++dx) {
            uint32_t src = image_head[dx];
            if (src & (0xFF << 24)) {
                head[x + dx] = blend_pixel(src, head[x + dx]);
            }
        }
        head = (uint32_t*)(((uint8_t*)head) + fb->pitch_bytes);
        image_head = (uint32_t*)(((uint8_t*)image_head) + image->pitch_bytes); 
    }
}

static void draw_image(const Framebuffer* fb, const Image* image, size_t x, size_t y) {
    uint32_t* image_head = image->pixels;
    uint32_t* head = (uint32_t*)(((uint8_t*)fb->pixels) + fb->pitch_bytes*y);
    for(size_t dy = 0; dy < image->height && y + dy < fb->height; ++dy) {
        for(size_t dx = 0; dx < image->width && x + dx < fb->width; ++dx) {
            if(image_head[dx] & (0xFF << 24)) {
                head[x + dx] = image_head[dx];
            }
        }
        head = (uint32_t*)(((uint8_t*)head) + fb->pitch_bytes);
        image_head = (uint32_t*)(((uint8_t*)image_head) + image->pitch_bytes); 
    }
}
static void draw_image_no_alpha(const Framebuffer* fb, const Image* image, size_t x, size_t y) {
    uint32_t* image_head = image->pixels;
    uint32_t* head = (uint32_t*)(((uint8_t*)fb->pixels) + fb->pitch_bytes*y);
    for(size_t dy = 0; dy < image->height && y + dy < fb->height; ++dy) {
        for(size_t dx = 0; dx < image->width && x + dx < fb->width; ++dx) {
            head[x + dx] = image_head[dx];
        }
        head = (uint32_t*)(((uint8_t*)head) + fb->pitch_bytes);
        image_head = (uint32_t*)(((uint8_t*)image_head) + image->pitch_bytes); 
    }
}

// Forward declarations
static void window_redraw_region(const Framebuffer* fb, const Window* win, const Rectangle* rect, bool skip_content);
static void redraw_region(const Framebuffer* fb, const Rectangle* rect);

static uint32_t abgr_to_argb(uint32_t a) {
    uint8_t alpha = (a >> 24) & 0xFF; 
    uint8_t blue = (a >> 16) & 0xFF;   
    uint8_t green = (a >> 8) & 0xFF; 
    uint8_t red = a & 0xFF;            
    return (alpha << 24) | (red << 16) | (green << 8) | blue;
}

static bool load_image(const char* file, Image* image) {
    int width, height, comp = 0;
    void* pixels = stbi_load(file, &width, &height, &comp, 4);
    if(!pixels) {
        error("Failed to load `%s`: %s", file, stbi_failure_reason());
        return false;
    }
    uint32_t* head = pixels;
    for(size_t y = 0; y < (size_t)height; ++y) {
        for(size_t x = 0; x < (size_t)width; ++x) {
            head[x] = abgr_to_argb(head[x]);
        }
        head += width;
    }
    image->pixels = pixels;
    image->width = width;
    image->height = height;
    image->pitch_bytes = width * sizeof(*image->pixels);
    return true;
}
// Shared global resources.
// Images, icons or what have you :D
static Image cursor    = { 0 };
static Image icon_x    = { 0 }, icon_x_hover    = { 0 },
             icon_min  = { 0 }, icon_min_hover  = { 0 },
             icon_hide = { 0 }, icon_hide_hover = { 0 };
static Image wallpaper_img = { 0 };

// (per user? per workspace?) state 
static Window* moving_window = NULL;
static int moving_window_dx = 0, moving_window_dy = 0;
// Track previous wireframe position for cleanup
static Rectangle prev_wireframe = {0, 0, 0, 0};
static bool prev_wireframe_valid = false;
// (per workspace?)
static struct list_head windows = { 0 };
// ....
static Framebuffer fb0;
// Input state
static size_t mouse_x = 0,
              mouse_y = 0;
// Helper functions
static Rectangle cursor_rect(void) {
    return (Rectangle){mouse_x, mouse_y, mouse_x + cursor.width, mouse_y + cursor.height};
}
static Vec2size cursor_point(void) {
    return (Vec2size){mouse_x, mouse_y};
}
static bool collides_cursor(const Rectangle* rect) {
    Vec2size cursor = cursor_point();
    return rect_collide_point(rect, &cursor);
}
// Components of the screen
static void wallpaper_redraw_region(const Framebuffer* fb, const Rectangle* rect) {
    if(wallpaper_img.pixels) {
        // Stretch wallpaper to screen size
        for(size_t y = rect->t; y < rect->b; y++) {
            size_t src_y = (y * wallpaper_img.height) / fb->height;
            uint32_t* src_row = (uint32_t*)((uint8_t*)wallpaper_img.pixels + src_y * wallpaper_img.pitch_bytes);
            uint32_t* dst_row = (uint32_t*)((uint8_t*)fb->pixels + y * fb->pitch_bytes);
            
            for(size_t x = rect->l; x < rect->r; x++) {
                size_t src_x = (x * wallpaper_img.width) / fb->width;
                dst_row[x] = src_row[src_x];
            }
        }
    } else {
        fill_rect(fb, rect, 0x000000);
    }
}
// Window:
enum {
    WINDOW_BORDER_L,
    WINDOW_BORDER_T,
    WINDOW_BORDER_R,
    WINDOW_BORDER_B,
    WINDOW_BORDER_COUNT
};
// NOTE: I guess we can make this more generic as "rect_get_edge" and provide thickness externally
// but I don't really care for now
static Rectangle window_get_border_rect(const Window* win, size_t n) {
    switch(n) {
    case WINDOW_BORDER_L:
        return (Rectangle) {
            win->rect.l, win->rect.t,
            win->rect.l + win->border_thick, win->rect.b
        };
    case WINDOW_BORDER_T:
        return (Rectangle) {
            win->rect.l, win->rect.t,
            win->rect.r, win->rect.t + win->border_thick
        };
    case WINDOW_BORDER_R:
        return (Rectangle) {
            win->rect.r - win->border_thick, win->rect.t,
            win->rect.r, win->rect.b
        };
    case WINDOW_BORDER_B:
        return (Rectangle) {
            win->rect.l, win->rect.b - win->border_thick,
            win->rect.r, win->rect.b    
        };
    }
    unreachable("window_get_border_rect n=%zu", n);
}
static Rectangle window_get_menu_rect(const Window* win) {
    return (Rectangle){
        win->rect.l + win->border_thick, win->rect.t + win->border_thick,
        win->rect.r - win->border_thick, win->rect.t + win->border_thick + win->menu_height
    };
}
static Vec2size menu_get_x_pos(const Window* win) {
    return (Vec2size) {
        win->rect.r - win->border_thick - 14*1,
        win->rect.t + win->border_thick
    };
}
static Rectangle menu_get_x_rect(const Window* win) {
    Vec2size pos = menu_get_x_pos(win);
    return (Rectangle) {
        pos.x, pos.y,
        pos.x + 14, pos.y + 14
    };
}
static Vec2size menu_get_min_pos(const Window* win) {
    return (Vec2size) {
        win->rect.r - win->border_thick - 14*2,
        win->rect.t + win->border_thick
    };
}
static Rectangle menu_get_min_rect(const Window* win) {
    Vec2size pos = menu_get_min_pos(win);
    return (Rectangle) {
        pos.x, pos.y,
        pos.x + 14, pos.y + 14
    };
}
static Vec2size menu_get_hide_pos(const Window* win) {
    return (Vec2size) {
        win->rect.r - win->border_thick - 14*3,
        win->rect.t + win->border_thick
    };
}
static Rectangle menu_get_hide_rect(const Window* win) {
    Vec2size pos = menu_get_hide_pos(win);
    return (Rectangle) {
        pos.x, pos.y,
        pos.x + 14, pos.y + 14
    };
}
static void menu_redraw_region(const Framebuffer* fb, const Window* win, const Rectangle* rect) {
    fill_rect(fb, rect, win->menu_color);
    Rectangle x_rect = menu_get_x_rect(win);
    if(rect_collides(&x_rect, rect)) {
        fill_rect(fb, &x_rect, win->menu_color);
        // TODO: This can be optimised to only draw a part of the image I guess.
        draw_image(fb, collides_cursor(&x_rect) ? &icon_x_hover : &icon_x, x_rect.l, x_rect.t);
    }
    Rectangle min_rect = menu_get_min_rect(win);
    if(rect_collides(&min_rect, rect)) {
        fill_rect(fb, &min_rect, win->menu_color);
        // TODO: This can be optimised to only draw a part of the image I guess.
        draw_image(fb, collides_cursor(&min_rect) ? &icon_min_hover : &icon_min, min_rect.l, min_rect.t);
    }
    Rectangle hide_rect = menu_get_hide_rect(win);
    if(rect_collides(&hide_rect, rect)) {
        fill_rect(fb, &hide_rect, win->menu_color);
        // TODO: This can be optimised to only draw a part of the image I guess.
        draw_image(fb, collides_cursor(&hide_rect) ? &icon_hide_hover : &icon_hide, hide_rect.l, hide_rect.t);
    }
}
static Rectangle window_get_content_rect(const Window* win) {
    return (Rectangle) {
        win->rect.l + win->border_thick, win->rect.t + win->border_thick + win->menu_height,
        win->rect.r - win->border_thick, win->rect.b - win->border_thick
    };
}

// Compute blur from wallpaper for a window's content area
// Writes directly to the provided destination buffer (compact, pitch = width * 4)
// Caller must ensure dst has size w*h*sizeof(uint32_t)
static void compute_blur(const Rectangle* content_rect, uint32_t* dst) {
    const int BLUR_RADIUS = 7;  // 15x15 kernel - glassmorphism style
    size_t w = content_rect->r - content_rect->l;
    size_t h = content_rect->b - content_rect->t;
    if (w == 0 || h == 0) return;

    for (size_t y = 0; y < h; ++y) {
        uint32_t* dst_row = dst + y * w;
        size_t screen_y = content_rect->t + y;
        
        for (size_t x = 0; x < w; ++x) {
            size_t screen_x = content_rect->l + x;
            
            uint32_t sum_r = 0, sum_g = 0, sum_b = 0, sum_a = 0;
            int count = 0;
            
            for (int dy = -BLUR_RADIUS; dy <= BLUR_RADIUS; ++dy) {
                for (int dx = -BLUR_RADIUS; dx <= BLUR_RADIUS; ++dx) {
                    size_t sy = screen_y + dy;
                    size_t sx = screen_x + dx;
                    if (sy < fb0.height && sx < fb0.width) {
                        size_t src_y = (sy * wallpaper_img.height) / fb0.height;
                        size_t src_x = (sx * wallpaper_img.width) / fb0.width;
                        if (src_y >= wallpaper_img.height) src_y = wallpaper_img.height - 1;
                        if (src_x >= wallpaper_img.width) src_x = wallpaper_img.width - 1;
                        uint32_t* src_row = (uint32_t*)((uint8_t*)wallpaper_img.pixels + src_y * wallpaper_img.pitch_bytes);
                        uint32_t p = src_row[src_x];
                        sum_a += (p >> 24) & 0xFF;
                        sum_r += (p >> 16) & 0xFF;
                        sum_g += (p >> 8) & 0xFF;
                        sum_b += p & 0xFF;
                        count++;
                    }
                }
            }
            if (count > 0) {
                dst_row[x] = ((sum_a / count) << 24) | ((sum_r / count) << 16) | ((sum_g / count) << 8) | (sum_b / count);
            }
        }
    }
}

// Ensure window's cached blur is valid for its current content rect
// Regenerates if window moved/resized
static void ensure_cached_blur(Window* win) {
    Rectangle content_rect = window_get_content_rect(win);
    size_t w = content_rect.r - content_rect.l;
    size_t h = content_rect.b - content_rect.t;
    if (w == 0 || h == 0) return;

    bool cache_valid = win->cached_blur_valid &&
                       win->cached_blur_w == w &&
                       win->cached_blur_h == h &&
                       win->cached_blur_x == (int)content_rect.l &&
                       win->cached_blur_y == (int)content_rect.t;

    if (!cache_valid) {
        if (win->cached_blur) free(win->cached_blur);
        win->cached_blur = malloc(w * h * sizeof(uint32_t));
        if (!win->cached_blur) return;
        win->cached_blur_w = w;
        win->cached_blur_h = h;
        win->cached_blur_x = content_rect.l;
        win->cached_blur_y = content_rect.t;
        win->cached_blur_valid = true;
        compute_blur(&content_rect, win->cached_blur);
    }
}

// Invalidate cached blur when window moves/resizes
static void invalidate_cached_blur(Window* win) {
    win->cached_blur_valid = false;
}

// Composite a single window onto the framebuffer with proper alpha blending
// This draws: border, menu, and content (with alpha blending)
// If skip_content is true, only draw borders and menu (for move optimization)
static void composite_window(const Framebuffer* fb, const Window* win, const Rectangle* clip, bool skip_content) {
    // Draw borders
    for(size_t i = 0; i < WINDOW_BORDER_COUNT; ++i) {
        Rectangle border_rect = window_get_border_rect(win, i);
        if(rect_collides(&border_rect, clip)) {
            Rectangle area = rect_collision_rect(&border_rect, clip);
            fill_rect(fb, &area, win->border_color);
        }
    }
    
    // Draw content with alpha blending (skip during move for performance)
    if (!skip_content) {
        Rectangle content_rect = window_get_content_rect(win);
        if(rect_collides(&content_rect, clip) && win->content) {
            Rectangle area = rect_collision_rect(&content_rect, clip);
            
// Draw background for content area
            if (win->has_transparency && win->blur_background && wallpaper_img.pixels) {
                // Ensure cached blur is up to date
                ensure_cached_blur((Window*)win);
                
                // Use cached blurred background
                if (win->cached_blur && win->cached_blur_valid) {
                    Rectangle content_rect = window_get_content_rect(win);
                    Rectangle area = rect_collision_rect(&content_rect, clip);
                    size_t local_l = area.l - content_rect.l;
                    size_t local_t = area.t - content_rect.t;
                    size_t aw = area.r - area.l;
                    size_t ah = area.b - area.t;
                    
                    uint32_t* dst = (uint32_t*)(((uint8_t*)fb->pixels) + fb->pitch_bytes * area.t) + area.l;
                    for (size_t y = 0; y < ah; ++y) {
                        uint32_t* src_row = win->cached_blur + (local_t + y) * win->cached_blur_w + local_l;
                        for (size_t x = 0; x < aw; ++x) {
                            dst[x] = src_row[x];
                        }
                        dst = (uint32_t*)(((uint8_t*)dst) + fb->pitch_bytes);
                    }
                }
            } else if (win->has_transparency) {
                // Transparency without blur: just draw normal wallpaper
                wallpaper_redraw_region(fb, &area);
            } else {
                // No transparency: draw wallpaper as base
                wallpaper_redraw_region(fb, &area);
            }
            
            // Composite window content with alpha blending
            size_t local_l_content = area.l - content_rect.l;
            size_t local_t_content = area.t - content_rect.t;
            size_t content_pitch_bytes = (content_rect.r - content_rect.l) * sizeof(*win->content);
            Image image = {
                .pixels = ((uint32_t*)(((uint8_t*)win->content) + (local_t_content * content_pitch_bytes))) + (local_l_content),
                .width = area.r - area.l,
                .height = area.b - area.t,
                .pitch_bytes = (content_rect.r - content_rect.l) * sizeof(*win->content)
            };
            
            if (win->has_transparency) {
                draw_image_alpha(fb, &image, area.l, area.t);
            } else {
                draw_image_no_alpha(fb, &image, area.l, area.t);
            }
        }
    }
    
    // Draw menu bar
    Rectangle menu_rect = window_get_menu_rect(win);
    if(rect_collides(&menu_rect, clip)) {
        Rectangle area = rect_collision_rect(&menu_rect, clip);
        menu_redraw_region(fb, win, &area);
    }
}

static void redraw_region(const Framebuffer* fb, const Rectangle* rect) {
    Rectangle clipped = *rect;
    if(clipped.l > fb->width) clipped.l = fb->width;
    if(clipped.t > fb->height) clipped.t = fb->height;
    if(clipped.r > fb->width) clipped.r = fb->width;
    if(clipped.b > fb->height) clipped.b = fb->height;
    if(clipped.l == clipped.r || clipped.b == clipped.t) return;
    
    // Check if any window covers this region
    for(struct list_head* head = windows.next; head != &windows; head = head->next) {
        const Window* win = (Window*)head;
        if(rect_collides(&clipped, &win->rect)) {
            Rectangle area = rect_collision_rect(&clipped, &win->rect);
            
            // رندر کردن قسمت‌های پشت پنجره
            if(area.l != clipped.l) 
                redraw_region(fb, &(Rectangle){clipped.l, clipped.t, area.l, clipped.b});
            if(area.r != clipped.r) 
                redraw_region(fb, &(Rectangle){area.r, clipped.t, clipped.r, clipped.b});
            if(area.t != clipped.t) 
                redraw_region(fb, &(Rectangle){clipped.l, clipped.t, clipped.r, area.t});
            if(area.b != clipped.b) 
                redraw_region(fb, &(Rectangle){clipped.l, area.b, clipped.r, clipped.b});
            
            // رندر کردن خود پنجره (با چک کردن معتبر بودن)
            if (win && win->content) {
                window_redraw_region(fb, win, &area, false);
            }
            return;
        }
    }
    
    wallpaper_redraw_region(fb, &clipped);
}

static void window_redraw_region(const Framebuffer* fb, const Window* win, const Rectangle* rect, bool skip_content) {
    // چک کردن اشاره‌گرها
    if (!win || !win->content) return;
    
    composite_window(fb, win, rect, skip_content);
}

static void draw_window(const Framebuffer* fb, const Window* win) {
    window_redraw_region(fb, win, &win->rect, false);
}

// Draw a simple wireframe rectangle (classic Windows 95/XP style drag outline)
static void draw_drag_outline(const Framebuffer* fb, const Rectangle* rect, uint32_t color) {
    // Top border
    fill_rect(fb, &(Rectangle){rect->l, rect->t, rect->r, rect->t + 2}, color);
    // Bottom border
    fill_rect(fb, &(Rectangle){rect->l, rect->b - 2, rect->r, rect->b}, color);
    // Left border
    fill_rect(fb, &(Rectangle){rect->l, rect->t, rect->l + 2, rect->b}, color);
    // Right border
    fill_rect(fb, &(Rectangle){rect->r - 2, rect->t, rect->r, rect->b}, color);
}

static void move_window(const Framebuffer* fb, Window* win, size_t x, size_t y) {
    size_t w = win->rect.r - win->rect.l;
    size_t h = win->rect.b - win->rect.t;
    
    // Strict clamping - NEVER go off-screen
    if(x > fb->width - w) x = fb->width - w;
    if(y > fb->height - h) y = fb->height - h;
    if((int)x < 0) x = 0;
    if((int)y < 0) y = 0;
    
    Rectangle new_rect = {x, y, x + w, y + h};
    
    // Draw wireframe outline at new position (no blur, no content, very fast)
    draw_drag_outline(fb, &new_rect, 0xFFFFFFFF);  // White outline
}
static void load_image_required(const char* path, Image* image) {
    if(!load_image(path, image)) exit(1);
}
static void load_resources(void) {
    load_image_required("./res/cursor.png", &cursor);
    load_image_required("./res/X_nohover.png", &icon_x);
    load_image_required("./res/X_hover.png", &icon_x_hover);
    load_image_required("./res/Min_nohover.png", &icon_min);
    load_image_required("./res/Min_hover.png", &icon_min_hover);
    load_image_required("./res/Hide_nohover.png", &icon_hide);
    load_image_required("./res/Hide_hover.png", &icon_hide_hover);
    load_image_required("./res/wallpaper.png", &wallpaper_img);
}
static intptr_t load_framebuffer(const char* path, Framebuffer* fb) {
    intptr_t e = open(path, O_WRONLY);
    if(e < 0) return e;
    uintptr_t fb_handle = (uintptr_t)e;
    FbStats stats;
    if((e=fbget_stats(fb_handle, &stats)) < 0) {
        close(fb_handle);
        return e;
    }
    if(stats.bpp != 32) {
        error("Unsupported fb.bpp == %zu", (size_t)stats.bpp);
        close(fb_handle);
        return -UNSUPPORTED;
    }
    fb->width  = fb->hw_width  = stats.width;
    fb->height = fb->hw_height = stats.height;
    fb->hw_pitch_bytes = stats.pitch_bytes;
    // Allocate front buffer
    fb->pixels = malloc(fb->width * fb->height * sizeof(*fb->pixels));
    fb->pitch_bytes = fb->width * sizeof(*fb->pixels);
    if(!fb->pixels) {
        close(fb_handle);
        return -NOT_ENOUGH_MEM;
    }
    memset(fb->pixels, 0, sizeof(*fb->pixels));
    fb->hw_pixels = mmap(NULL, 0, PROT_WRITE, MAP_PRIVATE, fb_handle, 0);
    if(fb->hw_pixels == MAP_FAILED) {
        free(fb->pixels);
        close(fb_handle);
        return -1;
    }
    close(fb_handle);
    // For debug testing purposes.
    // We'll set the framebuffer to the bottom right so we see the logs
    // fb->hw_width = fb->width /= 2;
    // fb->hw_height = fb->height /= 2;
    // fb->hw_pixels = (uint32_t*)(((uint8_t*)fb->hw_pixels) + fb->hw_height * fb->hw_pitch_bytes) + fb->width;
    return 0;
}
void flush_framebuffer(Framebuffer* fb) {
    uint32_t* hw_head = fb->hw_pixels;
    uint32_t* head = fb->pixels;
    assert(fb->height == fb->hw_height);
    assert(fb->width == fb->hw_width);
    for(size_t y = 0; y < fb->height; ++y) {
        for(size_t x = 0; x < fb->width; ++x) hw_head[x] = head[x];
        hw_head = (uint32_t*)(((uint8_t*)hw_head) + fb->hw_pitch_bytes);
        head    = (uint32_t*)(((uint8_t*)head) + fb->pitch_bytes);
    }
}

typedef struct {
    uint32_t payload_size;
    uint16_t tag;
} Packet;
intptr_t read_packet(int fd, Packet* packet) {
    intptr_t e;
    e = gtread_u32(fd, &packet->payload_size);
    if(e < 0) {
        error("Failed to read on fd. %s", status_str(e));
        return e;
    }
    if(packet->payload_size < sizeof(packet->tag)) {
        error("Payload size may not be less than %zu!", sizeof(packet->tag));
        return -SIZE_MISMATCH;
    }
    e = gtread_u16(fd, &packet->tag);
    if(e < 0) {
        error("Failed to read tag: %s", status_str(e));
        return e;
    } 
    packet->payload_size -= sizeof(packet->tag);
    return 0;
}
size_t write_memory_packet(void* buf, uint32_t payload_size, uint16_t tag) {
    payload_size += sizeof(tag);
    size_t n = 0;
    *(uint32_t*)(((char*)buf) + n) = payload_size;
    n += 4;
    *(uint16_t*)(((char*)buf) + n) = tag;
    n += 2;
    return n;
}
intptr_t send_response_packet(int fd, int resp) {
    char buf[128];
    size_t n = 0;
    n += write_memory_packet(buf + n, sizeof(uint32_t), WM_PACKET_TAG_RESULT);
    *(uint32_t*)(((char*)buf) + n) = resp;
    n += 4;
    return gtwrite_exact(fd, buf, n);
}
typedef ssize_t (*wm_write_t)(void*, const void*, size_t);
intptr_t send_event(uintptr_t fd, const WmEvent* event) {
    char buf[128];
    size_t n = 0;
    n += write_memory_packet(buf + n, size_WmEvent(event), WM_PACKET_TAG_EVENT);
    n += write_memory_WmEvent(buf + n, event);
    return gtwrite_exact(fd, buf, n);
}
void client_thread(void* client_void) {
    Client* client = client_void;
    intptr_t e;
    Packet packet = { 0 };
    struct {
        uint8_t* items;
        size_t len, cap;
    } payload_buffer = { 0 };
    for(;;) {
        if((e = read_packet(client->fd, &packet)) < 0) {
            error("Failed to read_packet: %s", status_str(e));
            break;
        }
        switch(packet.tag) {
        case WM_PACKET_TAG_CREATE_WINDOW: {
            if(packet.payload_size < min_WmCreateWindowInfo_size) {
                error("Packet too small in WmCreateWindowInfo");
                send_response_packet(client->fd, -SIZE_MISMATCH);
                continue;
            }
            if(packet.payload_size > max_WmCreateWindowInfo_size) {
                error("Packet too big in WmCreateWindowInfo");
                send_response_packet(client->fd, -LIMITS);
                continue;
            }
            da_reserve(&payload_buffer, packet.payload_size);
            if((e = gtread_exact(client->fd, payload_buffer.items, packet.payload_size)) < 0) {
                error("Failed to read payload for WmCreateWindowInfo");
                send_response_packet(client->fd, e);
                continue;
            }
            WmCreateWindowInfo info = { 0 };
            if((e=read_memory_WmCreateWindowInfo(payload_buffer.items, packet.payload_size, &info)) < 0) {
                error("Failed to read WmCreateWindowInfo");
                send_response_packet(client->fd, e);
                continue;
            }

            trace("Creating window `%s`", info.title);
            Window* window = malloc(sizeof(*window));
            if(!window) {
                cleanup_WmCreateWindowInfo(&info);
                error("Out of memory");
                send_response_packet(client->fd, -NOT_ENOUGH_MEM);
                continue;
            }
            memset(window, 0, sizeof(*window));
            list_init(&window->list);
            window->parent_client = client;
            memcpy(window->name, info.title, info.title_len);
            // TODO: Maybe place randomly
            if(info.x == (uint32_t)-1) info.x = 100;
            if(info.y == (uint32_t)-1) info.y = 100;

            window->border_thick = 2;
            window->border_color = 0x000000;
            window->menu_color = 0x000000;
            window->menu_height = 16;
            // Compositing defaults
            window->alpha = 255;
            window->has_transparency = false;
            window->blur_background = false;
            // Cached blur defaults
            window->cached_blur = NULL;
            window->cached_blur_w = 0;
            window->cached_blur_h = 0;
            window->cached_blur_x = 0;
            window->cached_blur_y = 0;
            window->cached_blur_valid = false;
            // Apply window flags from client
            if (info.flags & WM_WINDOW_FLAG_TRANSPARENT) {
                window->has_transparency = true;
                window->alpha = 255;  // Per-pixel alpha from content
            }
            if (info.flags & WM_WINDOW_FLAG_BLUR_BACKGROUND) {
                window->blur_background = true;
            }

            window->rect.l = info.x;
            window->rect.t = info.y;
            window->rect.r = info.x + info.width + window->border_thick * 2;
            window->rect.b = info.y + info.height + window->menu_height + window->border_thick * 2;

            // TODO: check for out of memory
            // memset(window->content, 0, (content.r-content.l) * (content.b-content.t) * sizeof(uint32_t));
            // window->clear_color = 0xFF00FF00;
            Rectangle content = window_get_content_rect(window);
            window->content = realloc(window->content, (content.r-content.l) * (content.b-content.t) * sizeof(uint32_t));
            assert(window->content && "Ran out of memory");
            memset(window->content, 0, (content.r-content.l) * (content.b-content.t) * sizeof(uint32_t));
            size_t width = (content.r-content.l);
            size_t height = (content.b-content.t);
            da_push(&client->child_windows, window);
            list_insert(&windows, &window->list);
            redraw_region(&fb0, &window->rect);
            flush_framebuffer(&fb0);
            window->child_index = client->child_windows.len-1;
            send_response_packet(client->fd, window->child_index);
            cleanup_WmCreateWindowInfo(&info);
        } break;
        case WM_PACKET_TAG_CREATE_SHM_REGION: {
            if(packet.payload_size < min_WmCreateSHMRegion_size) {
                error("Packet too small in WmCreateSHMRegion");
                send_response_packet(client->fd, -SIZE_MISMATCH);
                continue;
            }
            if(packet.payload_size > max_WmCreateSHMRegion_size) {
                error("Packet too big in WmCreateSHMRegion");
                send_response_packet(client->fd, -LIMITS);
                continue;
            }
            da_reserve(&payload_buffer, packet.payload_size);
            if((e = gtread_exact(client->fd, payload_buffer.items, packet.payload_size)) < 0) {
                error("Failed to read payload for WmCreateSHMRegion");
                send_response_packet(client->fd, e);
                continue;
            }
            WmCreateSHMRegion info = { 0 };
            if((e=read_memory_WmCreateSHMRegion(payload_buffer.items, packet.payload_size, &info)) < 0) {
                error("Failed to read WmCreateSHMRegion");
                send_response_packet(client->fd, e);
                continue;
            }
            SHMRegion region = {
                .key = -1,
                .addr = NULL,
                .size = info.size,
            };
            // TODO: sanitize the info.size.
            region.key = _shmcreate(info.size);
            if(region.key < 0) {
                error("Failed to create SHMRegion: %s", status_str(region.key));
                send_response_packet(client->fd, region.key);
                continue;
            }
            if((e = _shmmap(region.key, &region.addr)) < 0) {
                error("Failed to map: %s", status_str(e));
                _shmrem(region.key);
                send_response_packet(client->fd, e);
                continue;
            }
            da_push(&client->shm_regions, region);
            send_response_packet(client->fd, region.key);
        } break;
        case WM_PACKET_TAG_DRAW_SHM_REGION: {
            if(packet.payload_size < min_WmDrawSHMRegion_size) {
                error("Packet too small in WmDrawSHMRegion");
                send_response_packet(client->fd, -SIZE_MISMATCH);
                continue;
            }
            if(packet.payload_size > max_WmDrawSHMRegion_size) {
                error("Packet too big in WmDrawSHMRegion");
                send_response_packet(client->fd, -LIMITS);
                continue;
            }
            da_reserve(&payload_buffer, packet.payload_size);
            if((e = gtread_exact(client->fd, payload_buffer.items, packet.payload_size)) < 0) {
                error("Failed to read payload for WmDrawSHMRegion");
                send_response_packet(client->fd, e);
                continue;
            }
            WmDrawSHMRegion info = { 0 };
            if((e=read_memory_WmDrawSHMRegion(payload_buffer.items, packet.payload_size, &info)) < 0) {
                error("Failed to read WmDrawSHMRegion");
                send_response_packet(client->fd, e);
                continue;
            }
            if(info.pitch_bytes % 4 != 0) {
                error("Invalid pitch_bytes alignment in WmDrawSHMRegion");
                send_response_packet(client->fd, -SIZE_MISMATCH);
                continue;
            }
            if(info.offset_bytes % 4 != 0) {
                error("Invalid offset alignment in WmDrawSHMRegion");
                send_response_packet(client->fd, -SIZE_MISMATCH);
                continue;
            }
            if(info.window >= client->child_windows.len || !client->child_windows.items[info.window]) {
                error("Invalid window handle in WmDrawSHMRegion");
                send_response_packet(client->fd, -INVALID_HANDLE);
                continue;
            }
            if(info.width == 0 || info.height == 0) {
                error("Invalid width or height in WmDrawSHMRegion");
                send_response_packet(client->fd, -INVALID_PARAM);
                continue;
            }
            Window* window = client->child_windows.items[info.window];
            Rectangle content = window_get_content_rect(window);
            size_t width  = (content.r-content.l);
            size_t height = (content.b-content.t);
            if(info.window_x >= width) {
                error("Invalid window_x in WmDrawSHMRegion");
                send_response_packet(client->fd, -INVALID_OFFSET);
                continue;
            }
            if(info.window_y >= height) {
                error("Invalid window_x in WmDrawSHMRegion");
                send_response_packet(client->fd, -INVALID_OFFSET);
                continue;
            }
            if(info.width > width || info.window_x + info.width > width) {
                error("Invalid width in WmDrawSHMRegion");
                send_response_packet(client->fd, -INVALID_OFFSET);
                continue;
            }
            if(info.window_y >= height) {
                error("Invalid window_y in WmDrawSHMRegion");
                send_response_packet(client->fd, -INVALID_OFFSET);
                continue;
            }
            if(info.height > height || info.window_y + info.height > height) {
                error("Invalid height in WmDrawSHMRegion");
                send_response_packet(client->fd, -INVALID_OFFSET);
                continue;
            }

            int shm_region = -1;
            for(size_t i = 0; i < client->shm_regions.len; ++i) {
                if(client->shm_regions.items[i].key == info.shm_key) {
                    shm_region = i;
                }
            }
            if(shm_region < 0) {
                error("Invalid shm_region");
                send_response_packet(client->fd, -INVALID_HANDLE);
                continue;
            }
            SHMRegion* shm = &client->shm_regions.items[shm_region];
            // TODO: bound check this sheize
            uint32_t* pixels = (uint32_t*)(((uint8_t*)shm->addr) + info.offset_bytes);
            uint32_t* head = window->content + info.window_y * width + info.window_x;
            for(size_t dy = 0; dy < info.height; ++dy) {
                for(size_t dx = 0; dx < info.width; ++dx) {
                    head[dx] = pixels[dx];
                }
                pixels = (uint32_t*)(((uint8_t*)pixels) + info.pitch_bytes);
                head += width;
            }
            // TODO: don't redraw entire window content.
            redraw_region(&fb0, &window->rect);
            flush_framebuffer(&fb0);
            send_response_packet(client->fd, 0);
        } break;
        default:
            // TODO: consume packet somehow?
            error("Unsupported packet %04X", packet.tag);
            send_response_packet(client->fd, -UNSUPPORTED);
        }
    }
    close(client->fd);
    for(size_t i = 0; i < client->child_windows.len; ++i) {
        Window* window = client->child_windows.items[i];
        if(window == moving_window) moving_window = NULL;
        Rectangle rect = window->rect;
        list_remove(&window->list);
        if (window->cached_blur) free(window->cached_blur);
        free(window);
        redraw_region(&fb0, &rect);
    }
    free(client->child_windows.items);
    for(size_t i = 0; i < client->shm_regions.len; ++i) {
        _shmrem(client->shm_regions.items[i].key);
    }
    free(client->shm_regions.items);
    list_remove(&client->list);
    free(client);
    flush_framebuffer(&fb0);
}
Client* client_new(int fd) {
    Client* me = malloc(sizeof(*me));
    if(!me) return NULL;
    memset(me, 0, sizeof(*me));
    me->fd = fd;
    return me;
}
uintptr_t run(size_t applet_number, char** argv) {
    intptr_t e = fork();
    if(e == 0) {
#if 1
        close(STDOUT_FILENO);
        char pathbuf[120];
        sprintf(pathbuf, "/applet%zu.log", applet_number);
        assert(open(pathbuf, O_RDWR | O_CREAT) >= 0);
#else
        (void)applet_number;
#endif
        execvp(argv[0], argv);
    } else if (e < 0) {
        fprintf(stderr, "ERROR: Failed to fork: %s\n", status_str(e));        
        abort();
    }
    return e;
}
void spawn_init_applets(void) {
    const char* applets[] = {
        "/user/miniterm",
    };
    for(size_t i = 0; i < (sizeof(applets)/sizeof(*applets)); ++i) {
        const char* argv[] = {
            applets[i], NULL
        };
        run(i, (char**)argv);
    }
}
static struct list_head clients;
intptr_t start_server(void) {
    int server = socket(AF_MINOS, SOCK_STREAM, 0);
    if(server < 0) {
        perror("socket");
        return 1;
    }
    struct sockaddr_minos minos;
    minos.sminos_family = AF_MINOS;
    const char* addr = "/sockets/wm";
    strcpy(minos.sminos_path, addr);
    if(bind(server, (struct sockaddr*)&minos, sizeof(minos)) < 0) {
        error("Failed to create bind socket `%s`: %s", addr, strerror(errno));
        return 1;
    }
    info("Bound socket");
    if(listen(server, 50) < 0) {
        error("Failed to listen on socket: %s", strerror(errno));
        return 1;
    }
    spawn_init_applets();
    for(;;) {
        int client_fd = gtaccept(server, NULL, NULL);
        if(client_fd < 0) {
            perror("accept client");
            break;
        }
        Client* client = client_new(client_fd);
        if(!client) {
            error("Ran out of memory for client!");
            close(client_fd);
            continue;
        }
        list_insert(&clients, &client->list);
        gtgo(client_thread, (void*)client);
    }
    return 0;
}
enum {
    GUI_MOUSE_EVENT_MOVE,
    GUI_MOUSE_EVENT_UP,
    GUI_MOUSE_EVENT_DOWN
};
void handle_mouse_event(int what, int x, int y, int button) {
    switch(what) {
    case GUI_MOUSE_EVENT_MOVE: {
        Rectangle rect = cursor_rect();
        mouse_x = x;
        mouse_y = y;
        if(moving_window) {
            int wx = mouse_x - moving_window_dx, wy = mouse_y - moving_window_dy;
            if(wx < 0) wx = 0;
            if(wy < 0) wy = 0;
            if((size_t)wx + (moving_window->rect.r - moving_window->rect.l) > fb0.width)
                wx = fb0.width - (moving_window->rect.r - moving_window->rect.l);
            if((size_t)wy + (moving_window->rect.b - moving_window->rect.t) > fb0.height)
                wy = fb0.height - (moving_window->rect.b - moving_window->rect.t);
            // Just draw wireframe outline at new position (don't update window rect yet)
            draw_drag_outline(&fb0, &(Rectangle){wx, wy, wx + (moving_window->rect.r - moving_window->rect.l), wy + (moving_window->rect.b - moving_window->rect.t)}, 0xFFFFFFFF);
        } else {
            redraw_region(&fb0, &rect);
            draw_image(&fb0, &cursor, mouse_x, mouse_y);
            flush_framebuffer(&fb0);
        }
    } break;
    case GUI_MOUSE_EVENT_UP:
        if(button == MOUSE_BUTTON_CODE_LEFT) {
            if (moving_window) {
                // Move ended - restore old position background, move window, draw full window at new position
                int wx = mouse_x - moving_window_dx, wy = mouse_y - moving_window_dy;
                if(wx < 0) wx = 0;
                if(wy < 0) wy = 0;
                if((size_t)wx + (moving_window->rect.r - moving_window->rect.l) > fb0.width)
                    wx = fb0.width - (moving_window->rect.r - moving_window->rect.l);
                if((size_t)wy + (moving_window->rect.b - moving_window->rect.t) > fb0.height)
                    wy = fb0.height - (moving_window->rect.b - moving_window->rect.t);
                
                Rectangle old_rect = moving_window->rect;
                // Erase wireframe by restoring background at old position and new position
                if (prev_wireframe_valid) {
                    wallpaper_redraw_region(&fb0, &prev_wireframe);
                    prev_wireframe_valid = false;
                }
                
                // CRITICAL: Update window rect FIRST so redraw_region sees it at new position
                moving_window->rect.l = wx;
                moving_window->rect.t = wy;
                moving_window->rect.r = wx + (old_rect.r - old_rect.l);
                moving_window->rect.b = wy + (old_rect.b - old_rect.t);
                
                // Now redraw old position (window no longer there, wallpaper will be drawn)
                redraw_region(&fb0, &old_rect);
                // Redraw new position (window now there, will be drawn with full blur)
                redraw_region(&fb0, &moving_window->rect);
                
                // Invalidate cache and redraw full window with blur
                invalidate_cached_blur(moving_window);
                draw_window(&fb0, moving_window);
                flush_framebuffer(&fb0);
            }
            moving_window = NULL;
            moving_window_dx = 0;
            moving_window_dy = 0;
        }
        break;
    case GUI_MOUSE_EVENT_DOWN: {
        Vec2size cursorp = cursor_point();
        for(struct list_head* head = windows.next; head != &windows; head = head->next) {
            Window* win = (Window*)head;
            if(rect_collide_point(&win->rect, &cursorp)) {
                Rectangle content_rect = window_get_content_rect(win);
                Rectangle menu_rect = window_get_menu_rect(win);
                if(rect_collide_point(&content_rect, &cursorp)) {
                    // Send mouse click event
                } else if (rect_collide_point(&menu_rect, &cursorp)) {
                    Rectangle x_button    = menu_get_x_rect(win), 
                              hide_button = menu_get_hide_rect(win),
                              min_button  = menu_get_min_rect(win);
                    if(rect_collide_point(&x_button, &cursorp)) {
                        // Send close signal as a key event
                        WmEvent event = { 0 };
                        event.window = win->child_index;
                        event.event = WM_EVENT_KEY_DOWN;
                        WM_SETKEY(&event, WM_KEY_CLOSE);
                        WM_SETKEYCODE(&event, 0);
                        send_event(win->parent_client->fd, &event);
    
                        // Client disconnect will be handled by client_thread
                        info("Close");
                        return;
                    } else if (rect_collide_point(&min_button, &cursorp)) {
                        info("Minimize");
                    } else if (rect_collide_point(&hide_button, &cursorp)) {
                        info("Hide");
                    } else {
                        if(button == MOUSE_BUTTON_CODE_LEFT) {
                            moving_window = win;
                            moving_window_dx = cursorp.x - win->rect.l;
                            moving_window_dy = cursorp.y - win->rect.t;
                            info("Move");
                            list_remove(&win->list);
                            list_append(&windows, &win->list);
                            draw_window(&fb0, win);
                            draw_image(&fb0, &cursor, mouse_x, mouse_y);
                            flush_framebuffer(&fb0);
                            return;
                        }
                    }
                }
                list_remove(&win->list);
                list_append(&windows, &win->list);
                // TODO: Optimise this to not redraw the entire window.
                draw_window(&fb0, win);
                draw_image(&fb0, &cursor, mouse_x, mouse_y);
                flush_framebuffer(&fb0);
                return;
            }
        }
    } break;
    }
}
void mouse_thread(void*) {
    intptr_t e;
    if((e=open("/devices/mouse", O_RDONLY)) < 0) {
        error("Failed to open mouse: %s", status_str(e));
        return;
    }
    uintptr_t mouse = e;
    int x = 0, y = 0;
    MouseEvent events[16] = { 0 };
    for(;;) {
        if((e = gtread(mouse, events, sizeof(events))) < 0) {
            error("Failed to read on mouse: %s", status_str(e));
            break;
        }
        size_t n = e/sizeof(MouseEvent);
        for(size_t i = 0; i < n; ++i) {
            MouseEvent* ev = &events[i];
            switch(ev->kind) {
            case MOUSE_EVENT_KIND_MOVE: {
                x += ev->as.move.delta_x;
                y += ev->as.move.delta_y;
                if(x < 0) x = 0;
                else if ((size_t)x + cursor.width > fb0.width) x = fb0.width - cursor.width;
                if(y < 0) y = 0;
                else if ((size_t)y + cursor.height > fb0.height) y = fb0.height - cursor.height;    
                Rectangle old_cursor = cursor_rect();
                mouse_x = x;
                mouse_y = y;
                if(moving_window) {
                    int wx = x - moving_window_dx;
                    int wy = y - moving_window_dy;
                    if(wx < 0) wx = 0;
                    if(wy < 0) wy = 0;
                    if((size_t)wx + (moving_window->rect.r - moving_window->rect.l) > fb0.width)
                        wx = fb0.width - (moving_window->rect.r - moving_window->rect.l);
                    if((size_t)wy + (moving_window->rect.b - moving_window->rect.t) > fb0.height)
                        wy = fb0.height - (moving_window->rect.b - moving_window->rect.t);
                    
                    Rectangle new_wireframe = {wx, wy, wx + (moving_window->rect.r - moving_window->rect.l), wy + (moving_window->rect.b - moving_window->rect.t)};
                    
                    // Erase previous wireframe by restoring background in that area
                    if (prev_wireframe_valid) {
                        wallpaper_redraw_region(&fb0, &prev_wireframe);
                    }
                    
                    // Draw new wireframe
                    draw_drag_outline(&fb0, &new_wireframe, 0xFFFFFFFF);
                    
                    // Track for next frame cleanup
                    prev_wireframe = new_wireframe;
                    prev_wireframe_valid = true;
                } else {
                    redraw_region(&fb0, &old_cursor);
                    draw_image(&fb0, &cursor, mouse_x, mouse_y);
                    flush_framebuffer(&fb0);
                }
            } break;
            case MOUSE_EVENT_KIND_BUTTON:
                handle_mouse_event(ev->as.button & MOUSE_BUTTON_ON_MASK ? GUI_MOUSE_EVENT_DOWN : GUI_MOUSE_EVENT_UP, -1, -1, ev->as.button & MOUSE_BUTTON_CODE_MASK);
                break;
            }
        }
        flush_framebuffer(&fb0);
    }
    close(mouse);
}
#include <minos/key.h>
#include <ctype.h>
#define KEY_BYTES ((MINOS_KEY_COUNT+7)/8)
typedef struct {
    // Is key down?
    uint8_t state[KEY_BYTES];
} Keyboard;
static void key_set(Keyboard* kb, uint16_t code, uint8_t released) {
    uint8_t down = released == 0;
    // debug_assert(code < ARRAY_LEN(kb->state));
    kb->state[code/8] &= ~(1 << (code % 8));
    kb->state[code/8] |= down << (code%8);
}
static bool key_get(Keyboard* kb, uint16_t code) {
    //debug_assert(code < ARRAY_LEN(kb->state));
    return kb->state[code/8] & (1<<(code%8));
}

static uint8_t US_QWERTY_SHIFTED[256] = {
   0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
   0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
   0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
   0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
   0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x22,
   0x28, 0x29, 0x2A, 0x2B, 0x3C, 0x5F, 0x3E, 0x3F,
   0x29, 0x21, 0x40, 0x23, 0x24, 0x25, 0x5E, 0x26,
   0x2A, 0x28, 0x3A, 0x3A, 0x3C, 0x2B, 0x3E, 0x3F,
   0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
   0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
   0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
   0x58, 0x59, 0x5A, 0x7B, 0x7C, 0x7D, 0x5E, 0x5F,
   0x7E, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
   0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
   0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
   0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
   0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
   0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
   0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
   0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
   0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
   0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
   0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
   0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
   0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
   0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
   0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7,
   0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
   0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7,
   0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
   0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
   0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};
static int key_unicode(Keyboard* keyboard, uint16_t code) {
    switch(code) {
    case MINOS_KEY_ENTER:
        if(key_get(keyboard, MINOS_KEY_ENTER)) return '\n';
        return 0;
    default:
        if(code >= 256 || !key_get(keyboard, code)) return 0;
        if(key_get(keyboard, MINOS_KEY_LEFT_SHIFT) || key_get(keyboard, MINOS_KEY_RIGHT_SHIFT)) {
            return US_QWERTY_SHIFTED[code];
        }
        if(code >= 'A' && code <= 'Z') return code-'A' + 'a';
        return code;
    }
}

void keyboard_thread(void*) { 
    intptr_t e;
    if((e=open("/devices/keyboard", O_RDONLY)) < 0) {
        error("Failed to open keyboard: %s", status_str(e));
        return;
    }
    uintptr_t keyboard = e;
    Keyboard kb_state = { 0 };
    for(;;) {
        Key key;
        gtread(keyboard, &key, sizeof(key));
        key_set(&kb_state, key.code, key.attribs);
        if(key_unicode(&kb_state, key.code) == 'e' && key_get(&kb_state, MINOS_KEY_LEFT_ALT) && key_get(&kb_state, MINOS_KEY_LEFT_CTRL)) 
            break;
        
        // Test shortcuts for compositing (only when windows exist)
        if (!list_empty(&windows)) {
            Window* window = (Window*)windows.next;
            // Toggle transparency with Ctrl+T
            if(key_unicode(&kb_state, key.code) == 't' && key_get(&kb_state, MINOS_KEY_LEFT_CTRL) && !(key.attribs & KEY_ATTRIB_RELEASE)) {
                window->has_transparency = !window->has_transparency;
                info("Transparency %s", window->has_transparency ? "enabled" : "disabled");
                redraw_region(&fb0, &window->rect);
                flush_framebuffer(&fb0);
            }
            // Toggle blur with Ctrl+B
            if(key_unicode(&kb_state, key.code) == 'b' && key_get(&kb_state, MINOS_KEY_LEFT_CTRL) && !(key.attribs & KEY_ATTRIB_RELEASE)) {
                window->blur_background = !window->blur_background;
                info("Blur %s", window->blur_background ? "enabled" : "disabled");
                redraw_region(&fb0, &window->rect);
                flush_framebuffer(&fb0);
            }
        }
        
        if(list_empty(&windows))
            continue;
        Window* window = (Window*)windows.next;
        WmEvent event = { 0 };
        event.window = window->child_index;
        event.event = (key.attribs & KEY_ATTRIB_RELEASE) ? WM_EVENT_KEY_UP : WM_EVENT_KEY_DOWN;
        WM_SETKEY(&event, key.code);
        WM_SETKEYCODE(&event, key_unicode(&kb_state, key.code));
        send_event(window->parent_client->fd, &event);
    }
    Rectangle rect = {
        0, 0,
        fb0.width, fb0.height
    };
    fill_rect(&fb0, &rect, 0xFF212121);
    flush_framebuffer(&fb0);

    for(struct list_head* head = clients.next; head != &clients; head = head->next) {
        Client* client = (Client*)head;
        close(client->fd);
    }
    exit(0);
}
int main(void) {
    intptr_t e;
    gtinit();
    list_init(&windows);
    list_init(&clients);
    trace("Loading resources");
    load_resources();
    trace("Loading framebuffer");
    if((e=load_framebuffer("/devices/fb0", &fb0)) < 0) {
        error("load_framebuffer failed: %s", status_str(e));
        return 1;
    }
    Rectangle rect = { 
        0, 0,
        fb0.width, fb0.height
    };
    wallpaper_redraw_region(&fb0, &rect);
    flush_framebuffer(&fb0);
    gtgo(keyboard_thread, NULL);
    gtgo(mouse_thread, NULL);
    trace("Starting server");
    start_server();
    return 0;
}
