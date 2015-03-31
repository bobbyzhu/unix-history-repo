/*-
 * Copyright 2013-2015 John Wehle <john@feith.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#ifndef	_ARM_AMLOGIC_AML8726_MMC_H
#define	_ARM_AMLOGIC_AML8726_MMC_H


#define	AML_MMC_ALIGN_DMA			4
#define	AML_MMC_MAX_DMA				4096

/*
 * Timeouts are in milliseconds
 *
 * Read and write are per section 4.6.2 of the:
 *
 *   SD Specifications Part 1
 *   Physicaly Layer Simplified Specification
 *   Version 4.10
 */
#define	AML_MMC_CMD_TIMEOUT			50
#define	AML_MMC_READ_TIMEOUT			100
#define	AML_MMC_WRITE_TIMEOUT			500
#define	AML_MMC_MAX_TIMEOUT			5000

/*
 * Internally the timeout is implemented by counting clock cycles.
 *
 * Since the hardware implements timeouts by counting cycles
 * the minimum read / write timeout (assuming the minimum
 * conversion factor of 1 cycle per usec) is:
 *
 *   (8 bits * 512 bytes per block + 16 bits CRC) = 4112 usec
 */
#if ((AML_MMC_READ_TIMEOUT * 1000) < 4112 ||	\
    (AML_MMC_WRITE_TIMEOUT * 1000) < 4112)
#error "Single block timeout is smaller than supported"
#endif

#define	AML_MMC_CMD_ARGUMENT_REG		0

#define	AML_MMC_CMD_SEND_REG			4
#define	AML_MMC_CMD_REP_PKG_CNT_MASK		(0xffU << 24)
#define	AML_MMC_CMD_REP_PKG_CNT_SHIFT		24
#define	AML_MMC_CMD_CMD_HAS_DATA		(1 << 20)
#define	AML_MMC_CMD_CHECK_DAT0_BUSY		(1 << 19)
#define	AML_MMC_CMD_RESP_CRC7_FROM_8		(1 << 18)
#define	AML_MMC_CMD_RESP_HAS_DATA		(1 << 17)
#define	AML_MMC_CMD_RESP_NO_CRC7		(1 << 16)
#define	AML_MMC_CMD_RESP_BITS_MASK		(0xff << 8)
#define	AML_MMC_CMD_RESP_BITS_SHIFT		8
#define	AML_MMC_CMD_START_BIT			(0 << 7)
#define	AML_MMC_CMD_TRANS_BIT_HOST		(1 << 6)
#define	AML_MMC_CMD_INDEX_MASK			0x3f
#define	AML_MMC_CMD_INDEX_SHIFT			0

#define	AML_MMC_CONFIG_REG			8
#define	AML_MMC_CONFIG_WR_CRC_STAT_MASK		(7U << 29)
#define	AML_MMC_CONFIG_WR_CRC_STAT_SHIFT	29
#define	AML_MMC_CONFIG_WR_DELAY_MASK		(0x3f << 23)
#define	AML_MMC_CONFIG_WR_DELAY_SHIFT		23
#define	AML_MMC_CONFIG_DMA_ENDIAN_MASK		(3 << 21)
#define	AML_MMC_CONFIG_DMA_ENDIAN_NC		(0 << 21)
#define	AML_MMC_CONFIG_DMA_ENDIAN_SB		(1 << 21)
#define	AML_MMC_CONFIG_DMA_ENDIAN_SW		(2 << 21)
#define	AML_MMC_CONFIG_DMA_ENDIAN_SBW		(3 << 21)
#define	AML_MMC_CONFIG_BUS_WIDTH_MASK		(1 << 20)
#define	AML_MMC_CONFIG_BUS_WIDTH_1		(0 << 20)
#define	AML_MMC_CONFIG_BUS_WIDTH_4		(1 << 20)
#define	AML_MMC_CONFIG_DATA_NEG_EDGE		(1 << 19)
#define	AML_MMC_CONFIG_DONT_DELAY_DATA		(1 << 18)
#define	AML_MMC_CONFIG_CMD_ARG_BITS_MASK	(0x3f << 12)
#define	AML_MMC_CONFIG_CMD_ARG_BITS_SHIFT	12
#define	AML_MMC_CONFIG_CMD_POS_EDGE		(1 << 11)
#define	AML_MMC_CONFIG_CMD_NO_CRC		(1 << 10)
#define	AML_MMC_CONFIG_CMD_CLK_DIV_MASK		0x3ff
#define	AML_MMC_CONFIG_CMD_CLK_DIV_SHIFT	0

#define	AML_MMC_IRQ_STATUS_REG			12
#define	AML_MMC_IRQ_STATUS_TIMER_CNT_MASK	(0x1fffU << 19)
#define	AML_MMC_IRQ_STATUS_TIMER_CNT_SHIFT	19
#define	AML_MMC_IRQ_STATUS_TIMER_EN		(1 << 18)
#define	AML_MMC_IRQ_STATUS_TIMEOUT_IRQ		(1 << 16)
#define	AML_MMC_IRQ_STATUS_CMD_DONE_IRQ		(1 << 9)
#define	AML_MMC_IRQ_STATUS_WR_CRC16_OK		(1 << 7)
#define	AML_MMC_IRQ_STATUS_RD_CRC16_OK		(1 << 6)
#define	AML_MMC_IRQ_STATUS_RESP_CRC7_OK		(1 << 5)
#define	AML_MMC_IRQ_STATUS_CMD_BUSY		(1 << 4)
#define	AML_MMC_IRQ_STATUS_CLEAR_IRQ		0x10700

#define	AML_MMC_IRQ_CONFIG_REG			16
#define	AML_MMC_IRQ_CONFIG_SOFT_RESET		(1 << 15)
#define	AML_MMC_IRQ_CONFIG_CMD_DONE_EN		(1 << 4)

#define	AML_MMC_MULT_CONFIG_REG			20
#define	AML_MMC_MULT_CONFIG_RESP_INDEX_MASK	(0xf << 12)
#define	AML_MMC_MULT_CONFIG_RESP_INDEX_SHIFT	12
#define	AML_MMC_MULT_CONFIG_RESP_READOUT_EN	(1 << 8)
#define	AML_MMC_MULT_CONFIG_STREAM_8_MODE	(1 << 5)
#define	AML_MMC_MULT_CONFIG_STREAM_EN		(1 << 4)
#define	AML_MMC_MULT_CONFIG_PORT_MASK		3
#define	AML_MMC_MULT_CONFIG_PORT_A		0
#define	AML_MMC_MULT_CONFIG_PORT_B		1
#define	AML_MMC_MULT_CONFIG_PORT_C		2

#define	AML_MMC_DMA_ADDR_REG			24

#define	AML_MMC_EXTENSION_REG			28
#define	AML_MMC_EXTENSION_NO_CRC16		(1 << 30)
#define	AML_MMC_EXTENSION_PKT_SIZE_MASK		(0x3fff << 16)
#define	AML_MMC_EXTENSION_PKT_SIZE_SHIFT	16

#endif /* _ARM_AMLOGIC_AML8726_MMC_H */
