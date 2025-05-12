
#include <wafel/utils.h>
#include <wafel/patch.h>
#include <wafel/trampoline.h>
#include "uhs.h"
#include "iosu_ax88772.h"
#include "ax88179.h"


#define AX_CHIPCODE_MASK		0x70
#define AX_AX88772_CHIPCODE		0x00
#define AX_AX88772A_CHIPCODE		0x10
#define AX_AX88772B_CHIPCODE		0x20
#define AX_HOST_EN			0x01
#define AX_HEADERMODE_MASK 0x700

#define AX_CMD_STATMNGSTS_REG		0x09

u16 usb_vid_pids[][2] = {   
    { 0x0b95, 0x7720 }, // AX88772
    { 0x0b95, 0x772a }, // AX88772A
    { 0x05ac, 0x1402 }, // Apple AX88772A
    { 0x0b95, 0x772b }, // AX88772B
    { 0x0b95, 0x772c }, // AX88772C needs testing
};
#define USB_VID_PID_COUNT (sizeof(usb_vid_pids) / 4)

u16 ax88179_vid_pids[][2] = {
    { 0x0b95, 0x1790 }, // AX88772D / AX88772E / AX881790 needs testing
};
#define AX88179_VID_PID_COUNT (sizeof(ax88179_vid_pids) / 4)

#define UHS_IF_PROBE_CALLBACK_PTR 0x123b902c

void (*uhsIfProbeCallback)(void *context, UhsInterfaceProfile* profile) = (void*)0x123b8a34;

static u8 chipcode = 0;

static void claim_interface(void* context, UhsInterfaceProfile* profile){
    u16 *vidpid = (u16*)0x12456dd0; // (context+8298);
    vidpid[0] = profile->dev_desc.idVendor;
    vidpid[1] = profile->dev_desc.idProduct;
    debug_printf("AX88772b: Acquireing Interface %X:%X\n", vidpid[0], vidpid[1]);
    uhsIfProbeCallback(context, profile);
}

void ifprobe_callback_wrapper(void* context, UhsInterfaceProfile* profile){
    u16 vid = profile->dev_desc.idVendor;
    u16 pid = profile->dev_desc.idProduct;
    
    debug_printf("ifprobe_callback_wrapper %x:%x\n", vid,pid);

    for(int i=0; i<USB_VID_PID_COUNT; i++){
        if(vid == usb_vid_pids[i][0] && pid == usb_vid_pids[i][1]){
            set_ax88179_mode(false);
            claim_interface(context, profile);
            return;  
        }
    }

    for(int i=0; i<AX88179_VID_PID_COUNT; i++){
        if(vid == ax88179_vid_pids[i][0] && pid == ax88179_vid_pids[i][1]){
            set_ax88179_mode(true);
            claim_interface(context, profile);
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

void read_chipcode_hook(trampoline_state *regs){
    int ret = ax8817xReadCommand((void*)regs->r[0], (void*)regs->r[1], AX_CMD_STATMNGSTS_REG, 0, 0, 1, &chipcode);
    chipcode &= AX_CHIPCODE_MASK;
    debug_printf("Read stms register returned %i, Chipcode: 0x%x\n", ret, chipcode);
}

void rx_control_hook(trampoline_state *regs){
    if(chipcode >= AX_AX88772B_CHIPCODE){
        regs->r[1] &= ~AX_HEADERMODE_MASK; // meansing of these bits changed
    }
    debug_printf("Setting rx control register to 0x%x\n", regs->r[1]);
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

    // get chipcode
    trampoline_hook_before(0x123ba1ec, read_chipcode_hook);

    // fix value written to RX Control Register
    trampoline_hook_before(0x123ba608, rx_control_hook);

    ax88179_apply_patches();
}

// This fn runs before MCP's main thread, and can be used
// to perform late patches and spawn threads under MCP.
// It must return.
void mcp_main()
{

}
