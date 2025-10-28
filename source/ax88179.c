#include <stdbool.h>
#include <wafel/patch.h>
#include <wafel/trampoline.h>
#include <wafel/ios/thread.h>
#include "iosu_ax88772.h"

/* defines taken from https://github.com/torvalds/linux/blob/627277ba7c2398dc4f95cc9be8222bb2d9477800/drivers/net/usb/ax88179_178a.c#L17C1-L165C37 */
#define AX88179_PHY_ID				0x03
#define AX_EEPROM_LEN				0x100
#define AX88179_EEPROM_MAGIC			0x17900b95
#define AX_MCAST_FLTSIZE			8
#define AX_MAX_MCAST				64
#define AX_INT_PPLS_LINK			((u32)BIT(16))
#define AX_RXHDR_L4_TYPE_MASK			0x1c
#define AX_RXHDR_L4_TYPE_UDP			4
#define AX_RXHDR_L4_TYPE_TCP			16
#define AX_RXHDR_L3CSUM_ERR			2
#define AX_RXHDR_L4CSUM_ERR			1
#define AX_RXHDR_CRC_ERR			((u32)BIT(29))
#define AX_RXHDR_DROP_ERR			((u32)BIT(31))
#define AX_ACCESS_MAC				0x01
#define AX_ACCESS_PHY				0x02
#define AX_ACCESS_EEPROM			0x04
#define AX_ACCESS_EFUS				0x05
#define AX_RELOAD_EEPROM_EFUSE			0x06
#define AX_PAUSE_WATERLVL_HIGH			0x54
#define AX_PAUSE_WATERLVL_LOW			0x55

#define PHYSICAL_LINK_STATUS			0x02
	#define	AX_USB_SS		0x04
	#define	AX_USB_HS		0x02

#define GENERAL_STATUS				0x03
/* Check AX88179 version. UA1:Bit2 = 0,  UA2:Bit2 = 1 */
	#define	AX_SECLD		0x04

#define AX_SROM_ADDR				0x07
#define AX_SROM_CMD				0x0a
	#define EEP_RD			0x04
	#define EEP_BUSY		0x10

#define AX_SROM_DATA_LOW			0x08
#define AX_SROM_DATA_HIGH			0x09

#define AX_RX_CTL				0x0b
	#define AX_RX_CTL_DROPCRCERR	0x0100
	#define AX_RX_CTL_IPE		0x0200
	#define AX_RX_CTL_START		0x0080
	#define AX_RX_CTL_AP		0x0020
	#define AX_RX_CTL_AM		0x0010
	#define AX_RX_CTL_AB		0x0008
	#define AX_RX_CTL_AMALL		0x0002
	#define AX_RX_CTL_PRO		0x0001
	#define AX_RX_CTL_STOP		0x0000

#define AX_NODE_ID				0x10
#define AX_MULFLTARY				0x16

#define AX_MEDIUM_STATUS_MODE			0x22
	#define AX_MEDIUM_GIGAMODE	0x01
	#define AX_MEDIUM_FULL_DUPLEX	0x02
	#define AX_MEDIUM_EN_125MHZ	0x08
	#define AX_MEDIUM_RXFLOW_CTRLEN	0x10
	#define AX_MEDIUM_TXFLOW_CTRLEN	0x20
	#define AX_MEDIUM_RECEIVE_EN	0x100
	#define AX_MEDIUM_PS		0x200
	#define AX_MEDIUM_JUMBO_EN	0x8040

#define AX_MONITOR_MOD				0x24
	#define AX_MONITOR_MODE_RWLC	0x02
	#define AX_MONITOR_MODE_RWMP	0x04
	#define AX_MONITOR_MODE_PMEPOL	0x20
	#define AX_MONITOR_MODE_PMETYPE	0x40

#define AX_GPIO_CTRL				0x25
	#define AX_GPIO_CTRL_GPIO3EN	0x80
	#define AX_GPIO_CTRL_GPIO2EN	0x40
	#define AX_GPIO_CTRL_GPIO1EN	0x20

#define AX_PHYPWR_RSTCTL			0x26
	#define AX_PHYPWR_RSTCTL_BZ	0x0010
	#define AX_PHYPWR_RSTCTL_IPRL	0x0020
	#define AX_PHYPWR_RSTCTL_AT	0x1000

#define AX_RX_BULKIN_QCTRL			0x2e
#define AX_CLK_SELECT				0x33
	#define AX_CLK_SELECT_BCS	0x01
	#define AX_CLK_SELECT_ACS	0x02
	#define AX_CLK_SELECT_ULR	0x08

#define AX_RXCOE_CTL				0x34
	#define AX_RXCOE_IP		0x01
	#define AX_RXCOE_TCP		0x02
	#define AX_RXCOE_UDP		0x04
	#define AX_RXCOE_TCPV6		0x20
	#define AX_RXCOE_UDPV6		0x40

#define AX_TXCOE_CTL				0x35
	#define AX_TXCOE_IP		0x01
	#define AX_TXCOE_TCP		0x02
	#define AX_TXCOE_UDP		0x04
	#define AX_TXCOE_TCPV6		0x20
	#define AX_TXCOE_UDPV6		0x40

#define AX_LEDCTRL				0x73

#define GMII_PHY_PHYSR				0x11
	#define GMII_PHY_PHYSR_SMASK	0xc000
	#define GMII_PHY_PHYSR_GIGA	0x8000
	#define GMII_PHY_PHYSR_100	0x4000
	#define GMII_PHY_PHYSR_FULL	0x2000
	#define GMII_PHY_PHYSR_LINK	0x400

#define GMII_LED_ACT				0x1a
	#define	GMII_LED_ACTIVE_MASK	0xff8f
	#define	GMII_LED0_ACTIVE	BIT(4)
	#define	GMII_LED1_ACTIVE	BIT(5)
	#define	GMII_LED2_ACTIVE	BIT(6)

#define GMII_LED_LINK				0x1c
	#define	GMII_LED_LINK_MASK	0xf888
	#define	GMII_LED0_LINK_10	BIT(0)
	#define	GMII_LED0_LINK_100	BIT(1)
	#define	GMII_LED0_LINK_1000	BIT(2)
	#define	GMII_LED1_LINK_10	BIT(4)
	#define	GMII_LED1_LINK_100	BIT(5)
	#define	GMII_LED1_LINK_1000	BIT(6)
	#define	GMII_LED2_LINK_10	BIT(8)
	#define	GMII_LED2_LINK_100	BIT(9)
	#define	GMII_LED2_LINK_1000	BIT(10)
	#define	LED0_ACTIVE		BIT(0)
	#define	LED0_LINK_10		BIT(1)
	#define	LED0_LINK_100		BIT(2)
	#define	LED0_LINK_1000		BIT(3)
	#define	LED0_FD			BIT(4)
	#define	LED0_USB3_MASK		0x001f
	#define	LED1_ACTIVE		BIT(5)
	#define	LED1_LINK_10		BIT(6)
	#define	LED1_LINK_100		BIT(7)
	#define	LED1_LINK_1000		BIT(8)
	#define	LED1_FD			BIT(9)
	#define	LED1_USB3_MASK		0x03e0
	#define	LED2_ACTIVE		BIT(10)
	#define	LED2_LINK_1000		BIT(13)
	#define	LED2_LINK_100		BIT(12)
	#define	LED2_LINK_10		BIT(11)
	#define	LED2_FD			BIT(14)
	#define	LED_VALID		BIT(15)
	#define	LED2_USB3_MASK		0x7c00

#define GMII_PHYPAGE				0x1e
#define GMII_PHY_PAGE_SELECT			0x1f
	#define GMII_PHY_PGSEL_EXT	0x0007
	#define GMII_PHY_PGSEL_PAGE0	0x0000
	#define GMII_PHY_PGSEL_PAGE3	0x0003
	#define GMII_PHY_PGSEL_PAGE5	0x0005


static bool ax88179 = false;

void set_ax88179_mode(bool on){
    ax88179 = on;
}

int ax8817xReadCommand_debug(void *context, void *param_2, int cmd, int index, int value, u32 size, void *buf){
	int ret = ax8817xReadCommand(context, param_2, cmd, index, value, size, buf);
	debug_printf("AX88179: readCommand(%p, %p, 0x%x, %i, %i, %u, %p) -> %d\n", context, param_2, cmd, index, value, size, buf, ret);
	return ret;
}

int ax8817xWriteCommand_debug(void *context, void *param_2, int cmd, int index, int value, u32 size, void *buf){
	int ret = ax8817xWriteCommand(context, param_2, cmd, index, value, size, buf);
	debug_printf("AX88179: writeCommand(%p, %p, 0x%x, %i, %i, %u, %p) -> %d\n", context, param_2, cmd, index, value, size, buf, ret);
	return ret;
}

int read_eeprom_hook(void *context, void *param_2, int cmd, int index, rw_func* org_read, int lr, int value, size_t size, void *buf){
    debug_printf("AX88179: read_eeprom(%p, %p, 0x%x, %i, %i, %u, %p)\n", context, param_2, cmd, index, value, size, buf);
    if(ax88179){
        cmd = AX_ACCESS_EEPROM;
    }
    int ret = org_read(context, param_2, cmd, index, value, size, buf);
    debug_printf("AX88179: read_eeprom(%p, %p, 0x%x, %i, %i, %u, %p) -> %i\n", context, param_2, cmd, index, value, size, buf, ret);
    return ret;
}

// https://github.com/torvalds/linux/blob/95d3481af6dc90fd7175a7643fd108cdcb808ce5/drivers/net/usb/ax88179_178a.c#L1686
int read_phyid(void *context, void *param2){
	return ax8817xReadCommand_debug(context, param2, AX_ACCESS_PHY, AX88179_PHY_ID, GMII_PHY_PHYSR, 2, context + 0x51e);
}

void power_up_phy(void *context, void *param2){
	debug_printf("AX88179: Power up PHY\n");
	u8 buf[5] = { 0 };
	u16 *tmp16 = (u16 *)buf;
	ax8817xWriteCommand_debug(context, param2, AX_ACCESS_MAC, AX_PHYPWR_RSTCTL, 2, 2, tmp16);

	*tmp16 = AX_PHYPWR_RSTCTL_IPRL;
	ax8817xWriteCommand_debug(context, param2, AX_ACCESS_MAC, AX_PHYPWR_RSTCTL, 2, 2, tmp16);
	msleep(500);

	*buf = AX_CLK_SELECT_ACS | AX_CLK_SELECT_BCS;
	ax8817xWriteCommand_debug(context, param2, AX_ACCESS_MAC, AX_CLK_SELECT, 1, 1, buf);
	msleep(200);
}

void reset_hook(void  *context, void *parm2){
	read_phyid(context, parm2);
	power_up_phy(context, parm2);
}


int read_phyid_hook(void *context, void *param_2, int cmd, int index, rw_func* org_read, int lr, int value, size_t size, void *buf){
	int ret = org_read(context, param_2, AX_ACCESS_PHY, AX88179_PHY_ID, GMII_PHY_PHYSR, size, buf);
	debug_printf("AX88179: read_pyhid_hook -> %i, phyid: %d\n", context, param_2, cmd, index, value, size, buf, ret, *(u16*)buf);
	//power_up_phy(context, param_2);
	return ret;
}

void ax88179_apply_patches(void) {
    trampoline_blreplace(0x123b9fb4, read_eeprom_hook);
	trampoline_blreplace(0x123ba0a0, reset_hook);
	trampoline_blreplace(0x123ba1ec, read_phyid_hook);
}