#include <pluto.h>
#include <lava2d/lava2d.h>

int main(void) {
    PlutoInstance instance;
    pluto_create_instance(&instance);

    const int W = 800, H = 600;
    Lava2DContext ctx;
    lava2d_create(&ctx, &instance, W, H, "Tearing Debug");

    int frame = 0;
    
    for (;;) {
        // تست 1: همه چی رو با backbuffer بکشیم (fill_rect به جای clear)
        // اگه tearing نداشته باشیم، یعنی مشکل از clear بوده
        switch (frame % 3) {
            case 0:
                lava2d_fill_rect(&ctx, 0, 0, W, H, 0xFFFF0000);
                break;
            case 1:
                lava2d_fill_rect(&ctx, 0, 0, W, H, 0xFF00FF00);
                break;
            case 2:
                lava2d_fill_rect(&ctx, 0, 0, W, H, 0xFF0000FF);
                break;
        }
        
        // خطوط شبکه
        for (int y = 0; y < H; y += 30) {
            lava2d_fill_rect(&ctx, 0, y, W, 3, 0xFF000000);
        }
        for (int x = 0; x < W; x += 30) {
            lava2d_fill_rect(&ctx, x, 0, 3, H, 0xFF000000);
        }
        
        // مربع وسط
        lava2d_fill_rect(&ctx, W/2 - 50, H/2 - 50, 100, 100, 0xFFFF00FF);
        
        // تست 2: خط مورب متحرک برای نشون دادن tearing عمودی
        for (int i = 0; i < W; i += 10) {
            int y_pos = (i + frame * 5) % H;
            lava2d_fill_rect(&ctx, i, y_pos, 5, 5, 0xFFFFFFFF);
        }
        
        lava2d_present(&ctx);
        frame++;
    }

    return 0;
}
