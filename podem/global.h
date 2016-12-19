/**********************************************************************/
/*           automatic test pattern generation                        */
/*           global variables shared by all programs                  */
/*                                                                    */
/*           Author: Hi-Keung Tony Ma and K.T. Cheng                  */ 
/*           last update : 4/10/1996                                  */
/**********************************************************************/

#include"atpg.h"

#ifdef MAIN

wptr *sort_wlist;          /* sorted wire list with regard to level */
wptr *cktin;               /* input wire list */
wptr *cktout;              /* output wire list */
nptr *cktnode_f0;          /* frame 0 node list */
wptr *cktout_f0;           /* output wire list */
wptr *cktout_f1;           /* output wire list */
wptr hash_wlist[HASHSIZE]; /* hashed wire list */
nptr hash_nlist[HASHSIZE]; /* hashed node list */
int ncktwire;              /* total number of wires in the circuit */
int ncktwire_f0;           /* total number of wires in the circuit */
int ncktwire_f1;           /* total number of wires in the circuit */
int ncktnode;              /* total number of nodes in the circuit */
int ncktnode_f0;           /* total number of nodes in the circuit */
int ncktnode_f1;           /* total number of nodes in the circuit */
int ncktin;                /* number of primary inputs */
int ncktout;               /* number of primary outputs */
int ncktout_f0;            /* number of primary outputs */
int ncktout_f1;            /* number of primary outputs */
int in_vector_no;          /* number of test vectors generated */
int fsim_only;             /* flag to indicate fault simulation only */
int tdfsim_only;           /* flag to indicate transition delay fault simulation only */
int tdfatpg_only;           /* flag to indicate transition delay fault simulation only */
int sim_vectors;           /* number of simulation vectors */
char **vectors;            /* vector set */

#else

extern wptr *sort_wlist;          /* sorted wire list with regard to level */
extern wptr *cktin;               /* input wire list */
extern wptr *cktout;              /* output wire list */
extern nptr *cktnode_f0;          /* frame 0 node list */
extern wptr *cktout_f0;              /* output wire list */
extern wptr *cktout_f1;              /* output wire list */
extern wptr hash_wlist[HASHSIZE]; /* hashed wire list */
extern nptr hash_nlist[HASHSIZE]; /* hashed node list */
extern int ncktwire;              /* total number of wires in the circuit */
extern int ncktwire_f0;              /* total number of wires in the circuit */
extern int ncktwire_f1;              /* total number of wires in the circuit */
extern int ncktnode;              /* total number of nodes in the circuit */
extern int ncktnode_f0;              /* total number of nodes in the circuit */
extern int ncktnode_f1;              /* total number of nodes in the circuit */
extern int ncktin;                /* number of primary inputs */
extern int ncktout;               /* number of primary outputs */
extern int ncktout_f0;               /* number of primary outputs */
extern int ncktout_f1;               /* number of primary outputs */
extern int in_vector_no;          /* number of test vectors generated */
extern int fsim_only;             /* flag to indicate fault simulation only */
extern int tdfsim_only;           /* flag to indicate transition delay fault simulation only */
extern int tdfatpg_only;           /* flag to indicate transition delay fault simulation only */
extern int sim_vectors;           /* number of simulation vectors */
extern char **vectors;            /* vector set */

#endif
