/* fragment.c , fragment table */
#include<stdio.h>
#include<stdlib.h>
#include"fragment.h"
#include"machine.h"
#include"memory.h"

#define MAX_FRAGMENT_NUMBER 5000

struct fragment_t fragment_table[MAX_FRAGMENT_NUMBER];
int fragment_count = 0;
int last_fragment_addr = 0;

void print_fragment_table()
{
    int i;
    for(i=0;i<fragment_count;++i)
    {
        printf("\nindex: %d\n",i+1);
        printf("start address: %d\n",fragment_table[i].start_address);
        printf("end   address: %d\n",fragment_table[i].end_address);
        printf("size: %d\n",fragment_table[i].size);
    }
}

void set_fragment_table(int index,                //table index
                        md_addr_t start_address,  //fragment start address
                        md_addr_t end_address,    //fragment end address
                        md_addr_t size)           //fragment size
{ 
   fragment_table[index].start_address = start_address;
   fragment_table[index].end_address   = end_address;
   fragment_table[index].size          = size;
   fragment_table[index].in_L2         = 0;
   fragment_table[index].L2_access_count = 0;
   fragment_table[index].in_scratchpad = 0;
   fragment_table[index].in_type       = 0;
   fragment_table[index].next_ptr      = NULL;
   fragment_table[index].lo_hit        = 0;
   fragment_table[index].hi_hit        = 0;
   fragment_table[index].replace       = 0;
   fragment_table[index].penalty       = 0;
}

/**
 * 得到這個instruction需要translation的cycle count
 **/ 
int get_trans_time(md_inst_t inst)
{
    int count;
    int D,S,T;
    char *s;
    enum md_opcode op;

    MD_SET_OPCODE(op,inst);

    count = 0;
    s = MD_OP_FORMAT(op);
    while(*s)
    {
        switch (*s) {
            case 'd':
              D = RD;
              break;
            case 's':
              S = RS;
              break;
            case 't':
              T = RT;
              break;
            case 'b':
              S = BS;
              break;
            case 'D':
              D = FD;
              break;
            case 'S':
              S = FS;
              break;
            case 'T':
              T = FT;
              break;
            case 'j':
              D = OFS << 2;
              break;
            case 'o':
            case 'i':
              S = IMM;
              break;
            case 'H':
              S = SHAMT;
              break;
            case 'u':
              S = UIMM;
              break;
            case 'U':
              S = UIMM;
              break;
            case 'J':
              S = TARG << 2;
              break;
            case 'B':
              S = BCODE;
              break;
            default:
              //putchar(*s);
              count--;
              break;
        }
        s++;
        count++;
    }

    if(count == 3)
        return 217;
    if(count == 2)
        return 145;
    if(count == 1)
        return 130;
}

void generate_fragment_translation_time(struct mem_t *mem)
{
    int i;
    md_addr_t addr;
    md_inst_t inst;

    for(i=0 ;i<fragment_count ;++i)
    {
        for(addr  = fragment_table[i].start_address;
            addr <= fragment_table[i].end_address;
            addr += 8)
        {
            MD_FETCH_INST(inst, mem, addr);
            fragment_table[i].penalty += get_trans_time(inst);
        }
        /*
        printf("%d %d %d\n",fragment_table[i].start_address, 
                            fragment_table[i].end_address,
                            fragment_table[i].penalty);*/
    }
}

void create_fragment_table(FILE *stream)
{
    if(!stream)
        fprintf(stderr,"%s","there are no fragment file");

    int i=0;
    int start_address,end_address,size;

    fscanf(stream,"%d",&fragment_count);
    if (fragment_count > MAX_FRAGMENT_NUMBER)
    {
        printf("fragment_total > %d\n",MAX_FRAGMENT_NUMBER);
        exit(0);
    }

    for(i=0;i<fragment_count;++i)
    {
        fscanf(stream,"%d %d %d",&start_address,&end_address,&size);
        set_fragment_table(i,start_address,end_address,size);
    }
}

int compare_addr(const void *a, const void *b)
{
    const struct fragment_t arg1 = * (struct fragment_t *) a;
    const struct fragment_t arg2 = * (struct fragment_t *) b;
    if ( arg1.start_address < arg2.start_address ) 
        return -1;
    else if ( arg1.start_address >= arg2.start_address &&
              arg1.start_address <= arg2.end_address)
        return 0;
    else
        return 1;
}

struct fragment_t *query_fragment(md_addr_t addr)
{
    struct fragment_t f;
    f.start_address = addr;
    struct fragment_t *pos =
         (struct fragment_t *) bsearch (&f,
                                     fragment_table,
                                     fragment_count,
                                     sizeof(struct fragment_t),
                                     compare_addr);
    return pos;
}

void print_fragment_table_result(FILE *stream)
{
    if(!stream)
        stream = stderr;

    int i;
    for(i=0;i<fragment_count;++i)
    {
        fprintf(stream,"addr,lo_hit,hi_hit,replace,size:(%d,%d,%d,%d,%d)\n",
                                              fragment_table[i].start_address,
                                              fragment_table[i].lo_hit,
                                              fragment_table[i].hi_hit,
                                              fragment_table[i].replace,
                                              fragment_table[i].size);
    }
    
}

