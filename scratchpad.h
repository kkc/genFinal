/* scratchpad.h - scratchpad table interface */

#ifndef SCRATCHPAD_H
#define SCRATCHPAD_H
#include<stdio.h>
#include"machine.h"
#include"fragment.h"

/* a segment description */
struct segment_t
{
    struct fragment_t *ft_list;   /* memorize first fragments in segment */
    struct fragment_t *ft_tail;   /* memorize last fragments in segment */
    int               append_pos; /* the next fragment append to this position*/
    struct segment_t  *prev_ptr;  /* previous segment */
    struct segment_t  *next_ptr;  /* next segment */
};

typedef struct segment_t segment;

/*a fragment list that can remember fragment*/
struct fragment_list_t
{
    struct fragment_t *ft;
    struct fragment_list_t *next_ptr;
};

typedef struct fragment_list_t history_list_t;
typedef struct fragment_list_t removed_list_t;

/*scratchpad structure*/ 
struct scratchpad_t                                                                                               
{
    segment *lo_segment_list;
    segment *hi_segment_list;
    segment *lo_head_ptr;
    segment *hi_head_ptr;
    segment *lo_tail_ptr;
    segment *hi_tail_ptr;
    int     max_lo_size;     /* low scratchpad's segment number */
    int     max_hi_size;     /* high scratchpad's segment number */
    int     hi_dirty;        /* if hi scratchoad have accessed in the last short period */

    history_list_t *history_list;
    history_list_t *history_tail;
    int     history_list_size;
    int     max_history_list_size;

    struct fragment_t *removed_list[50];
    int    removed_total;

    int short_counter;
    int higher_counter;
    int hi_count;
    int lo_count;
};

/* segment functions */

/* init */
segment *segment_create();
segment *segment_list_create(int size);

/* actions */
void segment_append(struct scratchpad_t *st, segment *seg, struct fragment_t *ft);
void segment_flush(struct scratchpad_t *st, segment *seg, int segment_pos);

/* utils */
segment *get_prev_segment(struct scratchpad_t *st, segment *seg);
segment *get_next_segment(struct scratchpad_t *st, segment *seg, int segment_pos);

/* scratchpad functions */

/* init */
struct scratchpad_t *scratchpad_create(int max_lo_size,int max_hi_size);
/* access */
unsigned long int scratchpad_access(struct scratchpad_t *st,md_addr_t addr);

/* check this address in the scratchpad */
int scratchpad_probe(md_addr_t addr);
/* get scratchpad status */
int scratchpad_lo_full(struct scratchpad_t *st,struct fragment_t *ft);
int scratchpad_hi_full(struct scratchpad_t *st,struct fragment_t *ft);
int scratchpad_lo_empty(struct scratchpad_t *st);
int scratchpad_hi_empty(struct scratchpad_t *st);

void scratchpad_append(struct scratchpad_t *st,struct fragment_t *ft);

/* check if some fragment exist in the history */
int search_history_list(struct scratchpad_t *st,struct fragment_t *ft);
void insert_history_list(struct scratchpad_t *st,struct fragment_t *ft);
void pop_history_list(struct scratchpad_t *st);

int equal(struct fragment_t *ft1,struct fragment_t *ft2);

void update_lo_tail_cursor(struct scratchpad_t *st,int size);

void pop_scratchpad_lo(struct scratchpad_t *st);
void pop_scratchpad_hi(struct scratchpad_t *st);
void insert_scratchpad_hi(struct scratchpad_t *st,struct fragment_t *ft);
void enlarge_scratchpad_hi_size(struct scratchpad_t *st);
void enlarge_scratchpad_lo_size(struct scratchpad_t *st);
#endif /*SCRATCHPAD_H*/
