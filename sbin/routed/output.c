/*
 * Copyright (c) 1983, 1988, 1993
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
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
 */

#if !defined(lint) && !defined(sgi) && !defined(__NetBSD__)
static char sccsid[] = "@(#)output.c	8.1 (Berkeley) 6/5/93";
#elif defined(__NetBSD__)
static char rcsid[] = "$NetBSD$";
#endif
#ident "$Revision: 1.18 $"

#include "defs.h"


int update_seqno;


/* walk the tree of routes with this for output
 */
struct {
	struct sockaddr_in to;
	naddr	to_mask;
	naddr	to_net;
	naddr	to_std_mask;
	naddr	to_std_net;
	struct interface *ifp;		/* usually output interface */
	struct auth_key *a;
	char	metric;			/* adjust metrics by interface */
	int	npackets;
	int	gen_limit;
	u_int	state;
#define	    WS_ST_FLASH	    0x001	/* send only changed routes */
#define	    WS_ST_RIP2_ALL  0x002	/* send full featured RIPv2 */
#define	    WS_ST_AG	    0x004	/* ok to aggregate subnets */
#define	    WS_ST_SUPER_AG  0x008	/* ok to aggregate networks */
#define	    WS_ST_SUB_AG    0x010	/* aggregate subnets in odd case */
#define	    WS_ST_QUERY	    0x020	/* responding to a query */
#define	    WS_ST_TO_ON_NET 0x040	/* sending onto one of our nets */
#define	    WS_ST_DEFAULT   0x080	/* faking a default */
} ws;

/* A buffer for what can be heard by both RIPv1 and RIPv2 listeners */
struct ws_buf v12buf;
union pkt_buf ripv12_buf;

/* Another for only RIPv2 listeners */
struct ws_buf v2buf;
union pkt_buf rip_v2_buf;



void
bufinit(void)
{
	ripv12_buf.rip.rip_cmd = RIPCMD_RESPONSE;
	v12buf.buf = &ripv12_buf.rip;
	v12buf.base = &v12buf.buf->rip_nets[0];

	rip_v2_buf.rip.rip_cmd = RIPCMD_RESPONSE;
	rip_v2_buf.rip.rip_vers = RIPv2;
	v2buf.buf = &rip_v2_buf.rip;
	v2buf.base = &v2buf.buf->rip_nets[0];
}


/* Send the contents of the global buffer via the non-multicast socket
 */
int					/* <0 on failure */
output(enum output_type type,
       struct sockaddr_in *dst,		/* send to here */
       struct interface *ifp,
       struct rip *buf,
       int size)			/* this many bytes */
{
	struct sockaddr_in sin;
	int flags;
	char *msg;
	int res;
	naddr tgt_mcast;
	int soc;
	int serrno;

	sin = *dst;
	if (sin.sin_port == 0)
		sin.sin_port = htons(RIP_PORT);
#ifdef _HAVE_SIN_LEN
	if (sin.sin_len == 0)
		sin.sin_len = sizeof(sin);
#endif

	soc = rip_sock;
	flags = 0;

	switch (type) {
	case OUT_QUERY:
		msg = "Answer Query";
		if (soc < 0)
			soc = ifp->int_rip_sock;
		break;
	case OUT_UNICAST:
		msg = "Send";
		if (soc < 0)
			soc = ifp->int_rip_sock;
		flags = MSG_DONTROUTE;
		break;
	case OUT_BROADCAST:
		if (ifp->int_if_flags & IFF_POINTOPOINT) {
			msg = "Send";
		} else {
			msg = "Send bcast";
		}
		flags = MSG_DONTROUTE;
		break;
	case OUT_MULTICAST:
		if (ifp->int_if_flags & IFF_POINTOPOINT) {
			msg = "Send pt-to-pt";
		} else if (ifp->int_state & IS_DUP) {
			trace_act("abort multicast output via %s"
				  " with duplicate address",
				  ifp->int_name);
			return 0;
		} else {
			msg = "Send mcast";
			if (rip_sock_mcast != ifp) {
#ifdef MCAST_PPP_BUG
				/* Do not specifiy the primary interface
				 * explicitly if we have the multicast
				 * point-to-point kernel bug, since the
				 * kernel will do the wrong thing if the
				 * local address of a point-to-point link
				 * is the same as the address of an ordinary
				 * interface.
				 */
				if (ifp->int_addr == myaddr) {
					tgt_mcast = 0;
				} else
#endif
				tgt_mcast = ifp->int_addr;
				if (0 > setsockopt(rip_sock,
						   IPPROTO_IP, IP_MULTICAST_IF,
						   &tgt_mcast,
						   sizeof(tgt_mcast))) {
					serrno = errno;
					LOGERR("setsockopt(rip_sock,"
					       "IP_MULTICAST_IF)");
					errno = serrno;
					ifp = 0;
					return -1;
				}
				rip_sock_mcast = ifp;
			}
			sin.sin_addr.s_addr = htonl(INADDR_RIP_GROUP);
		}

	case NO_OUT_MULTICAST:
	case NO_OUT_RIPV2:
		break;
	}

	trace_rip(msg, "to", &sin, ifp, buf, size);

	res = sendto(soc, buf, size, flags,
		     (struct sockaddr *)&sin, sizeof(sin));
	if (res < 0
	    && (ifp == 0 || !(ifp->int_state & IS_BROKE))) {
		serrno = errno;
		msglog("%s sendto(%s%s%s.%d): %s", msg,
		       ifp != 0 ? ifp->int_name : "",
		       ifp != 0 ? ", " : "",
		       inet_ntoa(sin.sin_addr),
		       ntohs(sin.sin_port),
		       strerror(errno));
		errno = serrno;
	}

	return res;
}


/* Find the first key that has not expired, but settle for
 * the last key if they have all expired.
 * If no key is ready yet, give up.
 */
struct auth_key *
find_auth(struct interface *ifp)
{
	struct auth_key *ap, *res;
	int i;


	if (ifp == 0 || ifp->int_auth.type == RIP_AUTH_NONE)
		return 0;
	
	res = 0;
	ap = ifp->int_auth.keys;
	for (i = 0; i < MAX_AUTH_KEYS; i++, ap++) {
		if ((u_long)ap->start <= (u_long)clk.tv_sec) {
			if ((u_long)ap->end >= (u_long)clk.tv_sec)
				return ap;
			res = ap;
		}
	}
	return res;
}


void
clr_ws_buf(struct ws_buf *wb,
	   struct auth_key *ap,
	   struct interface *ifp)
{
	struct netauth *na;

	wb->lim = wb->base + NETS_LEN;
	wb->n = wb->base;
	bzero(wb->n, NETS_LEN*sizeof(*wb->n));

	/* install authentication if appropriate
	 */
	if (ap == 0)
		return;
	na = (struct netauth*)wb->n;
	if (ifp->int_auth.type == RIP_AUTH_PW) {
		na->a_family = RIP_AF_AUTH;
		na->a_type = RIP_AUTH_PW;
		bcopy(ap->key, na->au.au_pw, sizeof(na->au.au_pw));
		wb->n++;

	} else if (ifp->int_auth.type ==  RIP_AUTH_MD5) {
		na->a_family = RIP_AF_AUTH;
		na->a_type = RIP_AUTH_MD5;
		na->au.a_md5.md5_keyid = ap->keyid;
		na->au.a_md5.md5_auth_len = RIP_AUTH_PW_LEN;
		na->au.a_md5.md5_seqno = clk.tv_sec;
		wb->n++;
		wb->lim--;		/* make room for trailer */
	}
}


void
end_md5_auth(struct ws_buf *wb,
	     struct auth_key *ap)
{
	struct netauth *na, *na2;
	MD5_CTX md5_ctx;


	na = (struct netauth*)wb->base;
	na2 = (struct netauth*)wb->n;
	na2->a_family = RIP_AF_AUTH;
	na2->a_type = 1;
	bcopy(ap->key, na2->au.au_pw, sizeof(na2->au.au_pw));
	na->au.a_md5.md5_pkt_len = (char *)na2-(char *)(na+1);
	MD5Init(&md5_ctx);
	MD5Update(&md5_ctx, (u_char *)na,
		  (char *)(na2+1) - (char *)na);
	MD5Final(na2->au.au_pw, &md5_ctx);
	wb->n++;
}


/* Send the buffer
 */
static void
supply_write(struct ws_buf *wb)
{
	/* Output multicast only if legal.
	 * If we would multcast and it would be illegal, then discard the
	 * packet.
	 */
	switch (wb->type) {
	case NO_OUT_MULTICAST:
		trace_pkt("skip multicast to %s because impossible",
			  naddr_ntoa(ws.to.sin_addr.s_addr));
		break;
	case NO_OUT_RIPV2:
		break;
	default:
		if (ws.ifp->int_auth.type == RIP_AUTH_MD5)
			end_md5_auth(wb,ws.a);
		if (output(wb->type, &ws.to, ws.ifp, wb->buf,
			   ((char *)wb->n - (char*)wb->buf)) < 0
		    && ws.ifp != 0)
			if_sick(ws.ifp);
		ws.npackets++;
		break;
	}

	clr_ws_buf(wb,ws.a,ws.ifp);
}


/* put an entry into the packet
 */
static void
supply_out(struct ag_info *ag)
{
	int i;
	naddr mask, v1_mask, dst_h, ddst_h;
	struct ws_buf *wb;


	/* Skip this route if doing a flash update and it and the routes
	 * it aggregates have not changed recently.
	 */
	if (ag->ag_seqno < update_seqno
	    && (ws.state & WS_ST_FLASH))
		return;

	/* Skip this route if required by split-horizon.
	 */
	if (ag->ag_state & AGS_SPLIT_HZ)
		return;

	dst_h = ag->ag_dst_h;
	mask = ag->ag_mask;
	v1_mask = ripv1_mask_host(htonl(dst_h),
				  (ws.state & WS_ST_TO_ON_NET) ? ws.ifp : 0);
	i = 0;

	/* If we are sending RIPv2 packets that cannot (or must not) be
	 * heard by RIPv1 listeners, do not worry about sub- or supernets.
	 * Subnets (from other networks) can only be sent via multicast.
	 * A pair of subnet routes might have been promoted so that they
	 * are legal to send by RIPv1.
	 * If RIPv1 is off, use the multicast buffer.
	 */
	if ((ws.state & WS_ST_RIP2_ALL)
	    || ((ag->ag_state & AGS_RIPV2) && v1_mask != mask)) {
		/* use the RIPv2-only buffer */
		wb = &v2buf;

	} else {
		/* use the RIPv1-or-RIPv2 buffer */
		wb = &v12buf;

		/* Convert supernet route into corresponding set of network
		 * routes for RIPv1, but leave non-contiguous netmasks
		 * to ag_check().
		 */
		if (v1_mask > mask
		    && mask + (mask & -mask) == 0) {
			ddst_h = v1_mask & -v1_mask;
			i = (v1_mask & ~mask)/ddst_h;

			if (i > ws.gen_limit) {
				/* Punt if we would have to generate an
				 * unreasonable number of routes.
				 */
#ifdef DEBUG
				msglog("sending %s to %s as 1 instead"
				       " of %d routes",
				       addrname(htonl(dst_h),mask,1),
				       naddr_ntoa(ws.to.sin_addr.s_addr),
				       i+1);
#endif
				i = 0;

			} else {
				mask = v1_mask;
				ws.gen_limit -= i;
			}
		}
	}

	do {
		wb->n->n_family = RIP_AF_INET;
		wb->n->n_dst = htonl(dst_h);
		/* If the route is from router-discovery or we are
		 * shutting down, admit only a bad metric.
		 */
		wb->n->n_metric = ((stopint || ag->ag_metric < 1)
				   ? HOPCNT_INFINITY
				   : ag->ag_metric);
		HTONL(wb->n->n_metric);
		/* Any non-zero bits in the supposedly unused RIPv1 fields
		 * cause the old `routed` to ignore the route.
		 * That means the mask and so forth cannot be sent
		 * in the hybrid RIPv1/RIPv2 mode.
		 */
		if (ws.state & WS_ST_RIP2_ALL) {
			if (ag->ag_nhop != 0
			    && ((ws.state & WS_ST_QUERY)
				|| (ag->ag_nhop != ws.ifp->int_addr
				    && on_net(ag->ag_nhop,
					      ws.ifp->int_net,
					      ws.ifp->int_mask))))
				wb->n->n_nhop = ag->ag_nhop;
			wb->n->n_mask = htonl(mask);
			wb->n->n_tag = ag->ag_tag;
		}
		dst_h += ddst_h;

		if (++wb->n >= wb->lim)
			supply_write(wb);
	} while (i-- != 0);
}


/* supply one route from the table
 */
/* ARGSUSED */
static int
walk_supply(struct radix_node *rn,
	    struct walkarg *w)
{
#define RT ((struct rt_entry *)rn)
	u_short ags;
	char metric, pref;
	naddr dst, nhop;


	/* Do not advertise external remote interfaces or passive interfaces.
	 */
	if ((RT->rt_state & RS_IF)
	    && RT->rt_ifp != 0
	    && (RT->rt_ifp->int_if_flags & IS_PASSIVE)
	    && !(RT->rt_state & RS_MHOME))
		return 0;

	/* If being quiet about our ability to forward, then
	 * do not say anything unless responding to a query,
	 * except about our main interface.
	 */
	if (!supplier && !(ws.state & WS_ST_QUERY)
	    && !(RT->rt_state & RS_MHOME))
		return 0;

	dst = RT->rt_dst;

	/* do not collide with the fake default route */
	if (dst == RIP_DEFAULT
	    && (ws.state & WS_ST_DEFAULT))
		return 0;

	if (RT->rt_state & RS_NET_SYN) {
		if (RT->rt_state & RS_NET_INT) {
			/* Do not send manual synthetic network routes
			 * into the subnet.
			 */
			if (on_net(ws.to.sin_addr.s_addr,
				   ntohl(dst), RT->rt_mask))
				return 0;

		} else {
			/* Do not send automatic synthetic network routes
			 * if they are not needed becaus no RIPv1 listeners
			 * can hear them.
			 */
			if (ws.state & WS_ST_RIP2_ALL)
				return 0;

			/* Do not send automatic synthetic network routes to
			 * the real subnet.
			 */
			if (on_net(ws.to.sin_addr.s_addr,
				   ntohl(dst), RT->rt_mask))
				return 0;
		}
		nhop = 0;

	} else {
		/* Advertise the next hop if this is not a route for one
		 * of our interfaces and the next hop is on the same
		 * network as the target.
		 */
		if (!(RT->rt_state & RS_IF)
		    && RT->rt_gate != myaddr
		    && RT->rt_gate != loopaddr)
			nhop = RT->rt_gate;
		else
			nhop = 0;
	}

	metric = RT->rt_metric;
	ags = 0;

	if (RT->rt_state & RS_MHOME) {
		/* retain host route of multi-homed servers */
		;

	} else if (RT_ISHOST(RT)) {
		/* We should always aggregate the host routes
		 * for the local end of our point-to-point links.
		 * If we are suppressing host routes in general, then do so.
		 * Avoid advertising host routes onto their own network,
		 * where they should be handled by proxy-ARP.
		 */
		if ((RT->rt_state & RS_LOCAL)
		    || ridhosts
		    || (ws.state & WS_ST_SUPER_AG)
		    || on_net(dst, ws.to_net, ws.to_mask))
			ags |= AGS_SUPPRESS;

		if (ws.state & WS_ST_SUPER_AG)
			ags |= AGS_PROMOTE;

	} else if (ws.state & WS_ST_AG) {
		/* Aggregate network routes, if we are allowed.
		 */
		ags |= AGS_SUPPRESS;

		/* Generate supernets if allowed.
		 * If we can be heard by RIPv1 systems, we will
		 * later convert back to ordinary nets.
		 * This unifies dealing with received supernets.
		 */
		if ((RT->rt_state & RS_SUBNET)
		    || (ws.state & WS_ST_SUPER_AG))
			ags |= AGS_PROMOTE;

	}

	/* Do not send RIPv1 advertisements of subnets to other
	 * networks. If possible, multicast them by RIPv2.
	 */
	if ((RT->rt_state & RS_SUBNET)
	    && !(ws.state & WS_ST_RIP2_ALL)
	    && !on_net(dst, ws.to_std_net, ws.to_std_mask)) {
		ags |= AGS_RIPV2 | AGS_PROMOTE;
		if (ws.state & WS_ST_SUB_AG)
			ags |= AGS_SUPPRESS;
	}

	/* Do not send a route back to where it came from, except in
	 * response to a query.  This is "split-horizon".  That means not
	 * advertising back to the same network	and so via the same interface.
	 *
	 * We want to suppress routes that might have been fragmented
	 * from this route by a RIPv1 router and sent back to us, and so we
	 * cannot forget this route here.  Let the split-horizon route
	 * aggregate (suppress) the fragmented routes and then itself be
	 * forgotten.
	 *
	 * Include the routes for both ends of point-to-point interfaces
	 * since the other side presumably knows them as well as we do.
	 */
	if (RT->rt_ifp == ws.ifp && ws.ifp != 0
	    && !(ws.state & WS_ST_QUERY)
	    && (ws.state & WS_ST_TO_ON_NET)
	    && (!(RT->rt_state & RS_IF)
		|| ws.ifp->int_if_flags & IFF_POINTOPOINT)) {
		/* Poison-reverse the route instead of only not advertising it
		 * it is recently changed from some other route.
		 * In almost all cases, if there is no spare for the route
		 * then it is either old or a brand new route, and if it
		 * is brand new, there is no need for poison-reverse.
		 */
		metric = HOPCNT_INFINITY;
		if (RT->rt_poison_time < now_expire
		    || RT->rt_spares[1].rts_gate ==0) {
			ags |= AGS_SPLIT_HZ;
			ags &= ~(AGS_PROMOTE | AGS_SUPPRESS);
		}
	}

	/* Adjust the outgoing metric by the cost of the link.
	 */
	pref = metric + ws.metric;
	if (pref < HOPCNT_INFINITY) {
		/* Keep track of the best metric with which the
		 * route has been advertised recently.
		 */
		if (RT->rt_poison_metric >= metric
		    || RT->rt_poison_time < now_expire) {
			RT->rt_poison_time = now.tv_sec;
			RT->rt_poison_metric = metric;
		}
		metric = pref;

	} else {
		/* Do not advertise stable routes that will be ignored,
		 * unless we are answering a query.
		 * If the route recently was advertised with a metric that
		 * would have been less than infinity through this interface,
		 * we need to continue to advertise it in order to poison it.
		 */
		pref = RT->rt_poison_metric + ws.metric;
		if (!(ws.state & WS_ST_QUERY)
		    && (pref >= HOPCNT_INFINITY
			|| RT->rt_poison_time < now_garbage))
			return 0;

		metric = HOPCNT_INFINITY;
	}

	ag_check(dst, RT->rt_mask, 0, nhop, metric, pref,
		 RT->rt_seqno, RT->rt_tag, ags, supply_out);
	return 0;
#undef RT
}


/* Supply dst with the contents of the routing tables.
 * If this won't fit in one packet, chop it up into several.
 */
void
supply(struct sockaddr_in *dst,
       struct interface *ifp,		/* output interface */
       enum output_type type,
       int flash,			/* 1=flash update */
       int vers,			/* RIP version */
       int passwd_ok)			/* OK to include cleartext password */
{
	struct rt_entry *rt;
	int def_metric;


	ws.state = 0;
	ws.gen_limit = 1024;

	ws.to = *dst;
	ws.to_std_mask = std_mask(ws.to.sin_addr.s_addr);
	ws.to_std_net = ntohl(ws.to.sin_addr.s_addr) & ws.to_std_mask;

	if (ifp != 0) {
		ws.to_mask = ifp->int_mask;
		ws.to_net = ifp->int_net;
		if (on_net(ws.to.sin_addr.s_addr, ws.to_net, ws.to_mask))
			ws.state |= WS_ST_TO_ON_NET;

	} else {
		ws.to_mask = ripv1_mask_net(ws.to.sin_addr.s_addr, 0);
		ws.to_net = ntohl(ws.to.sin_addr.s_addr) & ws.to_mask;
		rt = rtfind(dst->sin_addr.s_addr);
		if (rt)
			ifp = rt->rt_ifp;
	}

	ws.npackets = 0;
	if (flash)
		ws.state |= WS_ST_FLASH;
	if (type == OUT_QUERY)
		ws.state |= WS_ST_QUERY;

	if ((ws.ifp = ifp) == 0) {
		ws.metric = 1;
	} else {
		/* Adjust the advertised metric by the outgoing interface
		 * metric.
		 */
		ws.metric = ifp->int_metric+1;
	}

	ripv12_buf.rip.rip_vers = vers;

	switch (type) {
	case OUT_BROADCAST:
		v2buf.type = ((ifp != 0 && (ifp->int_if_flags & IFF_MULTICAST))
			      ? OUT_MULTICAST
			      : NO_OUT_MULTICAST);
		v12buf.type = OUT_BROADCAST;
		break;
	case OUT_MULTICAST:
		v2buf.type = ((ifp != 0 && (ifp->int_if_flags & IFF_MULTICAST))
			      ? OUT_MULTICAST
			      : NO_OUT_MULTICAST);
		v12buf.type = OUT_BROADCAST;
		break;
	case OUT_UNICAST:
	case OUT_QUERY:
		v2buf.type = (vers == RIPv2) ? type : NO_OUT_RIPV2;
		v12buf.type = type;
		break;
	default:
		v2buf.type = type;
		v12buf.type = type;
		break;
	}

	if (vers == RIPv2) {
		/* full RIPv2 only if cannot be heard by RIPv1 listeners */
		if (type != OUT_BROADCAST)
			ws.state |= WS_ST_RIP2_ALL;
		if (!(ws.state & WS_ST_TO_ON_NET)) {
			ws.state |= (WS_ST_AG | WS_ST_SUPER_AG);
		} else if (ifp == 0 || !(ifp->int_state & IS_NO_AG)) {
			ws.state |= WS_ST_AG;
			if (type != OUT_BROADCAST
			    && (ifp == 0 || !(ifp->int_state&IS_NO_SUPER_AG)))
				ws.state |= WS_ST_SUPER_AG;
		}

	} else if (ifp == 0 || !(ifp->int_state & IS_NO_AG)) {
		ws.state |= WS_ST_SUB_AG;
	}

	ws.a = (vers == RIPv2) ? find_auth(ifp) : 0;
	if (ws.a != 0 && !passwd_ok && ifp->int_auth.type == RIP_AUTH_PW)
		ws.a = 0;
	clr_ws_buf(&v12buf,ws.a,ifp);
	clr_ws_buf(&v2buf,ws.a,ifp);

	/*  Fake a default route if asked and if there is not already
	 * a better, real default route.
	 */
	if (supplier && (def_metric = ifp->int_d_metric) != 0) {
		if (0 == (rt = rtget(RIP_DEFAULT, 0))
		    || rt->rt_metric+ws.metric >= def_metric) {
			ws.state |= WS_ST_DEFAULT;
			ag_check(0, 0, 0, 0, def_metric, def_metric,
				 0, 0, 0, supply_out);
		} else {
			def_metric = rt->rt_metric+ws.metric;
		}

		/* If both RIPv2 and the poor-man's router discovery
		 * kludge are on, arrange to advertise an extra
		 * default route via RIPv1.
		 */
		if ((ws.state & WS_ST_RIP2_ALL)
		    && (ifp->int_state & IS_PM_RDISC)) {
			ripv12_buf.rip.rip_vers = RIPv1;
			v12buf.n->n_family = RIP_AF_INET;
			v12buf.n->n_dst = htonl(RIP_DEFAULT);
			v12buf.n->n_metric = htonl(def_metric);
			v12buf.n++;
		}
	}

	(void)rn_walktree(rhead, walk_supply, 0);
	ag_flush(0,0,supply_out);

	/* Flush the packet buffers, provided they are not empty and
	 * do not contain only the password.
	 */
	if (v12buf.n != v12buf.base
	    && (v12buf.n > v12buf.base+1
		|| v12buf.base->n_family != RIP_AF_AUTH))
		supply_write(&v12buf);
	if (v2buf.n != v2buf.base
	    && (v2buf.n > v2buf.base+1
		|| v2buf.base->n_family != RIP_AF_AUTH))
		supply_write(&v2buf);

	/* If we sent nothing and this is an answer to a query, send
	 * an empty buffer.
	 */
	if (ws.npackets == 0
	    && (ws.state & WS_ST_QUERY))
		supply_write(&v12buf);
}


/* send all of the routing table or just do a flash update
 */
void
rip_bcast(int flash)
{
#ifdef _HAVE_SIN_LEN
	static struct sockaddr_in dst = {sizeof(dst), AF_INET};
#else
	static struct sockaddr_in dst = {AF_INET};
#endif
	struct interface *ifp;
	enum output_type type;
	int vers;
	struct timeval rtime;


	need_flash = 0;
	intvl_random(&rtime, MIN_WAITTIME, MAX_WAITTIME);
	no_flash = rtime;
	timevaladd(&no_flash, &now);

	if (rip_sock < 0)
		return;

	trace_act("send %s and inhibit dynamic updates for %.3f sec",
		  flash ? "dynamic update" : "all routes",
		  rtime.tv_sec + ((float)rtime.tv_usec)/1000000.0);

	for (ifp = ifnet; ifp != 0; ifp = ifp->int_next) {
		/* Skip interfaces not doing RIP.
		 * Do try broken interfaces to see if they have healed.
		 */
		if (IS_RIP_OUT_OFF(ifp->int_state))
			continue;

		/* skip turned off interfaces */
		if (!iff_alive(ifp->int_if_flags))
			continue;

		vers = (ifp->int_state & IS_NO_RIPV1_OUT) ? RIPv2 : RIPv1;

		if (ifp->int_if_flags & IFF_BROADCAST) {
			/* ordinary, hardware interface */
			dst.sin_addr.s_addr = ifp->int_brdaddr;

			/* If RIPv1 is not turned off, then broadcast so
			 * that RIPv1 listeners can hear.
			 */
			if (vers == RIPv2
			    && (ifp->int_state & IS_NO_RIPV1_OUT)) {
				type = OUT_MULTICAST;
			} else {
				type = OUT_BROADCAST;
			}

		} else if (ifp->int_if_flags & IFF_POINTOPOINT) {
			/* point-to-point hardware interface */
			dst.sin_addr.s_addr = ifp->int_dstaddr;
			type = OUT_UNICAST;

		} else if (ifp->int_state & IS_REMOTE) {
			/* remote interface */
			dst.sin_addr.s_addr = ifp->int_addr;
			type = OUT_UNICAST;

		} else {
			/* ATM, HIPPI, etc. */
			continue;
		}

		supply(&dst, ifp, type, flash, vers, 1);
	}

	update_seqno++;			/* all routes are up to date */
}


/* Ask for routes
 * Do it only once to an interface, and not even after the interface
 * was broken and recovered.
 */
void
rip_query(void)
{
#ifdef _HAVE_SIN_LEN
	static struct sockaddr_in dst = {sizeof(dst), AF_INET};
#else
	static struct sockaddr_in dst = {AF_INET};
#endif
	struct interface *ifp;
	struct rip buf;
	enum output_type type;


	if (rip_sock < 0)
		return;

	bzero(&buf, sizeof(buf));

	for (ifp = ifnet; ifp; ifp = ifp->int_next) {
		/* Skip interfaces those already queried.
		 * Do not ask via interfaces through which we don't
		 * accept input.  Do not ask via interfaces that cannot
		 * send RIP packets.
		 * Do try broken interfaces to see if they have healed.
		 */
		if (IS_RIP_IN_OFF(ifp->int_state)
		    || ifp->int_query_time != NEVER)
			continue;

		/* skip turned off interfaces */
		if (!iff_alive(ifp->int_if_flags))
			continue;

		buf.rip_vers = (ifp->int_state&IS_NO_RIPV1_OUT) ? RIPv2:RIPv1;
		buf.rip_cmd = RIPCMD_REQUEST;
		buf.rip_nets[0].n_family = RIP_AF_UNSPEC;
		buf.rip_nets[0].n_metric = htonl(HOPCNT_INFINITY);

		if (ifp->int_if_flags & IFF_BROADCAST) {
			/* ordinary, hardware interface */
			dst.sin_addr.s_addr = ifp->int_brdaddr;
			/* if RIPv1 is not turned off, then broadcast so
			 * that RIPv1 listeners can hear.
			 */
			if (buf.rip_vers == RIPv2
			    && (ifp->int_state & IS_NO_RIPV1_OUT)) {
				type = OUT_MULTICAST;
			} else {
				type = OUT_BROADCAST;
			}

		} else if (ifp->int_if_flags & IFF_POINTOPOINT) {
			/* point-to-point hardware interface */
			dst.sin_addr.s_addr = ifp->int_dstaddr;
			type = OUT_UNICAST;

		} else if (ifp->int_state & IS_REMOTE) {
			/* remote interface */
			dst.sin_addr.s_addr = ifp->int_addr;
			type = OUT_UNICAST;

		} else {
			/* ATM, HIPPI, etc. */
			continue;
		}

		ifp->int_query_time = now.tv_sec+SUPPLY_INTERVAL;
		if (output(type, &dst, ifp, &buf, sizeof(buf)) < 0)
			if_sick(ifp);
	}
}
