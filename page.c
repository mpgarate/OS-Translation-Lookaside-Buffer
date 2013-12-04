#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "mmu.h"
#include "page.h"
#include "cpu.h"

/* Set this to 1 to print out debug statements */
#define DEBUG 1

/* Macro to print verbose statements and flush stdout */
#ifdef DEBUG
  #define SAY(fmt)        SAY0(fmt)
  #define SAY0(fmt)         {printf(fmt); fflush(stdout);}
  #define SAY1(fmt,parm1)     {printf(fmt,parm1); fflush(stdout);}
  #define SAY2(fmt,parm1,parm2)   {printf(fmt,parm1,parm2); fflush(stdout);}
#endif


/* The following machine parameters are being used:
   
   Number of bits in an address:  32
   Page size: 4KB

   Page Table Type:  2 level page table
   Size of first level page table: 1024 entries
   Size of first level Page Table Entry:  32 bits
   Size of each 2nd Level Page Table: 1024 entries
   Size of 2nd Level Page Table Entry: 32 bits

   Bits of address giving index into first level page table: 10
   Bits of address giving index into second level page table: 10
   Bits of address giving offset into page: 12

*/

#define ADDRESS_SIZE 32
#define PAGE_SIZE 1<<12 //4KB
#define TABLE_ENTRIES 1024
#define ENTRY_SIZE 32

#define INDEX_MASK_L1 0x000008FF
#define INDEX_MASK_L2 0x000FF800
#define INDEX_L2_SHIFT 10


/* Each entry of a 2nd level page table has
   the following:
     Present/Absent bit: 1 bit
     Page Frame: 20 bits
*/


// This is the type definition for the 
// an entry in a second level page table

typedef unsigned int PT_ENTRY;


// This is declaration of the variable representing
// the first level page table

PT_ENTRY **first_level_page_table;


#define PRESENT_BIT_MASK   0x80000000
#define PF_NUMBER_MASK     0x000FFFFF


#define get_L1_index(vpage) (vpage & INDEX_MASK_L1)
#define get_L2_index(vpage) ((vpage & INDEX_MASK_L2) << INDEX_L2_SHIFT)

#define get_pf_number(entry) (entry & PF_NUMBER_MASK)

void clear_L1_entry(int i){
  if (i < TABLE_ENTRIES) first_level_page_table[i] = NULL;
}

void clear_L1_page_table(){
  int i = 0;
  for (i=0;i<TABLE_ENTRIES;i++){
    clear_L1_entry(i);
  }
}

// This sets up the initial page table. The function
// is called by the MMU.
//
// Initially, all the entries of the first level 
// page table should be set to NULL. Later on, 
// when a new page is referenced by the CPU, the 
// second level page table for storing the entry
// for that new page should be created if it doesn't
// exist already.

void pt_initialize_page_table()
{
  first_level_page_table = malloc(TABLE_ENTRIES*ENTRY_SIZE);
  clear_L1_page_table();
}

// for performing DIV by 1024 to index into the
// first level page table
#define DIV_FIRST_PT_SHIFT 10  

// for performing MOD  1024 to index in a 
// second level page table
#define MOD_SECOND_PT_MASK 0x3FF


PAGEFRAME_NUMBER find_pf_number(int L1_index, int L2_index){
  PT_ENTRY* table_L2 = first_level_page_table[L1_index];
  if(table_L2 == NULL) return -1;
  PT_ENTRY entry = table_L2[L2_index];
  PAGEFRAME_NUMBER pf_number = get_pf_number(entry);
  if (pf_number == 0) return -1;
  return pf_number;
}

BOOL page_fault;  //set to true if there is a page fault

//This is called when there is a TLB_miss.
// Using the page table, this looks up the page frame 
// corresponding to the specified virtual page.
// If the desired page is not present, the variable page_fault
// should be set to TRUE (otherwise FALSE).
PAGEFRAME_NUMBER pt_get_pageframe(VPAGE_NUMBER vpage)
{

  int L1_index = get_L1_index(vpage);
  int L2_index = get_L2_index(vpage);

  PAGEFRAME_NUMBER pf_number = find_pf_number(L1_index,L2_index);
  if (pf_number == -1) {
    page_fault = TRUE;
  }
  else{
    page_fault = FALSE;
    return pf_number;
  }
}



// This inserts into the page table an entry mapping of the 
// the specified virtual page to the specified page frame.
// It might require the creation of a second-level page table
// to hold the entry, if it doesn't already exist.
void pt_update_pagetable(VPAGE_NUMBER vpage, PAGEFRAME_NUMBER pframe)
{


  // FILL THIS IN

  //don't forget to set the present bit for the new entry
}


// This clears a page table entry by clearing its present bit.
// It is called by the OS (in kernel.c) when a page is evicted
// from a page frame.
void pt_clear_page_table_entry(VPAGE_NUMBER vpage)
{
  // Fill this in
}


