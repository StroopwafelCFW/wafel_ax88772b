# wafel_ax88772b
Patches IOSU to use the AX88772B instead of AX88772B USB Ethernet Adapter.

**Warning:** with this plugin the original adapter with the AX88772 chipset will no longer be detected. 
Also this patch only applies the the Wii U side. vWii would need patches to the vIOS.



## How to use

- Copy the `ax88772b.ipx` to `/wiiu/ios_plugins`

## Building

```bash
export STROOPWAFEL_ROOT=/path/too/stroopwafel-repo
make
```

## 
