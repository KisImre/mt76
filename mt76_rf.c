/*
 * Copyright (C) 2017 Kis Imre Geza <k.iminet@gmail.com>
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

#include "mt76_rf.h"

#define CSR_RF_CFG  (0x0500)
#define RF_START_OP (0x00020000)
#define RF_BUSY     (0x00020000)
#define RF_READ     (0x00000000)
#define RF_WRITE    (0x00010000)
#define RF_ADDR(a)  (((a) & 0x3F) << 8)
#define RF_DATA(d)  ((d) & 0xFF)

static void mt76_rf_busyWait(struct mt76_dev *dev) {
    u8 i;
    u32 raw;

    for (i = 0; i < 100; i++) {
        raw = dev->bus->rr(dev, CSR_RF_CFG);
        if ((raw & RF_BUSY) == 0) {
            return;
        }

        udelay(100);
    }
    
    printk("rf_read timeout\n");
}

void mt76_rf_read(struct mt76_dev *dev, u8 address, u8* data) {
    u32 raw;

    printk("rf_read: %d\n", address);
    raw = RF_START_OP | RF_READ | RF_ADDR(address) | RF_DATA(0);
    dev->bus->wr(dev, CSR_RF_CFG, raw);
    mt76_rf_busyWait(dev);
    printk("rf_read result: %d = %02X\n", address, *data);
}

void mt76_rf_write(struct mt76_dev *dev, u8 address, u8 data) {
    u32 raw;
 
    printk("rf_write: %d\n", address);
    raw = RF_START_OP | RF_WRITE | RF_ADDR(address) | RF_DATA(data);
    dev->bus->wr(dev, CSR_RF_CFG, raw);
    mt76_rf_busyWait(dev);
    printk("rf_write finished");
}