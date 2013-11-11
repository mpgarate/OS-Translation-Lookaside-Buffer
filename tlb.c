#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "tlb.h"
#include "cpu.h"
#include "mmu.h"

/* Macro to print verbose statements and flush stdout */
#ifdef verbose
  #define v(fmt)        SAY0(fmt)
  #define v0(fmt)         {printf(fmt); fflush(stdout);}
  #define v1(fmt,parm1)     {printf(fmt,parm1); fflush(stdout);}
  #define v2(fmt,parm1,parm2)   {printf(fmt,parm1,parm2); fflush(stdout);}
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


/*************************************/
/***** Use masks to get values *******/
/*************************************/
/* vbit_and_vpage;  // 32 bits containing the valid bit and the 20bit
                                // virtual page number.
   mr_pframe;       // 32 bits containing the modified bit, reference bit,
                                // and 20-bit page frame number
*/
VPAGE_NUMBER get_vpage_number(TLB_ENTRY entry){
  return entry.vbit_and_vpage & VPAGE_MASK;
}

PAGEFRAME_NUMBER get_pageframe_number(TLB_ENTRY entry){
  return entry.mr_pframe & PFRAME_MASK;
}

int get_valid_bit(TLB_ENTRY entry){
  //SAY1("Getting valid bit: %d\n", entry.vbit_and_vpage & VBIT_MASK);
  return entry.vbit_and_vpage & VBIT_MASK;
}

int get_r_bit(TLB_ENTRY entry){
  return entry.mr_pframe & RBIT_MASK;
}

int get_m_bit(TLB_ENTRY entry){
  return entry.mr_pframe & MBIT_MASK;
}

void set_foo_bit(unsigned int foo, BOOL value, int mask){
  if(value){
    foo = foo | mask;
  }
  else{
    foo = foo & ~mask;
  }
}

void set_valid_bit(TLB_ENTRY entry, BOOL value){
  SAY1("Setting valid bit to %x \n", value);
  //SAY1("Entry contained: %x \n",  entry.vbit_and_vpage & VBIT_MASK);
  //SAY1("My attempt: %x \n",  entry.vbit_and_vpage | VBIT_MASK);
  //set_foo_bit(entry.vbit_and_vpage, value, VBIT_MASK);
  if (value){
    entry.vbit_and_vpage = entry.vbit_and_vpage | VBIT_MASK;
  }
  else {
    entry.vbit_and_vpage = entry.vbit_and_vpage & ~VBIT_MASK;
  }
  //SAY1("Valid bit is now %x \n", entry.vbit_and_vpage & VBIT_MASK);
}

void set_r_bit(TLB_ENTRY entry, BOOL r_bit){
  if (r_bit){
    entry.mr_pframe = entry.mr_pframe | RBIT_MASK;
  }
  else{
    entry.mr_pframe = entry.mr_pframe & ~RBIT_MASK;
  }
}

void set_m_bit(TLB_ENTRY entry, BOOL m_bit){
  if (m_bit){
    entry.mr_pframe = entry.mr_pframe | MBIT_MASK;
  }
  else{
    entry.mr_pframe = entry.mr_pframe & ~MBIT_MASK;
  }
}
void set_vpage(TLB_ENTRY entry, VPAGE_NUMBER vpage){
  entry.vbit_and_vpage = entry.vbit_and_vpage | VPAGE_MASK;
}

void set_pageframe(TLB_ENTRY entry, PAGEFRAME_NUMBER pf_number){

}



// Initialize the TLB (called by the mmu)
void tlb_initialize()
{
  //Here's how you can allocate a TLB of the right size
  tlb = (TLB_ENTRY *) malloc(num_tlb_entries * sizeof(TLB_ENTRY));

  //This is the mask to perform a MOD operation (see above)
  //If num_tlb_entries is 128 = 1000 0000
  //Then mask looks like:       0999 9999
  mod_tlb_entries_mask = num_tlb_entries - 1;  

  //Fill in rest here...

}

void clear_valid_bit(TLB_ENTRY entry){
  if(get_valid_bit(entry)){
    entry.vbit_and_vpage & VBIT_MASK;
  }
}

// This clears out the entire TLB, by clearing the
// valid bit for every entry.
void tlb_clear_all() 
{
  int i;
  for (i = 0; i<num_tlb_entries; i++){
    clear_valid_bit(tlb[i]);
  }
}


//clears all the R bits in the TLB
void tlb_clear_all_R_bits() 
{
  // FILL THIS IN
}

// This clears out the entry in the TLB for the specified
// virtual page, by clearing the valid bit for that entry.
void tlb_clear_entry(VPAGE_NUMBER vpage) {
  // FILL THIS IN
}


int find_by_vpage_number(VPAGE_NUMBER vpage){
  int i;
  for (i = 0; i < num_tlb_entries; i++){
    if(get_valid_bit(tlb[i])){
      SAY2("Checking %d against %d \n", vpage, get_vpage_number(tlb[i]));
      if (get_vpage_number(tlb[i]) == vpage) return i;
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
  tlb_miss = TRUE;
  int entry_index = find_by_vpage_number(vpage);
  if (entry_index <= num_tlb_entries){
    TLB_ENTRY entry = tlb[entry_index]; 
    tlb_miss = FALSE;
    set_r_bit(entry,TRUE);
    if (op == STORE) set_m_bit(entry,TRUE);
    SAY1("tlb_lookup returning %d \n", get_pageframe_number(entry));
    tlb_miss = FALSE;
    return get_pageframe_number(entry);
  }
}

void write_entry_to_mmu(TLB_ENTRY entry){
  mmu_modify_mbit_bitmap(get_pageframe_number(entry), get_m_bit(entry));
  mmu_modify_rbit_bitmap(get_pageframe_number(entry), get_r_bit(entry));
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
  

int clock_hand = 0;  // points to next TLB entry to consider evicting


void tlb_insert(VPAGE_NUMBER new_vpage,
                PAGEFRAME_NUMBER new_pframe,
                BOOL new_mbit,
                BOOL new_rbit) {

  int i = clock_hand;
  TLB_ENTRY entry = tlb[i];
  do {
    entry = tlb[i];
    //SAY1("tlb_insert is checking entry %d",i);
    if (!get_valid_bit(entry) || !get_r_bit(entry)){
      //SAY1("tlb_insert found entry %d",i);
      break;
    }
    else{
      /* Increment and loop */
      i = (i + 1) % num_tlb_entries;
    }
  } while (i != clock_hand);

  if (get_valid_bit(entry)){
    write_entry_to_mmu(entry);
  }

  if (!get_valid_bit(entry)){
    set_vpage(entry, new_vpage);
    set_pageframe(entry, new_pframe);
    set_m_bit(entry, new_mbit);
    set_r_bit(entry, new_rbit);
    set_valid_bit(entry,TRUE);
  }
  clock_hand = (i + 1) % num_tlb_entries;
}


//Writes the M  & R bits in the each valid TLB
//entry back to the M & R MMU bitmaps.
void tlb_write_back()
{
  //FILL THIS IN
}

