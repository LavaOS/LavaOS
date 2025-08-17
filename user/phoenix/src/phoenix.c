#include <phoenix.h>
#include <stdlib.h>
#include <string.h>

#define MAX_RESOURCES 1024

typedef enum { RES_NULL, RES_IMAGE, RES_BUFFER } phx_res_type;

typedef struct {
    phx_res_type type;
    void* ptr;
    size_t size;
    uint32_t width, height;
} phx_resource_entry;

static phx_resource_entry g_resources[MAX_RESOURCES] = {0};
static uint64_t g_next_id = 1;

static phx_handle add_resource(phx_res_type type, void* ptr, size_t size, uint32_t w, uint32_t h) {
    for(int i=0;i<MAX_RESOURCES;i++){
        if(g_resources[i].type == RES_NULL){
            g_resources[i].type=type;
            g_resources[i].ptr=ptr;
            g_resources[i].size=size;
            g_resources[i].width=w;
            g_resources[i].height=h;
            phx_handle handle = { g_next_id++ };
            return handle;
        }
    }
    phx_handle handle_fail = {0};
    return handle_fail; // fail
}

static phx_resource_entry* get_resource(phx_handle h) {
    for(int i=0;i<MAX_RESOURCES;i++){
        if(g_resources[i].type != RES_NULL && g_resources[i].ptr && h.id != 0){
            if(h.id == i+1) return &g_resources[i];
        }
    }
    return NULL;
}

// ===== API =====
phx_instance phxCreateInstance(void){ return add_resource(RES_NULL,NULL,0,0,0); }
phx_device phxCreateDevice(phx_instance i){ (void)i; return add_resource(RES_NULL,NULL,0,0,0); }

phx_swapchain phxCreateSwapchain(phx_device d, const phx_platform_fb* fb, const phx_swapchain_desc* desc){
    (void)d;
    (void)desc;
    void* mem = malloc(fb->height*fb->pitch);
    memset(mem,0,fb->height*fb->pitch);
    return add_resource(RES_IMAGE,mem,fb->height*fb->pitch,fb->width,fb->height);
}

phx_image phxCreateImage(phx_device d, const phx_image_desc* desc){
    (void)d;
    size_t sz = desc->width*desc->height*4;
    void* mem = malloc(sz);
    memset(mem,0,sz);
    return add_resource(RES_IMAGE,mem,sz,desc->width,desc->height);
}

phx_buffer phxCreateBuffer(phx_device d, const phx_buffer_desc* desc){
    (void)d;
    void* mem = malloc(desc->size);
    return add_resource(RES_BUFFER,mem,desc->size,0,0);
}

phx_pipeline phxCreatePipelineBasic(phx_device d){ (void)d; return add_resource(RES_NULL,NULL,0,0,0); }

phx_cmdlist phxBeginCommands(phx_device d){ (void)d; return add_resource(RES_NULL,NULL,0,0,0); }

void phxCmdClearColor(phx_cmdlist cl, phx_image img, float r,float g,float b,float a){
    phx_resource_entry* e = get_resource(img);
    if(!e || !e->ptr) return;
    uint8_t R=(uint8_t)(r*255),G=(uint8_t)(g*255),B=(uint8_t)(b*255),A=(uint8_t)(a*255);
    uint8_t* data = (uint8_t*)e->ptr;
    for(size_t i=0;i<e->size;i+=4){
        data[i+0]=R;
        data[i+1]=G;
        data[i+2]=B;
        data[i+3]=A;
    }
    (void)cl;
}

void phxCmdBindPipeline(phx_cmdlist cl, phx_pipeline p){ (void)cl; (void)p; }
void phxCmdBindVertexBuffer(phx_cmdlist cl, phx_buffer buf){ (void)cl; (void)buf; }
void phxCmdPushConstants(phx_cmdlist cl, const phx_push_constants* pc){ (void)cl; (void)pc; }
void phxCmdDraw(phx_cmdlist cl,uint32_t vtx_count,uint32_t first){ (void)cl; (void)vtx_count; (void)first; }
void phxEndCommands(phx_cmdlist cl){ (void)cl; }

void* phxAcquireNextImage(phx_swapchain sc, phx_image* out_img){
    if(out_img) *out_img=sc;
    return NULL;
}

void phxSubmit(phx_device d, phx_cmdlist cl){ (void)d; (void)cl; }
void phxPresent(phx_swapchain sc){ (void)sc; }
