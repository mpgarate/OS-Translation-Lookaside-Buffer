#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "tlb.h"
#include "cpu.h"
#include "mmu.h"

/* Macro to print verbose statements and flush stdout */
#ifdef verbose
  #define v(fmt)        v0(fmt)
  #define v0(fmt)         {printf(fmt); fflush(stdout);}
  #define v1(fmt,parm1)     {printf(fmt,parm1); fflush(stdout);}
  #define v2(fmt,parm1,parm2)   {printf(fmt,parm1,parm2); fflush(stdout);}
  #define v2(fmt,parm1,parm2,param3)   {printf(fmt,parm1,parm2,param3); fflush(stdout);}
#endif

/* Set this to 1 to print out debug statements */
#define DEBUG 1

/* Macro to print verbose statements and flush stdout */
#ifdef DEBUG
  #define SAY(fmt)        SAY0(fmt)
  #define SAY0(fmt)         {printf(fmt); fflush(stdout);}
  #define SAY1(fmt,parm1)     {printf(fmt,parm1); fflush(stdout);}
  #define SAY2(fmt,parm1,parm2)   {printf(fmt,parm1,parm2); fflush(stdout);}
#endif

#define ASSERT(result, name) {if(!result) {SAY1("Assertion %s failed.\n", name); exit(0);}}

/* This is some of the code that I wrote. You may use any of this code
   you like, but you certainly don't have to.
*/

/* I defined the TLB as an array of entries,
   each containing the following:
   Valid bit: 1 bit
   Virtual Page: 20 bits
   Modified bit: 1 bit
   Reference bit: 1 bit
   Page Frame: 20 bits (of which only 18 are meaningful given 1GB RAM)
*/

//You can use a struct to get a two-word entry.
typedef struct {
  unsigned int vbit_and_vpage;  // 32 bits containing the valid bit and the 20bit
                                // virtual page number.
  unsigned int mr_pframe;       // 32 bits containing the modified bit, reference bit,
                                // and 20-bit page frame number
} TLB_ENTRY;


// This is the actual TLB array. It should be dynamically allocated
// to the right size, depending on the num_tlb_entries value 
// assigned when the simulation started running.

TLB_ENTRY *tlb;  

// This is the TLB size (number of TLB entries) chosen by the 
// user. 

unsigned int num_tlb_entries;

  //Since the TLB size is a power of 2, I recommend setting a
  //mask to perform the MOD operation (which you will need to do
  //for your TLB entry evicition algorithm, see below).
unsigned int mod_tlb_entries_mask;

//this must be set to TRUE when there is a tlb miss, FALSE otherwise.
BOOL tlb_miss; 


//If you choose to use the same representation of a TLB
//entry that I did, then these are masks that can be used to 
//select the various fields of a TLB entry.

#define VBIT_MASK   0x80000000  //VBIT is leftmost bit of first word
#define VPAGE_MASK  0x000FFFFF            //lowest 20 bits of first word
#define RBIT_MASK   0x80000000  //RIT is leftmost bit of second word
#define MBIT_MASK   0x40000000  //MBIT is second leftmost bit of second word
#define PFRAME_MASK 0x000FFFFF            //lowest 20 bits of second word

#define LAST_BIT_OFFSET 31;
#define M_BIT_OFFSET 30;

/*************************************/
/***** Use masks to get values *******/
/*************************************/
/* vbit_and_vpage;  // 32 bits containing the valid bit and the 20bit
                                // virtual page number.
   mr_pframe;       // 32 bits containing the modified bit, reference bit,
                                // and 20-bit page frame number
*/
VPAGE_NUMBER get_vpage_number(int i){
  return tlb[i].vbit_and_vpage & VPAGE_MASK;
}

PAGEFRAME_NUMBER get_pageframe_number(int i){
  return tlb[i].mr_pframe & PFRAME_MASK;
}

int get_valid_bit(int i){
  return (tlb[i].vbit_and_vpage & VBIT_MASK) >> LAST_BIT_OFFSET;
}

int get_r_bit(int i){
  return (tlb[i].mr_pframe & RBIT_MASK) >> LAST_BIT_OFFSET;
}

int get_m_bit(int i){
  return (tlb[i].mr_pframe & MBIT_MASK) >> M_BIT_OFFSET;
}

void set_foo_bit(int i, BOOL value, int mask){
  if(value){
    tlb[i].mr_pframe = tlb[i].mr_pframe | mask;
  }
  else{
    tlb[i].mr_pframe = tlb[i].mr_pframe & ~mask;
  }
}

void set_valid_bit(int i, BOOL value){
  if (value == TRUE) tlb[i].vbit_and_vpage = tlb[i].vbit_and_vpage | VBIT_MASK;
  if (value == FALSE) tlb[i].vbit_and_vpage = tlb[i].vbit_and_vpage & ~VBIT_MASK;
  ASSERT((get_valid_bit(i) == value), "set_valid_bit");
}

void set_r_bit(int i, BOOL r_bit){
  set_foo_bit(i, r_bit, RBIT_MASK);
  ASSERT((get_r_bit(i) == r_bit), "set_r_bit");
}

void set_m_bit(int i, BOOL m_bit){
  set_foo_bit(i, m_bit, MBIT_MASK);
  ASSERT((get_m_bit(i) == m_bit), "set_m_bit");
}
void set_vpage(int i, VPAGE_NUMBER vpage){
  unsigned int masked_vpage = vpage & VPAGE_MASK;
  tlb[i].vbit_and_vpage = tlb[i].vbit_and_vpage & ~VPAGE_MASK;
  tlb[i].vbit_and_vpage = tlb[i].vbit_and_vpage | masked_vpage;
  ASSERT((get_vpage_number(i) == vpage), "set_vpage");
}

void set_pageframe(int i, PAGEFRAME_NUMBER pf_number){
  unsigned int masked_pfn = pf_number & PFRAME_MASK;
  tlb[i].mr_pframe = tlb[i].mr_pframe & ~PFRAME_MASK;
  tlb[i].mr_pframe = tlb[i].mr_pframe | masked_pfn;
  ASSERT((get_pageframe_number(i) == pf_number), "set_pageframe");
}

void clear_valid_bit(int i){
  if(get_valid_bit(i)){
    tlb[i].vbit_and_vpage = tlb[i].vbit_and_vpage & ~VBIT_MASK;
  }
  ASSERT((get_valid_bit(i) == 0), "clear_valid_bit");
}
void clear_r_bit(int i){
  if(get_valid_bit(i)){
    tlb[i].mr_pframe = tlb[i].mr_pframe & ~RBIT_MASK;
  }
  ASSERT((get_r_bit(i) == 0), "clear_r_bit");
}



void print_entry(int i){
  SAY1("%x        ",i);
  SAY1("%x          ",get_valid_bit(i));
  SAY1("%x       ",get_vpage_number(i));
  SAY1("%x         ",get_m_bit(i));
  SAY1("%x         ",get_r_bit(i));
  SAY1("%x          ",get_pageframe_number(i));
  SAY("\n");
}

void print_table(){
  SAY("---------------   PRINTING TABLE   ------------------\n");
  SAY("-----------------------------------------------------\n");
  SAY("PID --- VALID --- VP_NO --- MOD --- REF --- PF_NO ---\n");
  int i = 0;
  for (i = 0; i< num_tlb_entries; i++){
    if (get_valid_bit(i)) print_entry(i);
  }
  SAY("-----------------------------------------------------\n");
}

// Initialize the TLB (called by the mmu)
void tlb_initialize()
{
  //Here's how you can allocate a TLB of the right size
  tlb = (TLB_ENTRY *) malloc(num_tlb_entries * sizeof(TLB_ENTRY));

  //This is the mask to perform a MOD operation (see above)
  mod_tlb_entries_mask = num_tlb_entries - 1;  

  //Fill in rest here...
  tlb_clear_all();
}


// This clears out the entire TLB, by clearing the
// valid bit for every entry.
void tlb_clear_all() 
{
  int i;
  for (i = 0; i<num_tlb_entries; i++){
    clear_valid_bit(i);
  }
}


//clears all the R bits in the TLB
void tlb_clear_all_R_bits() 
{
  int i;
  for (i = 0; i<num_tlb_entries+1; i++){
    clear_r_bit(i);
  }
}

// This clears out the entry in the TLB for the specified
// virtual page, by clearing the valid bit for that entry.
void tlb_clear_entry(VPAGE_NUMBER vpage) {
  int i = find_by_vpage_number(vpage);
  clear_valid_bit(i);
}


int find_by_vpage_number(VPAGE_NUMBER vpage){
  int i;
  for (i = 0; i < num_tlb_entries; i++){
    if(get_valid_bit(i)){
      if (get_vpage_number(i) == vpage) return i;
    }
  }
  return num_tlb_entries + 1; //impossible index
}


// Returns a page frame number if there is a TLB hit. If there is a TLB
// miss, then it sets tlb_miss (see above) to TRUE.  It sets the R
// bit of the entry and, if the specified operation is a STORE,
// sets the M bit.

PAGEFRAME_NUMBER tlb_lookup(VPAGE_NUMBER vpage, OPERATION op)
{
  int i = find_by_vpage_number(vpage);

  // Check if the index is within bounds. A value out of bounds means that the tlb entry
  // could not be located.
  if (i <= num_tlb_entries){
    tlb_miss = FALSE;
    set_r_bit(i,TRUE);
    if (op == STORE) set_m_bit(i,TRUE);
    return get_pageframe_number(i);
  }
  tlb_miss = TRUE;
}



// Uses an NRU clock algorithm, where the first entry with
// either a cleared valid bit or cleared R bit is chosen.

// Starting at the clock_hand'th entry, find first entry to
// evict with either valid bit  = 0 or the R bit = 0. If there
// is no such entry, then just evict the entry pointed to by
// the clock hand.

// Then, if the entry to evict has a valid bit = 1,
// write the M and R bits of the of entry back to the M and R
// bitmaps, respectively, in the MMU (see mmu_modify_rbit_bitmap, etc.
// in mmu.h)

// Then, insert the new vpage, pageframe, M bit, and R bit into the
// TLB entry that was just found (and possibly evicted).

// Finally, set clock_hand to point to the next entry after the
// entry found above.
  
void write_entry_to_mmu(i){
  mmu_modify_mbit_bitmap(get_pageframe_number(i), get_m_bit(i));
  mmu_modify_rbit_bitmap(get_pageframe_number(i), get_r_bit(i));
}

int clock_hand = 0;  // points to next TLB entry to consider evicting


void tlb_insert(VPAGE_NUMBER new_vpage,
                PAGEFRAME_NUMBER new_pframe,
                BOOL new_mbit,
                BOOL new_rbit)
{
  int i = clock_hand;
  do {
    if ((get_valid_bit(i) == 0) || (get_r_bit(i) == 0)){
      //SAY1("got index %d\n", i);
      break;
    }
    else{
      /* Increment and loop */
      i = (i + 1) % num_tlb_entries;
    }
    //SAY("didn't find an index\n");
  } while (i != clock_hand);

  if (get_valid_bit(i)) {
    write_entry_to_mmu(i);
    if (verbose) {
      printf("Evicting TLB entry, slot = %d, for pageframe %x. M bit = %d\n",i,new_pframe,new_mbit);
      //print_table();
      //SAY1("clock_hand is: %d\n", clock_hand);
    }
  }
  set_vpage(i, new_vpage);
  set_pageframe(i, new_pframe);
  set_m_bit(i, new_mbit);
  set_r_bit(i, new_rbit);
  set_valid_bit(i, TRUE);

  clock_hand = (i + 1) % num_tlb_entries;
}

//Writes the M & R bits in the each valid TLB
//entry back to the M & R MMU bitmaps.
void tlb_write_back()
{
  int i = 0;
  for (i = 0; i< num_tlb_entries; i++){
    if (get_valid_bit(i)) write_entry_to_mmu(i);
  }
}
