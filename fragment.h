/* fragment.h - fragment table interface */

#ifndef FRAGMENT_H
#define FRAGMENT_H
#include<stdio.h>
#include"machine.h"
#include"memory.h"

/*fragment structure*/ 
struct fragment_t                                                                                                 
{
    md_addr_t start_address;        /*fragment start address*/
    md_addr_t end_address;          /*fragment end address*/ 
    md_addr_t size;                 /*fragment size*/
    int penalty;                    /*translation time*/
    int in_scratchpad;              /*check if this fragment is in scratchpad */
    int in_type;                    /*0:in low, 1:in high*/
    int lo_hit;                     /*hit in low scratchpad*/
    int hi_hit;                     /*hit in high scratchpad*/
    int replace;                    /*replace count */
    int pos;                        /*the position in segment*/
    struct fragment_t *next_ptr;
};

void print_fragment_table();
void set_fragment_table(int index,                //table index
                        md_addr_t start_address,  //fragment start address
                        md_addr_t end_address,    //fragment end address
                        md_addr_t size);          //fragment size
void create_fragment_table(FILE *stream);
int compare_addr(const void *a, const void *b);

/* using address to query fragment from fragment_table */
struct fragment_t *query_fragment(md_addr_t addr);
int get_trans_time(md_inst_t inst);

#endif /*FRAGMENT_H*/
