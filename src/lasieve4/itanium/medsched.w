@* Scheduling function for the lattice siever.
Copyright (C) 2002 Jens Franke, T. Kleinjung.
This file is part of gnfs4linux, distributed under the terms of the 
GNU General Public Licence and WITHOUT ANY WARRANTY.

You should have received a copy of the GNU General Public License along
with this program; see the file COPYING.  If not, write to the Free
Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
@(medsched.h@>=
u16_t *medsched(u16_t*,u32_t*,u32_t*,u32_t**,u32_t,u32_t);

@
@c
#include <sys/types.h>
#include <limits.h>

#include "siever-config.h"
#include "medsched.h"
#include "../if.h"

#define L1_SIZE (1<<L1_BITS)
#define i_bits (I_bits-1)
#define n_i (1<<i_bits)

#define U16_SHIFT (CHAR_BIT*sizeof(u16_t))

u16_t *
medsched(ri,ij_ptr,ij_ptr_ub,sched_ptr,fbi_offs,ot)
     u16_t *ri;
     u32_t *ij_ptr,*ij_ptr_ub,**sched_ptr,fbi_offs,ot;
{
  u32_t ij;
  u32_t ot_mask,ot_tester;
  u32_t *ru,*sched;

  ru=(u32_t*)ri;
  sched=*sched_ptr;

  if(ot!=0) {
    ot_tester=(ot&1)|((ot&2)<<(i_bits-1));
    ot_mask=n_i|1;
  }

  while ( ij_ptr < ij_ptr_ub) {
    u16_t a,b;

    a=n_i-(ru[0]&(n_i-1));
    b=n_i-(ru[1]&(n_i-1));
    if(ot == 0) ij=*ij_ptr;
    else @<Calculate first sieving event from |ri|@>@;
    while(ij<L1_SIZE) {
      u16_t i;

      *(sched++)=(fbi_offs<<U16_SHIFT)|ij;
      i=ij&(n_i-1);
      if(i<b) ij+=ru[0];
      if(i>=a) ij+=ru[1];
    }
    ru+=2;
    *(ij_ptr++)=ij-L1_SIZE;
    fbi_offs++;
  }
  *sched_ptr=sched;
  return ru;
}

@
@<Calculate first sieving event from |ri|@>=
{
  ij=0;
  if( (ru[0]&ot_mask) == ot_tester ) ij=ru[0];
  else {
    if( (ru[1]&ot_mask) == (ot_tester^n_i) ) ij=ru[1];
    else {
      if((ru[0]&(n_i-1))<=(ru[1]&(n_i-1)) && ru[0]<=ru[1]) {
	/* This corresponds to the line
	   |if(b+c<=A && s<=t)| in recurrence6.w */
	if((ru[0]&(n_i-1))==(ru[1]&(n_i-1))) ij=ru[1]-ru[0];
	else ij=n_i;
	if(ot != 2)
	  Schlendrian("Exceptional situation for oddness type %u ?\n",
		      ot);
      }
      else ij=ru[0]+ru[1];
    }
  }
  ij=(ij+((~ot_tester)&n_i))/2;
}
