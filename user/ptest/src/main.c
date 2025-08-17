#include <stdio.h>
#include <phoenix.h>

// تعریف مینیمال dummy توابع برای تست
phx_instance phxCreateInstance(void) { phx_instance h={1}; return h; }
phx_device phxCreateDevice(phx_instance i) { phx_device h={2}; return h; }
phx_swapchain phxCreateSwapchain(phx_device d, const phx_platform_fb* fb, const phx_swapchain_desc* desc) { phx_swapchain h={3}; return h; }
phx_image phxCreateImage(phx_device d, const phx_image_desc* desc) { phx_image h={4}; return h; }
phx_buffer phxCreateBuffer(phx_device d, const phx_buffer_desc* desc) { phx_buffer h={5}; return h; }
phx_pipeline phxCreatePipelineBasic(phx_device d) { phx_pipeline h={6}; return h; }

phx_cmdlist phxBeginCommands(phx_device d) { phx_cmdlist h={7}; return h; }
void phxCmdClearColor(phx_cmdlist cl, phx_image img, float r, float g, float b, float a) {}
void phxCmdBindPipeline(phx_cmdlist cl, phx_pipeline p) {}
void phxCmdBindVertexBuffer(phx_cmdlist cl, phx_buffer buf) {}
void phxCmdPushConstants(phx_cmdlist cl, const phx_push_constants* pc) {}
void phxCmdDraw(phx_cmdlist cl, uint32_t vtx_count, uint32_t first) {}
void phxEndCommands(phx_cmdlist cl) {}

void* phxAcquireNextImage(phx_swapchain sc, phx_image* out_img) { if(out_img) *out_img=(phx_image){8}; return out_img; }
void phxSubmit(phx_device d, phx_cmdlist cl) {}
void phxPresent(phx_swapchain sc) {}

int main(void) {
    phx_instance inst = phxCreateInstance();
    phx_device dev = phxCreateDevice(inst);

    phx_platform_fb fb = {0};
    fb.width = 640;
    fb.height = 480;
    fb.pitch = 640*4;
    fb.format = 0;

    phx_swapchain sc = phxCreateSwapchain(dev, &fb, &(phx_swapchain_desc){640,480});
    phx_image img = phxCreateImage(dev, &(phx_image_desc){640,480});
    phx_buffer buf = phxCreateBuffer(dev, &(phx_buffer_desc){1024});
    phx_pipeline pipe = phxCreatePipelineBasic(dev);

    phx_cmdlist cl = phxBeginCommands(dev);
    phxCmdClearColor(cl, img, 1.0f, 0.0f, 0.0f, 1.0f);
    phxCmdBindPipeline(cl, pipe);
    phxCmdBindVertexBuffer(cl, buf);
    phx_push_constants pc = {{0}};
    phxCmdPushConstants(cl, &pc);
    phxCmdDraw(cl, 3, 0);
    phxEndCommands(cl);

    phxAcquireNextImage(sc, &img);
    phxSubmit(dev, cl);
    phxPresent(sc);

    printf("Phoenix test executed successfully!\n");
    return 0;
}
