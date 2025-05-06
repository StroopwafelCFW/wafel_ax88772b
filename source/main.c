
#include <wafel/utils.h>
#include <wafel/patch.h>
#include <wafel/trampoline.h>
#include "uhs.h"


u16 usb_vid_pids[][2] = {   { 0x0b95, 0x7720 }, // AX88772
                            { 0x0b95, 0x772a }, // AX88772A
                            { 0x0b95, 0x772b }, // AX88772B
                            { 0x0b95, 0x772c }, // AX88772C
                            { 0x05ac, 0x1402 }, // Apple AX88772
};
#define USB_VID_PID_COUNT (sizeof(usb_vid_pids) / 4)

#define UHS_IF_PROBE_CALLBACK_PTR 0x123b902c



void (*uhsIfProbeCallback)(void *context, UhsInterfaceProfile* profile) = (void*)0x123b8a34;

void ifprobe_callback_wrapper(void* context, UhsInterfaceProfile* profile){
    u16 vid = profile->dev_desc.idVendor;
    u16 pid = profile->dev_desc.idProduct;
    
    debug_printf("ifprobe_callback_wrapper %x:%x\n", vid,pid);

    for(int i=0; i<USB_VID_PID_COUNT; i++){
        if(vid == usb_vid_pids[i][0] && pid == usb_vid_pids[i][1]){
            u16 *vidpid = (u16*)0x12456dd0; // (context+8298);
            vidpid[0] = vid;
            vidpid[1] = pid;
            debug_printf("AX88772b: Acquireing Interface %X:%X\n", vid, pid);
            uhsIfProbeCallback(context, profile);
            return;  
        }
    }
}

int register_driver_hook(int *handles, UhsInterfaceFilter *filter, void* context, void* callback, int (*register_driver_org)(int*, UhsInterfaceFilter*, void*, void*)){
    filter->dev_class = 255;
    filter->dev_subclass = 255;
    filter->if_class = 255;
    filter->if_subclass = 255;
    filter->match_params = MATCH_DEV_CLASS | MATCH_DEV_SUBCLASS | MATCH_IF_CLASS | MATCH_IF_SUBCLASS;
    
    uhsIfProbeCallback = callback;
    return register_driver_org(handles, filter, context, ifprobe_callback_wrapper);
}

// This fn runs before everything else in kernel mode.
// It should be used to do extremely early patches
// (ie to BSP and kernel, which launches before MCP)
// It jumps to the real IOS kernel entry on exit.
__attribute__((target("arm")))
void kern_main()
{
    // Make sure relocs worked fine and mappings are good
    debug_printf("we in here AX88772B plugin kern %p\n", kern_main);

    debug_printf("init_linking symbol at: %08x\n", wafel_find_symbol("init_linking"));

    // allow multiple vid:pid
    trampoline_blreplace(0x123b8f7c, register_driver_hook);

    // fix value written to RX Control Register
    U32_PATCH_K(0x123ba6a4, 0x8a);
}

// This fn runs before MCP's main thread, and can be used
// to perform late patches and spawn threads under MCP.
// It must return.
void mcp_main()
{

}
