#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "mmu.h"
#include "page.h"
#include "cpu.h"

/* Set this to 1 to print out debug statements */
#define DEBUG 0

/* Macro to print verbose statements and flush stdout */
#ifdef DEBUG
  #define SAY(fmt)        SAY0(fmt)
  #define SAY0(fmt)         {printf(fmt); fflush(stdout);}
  #define SAY1(fmt,parm1)     {printf(fmt,parm1); fflush(stdout);}
  #define SAY2(fmt,parm1,parm2)   {printf(fmt,parm1,parm2); fflush(stdout);}
#endif

#ifdef verbose
  #define V(fmt)        V0(fmt)
  #define V0(fmt)         {printf(fmt); fflush(stdout);}
  #define V1(fmt,parm1)     {printf(fmt,parm1); fflush(stdout);}
  #define V2(fmt,parm1,parm2)   {printf(fmt,parm1,parm2); fflush(stdout);}
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

// #define ADDRESS_SIZE 32
// #define PAGE_SIZE 1<<12     //4KB
   
#define TABLE_ENTRIES 1024  // same for L1 and L2
#define ENTRY_SIZE 32       // same for L1 and L2

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


// for performing DIV by 1024 to index into the
// first level page table
#define DIV_FIRST_PT_SHIFT 10  

// for performing MOD 1024 to index in a 
// second level page table
#define MOD_SECOND_PT_MASK 0x3FF


#define PRESENT_BIT_MASK   0x80000000
#define PF_NUMBER_MASK     0x000FFFFF
#define PRESENT_BIT_SHIFT  31

/* Example: 0x3FF -> 0011 1111 1111 */

#define get_L1_index(vpage) (vpage >> DIV_FIRST_PT_SHIFT)
#define get_L2_index(vpage) (vpage & MOD_SECOND_PT_MASK)
#define get_pf_number(entry) (entry & PF_NUMBER_MASK)
#define get_present_bit(entry) ((entry & PRESENT_BIT_MASK) >> PRESENT_BIT_SHIFT)

void clear_L1_page_table(){
  int i = 0;
  for (i=0;i<TABLE_ENTRIES;i++){
    first_level_page_table[i] = NULL; //clear entry i
  }
}


void clear_L2_page_table(PT_ENTRY* table_L2){
  int i = 0;
  for (i=0; i<TABLE_ENTRIES;i++){
    table_L2[i] = 0; //clear L2 entry i
  }
}

void print_entry(int i, int j, PT_ENTRY* table_L2){
  if (get_present_bit(table_L2[j]))
    SAY2("[%d]-%x\n",j,table_L2[j]);
}

void print_all_entries(){
  int i = 0;
  int j = 0;
  PT_ENTRY* table_L2;
  for (i = 0; i< TABLE_ENTRIES; i++){
    table_L2 = first_level_page_table[i];
    if (table_L2 != NULL) {
      SAY1("Printing L1 %d\n",i);
      SAY("-----------------------------------------\n");
      for (j = 0; j<TABLE_ENTRIES; j++){
        print_entry(i,j,table_L2);
      }
      SAY("-----------------------------------------\n");
    }
  }
  exit(0);
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

PAGEFRAME_NUMBER find_pf_number(int L1_index, int L2_index){
  PT_ENTRY* table_L2 = first_level_page_table[L1_index];
  if(table_L2 == NULL){
    return -1;
  }
  PT_ENTRY entry = table_L2[L2_index];
  if (get_present_bit(entry) == 0){
    return -1;
  }
  else{
    PAGEFRAME_NUMBER pf_number = get_pf_number(entry);
    return pf_number; 
  }
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

PT_ENTRY* create_L2_page_table(int L1_index){
  PT_ENTRY* table_L2 = malloc(TABLE_ENTRIES*ENTRY_SIZE);
  clear_L2_page_table(table_L2);
  first_level_page_table[L1_index] = table_L2;
  return table_L2;
}

// This inserts into the page table an entry mapping of the 
// the specified virtual page to the specified page frame.
// It might require the creation of a second-level page table
// to hold the entry, if it doesn't already exist.
void pt_update_pagetable(VPAGE_NUMBER vpage, PAGEFRAME_NUMBER pframe)
{
  int L1_index = get_L1_index(vpage);
  int L2_index = get_L2_index(vpage);

  PT_ENTRY* table_L2 = first_level_page_table[L1_index];
  if(table_L2 == NULL) {
    table_L2 = create_L2_page_table(L1_index);
  }
  else{
    if (table_L2[L2_index] != 0) {
      table_L2[L2_index] = 0;
    }
  }

  unsigned int value = pframe | PRESENT_BIT_MASK;
  table_L2[L2_index] = value;
}


// This clears a page table entry by clearing its present bit.
// It is called by the OS (in kernel.c) when a page is evicted
// from a page frame.
void pt_clear_page_table_entry(VPAGE_NUMBER vpage)
{
  int L1_index = get_L1_index(vpage);
  int L2_index = get_L2_index(vpage);

  PT_ENTRY* table_L2 = first_level_page_table[L1_index];
  if(table_L2 == NULL) {
    SAY("Tried to remove a vpage that does not exist!\n");
    return;
  }
  table_L2[L2_index] = 0;
}


