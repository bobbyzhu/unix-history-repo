/*-
 * Copyright (c) 1982, 1986, 1988, 1990, 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)tcp_timer.c	8.2 (Berkeley) 5/24/95
 * $FreeBSD$
 */

#include "opt_inet6.h"
#include "opt_tcpdebug.h"
#include "opt_tcp_sack.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/mbuf.h>
#include <sys/mutex.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/sysctl.h>
#include <sys/systm.h>

#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_pcb.h>
#include <netinet/in_systm.h>
#ifdef INET6
#include <netinet6/in6_pcb.h>
#endif
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcpip.h>
#ifdef TCPDEBUG
#include <netinet/tcp_debug.h>
#endif

int	tcp_keepinit;
SYSCTL_PROC(_net_inet_tcp, TCPCTL_KEEPINIT, keepinit, CTLTYPE_INT|CTLFLAG_RW,
    &tcp_keepinit, 0, sysctl_msec_to_ticks, "I", "");

int	tcp_keepidle;
SYSCTL_PROC(_net_inet_tcp, TCPCTL_KEEPIDLE, keepidle, CTLTYPE_INT|CTLFLAG_RW,
    &tcp_keepidle, 0, sysctl_msec_to_ticks, "I", "");

int	tcp_keepintvl;
SYSCTL_PROC(_net_inet_tcp, TCPCTL_KEEPINTVL, keepintvl, CTLTYPE_INT|CTLFLAG_RW,
    &tcp_keepintvl, 0, sysctl_msec_to_ticks, "I", "");

int	tcp_delacktime;
SYSCTL_PROC(_net_inet_tcp, TCPCTL_DELACKTIME, delacktime,
    CTLTYPE_INT|CTLFLAG_RW, &tcp_delacktime, 0, sysctl_msec_to_ticks, "I",
    "Time before a delayed ACK is sent");

int	tcp_msl;
SYSCTL_PROC(_net_inet_tcp, OID_AUTO, msl, CTLTYPE_INT|CTLFLAG_RW,
    &tcp_msl, 0, sysctl_msec_to_ticks, "I", "Maximum segment lifetime");

int	tcp_rexmit_min;
SYSCTL_PROC(_net_inet_tcp, OID_AUTO, rexmit_min, CTLTYPE_INT|CTLFLAG_RW,
    &tcp_rexmit_min, 0, sysctl_msec_to_ticks, "I", "Minimum Retransmission Timeout");

int	tcp_rexmit_slop;
SYSCTL_PROC(_net_inet_tcp, OID_AUTO, rexmit_slop, CTLTYPE_INT|CTLFLAG_RW,
    &tcp_rexmit_slop, 0, sysctl_msec_to_ticks, "I", "Retransmission Timer Slop");

static int	always_keepalive = 1;
SYSCTL_INT(_net_inet_tcp, OID_AUTO, always_keepalive, CTLFLAG_RW,
    &always_keepalive , 0, "Assume SO_KEEPALIVE on all TCP connections");

static int	tcp_keepcnt = TCPTV_KEEPCNT;
	/* max idle probes */
int	tcp_maxpersistidle;
	/* max idle time in persist */
int	tcp_maxidle;

/*
 * Tcp protocol timeout routine called every 500 ms.
 * Updates timestamps used for TCP
 * causes finite state machine actions if timers expire.
 */
void
tcp_slowtimo()
{

	tcp_maxidle = tcp_keepcnt * tcp_keepintvl;
	INP_INFO_WLOCK(&tcbinfo);
	(void) tcp_timer_2msl_tw(0);
	INP_INFO_WUNLOCK(&tcbinfo);
}

int	tcp_syn_backoff[TCP_MAXRXTSHIFT + 1] =
    { 1, 1, 1, 1, 1, 2, 4, 8, 16, 32, 64, 64, 64 };

int	tcp_backoff[TCP_MAXRXTSHIFT + 1] =
    { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 512, 512, 512 };

static int tcp_totbackoff = 2559;	/* sum of tcp_backoff[] */

/*
 * TCP timer processing.
 */

void
tcp_timer_delack(xtp)
	void *xtp;
{
	struct tcpcb *tp = xtp;
	struct inpcb *inp;

	INP_INFO_RLOCK(&tcbinfo);
	inp = tp->t_inpcb;
	if (inp == NULL) {
		INP_INFO_RUNLOCK(&tcbinfo);
		return;
	}
	INP_LOCK(inp);
	INP_INFO_RUNLOCK(&tcbinfo);
	if (callout_pending(tp->tt_delack) || !callout_active(tp->tt_delack)) {
		INP_UNLOCK(inp);
		return;
	}
	callout_deactivate(tp->tt_delack);

	tp->t_flags |= TF_ACKNOW;
	tcpstat.tcps_delack++;
	(void) tcp_output(tp);
	INP_UNLOCK(inp);
}

void
tcp_timer_2msl(xtp)
	void *xtp;
{
	struct tcpcb *tp = xtp;
	struct inpcb *inp;
#ifdef TCPDEBUG
	int ostate;

	ostate = tp->t_state;
#endif
	INP_INFO_WLOCK(&tcbinfo);
	inp = tp->t_inpcb;
	if (inp == NULL) {
		INP_INFO_WUNLOCK(&tcbinfo);
		return;
	}
	INP_LOCK(inp);
	tcp_free_sackholes(tp);
	if (callout_pending(tp->tt_2msl) || !callout_active(tp->tt_2msl)) {
		INP_UNLOCK(tp->t_inpcb);
		INP_INFO_WUNLOCK(&tcbinfo);
		return;
	}
	callout_deactivate(tp->tt_2msl);
	/*
	 * 2 MSL timeout in shutdown went off.  If we're closed but
	 * still waiting for peer to close and connection has been idle
	 * too long, or if 2MSL time is up from TIME_WAIT, delete connection
	 * control block.  Otherwise, check again in a bit.
	 */
	if (tp->t_state != TCPS_TIME_WAIT &&
	    (ticks - (int)tp->t_rcvtime) <= tcp_maxidle)
		callout_reset(tp->tt_2msl, tcp_keepintvl,
			      tcp_timer_2msl, tp);
	else
		tp = tcp_close(tp);

#ifdef TCPDEBUG
	if (tp != NULL && (tp->t_inpcb->inp_socket->so_options & SO_DEBUG))
		tcp_trace(TA_USER, ostate, tp, (void *)0, (struct tcphdr *)0,
			  PRU_SLOWTIMO);
#endif
	if (tp != NULL)
		INP_UNLOCK(inp);
	INP_INFO_WUNLOCK(&tcbinfo);
}

/*
 * The timed wait queue contains references to each of the TCP sessions
 * currently in the TIME_WAIT state.  The queue pointers, including the
 * queue pointers in each tcptw structure, are protected using the global
 * tcbinfo lock, which must be held over queue iteration and modification.
 */
static TAILQ_HEAD(, tcptw)	twq_2msl;

void
tcp_timer_init(void)
{

	TAILQ_INIT(&twq_2msl);
}

void
tcp_timer_2msl_reset(struct tcptw *tw, int rearm)
{

	INP_INFO_WLOCK_ASSERT(&tcbinfo);
	INP_LOCK_ASSERT(tw->tw_inpcb);
	if (rearm)
		TAILQ_REMOVE(&twq_2msl, tw, tw_2msl);
	tw->tw_time = ticks + 2 * tcp_msl;
	TAILQ_INSERT_TAIL(&twq_2msl, tw, tw_2msl);
}

void
tcp_timer_2msl_stop(struct tcptw *tw)
{

	INP_INFO_WLOCK_ASSERT(&tcbinfo);
	TAILQ_REMOVE(&twq_2msl, tw, tw_2msl);
}

struct tcptw *
tcp_timer_2msl_tw(int reuse)
{
	struct tcptw *tw;

	INP_INFO_WLOCK_ASSERT(&tcbinfo);
	for (;;) {
		tw = TAILQ_FIRST(&twq_2msl);
		if (tw == NULL || (!reuse && (int)(tw->tw_time - ticks) > 0))
			break;
		INP_LOCK(tw->tw_inpcb);
		tcp_twclose(tw, reuse);
		if (reuse)
			return (tw);
	}
	return (NULL);
}

void
tcp_timer_keep(xtp)
	void *xtp;
{
	struct tcpcb *tp = xtp;
	struct tcptemp *t_template;
	struct inpcb *inp;
#ifdef TCPDEBUG
	int ostate;

	ostate = tp->t_state;
#endif
	INP_INFO_WLOCK(&tcbinfo);
	inp = tp->t_inpcb;
	if (!inp) {
		INP_INFO_WUNLOCK(&tcbinfo);
		return;
	}
	INP_LOCK(inp);
	if (callout_pending(tp->tt_keep) || !callout_active(tp->tt_keep)) {
		INP_UNLOCK(inp);
		INP_INFO_WUNLOCK(&tcbinfo);
		return;
	}
	callout_deactivate(tp->tt_keep);
	/*
	 * Keep-alive timer went off; send something
	 * or drop connection if idle for too long.
	 */
	tcpstat.tcps_keeptimeo++;
	if (tp->t_state < TCPS_ESTABLISHED)
		goto dropit;
	if ((always_keepalive || inp->inp_socket->so_options & SO_KEEPALIVE) &&
	    tp->t_state <= TCPS_CLOSING) {
		if ((ticks - (int)tp->t_rcvtime) >= tcp_keepidle + tcp_maxidle)
			goto dropit;
		/*
		 * Send a packet designed to force a response
		 * if the peer is up and reachable:
		 * either an ACK if the connection is still alive,
		 * or an RST if the peer has closed the connection
		 * due to timeout or reboot.
		 * Using sequence number tp->snd_una-1
		 * causes the transmitted zero-length segment
		 * to lie outside the receive window;
		 * by the protocol spec, this requires the
		 * correspondent TCP to respond.
		 */
		tcpstat.tcps_keepprobe++;
		t_template = tcpip_maketemplate(inp);
		if (t_template) {
			tcp_respond(tp, t_template->tt_ipgen,
				    &t_template->tt_t, (struct mbuf *)NULL,
				    tp->rcv_nxt, tp->snd_una - 1, 0);
			(void) m_free(dtom(t_template));
		}
		callout_reset(tp->tt_keep, tcp_keepintvl, tcp_timer_keep, tp);
	} else
		callout_reset(tp->tt_keep, tcp_keepidle, tcp_timer_keep, tp);

#ifdef TCPDEBUG
	if (inp->inp_socket->so_options & SO_DEBUG)
		tcp_trace(TA_USER, ostate, tp, (void *)0, (struct tcphdr *)0,
			  PRU_SLOWTIMO);
#endif
	INP_UNLOCK(inp);
	INP_INFO_WUNLOCK(&tcbinfo);
	return;

dropit:
	tcpstat.tcps_keepdrops++;
	tp = tcp_drop(tp, ETIMEDOUT);

#ifdef TCPDEBUG
	if (tp != NULL && (tp->t_inpcb->inp_socket->so_options & SO_DEBUG))
		tcp_trace(TA_USER, ostate, tp, (void *)0, (struct tcphdr *)0,
			  PRU_SLOWTIMO);
#endif
	if (tp != NULL)
		INP_UNLOCK(tp->t_inpcb);
	INP_INFO_WUNLOCK(&tcbinfo);
}

void
tcp_timer_persist(xtp)
	void *xtp;
{
	struct tcpcb *tp = xtp;
	struct inpcb *inp;
#ifdef TCPDEBUG
	int ostate;

	ostate = tp->t_state;
#endif
	INP_INFO_WLOCK(&tcbinfo);
	inp = tp->t_inpcb;
	if (!inp) {
		INP_INFO_WUNLOCK(&tcbinfo);
		return;
	}
	INP_LOCK(inp);
	if (callout_pending(tp->tt_persist) || !callout_active(tp->tt_persist)){
		INP_UNLOCK(inp);
		INP_INFO_WUNLOCK(&tcbinfo);
		return;
	}
	callout_deactivate(tp->tt_persist);
	/*
	 * Persistance timer into zero window.
	 * Force a byte to be output, if possible.
	 */
	tcpstat.tcps_persisttimeo++;
	/*
	 * Hack: if the peer is dead/unreachable, we do not
	 * time out if the window is closed.  After a full
	 * backoff, drop the connection if the idle time
	 * (no responses to probes) reaches the maximum
	 * backoff that we would use if retransmitting.
	 */
	if (tp->t_rxtshift == TCP_MAXRXTSHIFT &&
	    ((ticks - (int)tp->t_rcvtime) >= tcp_maxpersistidle ||
	     (ticks - (int)tp->t_rcvtime) >= TCP_REXMTVAL(tp) * tcp_totbackoff)) {
		tcpstat.tcps_persistdrop++;
		tp = tcp_drop(tp, ETIMEDOUT);
		goto out;
	}
	tcp_setpersist(tp);
	tp->t_flags |= TF_FORCEDATA;
	(void) tcp_output(tp);
	tp->t_flags &= ~TF_FORCEDATA;

out:
#ifdef TCPDEBUG
	if (tp != NULL && tp->t_inpcb->inp_socket->so_options & SO_DEBUG)
		tcp_trace(TA_USER, ostate, tp, (void *)0, (struct tcphdr *)0,
			  PRU_SLOWTIMO);
#endif
	if (tp != NULL)
		INP_UNLOCK(inp);
	INP_INFO_WUNLOCK(&tcbinfo);
}

void
tcp_timer_rexmt(xtp)
	void *xtp;
{
	struct tcpcb *tp = xtp;
	int rexmt;
	int headlocked;
	struct inpcb *inp;
#ifdef TCPDEBUG
	int ostate;

	ostate = tp->t_state;
#endif
	INP_INFO_WLOCK(&tcbinfo);
	headlocked = 1;
	inp = tp->t_inpcb;
	if (!inp) {
		INP_INFO_WUNLOCK(&tcbinfo);
		return;
	}
	INP_LOCK(inp);
	if (callout_pending(tp->tt_rexmt) || !callout_active(tp->tt_rexmt)) {
		INP_UNLOCK(inp);
		INP_INFO_WUNLOCK(&tcbinfo);
		return;
	}
	callout_deactivate(tp->tt_rexmt);
	tcp_free_sackholes(tp);
	/*
	 * Retransmission timer went off.  Message has not
	 * been acked within retransmit interval.  Back off
	 * to a longer retransmit interval and retransmit one segment.
	 */
	if (++tp->t_rxtshift > TCP_MAXRXTSHIFT) {
		tp->t_rxtshift = TCP_MAXRXTSHIFT;
		tcpstat.tcps_timeoutdrop++;
		tp = tcp_drop(tp, tp->t_softerror ?
			      tp->t_softerror : ETIMEDOUT);
		goto out;
	}
	INP_INFO_WUNLOCK(&tcbinfo);
	headlocked = 0;
	if (tp->t_rxtshift == 1) {
		/*
		 * first retransmit; record ssthresh and cwnd so they can
		 * be recovered if this turns out to be a "bad" retransmit.
		 * A retransmit is considered "bad" if an ACK for this
		 * segment is received within RTT/2 interval; the assumption
		 * here is that the ACK was already in flight.  See
		 * "On Estimating End-to-End Network Path Properties" by
		 * Allman and Paxson for more details.
		 */
		tp->snd_cwnd_prev = tp->snd_cwnd;
		tp->snd_ssthresh_prev = tp->snd_ssthresh;
		tp->snd_recover_prev = tp->snd_recover;
		if (IN_FASTRECOVERY(tp))
		  tp->t_flags |= TF_WASFRECOVERY;
		else
		  tp->t_flags &= ~TF_WASFRECOVERY;
		tp->t_badrxtwin = ticks + (tp->t_srtt >> (TCP_RTT_SHIFT + 1));
	}
	tcpstat.tcps_rexmttimeo++;
	if (tp->t_state == TCPS_SYN_SENT)
		rexmt = TCP_REXMTVAL(tp) * tcp_syn_backoff[tp->t_rxtshift];
	else
		rexmt = TCP_REXMTVAL(tp) * tcp_backoff[tp->t_rxtshift];
	TCPT_RANGESET(tp->t_rxtcur, rexmt,
		      tp->t_rttmin, TCPTV_REXMTMAX);
	/*
	 * Disable rfc1323 if we havn't got any response to
	 * our third SYN to work-around some broken terminal servers
	 * (most of which have hopefully been retired) that have bad VJ
	 * header compression code which trashes TCP segments containing
	 * unknown-to-them TCP options.
	 */
	if ((tp->t_state == TCPS_SYN_SENT) && (tp->t_rxtshift == 3))
		tp->t_flags &= ~(TF_REQ_SCALE|TF_REQ_TSTMP);
	/*
	 * If we backed off this far, our srtt estimate is probably bogus.
	 * Clobber it so we'll take the next rtt measurement as our srtt;
	 * move the current srtt into rttvar to keep the current
	 * retransmit times until then.
	 */
	if (tp->t_rxtshift > TCP_MAXRXTSHIFT / 4) {
#ifdef INET6
		if ((tp->t_inpcb->inp_vflag & INP_IPV6) != 0)
			in6_losing(tp->t_inpcb);
		else
#endif
		tp->t_rttvar += (tp->t_srtt >> TCP_RTT_SHIFT);
		tp->t_srtt = 0;
	}
	tp->snd_nxt = tp->snd_una;
	tp->snd_recover = tp->snd_max;
	/*
	 * Force a segment to be sent.
	 */
	tp->t_flags |= TF_ACKNOW;
	/*
	 * If timing a segment in this window, stop the timer.
	 */
	tp->t_rtttime = 0;
	/*
	 * Close the congestion window down to one segment
	 * (we'll open it by one segment for each ack we get).
	 * Since we probably have a window's worth of unacked
	 * data accumulated, this "slow start" keeps us from
	 * dumping all that data as back-to-back packets (which
	 * might overwhelm an intermediate gateway).
	 *
	 * There are two phases to the opening: Initially we
	 * open by one mss on each ack.  This makes the window
	 * size increase exponentially with time.  If the
	 * window is larger than the path can handle, this
	 * exponential growth results in dropped packet(s)
	 * almost immediately.  To get more time between
	 * drops but still "push" the network to take advantage
	 * of improving conditions, we switch from exponential
	 * to linear window opening at some threshhold size.
	 * For a threshhold, we use half the current window
	 * size, truncated to a multiple of the mss.
	 *
	 * (the minimum cwnd that will give us exponential
	 * growth is 2 mss.  We don't allow the threshhold
	 * to go below this.)
	 */
	{
		u_int win = min(tp->snd_wnd, tp->snd_cwnd) / 2 / tp->t_maxseg;
		if (win < 2)
			win = 2;
		tp->snd_cwnd = tp->t_maxseg;
		tp->snd_ssthresh = win * tp->t_maxseg;
		tp->t_dupacks = 0;
	}
	EXIT_FASTRECOVERY(tp);
	(void) tcp_output(tp);

out:
#ifdef TCPDEBUG
	if (tp != NULL && (tp->t_inpcb->inp_socket->so_options & SO_DEBUG))
		tcp_trace(TA_USER, ostate, tp, (void *)0, (struct tcphdr *)0,
			  PRU_SLOWTIMO);
#endif
	if (tp != NULL)
		INP_UNLOCK(inp);
	if (headlocked)
		INP_INFO_WUNLOCK(&tcbinfo);
}
