/*	dz.c	3.1	%H%	*/

/*
 *  DZ-11 Driver
 */
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/tty.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/map.h"
#include "../h/pte.h"
#include "../h/uba.h"
#include "../h/conf.h"
#include "../h/pdma.h"
 
#define DZADDR  (UBA0_DEV + 0160100)
#ifdef ERNIE
#define NDZ 	(3*8)
#else
#define	NDZ	(8)
#endif
 
#define BITS7	020
#define BITS8	030
#define TWOSB	040
#define PENABLE	0100
#define OPAR	0200
#define MSE	040		/* Master Scan Enable */
#define RIE	0100		/* Receiver Interrupt Enable */
#define TIE	040000		/* Transmit interrupt enable */
#define DZ_IEN	(MSE+RIE+TIE)
#define PERROR	010000
#define FRERROR	020000
#define SSPEED	7		/* std speed = 300 baud */

 
#define	dzlpr	dzrbuf
#define dzmsr	dzbrk
#define ON	1
#define OFF	0
 
int	dzstart();
int	dzxint();
struct	tty dz_tty[NDZ];
int	dz_cnt = { NDZ };

struct device {
	short	dzcsr;
	short	dzrbuf;
	char	dztcr;
	char	dzdtr;
	char	dztbuf;
	char	dzbrk;
};

struct pdma dzpdma[] = {
	(struct device *)(DZADDR), NULL, NULL, (int)&dz_tty[0], dzxint,
	(struct device *)(DZADDR), NULL, NULL, (int)&dz_tty[1], dzxint,
	(struct device *)(DZADDR), NULL, NULL, (int)&dz_tty[2], dzxint,
	(struct device *)(DZADDR), NULL, NULL, (int)&dz_tty[3], dzxint,
	(struct device *)(DZADDR), NULL, NULL, (int)&dz_tty[4], dzxint,
	(struct device *)(DZADDR), NULL, NULL, (int)&dz_tty[5], dzxint,
	(struct device *)(DZADDR), NULL, NULL, (int)&dz_tty[6], dzxint,
	(struct device *)(DZADDR), NULL, NULL, (int)&dz_tty[7], dzxint,
#ifdef ERNIE
	(struct device *)(DZADDR+010), NULL, NULL, (int)&dz_tty[8], dzxint,
	(struct device *)(DZADDR+010), NULL, NULL, (int)&dz_tty[9], dzxint,
	(struct device *)(DZADDR+010), NULL, NULL, (int)&dz_tty[10], dzxint,
	(struct device *)(DZADDR+010), NULL, NULL, (int)&dz_tty[11], dzxint,
	(struct device *)(DZADDR+010), NULL, NULL, (int)&dz_tty[12], dzxint,
	(struct device *)(DZADDR+010), NULL, NULL, (int)&dz_tty[13], dzxint,
	(struct device *)(DZADDR+010), NULL, NULL, (int)&dz_tty[14], dzxint,
	(struct device *)(DZADDR+010), NULL, NULL, (int)&dz_tty[15], dzxint,
	(struct device *)(DZADDR+020), NULL, NULL, (int)&dz_tty[16], dzxint,
	(struct device *)(DZADDR+020), NULL, NULL, (int)&dz_tty[17], dzxint,
	(struct device *)(DZADDR+020), NULL, NULL, (int)&dz_tty[18], dzxint,
	(struct device *)(DZADDR+020), NULL, NULL, (int)&dz_tty[19], dzxint,
	(struct device *)(DZADDR+020), NULL, NULL, (int)&dz_tty[20], dzxint,
	(struct device *)(DZADDR+020), NULL, NULL, (int)&dz_tty[21], dzxint,
	(struct device *)(DZADDR+020), NULL, NULL, (int)&dz_tty[22], dzxint,
	(struct device *)(DZADDR+020), NULL, NULL, (int)&dz_tty[23], dzxint,
#endif
};
char	dz_timer;
char	dz_speeds[] = {
	0, 020 , 021 , 022 , 023 , 024 , 0, 025,
	026 , 027 , 030 , 032 , 034 , 036 , 0 , 0,
};
 
/*ARGSUSED*/
dzopen(d, flag)
{
	register struct tty *tp;
	register dev;
	extern dzscan();
 
	dev = minor(d);
	if (dev >= dz_cnt) {
		u.u_error = ENXIO;
		return;
	}
	if (dz_timer == 0) {
		dz_timer++;
		timeout(dzscan, (caddr_t)0, 60);
	}
	tp = &dz_tty[dev];
	tp->t_addr = (caddr_t)&dzpdma[dev];
	tp->t_oproc = dzstart;
	tp->t_iproc = NULL;
	tp->t_state |= WOPEN;
	if ((tp->t_state & ISOPEN) == 0) {
		ttychars(tp);
		tp->t_ospeed = tp->t_ispeed = SSPEED;
		tp->t_flags = ODDP|EVENP|ECHO;
		/*tp->t_state |= HUPCLS;*/
		dzparam(dev);
	} else if (tp->t_state&XCLUDE && u.u_uid != 0) {
		u.u_error = EBUSY;
		return;
	}
	dzmodem(dev, ON);
	VOID spl5();
	while ((tp->t_state & CARR_ON) == 0) {
		tp->t_state |= WOPEN;
		sleep((caddr_t)&tp->t_rawq, TTIPRI);
	}
	VOID spl0();
	(*linesw[tp->t_line].l_open)(d, tp);
}
 
dzclose(d)
{
	register struct tty *tp;
	register dev;
 
	dev = minor(d);
	tp = &dz_tty[dev];
	(*linesw[tp->t_line].l_close)(tp);
	if (tp->t_state & HUPCLS)
		dzmodem(dev, OFF);
	ttyclose(tp);
}
 
dzread(d)
{
	register struct tty *tp;
 
	tp = &dz_tty[minor(d)];
	(*linesw[tp->t_line].l_read)(tp);
}
 
dzwrite(d)
{
	register struct tty *tp;
 
	tp = &dz_tty[minor(d)];
	(*linesw[tp->t_line].l_write)(tp);
}
 
dzrint(dev)
{
	register struct tty *tp;
	register int c;
	register struct device *dzaddr;
 
	dzaddr = dzpdma[dev*8].p_addr;
	while ((c = dzaddr->dzrbuf) < 0) {	/* char present */
		tp = &dz_tty[((c>>8)&07)|(dev<<3)];
		if (tp >= &dz_tty[dz_cnt])
			continue;
		if ((tp->t_state & ISOPEN) == 0) {
			wakeup((caddr_t)&tp->t_rawq);
			continue;
		}
		if (c & FRERROR)
			/* framing error = break */
			if (tp->t_flags & RAW)
				c = 0;		/* null for getty */
			else
				c = 0177;	/* DEL = interrupt */
		if (c & PERROR)	
			/* parity error */
			if (((tp->t_flags & (EVENP|ODDP)) == EVENP)
			  || ((tp->t_flags & (EVENP|ODDP)) == ODDP))
				continue;
		(*linesw[tp->t_line].l_rint)(c, tp);
	}
}
 
/*ARGSUSED*/
dzioctl(dev, cmd, addr, flag)
caddr_t addr;
dev_t dev;
{
	register struct tty *tp;
 
	tp = &dz_tty[minor(dev)];
	if (ttioccomm(cmd, tp, addr, dev)) {
		if (cmd==TIOCSETP || cmd==TIOCSETN)
			dzparam(minor(dev));
	} else
		u.u_error = ENOTTY;
}
 
dzparam(dev)
{
	register struct tty *tp;
	register struct device *dzaddr;
	register short lpr;
 
	tp = &dz_tty[dev];
	dzaddr = dzpdma[dev].p_addr;
	dzaddr->dzcsr = DZ_IEN;
	if (tp->t_ispeed == 0) {
		dzmodem(dev, OFF);		/* hang up line */
		return;
	}
	lpr = (dz_speeds[tp->t_ispeed]<<8) | (dev & 07);
	if (tp->t_flags & RAW)
		lpr |= BITS8;
	else
		lpr |= (BITS7|PENABLE);
	if ((tp->t_flags & EVENP) == 0)
		lpr |= OPAR;
	if (tp->t_ispeed == 3)
		lpr |= TWOSB; 			/* 110 baud: 2 stop bits */
	dzaddr->dzlpr = lpr;
}
 
dzxint(tp)
register struct tty *tp;
{
	register struct pdma *dp;
 
	dp = &dzpdma[tp-dz_tty];
	tp->t_state &= ~BUSY;
	if (tp->t_state & FLUSH)
		tp->t_state &= ~FLUSH;
	else
		ndflush(&tp->t_outq, dp->p_end-tp->t_outq.c_cf);
	if (tp->t_line)
		(*linesw[tp->t_line].l_start)(tp);
	else
		dzstart(tp);
	if (tp->t_outq.c_cc == 0 || (tp->t_state&BUSY)==0)
		dp->p_addr->dztcr &= ~(1 << ((tp-dz_tty) % 8));
}

dzstart(tp)
register struct tty *tp;
{
	register struct pdma *dp;
	register struct device *dzaddr;
	register cc;
	int sps;
	extern ttrstrt();
 
	dp = &dzpdma[tp-dz_tty];
	dzaddr = dp->p_addr;
	sps = spl5();
	if (tp->t_state & (TIMEOUT|BUSY|TTSTOP))
		goto out;
	if (tp->t_outq.c_cc <= TTLOWAT && tp->t_state&ASLEEP) {
		tp->t_state &= ~ASLEEP;
		if (tp->t_chan)
			mcstart(tp->t_chan, (caddr_t)&tp->t_outq);
		else
			wakeup((caddr_t)&tp->t_outq);
	}
	if (tp->t_outq.c_cc == 0)
		goto out;
	if (tp->t_flags&RAW)
		cc = ndqb(&tp->t_outq, 0);
	else {
		cc = ndqb(&tp->t_outq, 0200);
		if (cc == 0) {
			cc = getc(&tp->t_outq);
			timeout(ttrstrt, (caddr_t)tp, (cc&0177) + 6);
			tp->t_state |= TIMEOUT;
			goto out;
		}
	}
	tp->t_state |= BUSY;
	dp->p_end = dp->p_mem = tp->t_outq.c_cf;
	dp->p_end += cc;
	dzaddr->dztcr |= 1 << ((tp-dz_tty) % 8);
   out:
	splx(sps);
}
 
/*
 * Stop output on a line.
 * Assume call is made at spl6.
 */
/*ARGSUSED*/
dzstop(tp, flag)
register struct tty *tp;
{
	register struct pdma *dp;
	register int s;

	dp = &dzpdma[tp-dz_tty];
	s = spl6();
	if (tp->t_state & BUSY) {
		dp->p_end = dp->p_mem;
		if ((tp->t_state&TTSTOP)==0) {
			tp->t_state |= FLUSH;
		}
	}
	splx(s);
}
 
dzmodem(dev, flag)
register int dev;
{
	register struct device *dzaddr;
	register char bit;
 
	dzaddr = dzpdma[dev].p_addr;
	bit = 1<<(dev&07);
	if (flag == OFF)
		dzaddr->dzdtr &= ~bit;
	else
		dzaddr->dzdtr |= bit;
}
 
dzscan()
{
	register i;
	register struct device *dzaddr;
	register bit;
	register struct tty *tp;
 
	for (i = 0; i < dz_cnt ; i++) {
		dzaddr = dzpdma[i].p_addr;
		tp = &dz_tty[i];
		bit = 1<<(i&07);
		if (dzaddr->dzmsr & bit) {
			/* carrier present */
			if ((tp->t_state & CARR_ON) == 0) {
				wakeup((caddr_t)&tp->t_rawq);
				tp->t_state |= CARR_ON;
			}
		} else {
			if ((tp->t_state & CARR_ON)) {
				/* carrier lost */
				signal(tp->t_pgrp, SIGHUP);
				dzaddr->dzdtr &= ~bit;
				flushtty(tp);
			}
			tp->t_state &= ~CARR_ON;
		}
	}
	timeout(dzscan, (caddr_t)0, 2*HZ);
}
