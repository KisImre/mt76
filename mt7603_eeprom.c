/*
 * Copyright (C) 2016 Felix Fietkau <nbd@nbd.name>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "mt7603.h"
#include "mt7603_eeprom.h"

static int
mt7603_efuse_read(struct mt7603_dev *dev, u32 base, u16 addr, u8 *data)
{
	u32 val;
	int i;

	val = mt76_rr(dev, base + MT_EFUSE_CTRL);
	val &= ~(MT_EFUSE_CTRL_AIN |
		 MT_EFUSE_CTRL_MODE);
	val |= FIELD_PREP(MT_EFUSE_CTRL_AIN, addr & ~0xf);
	val |= MT_EFUSE_CTRL_KICK;
	mt76_wr(dev, base + MT_EFUSE_CTRL, val);

	if (!mt76_poll(dev, base + MT_EFUSE_CTRL, MT_EFUSE_CTRL_KICK, 0, 1000))
		return -ETIMEDOUT;

	udelay(2);

	val = mt76_rr(dev, base + MT_EFUSE_CTRL);
	if ((val & MT_EFUSE_CTRL_AOUT) == MT_EFUSE_CTRL_AOUT ||
	    WARN_ON_ONCE(!(val & MT_EFUSE_CTRL_VALID))) {
		memset(data, 0xff, 16);
		return 0;
	}

	for (i = 0; i < 4; i++) {
	    val = mt76_rr(dev, base + MT_EFUSE_RDATA(i));
	    put_unaligned_le32(val, data + 4 * i);
	}

	return 0;
}

static int
mt7603_efuse_init(struct mt7603_dev *dev)
{
	u32 base = mt7603_reg_map(dev, MT_EFUSE_BASE);
	int len = MT7603_EEPROM_SIZE;
	void *buf;
	int ret, i;

	if (mt76_rr(dev, base + MT_EFUSE_BASE_CTRL) & MT_EFUSE_BASE_CTRL_EMPTY)
		return 0;

	dev->mt76.otp.data = devm_kzalloc(dev->mt76.dev, len, GFP_KERNEL);
	dev->mt76.otp.size = len;
	if (!dev->mt76.otp.data)
		return -ENOMEM;

	buf = dev->mt76.otp.data;
	for (i = 0; i + 16 <= len; i += 16) {
		ret = mt7603_efuse_read(dev, base, i, buf + i);
		if (ret)
			return ret;
	}

	return 0;
}

static bool
mt7603_has_cal_free_data(struct mt7603_dev *dev, u8 *efuse)
{
	if (!efuse[MT_EE_TEMP_SENSOR_CAL])
		return false;

	if (get_unaligned_le16(efuse + MT_EE_TX_POWER_0_START_2G) == 0)
		return false;

	if (get_unaligned_le16(efuse + MT_EE_TX_POWER_1_START_2G) == 0)
		return false;

	if (!efuse[MT_EE_CP_FT_VERSION])
		return false;

	if (!efuse[MT_EE_XTAL_FREQ_OFFSET])
		return false;

	if (!efuse[MT_EE_XTAL_WF_RFCAL])
		return false;

	return true;
}


static void
mt7603_apply_cal_free_data(struct mt7603_dev *dev, u8 *efuse)
{
    static const u8 cal_free_bytes[] = {
    };

    static const u8 magicEeprom[] = {
        0x28, 0x76, 0x00, 0x02, 0x40, 0xA3, 0x6B, 0xC0, 0xE8, 0xD3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x40, 0xA3, 0x6B, 0xC0, 0xE8, 0xD5, 0x40, 0xA3,
        0x6B, 0xC0, 0xE8, 0xD4, 0x22, 0x34, 0x00, 0x20, 0xFF, 0xFF, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x82, 0x00, 0x00, 0x94, 0x40, 0xB4, 0xC0, 0xCA, 0x1E, 0xC0, 0xC0, 0xC2, 0x40, 0xCA, 0x1E, 0x9E,
        0x9E, 0x9E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xC6, 0xC6, 0xC4, 0xC4, 0xC4, 0xC0, 0xC0, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC0, 0xC0, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A, 0x22, 0x2A, 0x31, 0x35, 0x01, 0x35,
        0x39, 0x40, 0x46, 0x4D, 0x7F, 0x7F, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x11, 0x1D, 0x11, 0x1D, 0x1C, 0x35, 0x1C, 0x35, 0x1E, 0x35, 0x1E, 0x35, 0x17, 0x19, 0x17, 0x19,
        0x02, 0x00, 0x00, 0x00, 0x90, 0x00, 0x80, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x77, 0x00,
        0x11, 0x1D, 0x11, 0x1D, 0x15, 0x7F, 0x15, 0x7F, 0x17, 0x7F, 0x17, 0x7F, 0x10, 0x3B, 0x10, 0x3B,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    };

	u8 *eeprom = dev->mt76.eeprom.data;

	int n = ARRAY_SIZE(cal_free_bytes);
	int i;

	if (!mt7603_has_cal_free_data(dev, efuse)) {
	    return;
    }

	if (is_mt7628(dev)) {
		n -= 2;
    }

    for (i = 0; i < 512; i++) {
        eeprom[i] = magicEeprom[i];
    }

	for (i = 0; i < n; i++) {
	    int offset = cal_free_bytes[i];
	    eeprom[offset] = efuse[offset];
	}
}


static int
mt7603_eeprom_load(struct mt7603_dev *dev)
{
	int ret;

	ret = mt76_eeprom_init(&dev->mt76, MT7603_EEPROM_SIZE);
	if (ret < 0)
		return ret;

	return mt7603_efuse_init(dev);
}

static int mt7603_check_eeprom(struct mt76_dev *dev)
{
	u16 val = get_unaligned_le16(dev->eeprom.data);

	switch (val) {
	case 0x7628:
	case 0x7603:
		return 0;
	default:
		return -EINVAL;
	}
}


int mt7603_eeprom_init(struct mt7603_dev *dev)
{
	int ret;

	ret = mt7603_eeprom_load(dev);
	if (ret < 0) {
		return ret;
    }

	if (mt7603_check_eeprom(&dev->mt76) == 0) {
		mt7603_apply_cal_free_data(dev, dev->mt76.otp.data);
    } else {
		memcpy(dev->mt76.eeprom.data, dev->mt76.otp.data,
		       MT7603_EEPROM_SIZE);
    }

	dev->mt76.cap.has_2ghz = true;
	memcpy(dev->mt76.macaddr, dev->mt76.eeprom.data + MT_EE_MAC_ADDR,
	       ETH_ALEN);

	mt76_eeprom_override(&dev->mt76);

	return 0;
}
