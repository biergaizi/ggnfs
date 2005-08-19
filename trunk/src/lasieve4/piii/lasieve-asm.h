/* lasieve-asm.h
  Hacked up for inclusion in GGNFS by Chris Monico.

  Copyright (C) 2000 Jens Franke
  This file is part of mpqs4linux, distributed under the terms of the 
  GNU General Public Licence and WITHOUT ANY WARRANTY.

  You should have received a copy of the GNU General Public License along
  with this program; see the file COPYING.  If not, write to the Free
  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
  02111-1307, USA.
*/
#ifndef _LASIEVE_ASM_H
#define _LASIEVE_ASM_H

#include <limits.h>
#include <gmp.h>
#include <sys/types.h>
#ifndef _MSC_VER
#include <stdint.h>
#endif

#define ULL_NO_UL
#include "if.h"

/* siever-config.h */
#define L1_BITS 14
#define ULONG_RI
#define HAVE_CMOV
#define HAVE_SSIMD

#if defined(_MSC_VER) && !defined(__MINGW32__)
 typedef unsigned long ssize_t;
#endif

#define PREINVERT
#define HAVE_ASM_GETBC
#define ASM_SCHEDSIEVE


#if 1
#define N_PRIMEBOUNDS 7
#else
#define N_PRIMEBOUNDS 1
#endif
/*
extern const unsigned long *schedule_primebounds;
extern const unsigned long *schedule_sizebits;
*/

extern const unsigned long schedule_primebounds[N_PRIMEBOUNDS];
extern const unsigned long schedule_sizebits[N_PRIMEBOUNDS];
void siever_init(void);

#if !defined(_MSC_VER) || defined (__MINGW32__)
#define NAME(_a) asm(_a)
#else
#define NAME(_a) 
#endif

#if defined(_MSC_VER) && !defined(__MINGW32__)

#define inline __inline
#define modinv32(x) asm_modinv32(x)
volatile extern u32_t modulo32;
u32_t asm_modinv32(u32_t x);

static inline u32_t modsq32(u32_t x)
{	u32_t res;

	__asm
	{	mov		eax,[x]
		mul		[x]
		div		[modulo32]
		mov		res,edx
	}
	return res;
}

static inline u32_t modmul32(u32_t x, u32_t y)
{	u32_t res;
	
	__asm
	{	mov		eax,[x]
		mul		[y]
		div		[modulo32]
		mov		res,edx
	}
	return res;
}

#ifdef HAVE_CMOV

static inline u32_t modadd32(u32_t x, u32_t y)
{	u32_t res;
	
	__asm
	{
		xor		edx,edx
		mov		ecx,[y]
		add		ecx,[x]
		cmovc	edx,[modulo32]
	    cmp		ecx,[modulo32]
		cmovae	edx,[modulo32]
		sub		ecx,edx
		mov		[res],ecx
	}
	return res;
}

static inline u32_t modsub32(u32_t subtrahend, u32_t minuend)
{	u32_t res;
  
	__asm
	{	xor		edx,edx
		mov		ecx,[subtrahend]
		sub		ecx,[minuend]
		cmovb	edx,[modulo32]
		add		ecx,edx
		mov		[res],ecx
	}
	return res;
}

#else

static inline u32_t modadd32(u32_t x,u32_t y)
{	u32_t res;
	
	__asm
	{	
		mov		ecx,[x]
		add		ecx,[y]
		jc		l1f
		cmp		ecx,[modulo32]
		jb		l2f
l1f:	sub		ecx,[modulo32]
l2f:	mov		[res],ecx
	}
	return res;
}

static inline u32_t modsub32(u32_t subtrahend,u32_t minuend)
{	u32_t res;

	__asm
	{
		mov		ecx,[subtrahend]
		sub		ecx,[minuend]
		jae		l1f
		addl	ecx,[modulo32]
l1f:	mov		[res],ecx
	}
	return res;
}

#endif

#else

/* 32bit.h */
volatile extern u32_t modulo32 NAME("modulo32");
u32_t asm_modinv32(u32_t x) NAME("asm_modinv32");

#define modinv32(x) asm_modinv32(x)

static inline u32_t modsq32(u32_t x)
{ u32_t res,clobber;
  __asm__ volatile ("mull %%eax\n"
	   "divl modulo32" : "=d" (res), "=a" (clobber) : "a" (x) : "cc" );
  return res;
}

static inline u32_t modmul32(u32_t x,u32_t y)
{ u32_t res,clobber;
  __asm__ volatile ("mull %%ecx\n"
	   "divl modulo32" : "=d" (res), "=a" (clobber) : "a" (x), "c" (y) :
	   "cc");
  return res;
}

static inline u32_t modadd32(u32_t x,u32_t y)
{ u32_t res;
#ifdef HAVE_CMOV
  __asm__ volatile ("xorl %%edx,%%edx\n"
	   "addl %%eax,%%ecx\n"
	   "cmovc modulo32,%%edx\n"
	   "cmpl modulo32,%%ecx\n"
	   "cmovae modulo32,%%edx\n"
	   "subl %%edx,%%ecx\n"
	   "2:\n" : "=c" (res) : "a" (x), "c" (y) : "%edx", "cc");
#else
  __asm__ volatile ("addl %%eax,%%ecx\n"
	   "jc 1f\n"
	   "cmpl modulo32,%%ecx\n"
	   "jb 2f\n"
	   "1:\n"
	   "subl modulo32,%%ecx\n"
	   "2:\n" : "=c" (res) : "a" (x), "c" (y) : "cc");
#endif
  return res;
}

static inline u32_t modsub32(u32_t subtrahend,u32_t minuend)
{ u32_t res;
#ifdef HAVE_CMOV
  __asm__ volatile ("xorl %%edx,%%edx\n"
	   "subl %%eax,%%ecx\n"
	   "cmovbl modulo32,%%edx\n"
	   "addl %%edx,%%ecx\n"
	   "1:" : "=c" (res) : "a" (minuend), "c" (subtrahend) : "%edx", "cc");
#else
  __asm__ volatile ("subl %%eax,%%ecx\n"
	   "jae 1f\n"
	   "addl modulo32,%%ecx\n"
	   "1:" : "=c" (res) : "a" (minuend), "c" (subtrahend) : "cc" );
#endif
  return res;
}

#endif

/* lasched.h */
u32_t*lasched(u32_t*,u32_t*,u32_t*,u32_t,u32_t**,u32_t,u32_t);

/* medsched.h */
u32_t*medsched(u32_t*,u32_t*,u32_t*,u32_t**,u32_t,u32_t);

/* montgomery_mul.h */
#define NMAX_ULONGS   4

/*  montgomery_mul.c */
void init_montgomery_multiplication();
int set_montgomery_multiplication(mpz_t n);

/* sieve-from-sched.S */
int schedsieve(unsigned char x, unsigned char *sieve_interval, 
               u16_t *schedule_ptr, u16_t *sptr_ub) NAME("schedsieve");

/* basemath.c */
int asm_cmp(unsigned long * a, unsigned long * b);
int asm_cmp64(unsigned long * a, unsigned long * b);

/* gcd32.c */
u32_t gcd32(u32_t x, u32_t y);

/* psp.c */
int psp(mpz_t n);

/* mpqs.c */
long mpqs_factor(mpz_t N, long max_bits, mpz_t ** factors);
void complain(char *fmt, ...);

/* ri-aux.s */
unsigned long asm_getbc(u32_t r, u32_t p, u32_t A, u32_t *b, u32_t *s, u32_t *c, u32_t *t) NAME("asm_getbc");

/* mpqs_sieve.s */
int asm_sieve() NAME("asm_sieve");

/* mpqs_eval.s */
unsigned short asm_evaluate(unsigned long *, unsigned long *, unsigned short *, long) NAME("asm_evaluate");

/* mpqs_td.s */
int asm_td(unsigned short *, unsigned short, uint64_t*, unsigned long*) NAME("asm_td");

/* misc arithmetic files: */

#if !defined(_MSC_VER) || defined(__MINGW32__)

extern void asm_mulm64(unsigned long *, unsigned long *, unsigned long *) NAME("asm_mulm64");
extern void asm_mulm96(unsigned long *, unsigned long *, unsigned long *) NAME("asm_mulm96");
extern void asm_mulm128(unsigned long *, unsigned long *, unsigned long *) NAME("asm_mulm128");

extern void asm_sqm64(unsigned long *, unsigned long *) NAME("asm_sqm64");
extern void asm_sqm96(unsigned long *, unsigned long *) NAME("asm_sqm96");
extern void asm_sqm128(unsigned long *, unsigned long *) NAME("asm_sqm128");

extern void asm_add64(unsigned long *, unsigned long *) NAME("asm_add64");
extern void asm_add96(unsigned long *, unsigned long *) NAME("asm_add96");
extern void asm_add128(unsigned long *, unsigned long *) NAME("asm_add128");

extern void asm_diff64(unsigned long *, unsigned long *, unsigned long *) NAME("asm_diff64");
extern void asm_diff96(unsigned long *, unsigned long *, unsigned long *) NAME("asm_diff96");
extern void asm_diff128(unsigned long *, unsigned long *, unsigned long *) NAME("asm_diff128");

extern void asm_sub64(unsigned long *, unsigned long *, unsigned long *) NAME("asm_sub64");
extern void asm_sub96(unsigned long *, unsigned long *, unsigned long *) NAME("asm_sub96");
extern void asm_sub128(unsigned long *, unsigned long *, unsigned long *) NAME("asm_sub128");

extern void asm_add64_ui(unsigned long *, unsigned long) NAME("asm_add64_ui");
extern void asm_add96_ui(unsigned long *, unsigned long) NAME("asm_add96_ui");
extern void asm_add128_ui(unsigned long *, unsigned long) NAME("asm_add128_ui");

extern void asm_zero64(unsigned long *, unsigned long *) NAME("asm_zero64");
extern void asm_zero96(unsigned long *, unsigned long *) NAME("asm_zero96");
extern void asm_zero128(unsigned long *, unsigned long *) NAME("asm_zero128");

extern void asm_copy64(unsigned long *, unsigned long *) NAME("asm_copy64");
extern void asm_copy96(unsigned long *, unsigned long *) NAME("asm_copy96");
extern void asm_copy128(unsigned long *, unsigned long *) NAME("asm_copy128");

extern void asm_sub_n64(unsigned long *, unsigned long *) NAME("asm_sub_n64");
extern void asm_sub_n96(unsigned long *, unsigned long *) NAME("asm_sub_n96");
extern void asm_sub_n128(unsigned long *, unsigned long *) NAME("asm_sub_n128");

extern void asm_half64(unsigned long *) NAME("asm_half64");
extern void asm_half96(unsigned long *) NAME("asm_half96");
extern void asm_half128(unsigned long *) NAME("asm_half128");

#else

#define ASM_SUBS_DECLARED

extern void asm_mulm64(unsigned long *, unsigned long *, unsigned long *);
extern void asm_mulm96(unsigned long *, unsigned long *, unsigned long *);
extern void asm_mulm128(unsigned long *, unsigned long *, unsigned long *);

extern void asm_sqm64(unsigned long *, unsigned long *);
extern void asm_sqm96(unsigned long *, unsigned long *);
extern void asm_sqm128(unsigned long *, unsigned long *);

extern void asm_add64(unsigned long *, unsigned long *);
extern void asm_add96(unsigned long *, unsigned long *);
extern void asm_add128(unsigned long *, unsigned long *);

extern void asm_diff64(unsigned long *, unsigned long *, unsigned long *);
extern void asm_diff96(unsigned long *, unsigned long *, unsigned long *);
extern void asm_diff128(unsigned long *, unsigned long *, unsigned long *);

extern void asm_sub64(unsigned long *, unsigned long *, unsigned long *);
extern void asm_sub96(unsigned long *, unsigned long *, unsigned long *);
extern void asm_sub128(unsigned long *, unsigned long *, unsigned long *);

extern void asm_add64_ui(unsigned long *, unsigned long);
extern void asm_add96_ui(unsigned long *, unsigned long);
extern void asm_add128_ui(unsigned long *, unsigned long);

extern void asm_zero64(unsigned long *, unsigned long *);
extern void asm_zero96(unsigned long *, unsigned long *);
extern void asm_zero128(unsigned long *, unsigned long *);

extern void asm_copy64(unsigned long *, unsigned long *);
extern void asm_copy96(unsigned long *, unsigned long *);
extern void asm_copy128(unsigned long *, unsigned long *);

extern void asm_sub_n64(unsigned long *, unsigned long *);
extern void asm_sub_n96(unsigned long *, unsigned long *);
extern void asm_sub_n128(unsigned long *, unsigned long *);

extern void asm_half64(unsigned long *);
extern void asm_half96(unsigned long *);
extern void asm_half128(unsigned long *);

#endif

#endif