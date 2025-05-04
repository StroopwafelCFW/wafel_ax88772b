
#include <wafel/utils.h>
#include <wafel/patch.h>


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

    // change ID (VID:PID)
    U32_PATCH_K(0x12456dd0, 0x0B95772b);

    // fix value written to RX Control Register
    U32_PATCH_K(0x123ba6a4, 0x8a);
}

// This fn runs before MCP's main thread, and can be used
// to perform late patches and spawn threads under MCP.
// It must return.
void mcp_main()
{

}
