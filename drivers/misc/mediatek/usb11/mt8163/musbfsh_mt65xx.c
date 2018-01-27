#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>

#ifdef CONFIG_OF
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#endif

#include <linux/notifier.h>
#include <linux/fb.h>

#include "musb.h"
#include "musbfsh_core.h"
#include "musbfsh_mt65xx.h"
#ifdef CONFIG_MTK_CLKMGR
#include <mach/mt_clkmgr.h>
#endif

#define FRA (48)
#define PARA (25)
bool musbfsh_power = false;

struct mt_usb11_glue {
	struct device *dev;
	struct platform_device *musbfsh;
};
static const struct of_device_id apusb_of_ids[] = {
	{.compatible = "mediatek,mt8163-usb11",},
	{},
};


void usb11_hs_slew_rate_cal(void)
{
	unsigned long data;
	unsigned long x;
	unsigned char value;
	unsigned long start_time, timeout;
	unsigned int timeout_flag = 0;
	/*4 s1:enable usb ring oscillator.*/
	USB11PHY_WRITE8(0x15, 0x80);

	/*4 s2:wait 1us.*/
	udelay(1);

	/*4 s3:enable free run clock*/
	USB11PHY_WRITE8(0xf00 - 0x900 + 0x11, 0x01);
	/*4 s4:setting cyclecnt*/
	USB11PHY_WRITE8(0xf00 - 0x900 + 0x01, 0x04);
	/*4 s5:enable frequency meter*/
	USB11PHY_SET8(0xf00 - 0x900 + 0x03, 0x05);

	/*4 s6:wait for frequency valid.*/
	start_time = jiffies;
	timeout = jiffies + 3 * HZ;
	while (!(USB11PHY_READ8(0xf00 - 0x900 + 0x10) & 0x1)) {
		if (time_after(jiffies, timeout)) {
			timeout_flag = 1;
			break;
	    }
	}

	/*4 s7: read result.*/
	if (timeout_flag) {
		INFO("[USBPHY] Slew Rate Calibration: Timeout\n");
		value = 0x4;
	} else {
	    data = USB11PHY_READ32(0xf00 - 0x900 + 0x0c);
	    x = ((1024 * FRA * PARA) / data);
	    value = (unsigned char)(x / 1000);
		if (((x - value * 1000) / 100) >= 5)
			value += 1;
	    /* INFO("[USB11PHY]slew calibration:FM_OUT =%d, x=%d,value=%d\n",data,x,value);*/
	}

	/*4 s8: disable Frequency and run clock.*/
	USB11PHY_CLR8(0xf00 - 0x900 + 0x03, 0x05);	/*disable frequency meter*/
	USB11PHY_CLR8(0xf00 - 0x900 + 0x11, 0x01);	/*disable free run clock*/

	/*4 s9: */
	USB11PHY_WRITE8(0x15, value << 4);

	/*4 s10:disable usb ring oscillator.*/
	USB11PHY_CLR8(0x15, 0x80);
}

void mt65xx_usb11_phy_poweron(void)
{
	INFO("mt65xx_usb11_phy_poweron++\r\n");
	INFO("[Flow][USB11]%s:%d\n", __func__, __LINE__);
	/* enable_pll(UNIVPLL, "USB11"); */
	/*udelay(100); */
#if 0
	/* reverse preloader's sin @mt6575_usbphy.c */
	USB11PHY_CLR8(U1PHTCR2 + 3,
		force_usb11_avalid | force_usb11_bvalid | force_usb11_sessend |
		force_usb11_vbusvalid);
	USB11PHY_CLR8(U1PHTCR2 + 2,
		RG_USB11_AVALID | RG_USB11_BVALID | RG_USB11_SESSEND | RG_USB11_VBUSVALID);
	USB11PHY_CLR8(U1PHYCR1 + 2, force_usb11_en_fs_ls_rcv | force_usb11_en_fs_ls_tx);
	/**************************************/

	USB11PHY_SET8(U1PHYCR0 + 1, RG_USB11_FSLS_ENBGRI);

	USB11PHY_SET8(U1PHTCR2 + 3,
		force_usb11_avalid | force_usb11_sessend | force_usb11_vbusvalid);
	USB11PHY_SET8(U1PHTCR2 + 2, RG_USB11_AVALID | RG_USB11_VBUSVALID);
	USB11PHY_CLR8(U1PHTCR2 + 2, RG_USB11_SESSEND);
#endif


#if 0
	/*HQA LOOPBACK TEST. <FS>*/
	USB11PHY_CLR8(0x1a, 0x80);
	USB11PHY_CLR8(0x68, 0x08);

	USB11PHY_CLR8(0x68, 0x03);
	USB11PHY_SET8(0x68, 0x10);

	USB11PHY_SET8(0x68, 0x04);
	USB11PHY_CLR8(0x69, 0x03);
	USB11PHY_CLR8(0x69, 0x3C);

	USB11PHY_SET8(0x68, 0x80);

	USB11PHY_SET8(0x6a, 0x04);
	USB11PHY_SET8(0x6a, 0x01);
	USB11PHY_SET8(0x6a, 0x08);
	USB11PHY_SET8(0x6a, 0x02);

	USB11PHY_SET8(0x6a, 0x40);
	USB11PHY_SET8(0x6a, 0x80);
	USB11PHY_SET8(0x6a, 0x30);

	USB11PHY_SET8(0x68, 0x08);
	udelay(50);

	USB11PHY_SET8(0x63, 0x02);
	udelay(1);

	USB11PHY_SET8(0x63, 0x02);
	USB11PHY_SET8(0x63, 0x04);
	USB11PHY_CLR8(0x63, 0x08);

#endif



#if 0
	/*HQA LOOPBACK TEST. <HS>*/
	USB11PHY_CLR8(0x1a, 0x80);
	USB11PHY_CLR8(0x68, 0x08);

	USB11PHY_CLR8(0x68, 0x03);
	USB11PHY_CLR8(0x68, 0x30);

	USB11PHY_CLR8(0x68, 0x04);
	USB11PHY_CLR8(0x69, 0x03);
	USB11PHY_CLR8(0x69, 0x3C);

	USB11PHY_CLR8(0x68, 0xC0);

	USB11PHY_SET8(0x6a, 0x04);
	USB11PHY_SET8(0x6a, 0x01);
	USB11PHY_SET8(0x6a, 0x08);
	USB11PHY_SET8(0x6a, 0x02);

	USB11PHY_SET8(0x6a, 0x40);
	USB11PHY_SET8(0x6a, 0x80);
	USB11PHY_SET8(0x6a, 0x30);

	USB11PHY_SET8(0x68, 0x08);
	udelay(50);

	USB11PHY_SET8(0x63, 0x02);
	udelay(1);

	USB11PHY_SET8(0x63, 0x02);
	USB11PHY_SET8(0x63, 0x04);
	USB11PHY_CLR8(0x63, 0x08);

	usb11_hs_slew_rate_cal();

#endif

	udelay(50);

	USB11PHY_CLR8(0x6b, 0x04);
	USB11PHY_CLR8(0x6e, 0x01);

	USB11PHY_CLR8(0x1a, 0x80);

	/* remove in MT6588 ?????
	 USBPHY_CLR8(0x02, 0x7f);
	 USBPHY_SET8(0x02, 0x09);
	 USBPHY_CLR8(0x22, 0x03);
	*/

	USB11PHY_CLR8(0x6a, 0x04);
	/*USBPHY_SET8(0x1b, 0x08);*/

	/*force VBUS Valid          */
	USB11PHY_SET8(0x6C, 0x2C);
	USB11PHY_SET8(0x6D, 0x3C);
#if 1				/*joson*/
	/* VBUSVALID=0, AVALID=0, BVALID=0, SESSEND=1, IDDIG=X */
	USB11PHY_SET8(0x6c, 0x10);
	USB11PHY_CLR8(0x6c, 0x2e);
	USB11PHY_SET8(0x6d, 0x3e);

	/* wait */
	msleep(20);
	/* restart session */

	/* USB MAC ONand Host Mode */
	/* VBUSVALID=1, AVALID=1, BVALID=1, SESSEND=0, IDDIG=0 */
	USB11PHY_CLR8(0x6c, 0x10);
	USB11PHY_SET8(0x6c, 0x2c);
	USB11PHY_SET8(0x6d, 0x3e);
#endif
	udelay(800);

}


void mt65xx_usb11_phy_savecurrent(void)
{
	INFO("mt65xx_usb11_phy_savecurrent++\r\n");
	INFO("[Flow][USB11]%s:%d\n", __func__, __LINE__);
#if 0
	USB11PHY_SET8(U1PHTCR2 + 3,
		force_usb11_avalid | force_usb11_sessend | force_usb11_vbusvalid);
	USB11PHY_CLR8(U1PHTCR2 + 2, RG_USB11_AVALID | RG_USB11_VBUSVALID);
	USB11PHY_SET8(U1PHTCR2 + 2, RG_USB11_SESSEND);

	USB11PHY_CLR8(U1PHYCR0 + 1, RG_USB11_FSLS_ENBGRI);

	USB11PHY_SET8(U1PHYCR1 + 2, force_usb11_en_fs_ls_rcv | force_usb11_en_fs_ls_tx);
	USB11PHY_CLR8(U1PHYCR1 + 3, RG_USB11_EN_FS_LS_RCV | RG_USB11_EN_FS_LS_TX);
#endif

	/*4 1. swtich to USB function. (system register, force ip into usb mode.*/
	USB11PHY_CLR8(0x6b, 0x04);
	USB11PHY_CLR8(0x6e, 0x01);

	/*4 2. release force suspendm.*/
	USB11PHY_CLR8(0x6a, 0x04);
	/*4 3. RG_DPPULLDOWN./RG_DMPULLDOWN.*/
	USB11PHY_SET8(0x68, 0xc0);
	/*4 4. RG_XCVRSEL[1:0] =2'b01.*/
	USB11PHY_CLR8(0x68, 0x30);
	USB11PHY_SET8(0x68, 0x10);
	/*4 5. RG_TERMSEL = 1'b1*/
	USB11PHY_SET8(0x68, 0x04);
	/*4 6. RG_DATAIN[3:0]=4'b0000*/
	USB11PHY_CLR8(0x69, 0x3c);
	/*4 7.force_dp_pulldown, force_dm_pulldown, force_xcversel,force_termsel.*/
	USB11PHY_SET8(0x6a, 0xba);

	/*4 8.RG_USB20_BC11_SW_EN 1'b0*/
	USB11PHY_CLR8(0x1a, 0x80);
	/*4 9.RG_USB20_OTG_VBUSSCMP_EN 1'b0*/
	USB11PHY_CLR8(0x1a, 0x10);
	/*4 10. delay 800us.*/
	udelay(800);
	/*4 11. rg_usb20_pll_stable = 1*/
	USB11PHY_SET8(0x63, 0x02);

	udelay(1);
	/*4 12.  force suspendm = 1.*/
	USB11PHY_SET8(0x6a, 0x04);

	USB11PHY_CLR8(0x6C, 0x2C);
	USB11PHY_SET8(0x6C, 0x10);
	USB11PHY_CLR8(0x6D, 0x3C);

	/*4 13.  wait 1us*/
	udelay(1);
	/*4 14. turn off internal 48Mhz PLL.*/
	enable_phy_clock(false);
}

void mt81xx_usb11_phy_recover(void)
{
	INFO("[Flow][USB11]%s:%d\n", __func__, __LINE__);
	INFO("mt65xx_usb11_phy_recover++\r\n");
	/*4 1. turn on USB reference clock.     */
	/*enable_pll(UNIVPLL, "USB11"); */
	/*
	* swtich to USB function.
	* (system register, force ip into usb mode).
	*/
	USB11PHY_CLR8(0x6b, 0x04);
	USB11PHY_CLR8(0x6e, 0x01);

	/* RG_USB20_BC11_SW_EN = 1'b0 */
	USB11PHY_CLR8(0x1a, 0x80);

	/* RG_USB20_DP_100K_EN = 1'b0 */
	/* RG_USB20_DM_100K_EN = 1'b0 */
	USB11PHY_CLR8(0x22, 0x03);

	/* release force suspendm */
	USB11PHY_CLR8(0x6a, 0x04);

	udelay(800);

	/* force enter device mode */
	USB11PHY_CLR8(0x6c, 0x10);
	USB11PHY_SET8(0x6c, 0x2E);
	USB11PHY_SET8(0x6d, 0x3E);

	/* clean PUPD_BIST_EN */
	/* PUPD_BIST_EN = 1'b0 */
	/* PMIC will use it to detect charger type */
	/*USB11PHY_CLR8(0x1d, 0x10);*/

	/* force_uart_en = 1'b0 */
	USB11PHY_CLR8(0x6b, 0x04);
	/* RG_UART_EN = 1'b0 */
	USB11PHY_CLR8(0x6e, 0x01);
	/* force_uart_en = 1'b0 */
	USB11PHY_CLR8(0x6a, 0x04);

	USB11PHY_CLR8(0x68, 0xf4);
	USB11PHY_SET8(0x68, 0x08);

	/* RG_DATAIN[3:0] = 4'b0000 */
	USB11PHY_CLR8(0x69, 0x3c);

	USB11PHY_CLR8(0x6a, 0xba);

	/* RG_USB20_BC11_SW_EN = 1'b0 */
	USB11PHY_SET8(0x1a, 0x10);
	USB11PHY_CLR8(0x1a, 0x80);
	/* RG_USB20_OTG_VBUSSCMP_EN = 1'b1 */
	USB11PHY_SET8(0x1a, 0x10);

	udelay(800);

	/* force enter device mode */
	USB11PHY_CLR8(0x6c, 0x10);
	USB11PHY_SET8(0x6c, 0x2E);
	USB11PHY_SET8(0x6d, 0x3E);

	/* force enter host mode */
#if 1
	udelay(100);

	USB11PHY_SET8(0x6d, 0x3e);
	USB11PHY_SET8(0x6c, 0x10);
	USB11PHY_CLR8(0x6c, 0x2e);

	udelay(5);

	USB11PHY_CLR8(0x6c, 0x10);
	USB11PHY_SET8(0x6c, 0x2c);
#endif
	return;
	}

	void mt65xx_usb11_phy_recover(void)
	{
	INFO("[Flow][USB11]%s:%d\n", __func__, __LINE__);
	INFO("mt65xx_usb11_phy_recover++\r\n");
	/*4 1. turn on USB reference clock.  */
	enable_phy_clock(true);

#if 0
	USB11PHY_SET8(U1PHTCR2 + 3,
		force_usb11_avalid | force_usb11_sessend | force_usb11_vbusvalid);
	USB11PHY_SET8(U1PHTCR2 + 2, RG_USB11_AVALID | RG_USB11_VBUSVALID);
	USB11PHY_CLR8(U1PHTCR2 + 2, RG_USB11_SESSEND);

	USB11PHY_CLR8(U1PHYCR1 + 2, force_usb11_en_fs_ls_rcv | force_usb11_en_fs_ls_tx);
	USB11PHY_CLR8(U1PHYCR1 + 3, RG_USB11_EN_FS_LS_RCV | RG_USB11_EN_FS_LS_TX);

	USB11PHY_SET8(U1PHYCR0 + 1, RG_USB11_FSLS_ENBGRI);

	udelay(100);
#endif
	INFO("[Flow][USB11]%s:%d\n", __func__, __LINE__);
#if 1
	USB11PHY_CLR8(0x6b, 0x04);
	USB11PHY_CLR8(0x6e, 0x01);

	/* RG_USB20_BC11_SW_EN = 1'b0 */
	USB11PHY_CLR8(0x1a, 0x80);

	/* RG_USB20_DP_100K_EN = 1'b0 */
	/* RG_USB20_DM_100K_EN = 1'b0 */
	USB11PHY_CLR8(0x22, 0x03);

	/* release force suspendm */
	USB11PHY_CLR8(0x6a, 0x04);

	udelay(800);

	/* force enter device mode */
	USB11PHY_CLR8(0x6c, 0x10);
	USB11PHY_SET8(0x6c, 0x2E);
	USB11PHY_SET8(0x6d, 0x3E);

	/* clean PUPD_BIST_EN */
	/* PUPD_BIST_EN = 1'b0 */
	/* PMIC will use it to detect charger type */
	USB11PHY_CLR8(0x1d, 0x10);

	/* force_uart_en = 1'b0 */
	USB11PHY_CLR8(0x6b, 0x04);
	/* RG_UART_EN = 1'b0 */
	USB11PHY_CLR8(0x6e, 0x01);
	/* force_uart_en = 1'b0 */
	USB11PHY_CLR8(0x6a, 0x04);

	USB11PHY_CLR8(0x68, 0xf4);

	/* RG_DATAIN[3:0] = 4'b0000 */
	USB11PHY_CLR8(0x69, 0x3c);

	USB11PHY_CLR8(0x6a, 0xba);

	/* RG_USB20_BC11_SW_EN = 1'b0 */
	USB11PHY_CLR8(0x1a, 0x80);
	/* RG_USB20_OTG_VBUSSCMP_EN = 1'b1 */
	USB11PHY_SET8(0x1a, 0x10);

	udelay(800);

	/* force enter device mode */
	USB11PHY_CLR8(0x6c, 0x10);
	USB11PHY_SET8(0x6c, 0x2E);
	USB11PHY_SET8(0x6d, 0x3E);
	INFO("[Flow][USB11]%s:%d\n", __func__, __LINE__);
	/* force enter host mode */
#if 1
	udelay(100);

	USB11PHY_SET8(0x6d, 0x3e);
	USB11PHY_READ8(0x6d);
	USB11PHY_SET8(0x6c, 0x10);
	USB11PHY_CLR8(0x6c, 0x2e);

	udelay(5);

	USB11PHY_CLR8(0x6c, 0x10);
	USB11PHY_SET8(0x6c, 0x2c);
	USB11PHY_READ8(0x6c);
#endif
#endif
	INFO("[Flow][USB11]%s:%d\n", __func__, __LINE__);
#if 0
	/*4 2. wait 50 usec.*/
	udelay(50);
	/*4 3. force_uart_en = 1'b0*/
	USB11PHY_CLR8(0x6b, 0x04);
	/*4 4. RG_UART_EN = 1'b0*/
	USB11PHY_CLR8(0x6e, 0x1);
	/*4 5. force_uart_en = 1'b0*/
	USB11PHY_CLR8(0x6a, 0x04);

	/*4 6. RG_DPPULLDOWN = 1'b0*/
	USB11PHY_CLR8(0x68, 0x40);
	/*4 7. RG_DMPULLDOWN = 1'b0*/
	USB11PHY_CLR8(0x68, 0x80);
	/*4 8. RG_XCVRSEL = 2'b00*/
	USB11PHY_CLR8(0x68, 0x30);
	/*4 9. RG_TERMSEL = 1'b0*/
	USB11PHY_CLR8(0x68, 0x04);
	/*4 10. RG_DATAIN[3:0] = 4'b0000    */
	USB11PHY_CLR8(0x69, 0x3c);

	/*4 11. force_dp_pulldown = 1b'0*/
	USB11PHY_CLR8(0x6a, 0x10);
	/*4 12. force_dm_pulldown = 1b'0*/
	USB11PHY_CLR8(0x6a, 0x20);
	/*4 13. force_xcversel = 1b'0*/
	USB11PHY_CLR8(0x6a, 0x08);
	/*4 14. force_termsel = 1b'0   */
	USB11PHY_CLR8(0x6a, 0x02);
	/*4 15. force_datain = 1b'0*/
	USB11PHY_CLR8(0x6a, 0x80);

	/*4 16. RG_USB20_BC11_SW_EN 1'b0*/
	USB11PHY_CLR8(0x1a, 0x80);
	/*4 17. RG_USB20_OTG_VBUSSCMP_EN 1'b1   */
	USB11PHY_SET8(0x1a, 0x10);

	USB11PHY_SET8(0x6C, 0x2C);
	USB11PHY_SET8(0x6D, 0x3C);
	/*<4> 18. wait 800 usec.*/
	udelay(800);

	usb11_hs_slew_rate_cal();
#endif
}


static bool clock_enabled;

void mt65xx_usb11_clock_enable(bool enable)
{
	INFO("[Flow][USB]mt65xx_usb11_clock_enable++\r\n");
	if (enable) {
		if (clock_enabled) {	/*already enable*/
			/* do nothing */
			INFO("[Flow][USB]already enable\r\n");
		} else {
			enable_phy_clock(enable);
#ifdef CONFIG_MTK_CLKMGR
			enable_clock(MT_CG_INFRA_USB, "INFRA_USB");
			enable_clock(MT_CG_INFRA_USB_MCU, "INFRA_USB_MCU");
			enable_clock(MT_CG_INFRA_ICUSB, "INFRA_ICUSB");
#else
			INFO("[Flow][USB]enable usb11 clock ++\r\n");
			enable_mcu_clock(true);
			clk_enable(icusb_clk);
#endif
			clock_enabled = true;
		}
	} else {
		if (!clock_enabled)	{/*already disabled.*/
			/* do nothing */
			INFO("[Flow][USB]already disabled\r\n");
		} else {
#ifdef CONFIG_MTK_CLKMGR
			disable_clock(MT_CG_INFRA_USB, "INFRA_USB");
			disable_clock(MT_CG_INFRA_USB_MCU, "INFRA_USB_MCU");
			enable_clock(MT_CG_INFRA_ICUSB, "INFRA_ICUSB");
#else
			INFO("[Flow][USB]disable usb11 clock --\r\n");
			clk_disable(icusb_clk);
			enable_mcu_clock(false);
#endif
			enable_phy_clock(enable);
			clock_enabled = false;
		}
	}
}


void mt_usb11_poweron(struct musbfsh *musbfsh, int on)
{
	static bool recover;

	INFO("mt65xx_usb11_poweron++\r\n");
	INFO("[Flow][USB11]%s:%d\n", __func__, __LINE__);
	if (on) {
		if (musbfsh_power) {
			/* do nothing */
			INFO("[Flow][USB]power on\r\n");
		} else {
			mt65xx_usb11_clock_enable(true);
			INFO("[Flow][USB11]%s:%d\n", __func__, __LINE__);
			if (!recover) {
				INFO("[Flow][USB11]%s:%d\n", __func__, __LINE__);
				/*mt65xx_usb11_phy_poweron();*/
				mt65xx_usb11_phy_recover();
				/*mt81xx_usb11_phy_recover();*/
				recover = true;
			} else {
				INFO("[Flow][USB11]%s:%d\n", __func__, __LINE__);
				mt65xx_usb11_phy_recover();
				/*mt81xx_usb11_phy_recover();*/
			}
			musbfsh_power = true;
		}
	} else {
		if (!musbfsh_power) {
			/* do nothing */
			INFO("[Flow][USB]power off\r\n");
		} else {
			mt65xx_usb11_phy_savecurrent();
			mt65xx_usb11_clock_enable(false);
			musbfsh_power = false;
		}
	}
}

void mt_usb11_set_vbus(struct musbfsh *musbfsh, int is_on)
{
	INFO("is_on=%d\n", is_on);
#if 0
	mt_set_gpio_dir(GPIO_OTG_DRVVBUS_PIN, GPIO_DIR_OUT);
	if (1 == is_on) {
		if (oned)
			else {
				mt_set_gpio_out(GPIO_OTG_DRVVBUS_PIN, GPIO_OUT_ONE);
				oned = 1;
			}
	} else {
	if (!oned)
		else {
			mt_set_gpio_out(GPIO_OTG_DRVVBUS_PIN, GPIO_OUT_ZERO);
			oned = 0;
		}
	}
#endif
}

void musbfs_check_mpu_violation(u32 addr, int wr_vio)
{
	void __iomem *mregs = (void *)USB_BASE;

	INFO(KERN_CRIT "MUSB checks EMI MPU violation.\n");
	INFO(KERN_CRIT "addr = 0x%x, %s violation.\n", addr, wr_vio ? "Write" : "Read");
	INFO(KERN_CRIT "POWER = 0x%x,DEVCTL= 0x%x.\n", musbfsh_readb(mregs, MUSBFSH_POWER),
	musbfsh_readb((void __iomem *)USB11_BASE, MUSBFSH_DEVCTL));
	INFO(KERN_CRIT "DMA_CNTLch0 0x%04x,DMA_ADDRch0 0x%08x,DMA_COUNTch0 0x%08x\n",
	musbfsh_readw(mregs, 0x204), musbfsh_readl(mregs, 0x208), musbfsh_readl(mregs,
					0x20C));
	INFO(KERN_CRIT "DMA_CNTLch1 0x%04x,DMA_ADDRch1 0x%08x,DMA_COUNTch1 0x%08x\n",
	musbfsh_readw(mregs, 0x214), musbfsh_readl(mregs, 0x218), musbfsh_readl(mregs,
					0x21C));
	INFO(KERN_CRIT "DMA_CNTLch2 0x%04x,DMA_ADDRch2 0x%08x,DMA_COUNTch2 0x%08x\n",
	musbfsh_readw(mregs, 0x224), musbfsh_readl(mregs, 0x228), musbfsh_readl(mregs,
					0x22C));
	INFO(KERN_CRIT "DMA_CNTLch3 0x%04x,DMA_ADDRch3 0x%08x,DMA_COUNTch3 0x%08x\n",
	musbfsh_readw(mregs, 0x234), musbfsh_readl(mregs, 0x238), musbfsh_readl(mregs,
					0x23C));
	INFO(KERN_CRIT "DMA_CNTLch4 0x%04x,DMA_ADDRch4 0x%08x,DMA_COUNTch4 0x%08x\n",
	musbfsh_readw(mregs, 0x244), musbfsh_readl(mregs, 0x248), musbfsh_readl(mregs,
					0x24C));
	INFO(KERN_CRIT "DMA_CNTLch5 0x%04x,DMA_ADDRch5 0x%08x,DMA_COUNTch5 0x%08x\n",
	musbfsh_readw(mregs, 0x254), musbfsh_readl(mregs, 0x258), musbfsh_readl(mregs,
					0x25C));
	INFO(KERN_CRIT "DMA_CNTLch6 0x%04x,DMA_ADDRch6 0x%08x,DMA_COUNTch6 0x%08x\n",
	musbfsh_readw(mregs, 0x264), musbfsh_readl(mregs, 0x268), musbfsh_readl(mregs,
					0x26C));
	INFO(KERN_CRIT "DMA_CNTLch7 0x%04x,DMA_ADDRch7 0x%08x,DMA_COUNTch7 0x%08x\n",
	musbfsh_readw(mregs, 0x274), musbfsh_readl(mregs, 0x278), musbfsh_readl(mregs,
					0x27C));
}

int mt_usb11_init(struct musbfsh *musbfsh)
{
	INFO("++\n");
	if (!musbfsh) {
		ERR("musbfsh_platform_init,error,musbfsh is NULL");
		return -1;
	}

	INFO("[Flow][USB11]%s:%d\n", __func__, __LINE__);
	mt_usb11_poweron(musbfsh, true);
	return 0;
}

int mt_usb11_exit(struct musbfsh *musbfsh)
{
	INFO("++\n");
	mt_usb11_poweron(musbfsh, false);
	/* put it here because we can't shutdown PHY power during suspend */
	/* hwPowerDown(MT65XX_POWER_LDO_VUSB, "USB11");  */
	return 0;
}

void musbfsh_hcd_release(struct device *dev)
{
/*    INFO("musbfsh_hcd_release++,dev = 0x%08X.\n", (uint32_t)dev);*/
}

static const struct musbfsh_platform_ops mt_usb11_ops = {
	.init = mt_usb11_init,
	.exit = mt_usb11_exit,
	.set_vbus = mt_usb11_set_vbus,
	.set_power = mt_usb11_poweron,
};


#ifdef CONFIG_OF
static struct regulator *hub_vgp = NULL;
static struct pinctrl_state *hub_rst_high = NULL;
static struct pinctrl_state *hub_rst_low = NULL;
static struct pinctrl_state *hub_rst_default = NULL;
static struct pinctrl *pinctrl;
static struct pinctrl_state *pinctrl_pogo = NULL;
static struct pinctrl_state *pinctrl_pogo_low = NULL;
static struct pinctrl_state *pinctrl_pogo_high = NULL;
static struct pinctrl_state *pinctrl_vdd_5v_en = NULL;
static struct pinctrl_state *pinctrl_vdd_5v_en_low = NULL;
static struct pinctrl_state *pinctrl_vdd_5v_en_high = NULL;

#define TYPE_HUB_RST  1
#define TYPE_POGO_EN  2
#define TYPE_VDD_5V_EN 3

static int pogo_hub_get_gpio(struct device *dev)
{
	int ret = 0;

	pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(pinctrl) || NULL == pinctrl) {
		dev_err(dev, "Cannot find hub rest pinctrl!");
		ret = PTR_ERR(pinctrl);
		return ret;
	}
	/*USB power pin lookup */
	hub_rst_default = pinctrl_lookup_state(pinctrl, "hub_rst_init");
	if (IS_ERR(hub_rst_default) || NULL == hub_rst_default) {
		ret = PTR_ERR(hub_rst_default);
		pr_debug("%s : pinctrl err, hub_rst_default\n", __func__);
	}
	hub_rst_high = pinctrl_lookup_state(pinctrl, "hub_rst_high");
	if (IS_ERR(hub_rst_high) || NULL == hub_rst_high) {
		ret = PTR_ERR(hub_rst_high);
		pr_debug("%s : pinctrl err, hub_rst_high\n", __func__);
	}
	hub_rst_low = pinctrl_lookup_state(pinctrl, "hub_rst_low");
	if (IS_ERR(hub_rst_low) || NULL == hub_rst_low) {
		ret = PTR_ERR(hub_rst_low);
		pr_debug("%s : pinctrl err, hub_rst_low\n", __func__);
	}
	
	pinctrl_pogo = pinctrl_lookup_state(pinctrl, "pogo_init");
	if (IS_ERR(pinctrl_pogo)) {
		ret = PTR_ERR(pinctrl_pogo);
		dev_err(dev, "Cannot find usb pinctrl pogo\n");
	}

	pinctrl_pogo_low = pinctrl_lookup_state(pinctrl, "pogo_low");
	if (IS_ERR(pinctrl_pogo_low)) {
		ret = PTR_ERR(pinctrl_pogo_low);
		dev_err(dev, "Cannot find usb pinctrl pogo_low\n");
	}

	pinctrl_pogo_high = pinctrl_lookup_state(pinctrl, "pogo_high");
	if (IS_ERR(pinctrl_pogo_high)) {
		ret = PTR_ERR(pinctrl_pogo_high);
		dev_err(dev, "Cannot find usb pinctrl pogo_high\n");
	}
	
	pinctrl_vdd_5v_en = pinctrl_lookup_state(pinctrl, "vdd_v5_en_init");
	if (IS_ERR(pinctrl_vdd_5v_en)) {
		ret = PTR_ERR(pinctrl_vdd_5v_en);
		dev_err(dev, "Cannot find usb pinctrl pinctrl_vdd_5v_en\n");
	}

	pinctrl_vdd_5v_en_low = pinctrl_lookup_state(pinctrl, "vdd_v5_en_low");
	if (IS_ERR(pinctrl_vdd_5v_en_low)) {
		ret = PTR_ERR(pinctrl_vdd_5v_en_low);
		dev_err(dev, "Cannot find usb pinctrl pinctrl_vdd_5v_en_low\n");
	}

	pinctrl_vdd_5v_en_high = pinctrl_lookup_state(pinctrl, "vdd_v5_en_high");
	if (IS_ERR(pinctrl_vdd_5v_en_high)) {
		ret = PTR_ERR(pinctrl_vdd_5v_en_high);
		dev_err(dev, "Cannot find usb pinctrl pinctrl_vdd_5v_en_high\n");
	}
	return ret;
}

static bool my_set_gpio(int type, int val)
{
	struct pinctrl_state * pCtrl = NULL;
	char * pStrDebug = NULL;
	
	if ( NULL == pinctrl || IS_ERR(pinctrl)) {
		return false;
	}
	
	if (val == 0) {
		switch(type) {
		case TYPE_HUB_RST:
			pCtrl = hub_rst_low;
			pStrDebug = "hub_rst";
			break;
		case TYPE_POGO_EN:
			pCtrl = pinctrl_pogo_low;
			pStrDebug = "pogo_en";
			break;
		case TYPE_VDD_5V_EN:
			pCtrl = pinctrl_vdd_5v_en_low;
			pStrDebug = "vdd_5vp01_en";
			break;
		default:
			pCtrl = NULL;
			pStrDebug = "null";
			break;
		}
		
		if ( pCtrl == NULL || IS_ERR(pCtrl)) return false;
			
		pinctrl_select_state(pinctrl, pCtrl);
		pr_debug("USB: %s set power off\n", pStrDebug);
	} else {
		switch(type) {
		case TYPE_HUB_RST:
			pCtrl = hub_rst_high;
			pStrDebug = "hub_rst";
			break;
		case TYPE_POGO_EN:
			pCtrl = pinctrl_pogo_high;
			pStrDebug = "pogo_en";
			break;
		case TYPE_VDD_5V_EN:
			pCtrl = pinctrl_vdd_5v_en_high;
			pStrDebug = "vdd_5vp01_en";
			break;
		default:
			pCtrl = NULL;
			pStrDebug = "null";
			break;
		}
		
		if ( pCtrl == NULL || IS_ERR(pCtrl)) return false;
			
		pinctrl_select_state(pinctrl, pCtrl);
		pr_debug("USB: %s set power on\n", pStrDebug);
	}
	return true;
}


/* get LDO supply */
static int hub_get_vgp_supply(struct device *dev)
{
	int ret = -1;
	struct regulator *hub_vgp_ldo;

	pr_err("USB: %s is going \n", __func__);

	hub_vgp_ldo = devm_regulator_get(dev, "usb_hub");
	if (IS_ERR(hub_vgp_ldo)) {
		ret = PTR_ERR(hub_vgp_ldo);
		dev_err(dev, "failed to get usb_hub LDO, %d\n", ret);
		return ret;
	}

	pr_debug("USB: USB get supply ok.\n");

	ret = regulator_enable(hub_vgp_ldo);
	/* get current voltage settings */
	ret = regulator_get_voltage(hub_vgp_ldo);
	pr_debug("USB LDO voltage = %d in LK stage\n", ret);

	hub_vgp = hub_vgp_ldo;

	return ret;
}

int hub_vgp_supply_enable(void)
{
	int ret;
	unsigned int volt;

	pr_debug("USB: hub_vgp_supply_enable\n");

	if (NULL == hub_vgp)
		return -1;

	pr_debug("USB: set regulator voltage hub_vgp voltage to 1.8V\n");
	/* set voltage to 1.8V */
	//ret = regulator_set_voltage(hub_vgp, 1800000, 1800000);
	ret = regulator_set_voltage(hub_vgp, 3300000, 3300000);
	if (ret != 0) {
		pr_err("USB: USB failed to set hub_vgp voltage: %d\n", ret);
		return ret;
	}

	/* get voltage settings again */
	volt = regulator_get_voltage(hub_vgp);
	if (volt == 3300000)
		pr_err("USB: check regulator voltage=3300000 pass!\n");
	else
		pr_err("USB: check regulator voltage=3300000 fail! (voltage: %d)\n", volt);

	ret = regulator_enable(hub_vgp);
	if (ret != 0) {
		pr_err("USB: Failed to enable hub_vgp: %d\n", ret);
		return ret;
	}

	return ret;
}

int hub_vgp_supply_disable(void)
{
	int ret = 0;
	unsigned int isenable;

	if (NULL == hub_vgp)
		return -1;

	/* disable regulator */
	isenable = regulator_is_enabled(hub_vgp);

	pr_debug("USB: USB query regulator enable status[0x%d]\n", isenable);

	if (isenable) {
		ret = regulator_disable(hub_vgp);
		if (ret != 0) {
			pr_err("USB: USB failed to disable hub_vgp: %d\n", ret);
			return ret;
		}
		/* verify */
		isenable = regulator_is_enabled(hub_vgp);
		if (!isenable)
			pr_err("USB: USB regulator disable pass\n");
	}

	return ret;
}
#endif

static void musbfsh_early_suspend(void)
{
	printk("%s: ===>begin\n", __func__);	
	
	if (my_set_gpio(TYPE_POGO_EN, 0)) {
		mdelay(20);
	}
	
	if (my_set_gpio(TYPE_HUB_RST, 0)) {
		mdelay(20);
	}
	
	hub_vgp_supply_disable();
	
	printk("%s: ===>end\n", __func__);	
}

static void musbfsh_late_resume(void)
{
	printk("%s: ===>begin\n", __func__);	
	
	if (0 == hub_vgp_supply_enable()) {
	    mdelay(20);
	}
	
    if (my_set_gpio(TYPE_HUB_RST, 1)) {
		mdelay(20);
	}
	
	if (my_set_gpio(TYPE_POGO_EN, 1)) {
		mdelay(20);
	}
	printk("%s: ===>end\n", __func__);	
}


static int _musb_notifier_callback(struct notifier_block *self ,unsigned long event , void *data)
{
	struct fb_event *evdata = data ; 
	int blank ; 
	
	//FUNC_ENTER(FUNC_LV_MODULE);
	
	/*skip if it's not a blank event */
	if(event != FB_EVENT_BLANK)
		return 0 ; 
		
	if(evdata == NULL)
		return 0 ;
		
	if(evdata->data == NULL)
		return 0 ; 
		
	blank = *(int *)evdata->data;
	printk("%s : blank = %d , event = %lu\n", __FUNCTION__,blank,event);
	
	switch(blank){
	/* resume */
	case FB_BLANK_UNBLANK:
	case FB_BLANK_NORMAL:
		musbfsh_late_resume();
		break;
		
	/* suspend */
	case FB_BLANK_POWERDOWN:
		musbfsh_early_suspend();
		break;
		
	default:
		break;
	}
	
	//FUNC_EXIT(FUNC_LV_MODULE);
	
	return 0;
}

static struct notifier_block _musb_notifier = { 
	.notifier_call  = _musb_notifier_callback,
}; 

static u64 mt_usb11_dmamask = DMA_BIT_MASK(32);

static int __init mt_usb11_probe(struct platform_device *pdev)
{
	struct musbfsh_hdrc_platform_data *pdata = pdev->dev.platform_data;
	struct platform_device *musbfsh;
	struct mt_usb11_glue *glue;
	struct musbfsh_hdrc_config *config;
	int ret = -ENOMEM;
	int musbfshid;
	struct device_node *np = pdev->dev.of_node;
	struct device *dev = &pdev->dev;

	INFO("[Flow][USB11]%s:%d\n", __func__, __LINE__);
	glue = kzalloc(sizeof(*glue), GFP_KERNEL);
	if (!glue) {
		dev_err(&pdev->dev, "failed to allocate glue context\n");
		goto err0;
	}

	/* get the musbfsh id */
	musbfshid = musbfsh_get_id(&pdev->dev, GFP_KERNEL);
	if (musbfshid < 0) {
		dev_err(&pdev->dev, "failed to allocate musbfsh id\n");
		ret = -ENOMEM;
		goto err1;
	}

	musbfsh = platform_device_alloc("musbfsh-hdrc", musbfshid);
	if (!musbfsh) {
		dev_err(&pdev->dev, "failed to allocate musb device\n");
		goto err2;
	}
#ifdef CONFIG_OF
	usb11_dts_np = pdev->dev.of_node;
	INFO("[usb11] usb11_dts_np %p\n", usb11_dts_np);
	/*usb_irq_number1 = irq_of_parse_and_map(pdev->dev.of_node, 0);
	usb_mac = (unsigned long)of_iomap(pdev->dev.of_node, 0);
	usb_phy_base = (unsigned long)of_iomap(pdev->dev.of_node, 1);*/
	pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		ERR("failed to allocate musb platform data\n");
		goto err2;
	}

	config = devm_kzalloc(&pdev->dev, sizeof(*config), GFP_KERNEL);
	if (!config) {
		ERR("failed to allocate musb hdrc config\n");
		goto err2;
	}
	of_property_read_u32(np, "mode", (u32 *)&pdata->mode);

	/*of_property_read_u32(np, "dma_channels",    (u32 *)&config->dma_channels); */
	of_property_read_u32(np, "num_eps", (u32 *)&config->num_eps);
	config->multipoint = of_property_read_bool(np, "multipoint");
	/*
	config->dyn_fifo = of_property_read_bool(np, "dyn_fifo");
	config->soft_con = of_property_read_bool(np, "soft_con");
	config->dma = of_property_read_bool(np, "dma");
	*/

	pdata->config = config;
	INFO("[Flow][USB11]mode = %d ,num_eps = %d,multipoint = %d\n", pdata->mode,
	config->num_eps, config->multipoint);
#endif

	musbfsh->id = musbfshid;
	musbfsh->dev.parent = &pdev->dev;
	musbfsh->dev.dma_mask = &mt_usb11_dmamask;
	musbfsh->dev.coherent_dma_mask = mt_usb11_dmamask;
#ifdef CONFIG_OF
	pdev->dev.dma_mask = &mt_usb11_dmamask;
	pdev->dev.coherent_dma_mask = mt_usb11_dmamask;
#endif

	glue->dev = &pdev->dev;
	glue->musbfsh = musbfsh;

	pdata->platform_ops = &mt_usb11_ops;

	platform_set_drvdata(pdev, glue);

	ret = platform_device_add_resources(musbfsh, pdev->resource, pdev->num_resources);
	if (ret) {
		dev_err(&pdev->dev, "failed to add resources\n");
		goto err3;
	}

	ret = platform_device_add_data(musbfsh, pdata, sizeof(*pdata));
	if (ret) {
		dev_err(&pdev->dev, "failed to add platform_data\n");
		goto err3;
	}

	ret = platform_device_add(musbfsh);

	if (ret) {
		dev_err(&pdev->dev, "failed to register musbfsh device\n");
		goto err3;
	}
	
	if(fb_register_client(&_musb_notifier))
		printk("%s : register FB client failed !\n",__FUNCTION__);
	
	hub_get_vgp_supply(dev);
	pogo_hub_get_gpio(dev);
	
	if (0 == hub_vgp_supply_enable()) {
	    mdelay(20);
	}
	
	if (my_set_gpio(TYPE_VDD_5V_EN, 1)) {
		mdelay(20);
	}
	
    if (my_set_gpio(TYPE_HUB_RST, 1)) {
		mdelay(20);
	}
	
	if (my_set_gpio(TYPE_POGO_EN, 1)) {
		mdelay(20);
	}

	return 0;

err3:
	platform_device_put(musbfsh);

err2:
	musbfsh_put_id(&pdev->dev, musbfshid);

err1:
	kfree(glue);

err0:
	return ret;
}

static int __exit mt_usb_remove(struct platform_device *pdev)
{
	struct mt_usb11_glue *glue = platform_get_drvdata(pdev);
	
	fb_unregister_client(&_musb_notifier);
	
	musbfsh_put_id(&pdev->dev, glue->musbfsh->id);
	platform_device_del(glue->musbfsh);
	platform_device_put(glue->musbfsh);
	kfree(glue);

	return 0;
}

static int mt_usb11_suspend(struct device *dev)
{
	printk("%s:=====================>\n", __func__);
	
	if (my_set_gpio(TYPE_VDD_5V_EN, 0)) {
		mdelay(20);
	}

	return 0;
}

static int mt_usb11_resume(struct device *dev)
{
	printk("%s:=====================>\n", __func__);
	
	
	if (my_set_gpio(TYPE_VDD_5V_EN, 1)) {
		mdelay(20);
	}
	return 0;
}

static const struct dev_pm_ops mt_usb11_dev_pm_ops = {
	.suspend = mt_usb11_suspend,
	.resume = mt_usb11_resume,
};

#ifndef MT_USB11_DEV_PM_OPS
#define MT_USB11_DEV_PM_OPS (&mt_usb11_dev_pm_ops)
#else
#define	MT_USB11_DEV_PM_OPS	NULL
#endif

static struct platform_driver mt_usb11_driver = {
	.remove = __exit_p(mt_usb_remove),
	.probe = mt_usb11_probe,
	.driver = {
	.name = "mt_usb11",
#ifdef CONFIG_OF
	.of_match_table = apusb_of_ids,
#endif
	.pm = MT_USB11_DEV_PM_OPS,
	},
};

int usb11_init(void)
{
	INFO("[Flow][USB11]%s:%d\n", __func__, __LINE__);
	return platform_driver_register(&mt_usb11_driver);
}

/*mubsys_initcall(usb11_init);*/

void usb11_exit(void)
{
	platform_driver_unregister(&mt_usb11_driver);
}

/*module_exit(usb11_exit) */