/*
 * rt5647_ioctl.h  --  RT5647 ALSA SoC audio driver IO control
 *
 * Copyright 2012 Realtek Microelectronics
 * Author: Bard <bardliao@realtek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/spi/spi.h>
#include <sound/soc.h>
#include "rt_codec_ioctl.h"
#include "rt5647_ioctl.h"
#include "rt5647.h"

hweq_t hweq_param[] = {
	{/* NORMAL */
		{
			{0, 0},
		},
		0x0000,
	},
	{/* CLUB */
		{
			{0xa4, 0x1bbc},
			{0xa5, 0x0c73},
			{0xeb, 0x030b},
			{0xec, 0xff24},
			{0xed, 0x1ea6},
			{0xee, 0xe4d9},
			{0xe7, 0x1c98},
			{0xe8, 0x589d},
			{0xe9, 0x01bd},
			{0xea, 0xd344},
			{0xe5, 0x01ca},
			{0xe6, 0x2940},
			{0xae, 0xd8cb},
			{0xaf, 0x1bbc},
			{0xb0, 0x0000},
			{0xb4, 0xef01},
			{0xb5, 0x1bbc},
			{0xb6, 0x0000},
			{0xba, 0xef01},
			{0xbb, 0x1bbc},
			{0xbc, 0x0000},
			{0xc0, 0x0257},
			{0xc1, 0x0000},
			{0xc4, 0x1cc9},
			{0xc5, 0x02eb},
			{0xc6, 0x1cee},
			{0xca, 0x0800},
			{0xcc, 0x0800},
			{0xa6, 0x1bbc},
			{0xa7, 0x0c73},
			{0xf5, 0x030b},
			{0xf6, 0xff24},
			{0xf7, 0x1ea6},
			{0xf8, 0xe4d9},
			{0xf1, 0x1c98},
			{0xf2, 0x589d},
			{0xf3, 0x01bd},
			{0xf4, 0xd344},
			{0xef, 0x01ca},
			{0xf0, 0x2940},
			{0xb1, 0xd8cb},
			{0xb2, 0x1bbc},
			{0xb3, 0x0000},
			{0xb7, 0xef01},
			{0xb8, 0x1bbc},
			{0xb9, 0x0000},
			{0xbd, 0xef01},
			{0xbe, 0x1bbc},
			{0xbf, 0x0000},
			{0xc2, 0x0257},
			{0xc3, 0x0000},
			{0xc7, 0x1cc9},
			{0xc8, 0x02eb},
			{0xc9, 0x1cee},
			{0xcb, 0x0800},
			{0xcd, 0x0800},
		},
		0x0082,
	},
	{/* SPK */
		{
			{0, 0},
		},
		0x0000,
	},
	{/* HP */
		{
			{0, 0},
		},
		0x0000,
	},
};
#define RT5647_HWEQ_LEN ARRAY_SIZE(hweq_param)

int rt5647_update_eqmode(
	struct snd_soc_codec *codec, int channel, int mode)
{
	struct rt_codec_ops *ioctl_ops = rt_codec_get_ioctl_ops();
	int i, upd_reg, reg, mask;

	if (codec == NULL ||  mode >= RT5647_HWEQ_LEN)
		return -EINVAL;

	dev_dbg(codec->dev, "%s(): mode=%d\n", __func__, mode);
	if (mode != NORMAL) {
		for(i = 0; i < EQ_REG_NUM; i++) {
			if(hweq_param[mode].par[i].reg)
				ioctl_ops->index_write(codec,
					hweq_param[mode].par[i].reg,
					hweq_param[mode].par[i].val);
			else
				break;
		}
	}
	switch (channel) {
	case EQ_CH_DAC:
		reg = RT5647_EQ_CTRL2;
		mask = 0x33fe;
		upd_reg = RT5647_EQ_CTRL1;
		break;
	case EQ_CH_ADC:
		reg = RT5647_ADC_EQ_CTRL2;
		mask = 0x01bf;
		upd_reg = RT5647_ADC_EQ_CTRL1;
		break;
	default:
		printk("Invalid EQ channel\n");
		return -EINVAL;
	}
	snd_soc_update_bits(codec, reg, mask, hweq_param[mode].ctrl);
	snd_soc_update_bits(codec, upd_reg,
		RT5647_EQ_UPD, RT5647_EQ_UPD);
	snd_soc_update_bits(codec, upd_reg, RT5647_EQ_UPD, 0);

	return 0;
}

int rt5647_ioctl_common(struct snd_hwdep *hw, struct file *file,
			unsigned int cmd, unsigned long arg)
{
	struct snd_soc_codec *codec = hw->private_data;
	struct rt_codec_cmd __user *_rt_codec = (struct rt_codec_cmd *)arg;
	struct rt_codec_cmd rt_codec;
	//struct rt_codec_ops *ioctl_ops = rt_codec_get_ioctl_ops();
	int *buf;
	static int eq_mode[EQ_CH_NUM];

	if (copy_from_user(&rt_codec, _rt_codec, sizeof(rt_codec))) {
		dev_err(codec->dev,"copy_from_user faild\n");
		return -EFAULT;
	}
	dev_dbg(codec->dev, "%s(): rt_codec.number=%d, cmd=%d\n",
			__func__, rt_codec.number, cmd);
	buf = kmalloc(sizeof(*buf) * rt_codec.number, GFP_KERNEL);
	if (buf == NULL)
		return -ENOMEM;
	if (copy_from_user(buf, rt_codec.buf, sizeof(*buf) * rt_codec.number)) {
		goto err;
	}
	
	switch (cmd) {
	case RT_SET_CODEC_HWEQ_IOCTL:
		if (eq_mode == *buf)
			break;
		eq_mode[*buf] = *(buf + 1);
		rt5647_update_eqmode(codec, eq_mode[*buf], *buf);
		break;

	case RT_GET_CODEC_ID:
		*buf = snd_soc_read(codec, RT5647_VENDOR_ID2);
		if (copy_to_user(rt_codec.buf, buf, sizeof(*buf) * rt_codec.number))
			goto err;
		break;
	default:
		break;
	}

	kfree(buf);
	return 0;

err:
	kfree(buf);
	return -EFAULT;
}
EXPORT_SYMBOL_GPL(rt5647_ioctl_common);
