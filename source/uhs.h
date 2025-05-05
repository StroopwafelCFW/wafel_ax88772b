#pragma once

#include <inttypes.h>

/* Determines which parameters to check */
#define MATCH_ANY             0x000
#define MATCH_DEV_VID         0x001
#define MATCH_DEV_PID         0x002
#define MATCH_DEV_CLASS       0x010
#define MATCH_DEV_SUBCLASS    0x020
#define MATCH_DEV_PROTOCOL    0x040
#define MATCH_IF_CLASS        0x080
#define MATCH_IF_SUBCLASS     0x100
#define MATCH_IF_PROTOCOL     0x200

/* Interface filter */
typedef struct
{
    uint16_t match_params;    /* Bitmask of above flags */
    uint16_t vid, pid;        /* Vendor ID and product ID */
    char unknown6[0xa - 0x6];
    uint8_t dev_class;        /* Device class */
    uint8_t dev_subclass;     /* Device subclass */
    uint8_t dev_protocol;     /* Device protocol */
    uint8_t if_class;         /* Interface class */
    uint8_t if_subclass;      /* Interface subclass */
    uint8_t if_protocol;      /* Interface protocol */ 
} PACKED UhsInterfaceFilter;

/* USB device descriptor */
typedef struct
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdUsb;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubclass;
    uint8_t bDeviceProtocol;
    uint8_t bMaxPacketSize;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t iManufacturer;
    uint8_t iProduct;
    uint8_t iSerialNumber;
    uint8_t bNumConfigurations;
} PACKED UhsDeviceDescriptor;

/* USB configuration descriptor */
typedef struct
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t wTotalLength;
    uint8_t bNumInterfaces;
    uint8_t bConfigurationValue;
    uint8_t iConfiguration;
    uint8_t bmAttributes;
    uint8_t bMaxPower;
} PACKED UhsConfigDescriptor;

/* USB interface descriptor */
typedef struct
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
} PACKED UhsInterfaceDescriptor;

/* Interface profile */
typedef struct
{
    uint32_t if_handle;
    char unknown4[0x28 - 0x4];
    UhsDeviceDescriptor dev_desc;
    UhsConfigDescriptor cfg_desc;
    UhsInterfaceDescriptor if_desc;
    char in_endpoints[0xdc - 0x4c];
    char out_endpoints[0x16c - 0xdc];
} PACKED UhsInterfaceProfile;