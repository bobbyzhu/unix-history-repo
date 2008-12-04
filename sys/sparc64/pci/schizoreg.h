/*-
 * Copyright (c) 2002 Jason L. Wright (jason@thought.net)
 * Copyright (c) 2005 by Marius Strobl <marius@FreeBSD.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULLAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *	from: OpenBSD: schizoreg.h,v 1.8 2005/05/19 18:28:59 mickey Exp
 * $FreeBSD$
 */

#ifndef _SPARC64_PCI_SCHIZOREG_H_
#define	_SPARC64_PCI_SCHIZOREG_H_

#define	STX_NINTR			5	/* 4 via OFW + 1 CDMA */
#define	STX_NRANGE			4
#define	SCZ_NREG			3
#define	TOM_NREG			4

#define	STX_PCI				0
#define	STX_CTRL			1
#define	STX_PCICFG			2
#define	STX_ICON			3

/* PCI configuration and status registers */
#define	STX_PCI_IOMMU			0x00200
#define	STX_PCI_IOMMU_CTXFLUSH		0x00218
#define	STX_PCI_IMAP_BASE		0x01000
#define	STX_PCI_ICLR_BASE		0x01400
#define	STX_PCI_INTR_RETRY_TIM		0x01a00
#define	SCZ_PCI_DMA_SYNC		0x01a08
#define	TOM_PCI_DMA_SYNC_COMP		0x01a10
#define	TOMXMS_PCI_DMA_SYNC_PEND	0x01a18
#define	STX_PCI_CTRL			0x02000
#define	STX_PCI_AFSR			0x02010
#define	STX_PCI_AFAR			0x02018
#define	STX_PCI_DIAG			0x02020
#define	TOM_PCI_IOC_CSR			0x02248
#define	TOM_PCI_IOC_TAG			0x02290
#define	TOM_PCI_IOC_DATA		0x02290
#define	STX_PCI_STRBUF			0x02800
#define	STX_PCI_STRBUF_CTXFLUSH		0x02818
#define	STX_PCI_IOMMU_SVADIAG		0x0a400
#define	STX_PCI_IOMMU_TLB_CMP_DIAG	0x0a408
#define	STX_PCI_IOMMU_QUEUE_DIAG	0x0a500
#define	STX_PCI_IOMMU_TLB_TAG_DIAG	0x0a580
#define	STX_PCI_IOMMU_TLB_DATA_DIAG	0x0a600
#define	STX_PCI_IOBIO_DIAG		0x0a808
#define	STX_PCI_STRBUF_CTXMATCH		0x10000

/* PCI IOMMU control registers */
#define	TOM_PCI_IOMMU_ERR_BAD_VA	0x0000000010000000ULL
#define	TOM_PCI_IOMMU_ERR_ILLTSBTBW	0x0000000008000000ULL
#define	TOM_PCI_IOMMU_ECC_ERR		0x0000000006000000ULL
#define	TOM_PCI_IOMMU_TIMEOUT_ERR	0x0000000004000000ULL
#define	TOM_PCI_IOMMU_INVALID_ERR	0x0000000002000000ULL
#define	TOM_PCI_IOMMU_PROTECTION_ERR	0x0000000000000000ULL
#define	TOM_PCI_IOMMU_ERRMASK						\
	(TOM_PCI_IOMMU_PROTECTION_ERR | TOM_PCI_IOMMU_INVALID_ERR |	\
	TOM_PCI_IOMMU_TIMEOUT_ERR | TOM_PCI_IOMMU_ECC_ERR)
#define	TOM_PCI_IOMMU_ERR		0x0000000001000000ULL

/* PCI control/status register */
#define	SCZ_PCI_CTRL_BUS_UNUS		0x8000000000000000ULL
#define	TOM_PCI_CTRL_DTO_ERR		0x4000000000000000ULL
#define	TOM_PCI_CTRL_DTO_IEN		0x2000000000000000ULL
#define	SCZ_PCI_CTRL_ESLCK		0x0008000000000000ULL
#define	SCZ_PCI_CTRL_ERRSLOT		0x0007000000000000ULL
#define	STX_PCI_CTRL_TTO_ERR		0x0000004000000000ULL
#define	STX_PCI_CTRL_RTRY_ERR		0x0000002000000000ULL
#define	STX_PCI_CTRL_MMU_ERR		0x0000001000000000ULL
#define	SCZ_PCI_CTRL_SBH_ERR		0x0000000800000000ULL
#define	STX_PCI_CTRL_SERR		0x0000000400000000ULL
#define	SCZ_PCI_CTRL_PCISPD		0x0000000200000000ULL
#define	TOM_PCI_CTRL_PRM		0x0000000040000000ULL
#define	TOM_PCI_CTRL_PRO		0x0000000020000000ULL
#define	TOM_PCI_CTRL_PRL		0x0000000010000000ULL
#define	STX_PCI_CTRL_PTO		0x0000000003000000ULL
#define	STX_PCI_CTRL_MMU_IEN		0x0000000000080000ULL
#define	STX_PCI_CTRL_SBH_IEN		0x0000000000040000ULL
#define	STX_PCI_CTRL_ERR_IEN		0x0000000000020000ULL
#define	STX_PCI_CTRL_ARB_PARK		0x0000000000010000ULL
#define	SCZ_PCI_CTRL_PCIRST		0x0000000000000100ULL
#define	STX_PCI_CTRL_ARB_MASK		0x00000000000000ffULL

/* PCI asynchronous fault status register */
#define	STX_PCI_AFSR_P_MA		0x8000000000000000ULL
#define	STX_PCI_AFSR_P_TA		0x4000000000000000ULL
#define	STX_PCI_AFSR_P_RTRY		0x2000000000000000ULL
#define	STX_PCI_AFSR_P_PERR		0x1000000000000000ULL
#define	STX_PCI_AFSR_P_TTO		0x0800000000000000ULL
#define	STX_PCI_AFSR_P_UNUS		0x0400000000000000ULL
#define	STX_PCI_AFSR_S_MA		0x0200000000000000ULL
#define	STX_PCI_AFSR_S_TA		0x0100000000000000ULL
#define	STX_PCI_AFSR_S_RTRY		0x0080000000000000ULL
#define	STX_PCI_AFSR_S_PERR		0x0040000000000000ULL
#define	STX_PCI_AFSR_S_TTO		0x0020000000000000ULL
#define	STX_PCI_AFSR_S_UNUS		0x0010000000000000ULL
#define	STX_PCI_AFSR_DWMASK		0x0000030000000000ULL
#define	STX_PCI_AFSR_BMASK		0x000000ff00000000ULL
#define	STX_PCI_AFSR_BLK		0x0000000080000000ULL
#define	STX_PCI_AFSR_CFG		0x0000000040000000ULL
#define	STX_PCI_AFSR_MEM		0x0000000020000000ULL
#define	STX_PCI_AFSR_IO			0x0000000010000000ULL

/* PCI diagnostic register */
#define	SCZ_PCI_DIAG_BADECC_DIS		0x0000000000000400ULL
#define	STX_PCI_DIAG_BYPASS_DIS		0x0000000000000200ULL
#define	STX_PCI_DIAG_TTO_DIS		0x0000000000000100ULL
#define	SCZ_PCI_DIAG_RTRYARB_DIS	0x0000000000000080ULL
#define	STX_PCI_DIAG_RETRY_DIS		0x0000000000000040ULL
#define	STX_PCI_DIAG_INTRSYNC_DIS	0x0000000000000020ULL
#define	STX_PCI_DIAG_DMAPARITY_INV	0x0000000000000008ULL
#define	STX_PCI_DIAG_PIODPARITY_INV	0x0000000000000004ULL
#define	STX_PCI_DIAG_PIOAPARITY_INV	0x0000000000000002ULL

/* Tomatillo I/O cache register */
#define	TOM_PCI_IOC_PW			0x0000000000080000ULL
#define	TOM_PCI_IOC_PRM			0x0000000000040000ULL
#define	TOM_PCI_IOC_PRO			0x0000000000020000ULL
#define	TOM_PCI_IOC_PRL			0x0000000000010000ULL
#define	TOM_PCI_IOC_PRM_LEN		0x000000000000c000ULL
#define	TOM_PCI_IOC_PRM_LEN_SHIFT	14
#define	TOM_PCI_IOC_PRO_LEN		0x0000000000003000ULL
#define	TOM_PCI_IOC_PRO_LEN_SHIFT	12
#define	TOM_PCI_IOC_PRL_LEN		0x0000000000000c00ULL
#define	TOM_PCI_IOC_PRL_LEN_SHIFT	10
#define	TOM_PCI_IOC_PREF_OFF		0x0000000000000038ULL
#define	TOM_PCI_IOC_PREF_OFF_SHIFT	3
#define	TOM_PCI_IOC_CPRM		0x0000000000000004ULL
#define	TOM_PCI_IOC_CPRO		0x0000000000000002ULL
#define	TOM_PCI_IOC_CPRL		0x0000000000000001ULL

/* Controller configuration and status registers */
/* Note that these are shared on Schizo but per-PBM on Tomatillo. */
#define	STX_CTRL_BUS_ERRLOG		0x00018
#define	STX_CTRL_ECCCTRL		0x00020
#define	STX_CTRL_UE_AFSR		0x00030
#define	STX_CTRL_UE_AFAR		0x00038
#define	STX_CTRL_CE_AFSR		0x00040
#define	STX_CTRL_CE_AFAR		0x00048
#define	STX_CTRL_PERF			0x07000
#define	STX_CTRL_PERF_CNT		0x07008

/* Safari/JBus error log register */
#define	STX_CTRL_BUS_ERRLOG_BADCMD	0x4000000000000000ULL
#define	SCZ_CTRL_BUS_ERRLOG_SSMDIS	0x2000000000000000ULL
#define	SCZ_CTRL_BUS_ERRLOG_BADMA	0x1000000000000000ULL
#define	SCZ_CTRL_BUS_ERRLOG_BADMB	0x0800000000000000ULL
#define	SCZ_CTRL_BUS_ERRLOG_BADMC	0x0400000000000000ULL
#define	TOM_CTRL_BUS_ERRLOG_SNOOP_GR	0x0000000000200000ULL
#define	TOM_CTRL_BUS_ERRLOG_SNOOP_PCI	0x0000000000100000ULL
#define	TOM_CTRL_BUS_ERRLOG_SNOOP_RD	0x0000000000080000ULL
#define	TOM_CTRL_BUS_ERRLOG_SNOOP_RDS	0x0000000000020000ULL
#define	TOM_CTRL_BUS_ERRLOG_SNOOP_RDSA	0x0000000000010000ULL
#define	TOM_CTRL_BUS_ERRLOG_SNOOP_OWN	0x0000000000008000ULL
#define	TOM_CTRL_BUS_ERRLOG_SNOOP_RDO	0x0000000000004000ULL
#define	SCZ_CTRL_BUS_ERRLOG_CPU1PS	0x0000000000002000ULL
#define	TOM_CTRL_BUS_ERRLOG_WDATA_PERR	0x0000000000002000ULL
#define	SCZ_CTRL_BUS_ERRLOG_CPU1PB	0x0000000000001000ULL
#define	TOM_CTRL_BUS_ERRLOG_CTRL_PERR	0x0000000000001000ULL
#define	SCZ_CTRL_BUS_ERRLOG_CPU0PS	0x0000000000000800ULL
#define	TOM_CTRL_BUS_ERRLOG_SNOOP_ERR	0x0000000000000800ULL
#define	SCZ_CTRL_BUS_ERRLOG_CPU0PB	0x0000000000000400ULL
#define	TOM_CTRL_BUS_ERRLOG_JBUS_ILL_B	0x0000000000000400ULL
#define	SCZ_CTRL_BUS_ERRLOG_CIQTO	0x0000000000000200ULL
#define	SCZ_CTRL_BUS_ERRLOG_LPQTO	0x0000000000000100ULL
#define	TOM_CTRL_BUS_ERRLOG_JBUS_ILL_C	0x0000000000000100ULL
#define	SCZ_CTRL_BUS_ERRLOG_SFPQTO	0x0000000000000080ULL
#define	SCZ_CTRL_BUS_ERRLOG_UFPQTO	0x0000000000000040ULL
#define	TOM_CTRL_BUS_ERRLOG_RD_PERR	0x0000000000000040ULL
#define	STX_CTRL_BUS_ERRLOG_APERR	0x0000000000000020ULL
#define	STX_CTRL_BUS_ERRLOG_UNMAP	0x0000000000000010ULL
#define	STX_CTRL_BUS_ERRLOG_BUSERR	0x0000000000000004ULL
#define	STX_CTRL_BUS_ERRLOG_TIMEOUT	0x0000000000000002ULL
#define	SCZ_CTRL_BUS_ERRLOG_ILL		0x0000000000000001ULL

/* ECC control register */
#define	STX_CTRL_ECCCTRL_EE		0x8000000000000000ULL
#define	STX_CTRL_ECCCTRL_UE		0x4000000000000000ULL
#define	STX_CTRL_ECCCTRL_CE		0x2000000000000000ULL

/* Uncorrectable error asynchronous fault status register */
#define	STX_CTRL_UE_AFSR_P_PIO		0x8000000000000000ULL
#define	STX_CTRL_UE_AFSR_P_DRD		0x4000000000000000ULL
#define	STX_CTRL_UE_AFSR_P_DWR		0x2000000000000000ULL
#define	STX_CTRL_UE_AFSR_S_PIO		0x1000000000000000ULL
#define	STX_CTRL_UE_AFSR_S_DRD		0x0800000000000000ULL
#define	STX_CTRL_UE_AFSR_S_DWR		0x0400000000000000ULL
#define	STX_CTRL_UE_AFSR_ERRPNDG	0x0300000000000000ULL
#define	STX_CTRL_UE_AFSR_BMASK		0x000003ff00000000ULL
#define	STX_CTRL_UE_AFSR_QOFF		0x00000000c0000000ULL
#define	STX_CTRL_UE_AFSR_AID		0x000000001f000000ULL
#define	STX_CTRL_UE_AFSR_PARTIAL	0x0000000000800000ULL
#define	STX_CTRL_UE_AFSR_OWNEDIN	0x0000000000400000ULL
#define	STX_CTRL_UE_AFSR_MTAGSYND	0x00000000000f0000ULL
#define	STX_CTRL_UE_AFSR_MTAG		0x000000000000e000ULL
#define	STX_CTRL_UE_AFSR_ECCSYND	0x00000000000001ffULL

/* Correctable error asynchronous fault status register */
#define	STX_CTRL_CE_AFSR_P_PIO		0x8000000000000000ULL
#define	STX_CTRL_CE_AFSR_P_DRD		0x4000000000000000ULL
#define	STX_CTRL_CE_AFSR_P_DWR		0x2000000000000000ULL
#define	STX_CTRL_CE_AFSR_S_PIO		0x1000000000000000ULL
#define	STX_CTRL_CE_AFSR_S_DRD		0x0800000000000000ULL
#define	STX_CTRL_CE_AFSR_S_DWR		0x0400000000000000ULL
#define	STX_CTRL_CE_AFSR_ERRPNDG	0x0300000000000000ULL
#define	STX_CTRL_CE_AFSR_BMASK		0x000003ff00000000ULL
#define	STX_CTRL_CE_AFSR_QOFF		0x00000000c0000000ULL
#define	STX_CTRL_CE_AFSR_AID		0x000000001f000000ULL
#define	STX_CTRL_CE_AFSR_PARTIAL	0x0000000000800000ULL
#define	STX_CTRL_CE_AFSR_OWNEDIN	0x0000000000400000ULL
#define	STX_CTRL_CE_AFSR_MTAGSYND	0x00000000000f0000ULL
#define	STX_CTRL_CE_AFSR_MTAG		0x000000000000e000ULL
#define	STX_CTRL_CE_AFSR_ECCSYND	0x00000000000001ffULL

/*
 * Safari/JBus performance control register
 * NB: for Tomatillo only events 0x00 through 0x08 are documented as
 * implemented.
 */
#define	SCZ_CTRL_PERF_ZDATA_OUT		0x0000000000000016ULL
#define	SCZ_CTRL_PERF_ZDATA_IN		0x0000000000000015ULL
#define	SCZ_CTRL_PERF_ORQFULL		0x0000000000000014ULL
#define	SCZ_CTRL_PERF_DVMA_WR		0x0000000000000013ULL
#define	SCZ_CTRL_PERF_DVMA_RD		0x0000000000000012ULL
#define	SCZ_CTRL_PERF_CYCPSESYS		0x0000000000000011ULL
#define	STX_CTRL_PERF_PCI_B		0x000000000000000fULL
#define	STX_CTRL_PERF_PCI_A		0x000000000000000eULL
#define	STX_CTRL_PERF_UPA		0x000000000000000dULL
#define	STX_CTRL_PERF_PIOINTRNL		0x000000000000000cULL
#define	TOM_CTRL_PERF_WRI_WRIS		0x000000000000000bULL
#define	STX_CTRL_PERF_INTRS		0x000000000000000aULL
#define	STX_CTRL_PERF_PRTLWRMRGBUF	0x0000000000000009ULL
#define	STX_CTRL_PERF_FGN_IO_HITS	0x0000000000000008ULL
#define	STX_CTRL_PERF_FGN_IO_TRNS	0x0000000000000007ULL
#define	STX_CTRL_PERF_OWN_CHRNT_HITS	0x0000000000000006ULL
#define	STX_CTRL_PERF_OWN_CHRNT_TRNS	0x0000000000000005ULL
#define	SCZ_CTRL_PERF_FGN_CHRNT_HITS	0x0000000000000004ULL
#define	STX_CTRL_PERF_FGN_CHRNT_TRNS	0x0000000000000003ULL
#define	STX_CTRL_PERF_CYCLES_PAUSE	0x0000000000000002ULL
#define	STX_CTRL_PERF_BUSCYC		0x0000000000000001ULL
#define	STX_CTRL_PERF_DIS		0x0000000000000000ULL
#define	STX_CTRL_PERF_CNT1_SHIFT	11
#define	STX_CTRL_PERF_CNT0_SHIFT	4

/* Safari/JBus performance counter register */
#define	STX_CTRL_PERF_CNT_MASK	0x00000000ffffffffULL
#define	STX_CTRL_PERF_CNT_CNT1_SHIFT	32
#define	STX_CTRL_PERF_CNT_CNT0_SHIFT	0

/* INO defines */
#define	STX_FB0_INO			0x2a	/* FB0 int. shared w/ UPA64s */
#define	STX_FB1_INO			0x2b	/* FB1 int. shared w/ UPA64s */
#define	STX_UE_INO			0x30	/* uncorrectable error */
#define	STX_CE_INO			0x31	/* correctable error */
#define	STX_PCIERR_A_INO		0x32	/* PCI bus A error */
#define	STX_PCIERR_B_INO		0x33	/* PCI bus B error */
#define	STX_BUS_INO			0x34	/* Safari/JBus error */
#define	STX_CDMA_A_INO			0x35	/* PCI bus A CDMA */
#define	STX_CDMA_B_INO			0x36	/* PCI bus B CDMA */
#define	STX_MAX_INO			0x37

/* Device space defines */
#define	STX_CONF_SIZE			0x1000000
#define	STX_CONF_BUS_SHIFT		16
#define	STX_CONF_DEV_SHIFT		11
#define	STX_CONF_FUNC_SHIFT		8
#define	STX_CONF_REG_SHIFT		0
#define	STX_IO_SIZE			0x1000000
#define	STX_MEM_SIZE			0x100000000

#define	STX_CONF_OFF(bus, slot, func, reg)				\
	(((bus) << STX_CONF_BUS_SHIFT) |				\
	((slot) << STX_CONF_DEV_SHIFT) |				\
	((func) << STX_CONF_FUNC_SHIFT) |				\
	((reg) << STX_CONF_REG_SHIFT))

/* Definitions for the Schizo/Tomatillo configuration space */
#define	STX_CS_DEVICE			0	/* bridge CS device number */
#define	STX_CS_FUNC			0	/* brdige CS function number */

/* Non-Standard registers in the configration space */
/*
 * NB: for Tomatillo the secondary and subordinate bus number registers
 * apparently are read-only although documented otherwise; writing to
 * them just triggers a PCI bus error interrupt or has no effect at best.
 */
#define	STX_CSR_SECBUS			0x40	/* secondary bus number */
#define	STX_CSR_SUBBUS			0x41	/* subordinate bus number */

/* Width of the physical addresses the IOMMU translates to */
#define	STX_IOMMU_BITS			43

#endif /* !_SPARC64_PCI_SCHIZOREG_H_ */
