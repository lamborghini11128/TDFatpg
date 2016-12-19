/**********************************************************************/
/*           Parallel-Fault Event-Driven Fault Simulator              */
/*                                                                    */
/*           Author: CCM                                              */
/*           last update : 22/10/2016                                 */
/**********************************************************************/

#include <stdio.h>
#include "global.h"
#include "miscell.h"

//DO YOUR PA#3 CODE HERE
//

#define num_of_patternTDF 16

unsigned int MaskTDF[16] = {0x00000003, 0x0000000c, 0x00000030, 0x000000c0,
    0x00000300, 0x00000c00, 0x00003000, 0x0000c000,
    0x00030000, 0x000c0000, 0x00300000, 0x00c00000,
    0x03000000, 0x0c000000, 0x30000000, 0xc0000000,};

unsigned int UnknownTDF[16] = {0x00000001, 0x00000004, 0x00000010, 0x00000040,
    0x00000100, 0x00000400, 0x00001000, 0x00004000,
    0x00010000, 0x00040000, 0x00100000, 0x00400000,
    0x01000000, 0x04000000, 0x10000000, 0x40000000,};






wptr first_faulty_wire;  // points to the start of the linked list 
extern int debug;
    fptr
transition_delay_fault_simulation(vectors,num_vectors,flist,total_detect_num)
    char *vectors[];
    int num_vectors;
    fptr flist;
    int *total_detect_num; 
{
    int sim();

    printf("vector num: %d\n", sim_vectors);
    printf("input num %d\n", ncktin);
    int i,j,nv,current_detect_num;
    fptr TDF_sim_a_vector();

    /* for every vector */
    for(i=num_vectors-1; i>=0; i--) {

        /* for every input, set its value to the current vector value */
        /// pattern 1 simulation
        for(j=0; j<ncktin; j++) {
            nv=ctoi(vectors[i][ j ]);
            sort_wlist[j]->value = nv;
        }
        
        for (j = 0; j < ncktwire; j++) {
            if (j < ncktin) 
                sort_wlist[j]->flag |= CHANGED;
            else 
                sort_wlist[j]->value = 2;
        }

        sim(); /* do a pattern 1 fault-free simulation, see sim.c */
        
        for (j = 0; j < ncktwire; j++) {
            sort_wlist[j]-> p1_value = sort_wlist[j] -> value;
        }

        // pattern 2
        for(j=0; j<ncktin; j++) {
            if( j == 0 )
                nv=ctoi(vectors[i][ ncktin ]);
            else
                nv=ctoi(vectors[i][ j - 1 ]);
            sort_wlist[j]->value = nv;
        }

    
        flist=TDF_sim_a_vector(flist,&current_detect_num);
        *total_detect_num += current_detect_num;
        //fprintf(stderr,"vector[%d] detects %d faults (%d)\n",i,
        //        current_detect_num,*total_detect_num);
    }
    return(flist);

	
	
}



/* fault simulate a single test vector */
    fptr
TDF_sim_a_vector(flist,num_of_current_detect)
    fptr flist;
    int *num_of_current_detect;
{
    wptr w,faulty_wire,wtemp;
    /* array of 16 fptrs, which points to the 16 faults in a simulation packet  */
    fptr simulated_fault_list[num_of_patternTDF];
    fptr f,ftemp;
    fptr sml;
    int fault_type;
    register int i,j,k,start_wire_index;
    register int num_of_fault;  
    int fault_sim_evaluate();
    wptr get_faulty_wire();
    int sim();
    static int test_no = 0;

    test_no++;
    num_of_fault = 0; // counts the number of faults in a packet

    /* num_of_current_detect is used to keep track of the number of undetected
     * faults detected by this vector, initialize it to zero */
    *num_of_current_detect = 0;

    /* Keep track of the minimum wire index of 16 faults in a packet.
     * the start_wire_index is used to keep track of the
     * first gate that needs to be evaluated.
     * This reduces unnecessary check of scheduled events.*/
    start_wire_index = 10000;
    first_faulty_wire = NULL;

    /* initialize the circuit - mark all inputs as changed and all other
     * nodes as unknown (2) */
    for (i = 0; i < ncktwire; i++) {
        if (i < ncktin) {
            sort_wlist[i]->flag |= CHANGED;
        }
        else {
            sort_wlist[i]->value = 2;
        }
    }

    sim(); /* do a fault-free simulation, see sim.c */
    if (debug) { display_io(); }

    /* expand the fault-free 0,1,2 value into 32 bits (2 = unknown)  
     * and store it in wire_value1 (good value) and wire_value2 (faulty value)*/
    for (i = 0; i < ncktwire; i++) {
        switch (sort_wlist[i]->value) {

            case 1: sort_wlist[i]->wire_value1 = ALL_ONE;  // 11 represents logic one
                    sort_wlist[i]->wire_value2 = ALL_ONE; break;
            case 2: sort_wlist[i]->wire_value1 = 0x55555555; // 01 represents unknown
                    sort_wlist[i]->wire_value2 = 0x55555555; break;
            case 0: sort_wlist[i]->wire_value1 = ALL_ZERO; // 00 represents logic zero
                    sort_wlist[i]->wire_value2 = ALL_ZERO; break;
        }
        sort_wlist[i]->pnext = NULL;
    } // for i

    /* walk through every undetected fault 
     * the undetected fault list is linked by pnext_undetect */
    for (f = flist; f; f = f->pnext_undetect) {
        if (f->detect == REDUNDANT) { continue;} /* ignore redundant faults */

        /* consider only active (aka. excited) fault
         * (sa1 with correct output of 0 or sa0 with correct output of 1) */
        if (f->fault_type != sort_wlist[f->to_swlist]->value &&  sort_wlist[f->to_swlist]->value !=  sort_wlist[f->to_swlist]->p1_value) 
        {

            /* if f is a primary output or is directly connected to an primary output
             * the fault is detected */
            if ((f->node->type == OUTPUT) ||
                    (f->io == GO && sort_wlist[f->to_swlist]->flag & OUTPUT)) {
                f->detect = TRUE;
            }
            else {

                /* if f is an gate output fault */
                if (f->io == GO) {

                    /* if this wire is not yet marked as faulty, mark the wire as faulty
                     * and insert the corresponding wire to the list of faulty wires. */
                    if (!(sort_wlist[f->to_swlist]->flag & FAULTY)) {
                        sort_wlist[f->to_swlist]->pnext = first_faulty_wire;
                        first_faulty_wire = sort_wlist[f->to_swlist];
                        first_faulty_wire->flag |= FAULTY;
                    }

                    /* add the fault to the simulated fault list and inject the fault */
                    simulated_fault_list[num_of_fault] = f;
                    inject_fault_value(sort_wlist[f->to_swlist], num_of_fault,
                            f->fault_type); 

                    /* mark the wire as having a fault injected 
                     * and schedule the outputs of this gate */
                    sort_wlist[f->to_swlist]->flag |= FAULT_INJECTED;
                    for (k = 0; k < sort_wlist[f->to_swlist]->nout; k++) {
                        sort_wlist[f->to_swlist]->onode[k]->owire[0]->flag |=
                            SCHEDULED;
                    }

                    /* increment the number of simulated faults in this packet */
                    num_of_fault++;
                    /* start_wire_index keeps track of the smallest level of fault in this packet.
                     * this saves simulation time.    */
                    start_wire_index = MIN(start_wire_index,f->to_swlist);
                }  // if gate output fault

                /* the fault is a gate input fault */
                else {

                    /* if the fault is propagated, set faulty_wire equal to the faulty wire.
                     * faulty_wire is the gate output of f.  */
                    if (faulty_wire = get_faulty_wire(f,&fault_type)) {

                        /* if the faulty_wire is a primary output, it is detected */
                        if (faulty_wire->flag & OUTPUT) {
                            f->detect = TRUE;
                        }

                        else {

                            /* if faulty_wire is not already marked as faulty, mark it as faulty
                             * and add the wire to the list of faulty wires. */
                            if (!(faulty_wire->flag & FAULTY)) {
                                faulty_wire->pnext = first_faulty_wire;
                                first_faulty_wire = faulty_wire;
                                first_faulty_wire->flag |= FAULTY;
                            }

                            /* add the fault to the simulated list and inject it */
                            simulated_fault_list[num_of_fault] = f;
                            inject_fault_value(faulty_wire, num_of_fault,
                                    fault_type);

                            /* mark the faulty_wire as having a fault injected 
                             *  and schedule the outputs of this gate */
                            faulty_wire->flag |= FAULT_INJECTED;
                            for (k = 0; k < faulty_wire->nout; k++) {
                                faulty_wire->onode[k]->owire[0]->flag |=
                                    SCHEDULED;
                            }


                            num_of_fault++;
                            start_wire_index = MIN(start_wire_index, f->to_swlist);
                        }
                    }
                }
            } // if  gate input fault
        } // if fault is active


        /*
         * fault simulation of a packet 
         */

        /* if this packet is full (16 faults)
         * or there is no more undetected faults remaining,
         * do the fault simulation */
        if ((num_of_fault == num_of_patternTDF) || !(f->pnext_undetect)) {

            /* starting with start_wire_index, evaulate all scheduled wires
             * start_wire_index helps to save time. */
            for (i = start_wire_index; i < ncktwire; i++) {
                if (sort_wlist[i]->flag & SCHEDULED) {
                    sort_wlist[i]->flag &= ~SCHEDULED;
                    fault_sim_evaluate(sort_wlist[i]);
                }
            } /* event evaluations end here */

            /* check detection and reset wires' faulty values
             * back to fault-free values.
             */
            for (w = first_faulty_wire; w; w = wtemp) {
                wtemp = w->pnext;
                w->pnext = NIL(struct WIRE);
                //printf("before : %d\n", w->flag);
                w->flag &= ~FAULTY;
                w->flag &= ~FAULT_INJECTED;
                w->fault_flag &= ALL_ZERO;
                //printf("after  : %d\n", w->flag);
                if (w->flag & OUTPUT) { // if primary output 
                    for (i = 0; i < num_of_fault; i++) { // check every undetected fault
                        if (!(simulated_fault_list[i]->detect)) {
                            if ((w->wire_value2 & MaskTDF[i]) ^      // if value1 != value2
                                    (w->wire_value1 & MaskTDF[i])) {
                                if (((w->wire_value2 & MaskTDF[i]) ^ UnknownTDF[i])&&  // and not unknowns
                                        ((w->wire_value1 & MaskTDF[i]) ^ UnknownTDF[i])){
                                    sml = simulated_fault_list[i];
                                    if ( sort_wlist[sml->to_swlist]->value !=  sort_wlist[sml->to_swlist]->p1_value ) 
                                        simulated_fault_list[i]->detect = TRUE;  // then the fault is detected
                                }
                            }
                        }
                    }
                }
                w->wire_value2 = w->wire_value1;  // reset to fault-free values
            }  // for w = first_faulty_wire
            /*check();
              check2();*/
            num_of_fault = 0;  // reset the counter of faults in a packet
            start_wire_index = 10000;  //reset this index to a very large value.
            first_faulty_wire = NULL;
        } // end fault sim of a packet

    }  // end loop. for f = flist


    /* the following two loops are both for fault dropping  */  
    /* drop detected faults from the FRONT of the undetected fault list */
    while(flist) {
        if (flist->detect == TRUE) {
            (*num_of_current_detect) += flist->eqv_fault_num;
            f = flist->pnext_undetect;
            flist->pnext_undetect = NULL;
            flist = f;
        }
        else {break;}
    }

    /* drop detected faults from WITHIN the undetected fault list*/
    if (flist) {
        for (f = flist; f->pnext_undetect; f = ftemp) {
            if (f->pnext_undetect->detect == TRUE) { 
                (*num_of_current_detect) += f->pnext_undetect->eqv_fault_num;
                f->pnext_undetect = f->pnext_undetect->pnext_undetect;
                ftemp = f;
            }
            else {
                ftemp = f->pnext_undetect;
            }
        }
    }
    return(flist);
}/* end of fault_sim_a_vector */


