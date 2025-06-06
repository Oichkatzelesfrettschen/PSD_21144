/*
 * Copyright (c) 1982, 1986, 1990, 1993
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
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)in_pcb.h	8.1 (Berkeley) 6/10/93
 */

/*
 * ARGO TP
 *
 * $Header: tp_pcb.h,v 5.2 88/11/18 17:09:32 nhall Exp $
 * $Source: /usr/argo/sys/netiso/RCS/tp_pcb.h,v $
 *
 *
 * This file defines the transport protocol control block (tpcb).
 * and a bunch of #define values that are used in the tpcb.
 */

#ifndef _NETTPI_TPI_PCB_H_
#define _NETTPI_TPI_PCB_H_

#include <sys/queue.h>

/* sockaddr union structure */
union tpi_sockaddr_union {
	struct sockaddr_in 			tsu_sin4;	/* ipv4 */
	struct sockaddr_in6 		tsu_sin6;	/* ipv6 */
	struct sockaddr_iso 		tsu_siso;	/* iso */
	struct sockaddr_ns 			tsu_sns; 	/* xns */
	struct sockaddr_x25			tsu_sx25;	/* x25 */
	// x25, atm, atalk?
};

/* addr union structure */
union tpi_addr_union {
	struct in_addr 				tau_in4;	/* ipv4 */
	struct in6_addr 			tau_in6;	/* ipv6 */
	struct iso_addr				tau_iso;	/* iso */
	struct ns_addr				tau_ns;		/* xns */
	struct x25_addr				tau_x25;	/* x25 */
};

/* Transport Local structure */
struct tpi_local {
	struct tpipcb_hdr			tpl_head;
	union tpi_sockaddr_union	tpl_lsockaddr;	/* local sockaddr */
	union tpi_addr_union		tpl_laddr;		/* local addr */
	u_int16_t					tpl_lport;		/* local port */
};

/* Transport Foreign structure */
struct tpi_foreign {
	struct tpipcb_hdr			tpf_head;
	union tpi_sockaddr_union	tpf_fsockaddr;	/* foreign sockaddr */
	union tpi_addr_union		tpf_faddr;		/* foreign addr */
	u_int16_t					tpf_fport;		/* foreign port */
};

/* Transport pcb structure */
struct tpipcb_hdr {
	CIRCLEQ_ENTRY(tpipcb_hdr) 	tpph_queue;
	LIST_ENTRY(tpipcb_hdr)    	tpph_fhash;     /* tp foreign hash */
	LIST_ENTRY(tpipcb_hdr)    	tpph_lhash;     /* tp local hash */
	int							tpph_af;		/* address family */
	caddr_t 					tpph_ppcb;		/* pointer to per-protocol pcb */
	int	  						tpph_state;		/* bind/connect state */
	struct socket 				*tpph_socket;	/* back pointer to socket */
	struct tpipcbtable 			*tpph_table;
};

LIST_HEAD(tpipcbhead, tpipcb_hdr);
CIRCLEQ_HEAD(tpipcbqueue, tpipcb_hdr);

struct tpipcbtable {
	struct tpipcbqueue	tppt_queue;
	struct tpipcbhead	*tppt_lhashtbl;
	struct tpipcbhead	*tppt_fhashtbl;
	u_long	  			tppt_lhash;
	u_long	  			tppt_fhash;
};

struct tpipcb {
	struct tpipcb_hdr	tpp_head;
#define tpp_lhash		tpp_head.tpph_lhash
#define tpp_fhash		tpp_head.tpph_fhash
#define tpp_queue		tpp_head.tpph_queue
#define tpp_af			tpp_head.tpph_af
#define tpp_ppcb		tpp_head.tpph_ppcb
#define tpp_state		tpp_head.tpph_state
#define tpp_socket		tpp_head.tpph_socket
#define tpp_table		tpp_head.tpph_table

	struct tpi_local	tpp_local;
#define	tpp_laddr 		tpp_local.tpl_laddr
#define	tpp_lport 		tpp_local.tpl_lport

	struct tpi_foreign	tpp_foreign;
#define	tpp_faddr 		tpp_foreign.tpf_faddr
#define	tpp_fport 		tpp_foreign.tpf_fport

	short 				tpp_retrans;	/* # times can still retrans */
	caddr_t				tpp_npcb;		/* to lower layer pcb */
	struct tpi_protosw	*tpp_tpproto;	/* lower-layer dependent routines */
	struct rtentry		**tpp_routep;	/* obtain mtu; inside npcb */
};

#define TPI_LOCAL 		0x01
#define TPI_FOREIGN 	0x02


uint32_t tpi_pcbnethash(void *, uint16_t);

#endif /* _NETTPI_TPI_PCB_H_ */
