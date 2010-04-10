/* scratchpad - scatchpad module routines */

#include<stdio.h>
#include<stdlib.h>
#include"machine.h"
#include"scratchpad.h"
#include"fragment.h"

#define SEGMENT_SIZE 256
extern FILE *fp_result;

struct scratchpad_t *
scratchpad_create(int lo_scratchpad_size,
                  int hi_scratchpad_size)
{
    struct scratchpad_t *st = 
        (struct scratchpad_t *) malloc (1 * sizeof(struct scratchpad_t));
    st->max_lo_size = lo_scratchpad_size / SEGMENT_SIZE;
    st->max_hi_size = hi_scratchpad_size / SEGMENT_SIZE;
    st->lo_segment_list = segment_list_create(st->max_lo_size);
    st->hi_segment_list = segment_list_create(st->max_hi_size);
    st->lo_head_ptr = st->lo_tail_ptr = st->lo_segment_list;
    st->hi_head_ptr = st->hi_tail_ptr = st->hi_segment_list;
    
    st->history_list = st->history_tail = NULL;
    st->history_list_size = 0;
    st->max_history_list_size = 49;

    st->short_counter = 0;
    st->higher_counter = 0;
    st->removed_total = 0;

    st->hi_count = 0;
    st->lo_count = st->max_lo_size;
}

segment *segment_create()
{
    segment *seg = (segment *) malloc (1*sizeof(segment));
    seg->ft_list  = NULL;
    seg->ft_tail  = NULL;
    seg->prev_ptr = NULL;
    seg->next_ptr = NULL;
    seg->append_pos = 0;
    return seg;
}

segment *segment_list_create(int size)
{
    if (size == 0)
        return NULL;

    segment *seg_list = NULL;

    seg_list = segment_create();

    int i;
    for(i=1;i<size;++i)
    {
        segment *segs = segment_create();
        segs->next_ptr = seg_list;
        seg_list->prev_ptr = segs;
        seg_list = segs;
    }

    return seg_list;
}

segment *get_prev_hi_segment(struct scratchpad_t *st, segment *seg)
{
    if(seg->prev_ptr != NULL)
        return seg->prev_ptr;
    else
    {
        segment *iter = seg;
        while(iter->next_ptr != NULL)
            iter = iter->next_ptr;
        return iter;
    }
}

segment *get_prev_segment(struct scratchpad_t *st, segment *seg)
{
    if(seg->prev_ptr != NULL)
        return seg->prev_ptr;
    else
    {
        segment *iter = seg;
        while(iter->next_ptr != NULL)
            iter = iter->next_ptr;
        return iter;
    }
}

segment *get_next_segment(struct scratchpad_t *st, segment *seg, int segment_pos)
{
    if(seg->next_ptr != NULL)
        return seg->next_ptr;
    else
    {
        if (segment_pos == 0)
            return st->lo_segment_list;
        else
            return st->hi_segment_list;
    }
}

segment *get_next_hi_segment(struct scratchpad_t *st, segment *seg)
{
    if(seg->next_ptr != NULL)
        return seg->next_ptr;
    else
        return st->hi_segment_list;
}

int scratchpad_hi_empty(struct scratchpad_t *st)
{
    if (st->hi_segment_list == NULL)
        return 1;

    return 0;
}

int scratchpad_lo_empty(struct scratchpad_t *st)
{
    if (st->lo_head_ptr == st->lo_tail_ptr &&
        st->lo_head_ptr->ft_list == NULL)
        return 1;
    
    return 0;
}

int scratchpad_hi_full(struct scratchpad_t *st, struct fragment_t *ft)
{
    if (st->hi_tail_ptr->append_pos + ft->size > SEGMENT_SIZE)
    {
        return 1;
        /*
        if(st->hi_tail_ptr == st->hi_head_ptr)
            return 1;

        segment *next_seg = get_next_hi_segment(st, st->hi_tail_ptr);
        if(next_seg == st->hi_head_ptr)
            return 1;*/
    }
    return 0;
}

int scratchpad_lo_full(struct scratchpad_t *st, struct fragment_t *ft)
{
    if (st->lo_tail_ptr != st->lo_head_ptr &&
        st->lo_tail_ptr->append_pos + ft->size >= SEGMENT_SIZE)
    {
        segment *next_seg = get_next_segment(st, st->lo_tail_ptr, 0);
        if(next_seg == st->lo_head_ptr)
            return 1;
    }
    return 0;
}

int scratchpad_hi_dirty(struct scratchpad_t *st)
{
    return st->hi_dirty;
}

void segment_hi_append(struct scratchpad_t *st, segment *seg, struct fragment_t *ft)
{
    // empty
    if (seg->ft_list == NULL && seg->ft_tail == NULL)
        seg->ft_list = seg->ft_tail = ft;
    // append
    else 
    {
        seg->ft_tail->next_ptr = ft;
        seg->ft_tail = ft;
    }

    // check position and segment boundry
    ft->pos = seg->append_pos;
    seg->append_pos += ft->size;

    if (seg->append_pos > SEGMENT_SIZE)
    {
        segment *next_seg = get_next_hi_segment(st,seg);
        next_seg->append_pos = seg->append_pos - SEGMENT_SIZE;
        seg->append_pos = SEGMENT_SIZE;
        st->hi_tail_ptr = next_seg;
    }

    //update fragment status
    ft->in_scratchpad = 1;
    //ft->hi_hit++;
    ft->in_type = 1;

    //fprintf(fp_result,"\nappend address:%d, size: %d\n",ft->start_address, ft->size);
}

void segment_append(struct scratchpad_t *st, segment *seg, struct fragment_t *ft)
{
    // empty
    if (seg->ft_list == NULL && seg->ft_tail == NULL)
        seg->ft_list = seg->ft_tail = ft;
    // append
    else 
    {
        seg->ft_tail->next_ptr = ft;
        seg->ft_tail = ft;
    }

    // check position and segment boundry
    ft->pos = seg->append_pos;
    seg->append_pos += ft->size;

    if (seg->append_pos >= SEGMENT_SIZE)
    {
        segment *next_seg = get_next_segment(st,seg,0);
        next_seg->append_pos = seg->append_pos - SEGMENT_SIZE;
        seg->append_pos = SEGMENT_SIZE;
        st->lo_tail_ptr = next_seg;
    }

    //update fragment status
    ft->in_scratchpad = 1;
    ft->lo_hit++;
    ft->in_type = 0;

    //fprintf(fp_result,"\nappend address:%d, size: %d\n",ft->start_address, ft->size);
}

unsigned long int 
scratchpad_access(struct scratchpad_t *st, md_addr_t addr)
{
   if (st->lo_count + st->hi_count != st->max_lo_size)
   {
       printf("error");
       exit(1);
   }

   //fprintf(fp_result,"access addr:%d ",addr);
   unsigned long int lat = 0;  //latency
   struct fragment_t *ft = query_fragment(addr); 

   if ( ft->in_scratchpad )
   {
     if( ft->in_type == 0)
     {
        ft->lo_hit++;
        if (st->hi_dirty)
            st->short_counter = 1;
        else
            st->short_counter++;

        //fprintf(fp_result,"in low\n");
     }
     else if (ft->in_type == 1)
     {
        ft->hi_hit++;
        st->hi_dirty = 1;
        st->higher_counter++;
        //fprintf(fp_result,"in high\n");
     }

     /*
     if (st->short_counter == 25)
     {
        if ( !scratchpad_hi_empty(st) )
            scratchpad_lo_enlarge(st);
        st->short_counter = 0;
     }*/

     return lat;
   }

   if (st->short_counter >= 130)
   {
       if ( !scratchpad_hi_empty(st) )
           scratchpad_lo_enlarge(st);
       st->short_counter = 0;
       fprintf(fp_result,"%d\n",st->higher_counter);
       st->higher_counter = 0;
   }

   // fragment is not in scratchpad
   // no hit and we have move fragment to scratchpad
   scratchpad_append(st,ft);
   lat += ft->penalty;       //translation penalty
   st->hi_dirty = 0;

   //fprintf(fp_result,"%d %d\n",addr,lat);
   return lat;
}

segment *get_last_lo_segment(struct scratchpad_t *st)
{
    segment *iter = st->lo_segment_list;
    while(iter->next_ptr != NULL)
    {
        iter = iter->next_ptr;
    }
    return iter;
}

segment *get_last_hi_segment(struct scratchpad_t *st)
{
    segment *iter = st->hi_segment_list;
    while(iter->next_ptr != NULL)
    {
        iter = iter->next_ptr;
    }
    return iter;
}

segment *scratchpad_lo_smaller(struct scratchpad_t *st)
{
    segment *last_seg = get_last_lo_segment(st);
    last_seg->prev_ptr->next_ptr = NULL;
    return last_seg;
}

segment *scratchpad_hi_smaller(struct scratchpad_t *st)
{
    segment *last_seg = get_last_hi_segment(st);
    last_seg->prev_ptr->next_ptr = NULL;
    return last_seg;
}

void 
scratchpad_lo_enlarge(struct scratchpad_t *st)
{
   st->lo_count++;
   st->hi_count--;
   //fprintf(fp_result,"enlarge lo\n");
   segment *free_seg = get_last_hi_segment(st);
   segment_flush(st, free_seg, 1);

   if (free_seg == st->hi_segment_list)
       st->hi_segment_list = NULL;
   if (free_seg == st->hi_tail_ptr)
       st->hi_tail_ptr = get_prev_hi_segment(st,st->hi_tail_ptr);
   if (free_seg == st->hi_head_ptr)
       st->hi_head_ptr = get_next_hi_segment(st,st->hi_head_ptr);

   if (free_seg->prev_ptr != NULL)
   {
       free_seg->prev_ptr->next_ptr = NULL;
       free_seg->prev_ptr = NULL;
   }

   if ( st->lo_head_ptr == NULL)
   {
       st->lo_head_ptr = free_seg;
       st->lo_tail_ptr = free_seg;
   }
   else
   {
       segment *last_lo_seg = get_last_lo_segment(st);
       last_lo_seg->next_ptr = free_seg;
       free_seg->prev_ptr = last_lo_seg;
   }
}

void 
scratchpad_hi_enlarge(struct scratchpad_t *st)
{
   st->hi_count++;
   st->lo_count--;
   //fprintf(fp_result,"enlarge hi\n");
   segment *free_seg = get_last_lo_segment(st);
   segment_flush(st, free_seg, 0);

   if (free_seg == st->lo_segment_list)
   {
      printf("hi is much larger");
      exit(1);
   }
   if (free_seg == st->lo_tail_ptr)
       st->lo_tail_ptr = get_prev_segment(st,st->lo_tail_ptr);
   if (free_seg == st->lo_head_ptr)
       st->lo_head_ptr = get_next_segment(st,st->lo_head_ptr,0);

   if (free_seg->prev_ptr != NULL)
   {
       free_seg->prev_ptr->next_ptr = NULL;
       free_seg->prev_ptr = NULL;
   }

   if ( st->hi_segment_list == NULL)
   {
       st->hi_segment_list = free_seg;
       st->hi_head_ptr = free_seg;
       st->hi_tail_ptr = free_seg;
   }
   else
   {
       segment *last_hi_seg = get_last_hi_segment(st);
       last_hi_seg->next_ptr = free_seg;
       free_seg->prev_ptr = last_hi_seg;
   }
}

void 
scratchpad_hi_append(struct scratchpad_t *st, 
                     struct fragment_t *ft)
{
    //fprintf(fp_result,"hi append addr: %d\n", ft->start_address);
    st->short_counter = 0;
    if ( scratchpad_hi_empty(st) )
    {
        scratchpad_hi_enlarge(st);
        segment *seg = st->hi_head_ptr;
        segment_hi_append(st, seg, ft);
        return;
    }

    if ( scratchpad_hi_full(st, ft) )
    {
        if ( scratchpad_hi_dirty(st) && get_last_lo_segment(st) != NULL && st->lo_count > 2)
        {
           scratchpad_hi_enlarge(st);
           //st->hi_tail_ptr = get_next_hi_segment(st, st->hi_tail_ptr);
        }
        else
        {
           //fprintf(fp_result,"hi full\n");
           segment *seg = st->hi_head_ptr;
           segment_flush(st,seg,1);
           segment *next_seg = get_next_segment(st, seg, 1);
           if (next_seg != NULL)
               st->hi_head_ptr = next_seg;
        }

        if (st->hi_tail_ptr->append_pos == SEGMENT_SIZE)
        {
            segment *next_seg = get_next_hi_segment(st, st->hi_tail_ptr);
            next_seg->append_pos = st->hi_tail_ptr->append_pos - SEGMENT_SIZE;
            st->hi_tail_ptr->append_pos = SEGMENT_SIZE;
            st->hi_tail_ptr = next_seg;
        }
    }

    segment *seg = st->hi_tail_ptr;
    segment_hi_append(st, seg, ft);
}

void check_removed_list(struct scratchpad_t *st)
{
    int i;
    for(i=0 ; i < st->removed_total; ++i)
    {
        if (search_history_list(st, st->removed_list[i]))
        {
//            fprintf(fp_result,"find in history:%d\n",st->removed_list[i]->start_address);
            if (get_last_lo_segment(st) != st->lo_segment_list->next_ptr)
                scratchpad_hi_append(st, st->removed_list[i]);
//            if ( st->lo_count > 2)
        }
        insert_history_list(st, st->removed_list[i]);
    }
    st->removed_total = 0;
}

void 
scratchpad_append(struct scratchpad_t *st,
                  struct fragment_t *ft)
{
   if( scratchpad_lo_empty(st) )   //scratchpad is empty?
   {
       segment *seg = st->lo_head_ptr;
       segment_append(st,seg,ft);
       return;
   }

   // insert but first consider scratchpad is full 
   if( scratchpad_lo_full(st,ft) )    //pop from linked list's head
   {
       //fprintf(fp_result,"\nlo full\n");
       segment *seg = st->lo_head_ptr;
       segment_flush(st,seg,0);
       segment *next_seg = get_next_segment(st, seg, 0);
       st->lo_head_ptr = next_seg;
   }

   segment *seg = st->lo_tail_ptr;
   segment_append(st,seg,ft);
   check_removed_list(st);
}

int last_segment_fragment_cross(struct scratchpad_t *st, segment *seg)
{
    segment *prev_seg = get_prev_segment(st, seg);
    if (prev_seg == seg)
        return 0;
    else if (prev_seg->ft_tail != NULL &&
             prev_seg->ft_tail->pos + prev_seg->ft_tail->size > SEGMENT_SIZE)
        return 1;
    else
        return 0;
}

/* segment_pos: 0 in low , 1 in high*/
void segment_flush(struct scratchpad_t *st, segment *seg, int segment_pos)
{
   if (seg->ft_list == NULL && seg->append_pos == 0)
       return;

   struct fragment_t *removed_ft;
   struct fragment_t *iter;

   if (last_segment_fragment_cross(st, seg) )
   {
       segment *prev_seg = get_prev_segment(st, seg);
       removed_ft = prev_seg->ft_tail;

       if (removed_ft->pos + removed_ft->size <= SEGMENT_SIZE)
          printf("there are some error!");
       
       // update previous segment's next appending position
       prev_seg->append_pos = removed_ft->pos;

       // handle segment's fragment list
       if (prev_seg->ft_list == prev_seg->ft_tail)
           prev_seg->ft_list = prev_seg->ft_tail = NULL;
       else
       {
           iter = prev_seg->ft_list;
           while(iter->next_ptr != prev_seg->ft_tail)
              iter = iter->next_ptr;

           iter->next_ptr = NULL;
           prev_seg->ft_tail = iter;
       }
       remove_fragment(st, removed_ft);
   }

   while(seg->ft_list != NULL)
   {
       removed_ft = seg->ft_list;
       seg->ft_list = seg->ft_list->next_ptr;
       remove_fragment(st, removed_ft);
   }
   seg->ft_tail = NULL;

   seg->append_pos = 0;
   segment *next_seg = get_next_segment(st, seg, segment_pos);
   if ( next_seg != st->hi_segment_list)
       next_seg->append_pos = 0;
}

void
remove_fragment(struct scratchpad_t *st,
                struct fragment_t *removed_ft)
{
    //update fragment status
    removed_ft->in_scratchpad = 0;
    removed_ft->replace++;
    removed_ft->next_ptr = NULL;
    //fprintf(fp_result,"remove address:%d, size: %d\n",
    //        removed_ft->start_address, removed_ft->size);
    
    if (removed_ft->in_type == 0)
        st->removed_list[st->removed_total++] = removed_ft;

    removed_ft->in_type = 0;
}

int
search_history_list(struct scratchpad_t *st,
                    struct fragment_t *ft)
{
    int count = 0;
    history_list_t *history_iter = st->history_list;
    while(history_iter != NULL)
    {
        if(equal(ft,history_iter->ft))
            count++;

        history_iter = history_iter->next_ptr;
    }
    return count;
}

void
insert_history_list(struct scratchpad_t *st,
                    struct fragment_t *ft)
{
    history_list_t *ht = malloc (sizeof(history_list_t));
    ht->ft = ft;
    ht->next_ptr = NULL;

    if(st->history_list_size == 0)
        st->history_list = st->history_tail = ht;
    else
    {
        st->history_tail->next_ptr = ht;
        st->history_tail = ht;
    }
    
    st->history_list_size++;

    if(st->history_list_size > st->max_history_list_size)
        pop_history_list(st);
}

void
pop_history_list(struct scratchpad_t *st)
{
    if(st->history_list_size == 0)
    {
        printf("error: there are no fragment can be pop");
        exit(1);
    }

    free(st->history_list);
    st->history_list = st->history_list->next_ptr;
    st->history_list_size--;
}

int 
equal(struct fragment_t *ft1,
      struct fragment_t *ft2)
{
    if(ft1 == ft2)
        return 1;
    else
        return 0;
}

int scratchpad_probe(md_addr_t addr)
{
    struct fragment_t *ft = query_fragment(addr);
    if (ft == NULL)
    {
        printf("scratchpad_probe can find fragmnet!!! it's wrong!!!!");
        exit(1);
    }

    if(ft->in_scratchpad == 0)
        return 0;
    else
        return 1;
}
