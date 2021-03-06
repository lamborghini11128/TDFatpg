/**********************************************************************/
/*           miscellaneous macro substitutions and declarations       */
/*                                                                    */
/*           Author: Tony Hi Keung Ma                                 */
/*           last update : 3/26/1986                                  */
/**********************************************************************/
#ifndef MISCELL_H
#define MISCELL_H

#define ALLOC(num,type)    (type *)calloc(num,sizeof(type))
#define NIL(type)      ((type *)0)
#define MAYBE          2
#define TRUE           1
#define FALSE          0
#define REDUNDANT      3
#define STUCK0         0
#define STUCK1         1
#define ALL_ONE        0xffffffff // for parallel fault sim; 2 ones represent a logic one
#define ALL_ZERO       0x00000000 // for parallel fault sim; 2 zeros represent a logic zero
#define MAX(A,B) ((A) > (B) ? (A) : (B))
#define MIN(A,B) ((A) < (B) ? (A) : (B))
#define ABS(A) ((A) < 0 ? -(A) : (A))

typedef struct FAULT *fptr;
typedef char *string;
typedef struct PIASSIGN *pptr;

struct FAULT {
    nptr node;            /* gate under test(NIL if PI/PO fault) */
    short io;             /* 0 = GI; 1 = GO */
    short index;          /* index for GI fault. it represents the  
			     associated gate input index number for this GI fault */   
    short fault_type;     /* s-a-1 or s-a-0 fault */
    short activate;
    short detect;         /* detection flag */
    short sim_detect;     /* fault sim detection flag */
    short test_tried;     /* flag to indicate test is being tried */
    int  eqv_fault_num;  /* number of equivalent faults */
    fptr pnext;           /* pointer to the next element in the list */
    fptr pprev;           /* pointer to the previous element in the list */
    fptr pnext_undetect;  /* pointer to next undetected fault */
    fptr pnext_detected;  
	int  to_swlist;      /* index to the sort_wlist[] */ 
	int  fault_no;         /* fault index */
    int  det_num;          /* detection number */
    pptr piassign;         /* pi assignment */
    int* detect_by;
};

fptr first_fault;         /* pointer to the first element of the fault list */

struct PIASSIGN{
    int  index_value;       /* pi index (s) and its value (t) => s * 2  + t */
    pptr pnext;            /* pointer to next PIASSIGN */    

};

/* possible values for fault->faulty_net_type */
#define GI 0
#define GO 1

#endif
