/*	$NetBSD: ksyms.h,v 1.37 2017/11/06 17:56:25 christos Exp $	*/

/*
 * Copyright (c) 2001, 2003 Anders Magnusson (ragge@ludd.luth.se).
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SYS_KSYMS_H_
#define _SYS_KSYMS_H_

#if !defined(ELFSIZE)
#define ELFSIZE KERN_ELFSIZE
#endif

#include <sys/exec_elf.h>

#ifdef _KSYMS_PRIVATE
#include <sys/ioccom.h>
#include <sys/queue.h>
#include <sys/stdint.h>

TAILQ_HEAD(ksyms_symhead, ksyms_symtab);
struct ksyms_symtab {
	TAILQ_ENTRY(ksyms_symtab) 	sd_queue; 		/* All active tables */
	const char 					*sd_name;		/* Name of this table */
	Elf_Sym 					*sd_symstart;	/* Address of symbol table */
	uintptr_t		 			sd_minsym;		/* symbol with minimum value */
	uintptr_t 					sd_maxsym;		/* symbol with maximum value */
	char 						*sd_strstart;	/* Address of corresponding string table */
	int 						sd_usroffset;	/* Real address for userspace */
	int 						sd_symsize;		/* Size in bytes of symbol table */
	int 						sd_strsize;		/* Size of string table */
	int 						sd_nglob;		/* Number of global symbols */
	bool 						sd_gone;		/* dead but around for open() */
	void 						*sd_ctfstart;	/* Address of CTF contents */
	int 						sd_ctfsize;		/* Size in bytes of CTF contents */
	uint32_t 					*sd_nmap;		/* Name map for sorted symbols */
	int 						sd_nmapsize;	/* Total span of map */
};

/*
 * Static allocated ELF header.
 * Basic info is filled in at attach, sizes at open.
 */
#define	SHNOTE		1
#define	SYMTAB		2
#define	STRTAB		3
#define	SHSTRTAB	4
#define	SHBSS		5
#define	SHCTF		6
#define	NSECHDR		7

#define	NPRGHDR		1
#define	SHSTRSIZ	64

struct ksyms_hdr {
	Elf_Ehdr	kh_ehdr;
	Elf_Phdr	kh_phdr[NPRGHDR];
	Elf_Shdr	kh_shdr[NSECHDR];
	char 		kh_strtab[SHSTRSIZ];
	/* 0=NameSize, 1=DescSize, 2=Tag, 3="NetB", 4="SD\0\0", 5=Version */
	int32_t		kh_note[6];
};

static int 			ksyms_maxlen;
static bool_t 			ksyms_isopen;
static bool_t 			ksyms_initted;
static bool_t 			ksyms_loaded;
static struct lock_object 	ksyms_lock;
static struct ksyms_symtab 	kernel_symtab;

/*
 * used by savecore(8) so non-static
 */
extern struct ksyms_hdr 	ksyms_hdr;
extern int 			ksyms_symsz;
extern int 			ksyms_strsz;
extern int 			ksyms_ctfsz;	/* this is not currently used by savecore(8) */
extern struct ksyms_symhead 	ksyms_symtabs;

static void ksyms_sizes_calc(void);
#endif	/* _KSYMS_PRIVATE */
#if defined(_KERNEL)
/*
 * Definitions used in ksyms_getname() and ksyms_getval().
 */
#define	KSYMS_CLOSEST	0001	/* Nearest lower match */
#define	KSYMS_EXACT		0002	/* Only exact match allowed */
#define KSYMS_EXTERN	0000	/* Only external symbols (pseudo) */
#define KSYMS_PROC		0100	/* Procedures only */
#define KSYMS_ANY		0200	/* Also local symbols (DDB use only) */

/*
 * Prototypes
 */
int 	ksyms_addsymtab(const char *, void *, size_t, char *, size_t);
void 	ksyms_delsymtab(void);
void 	ksyms_init(void);
void 	ksyms_addsyms_elf(int, void *, void *);
void 	ksyms_addsyms_explicit(void *, void *, size_t, void *, size_t);

#endif /* _KERNEL */
#endif /* _SYS_KSYMS_H_ */
