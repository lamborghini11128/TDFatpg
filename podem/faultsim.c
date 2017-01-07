/**********************************************************************/
/*           Parallel-Fault Event-Driven Fault Simulator              */
/*                                                                    */
/*           Author: Tony Hi Keung Ma                                 */
/*           last update : 4/11/1986                                  */
/**********************************************************************/


#include <stdio.h>
#include "global.h"
#include "miscell.h"

/* pack 16 faults into one packet.  simulate 16 faults togeter. 
 * the following variable name is somewhat misleading */
#define num_of_pattern 16

unsigned int Mask[16] = {0x00000003, 0x0000000c, 0x00000030, 0x000000c0,
    0x00000300, 0x00000c00, 0x00003000, 0x0000c000,
    0x00030000, 0x000c0000, 0x00300000, 0x00c00000,
    0x03000000, 0x0c000000, 0x30000000, 0xc0000000,};

unsigned int Unknown[16] = {0x00000001, 0x00000004, 0x00000010, 0x00000040,
    0x00000100, 0x00000400, 0x00001000, 0x00004000,
    0x00010000, 0x00040000, 0x00100000, 0x00400000,
    0x01000000, 0x04000000, 0x10000000, 0x40000000,};

/* The faulty_wire contains a list of wires that 
 * change their values in the fault simulation for a particular packet.
 * (that is wire_value1 != wire_value2) 
 * Note that the wire themselve are not necessarily a fault site.
 * The list is linked by the pnext pointers */
wptr first_faulty_wire;  // points to the start of the linked list 
extern int debug;

/* fault simulate a set of test vectors */
    fptr
fault_simulate_vectors(vectors,num_vectors,flist,total_detect_num)
    char *vectors[];
    int num_vectors;
    fptr flist;
    int *total_detect_num;
{
    int i,j,nv,current_detect_num;
    fptr fault_sim_a_vector();

    /* for every vector */
    for(i=num_vectors-1; i>=0; i--) {

        /* for every input, set its value to the current vector value */
        for(j=0; j<ncktin; j++) {
            nv=ctoi(vectors[i][j]);
            sort_wlist[j]->value = nv;
        }

        flist=fault_sim_a_vector(flist,&current_detect_num);
        *total_detect_num += current_detect_num;
        fprintf(stderr,"vector[%d] detects %d faults (%d)\n",i,
                current_detect_num,*total_detect_num);
    }
    return(flist);
}// fault_simulate_vectors

/* fault simulate a single test vector */
    fptr
fault_sim_a_vector(flist,num_of_current_detect)
    fptr flist;
    int *num_of_current_detect;
{
    wptr w,faulty_wire,wtemp;
    /* array of 16 fptrs, which points to the 16 faults in a simulation packet  */
    fptr simulated_fault_list[num_of_pattern];
    fptr f,ftemp;
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
        if (f->fault_type != sort_wlist[f->to_swlist]->value) {

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
        if ((num_of_fault == num_of_pattern) || !(f->pnext_undetect)) {

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
                            if ((w->wire_value2 & Mask[i]) ^      // if value1 != value2
                                    (w->wire_value1 & Mask[i])) {
                                if (((w->wire_value2 & Mask[i]) ^ Unknown[i])&&  // and not unknowns
                                        ((w->wire_value1 & Mask[i]) ^ Unknown[i])){
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

/* fault simulate a single test vector */
fault_sim_a_vector_frame01(flist,num_of_current_detect)
    fptr flist;
    int *num_of_current_detect;
{
    wptr w,faulty_wire,wtemp, wnext;
    /* array of 16 fptrs, which points to the 16 faults in a simulation packet  */
    fptr simulated_fault_list[num_of_pattern];
    fptr f,ftemp, f_detected;
    int fault_type;
    register int i,j,k,start_wire_index, l;
    register int num_of_fault; 
    pptr p, ptemp, plist;
    int fault_sim_evaluate();
    wptr get_faulty_wire();
    int sim();
    static int test_no = 0;
    void display_fault();

    test_no++;
    num_of_fault = 0; // counts the number of faults in a packet
    f_detected = NIL( struct FAULT);
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
    for( l = 0; l < detection_num; l++ )
        if( f = det_flist[l] )
            break;

    while(f)
    {
        
        ftemp = f -> pnext;
        if( !ftemp )
        {
            for( l = f -> det_num + 1; l < detection_num; l++ )
                if( ftemp = det_flist[l] )
                    break;
        }

        /*
        if( ftemp )
            printf( " Sim falut next no: %d\n", ftemp-> fault_no );
        */


        if( f -> detect == REDUNDANT ) 
        {
            f = ftemp;
            continue;
        }
        /* consider only active (aka. excited) fault
         * (sa1 with correct output of 0 or sa0 with correct output of 1) */
        if (f->fault_type != sort_wlist[f->to_swlist]->value) {

            /* if f is a primary output or is directly connected to an primary output
             * the fault is detected */
            if ((f->node->type == OUTPUT) || (f->io == GO && sort_wlist[f->to_swlist]->flag & OUTPUT)) {
                //f->detect = TRUE;
                w = sort_wlist[ f -> to_swlist ];
                if( w -> flag & INPUT)
                    w = w -> nxtpi;
                else
                    w = w -> inode[0] -> fnext -> owire[0];

                if( w -> value == f -> fault_type )
                {
                    f -> pnext_detected = f_detected;
                    f_detected = f;
                    f -> sim_detect = 1;
                }
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
                            //f->detect = TRUE;
                            w = sort_wlist[ f -> to_swlist ];
                            if( w -> flag & INPUT)
                                w = w -> nxtpi;
                            else
                                w = w -> inode[0] -> fnext -> owire[0];

                            if( w -> value == f -> fault_type )
                            {
                                f -> pnext_detected = f_detected;
                                f_detected = f;
                                f -> sim_detect = 1;
                            }


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
        }

        /*
         * fault simulation of a packet 
         */

        /* if this packet is full (16 faults)
         * or there is no more undetected faults remaining,
         * do the fault simulation */
        if ((num_of_fault == num_of_pattern) || !(ftemp)) {

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
                //printf( "wire name %s\n", w -> name );
                //printf("after  : %d\n", w->flag);
                if (w->flag & OUTPUT) { // if primary output 
                    for (i = 0; i < num_of_fault; i++) { // check every undetected fault
                        
                        if (!(simulated_fault_list[i]->detect) && !(simulated_fault_list[i] -> sim_detect)) {
                            if ((w->wire_value2 & Mask[i]) ^      // if value1 != value2
                                    (w->wire_value1 & Mask[i])) {
                                if (((w->wire_value2 & Mask[i]) ^ Unknown[i])&&  // and not unknowns
                                        ((w->wire_value1 & Mask[i]) ^ Unknown[i])){
        
                                    wnext = sort_wlist[ simulated_fault_list[ i ] -> to_swlist ];
                                    if( wnext -> flag & INPUT)
                                        wnext = wnext -> nxtpi;
                                    else
                                        wnext = wnext -> inode[0] -> fnext -> owire[0];
                                    /*
                                    if( simulated_fault_list[i] -> fault_no == 17 )
                                    {
                                        printf( "   wire name: %s \n", wnext -> name);
                                        printf( "   fault %d \n", simulated_fault_list[i] -> fault_type );
                                        printf( "   wnext %d \n", wnext -> wire_value1 & Mask[i] );
                                    }
                                    */
                                    if( (wnext -> wire_value2 & Mask[i] ^ Unknown[i]) && 
                                        ((wnext -> wire_value2 & Mask[i]) == (Mask[i] * simulated_fault_list[i] -> fault_type))) 
                                    {
                                        //remove_fault( simulated_fault_list[i]->detect, 0 ); //then the fault is detected
                                        simulated_fault_list[i] -> pnext_detected = f_detected;
                                        f_detected = simulated_fault_list[i];
                                        f_detected -> sim_detect = 1;
                                    }
                                    //simulated_fault_list[i]->detect = TRUE;  // then the fault is detected
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

        f = ftemp;
    }

/* the following two loops are both for fault dropping  */  
/* drop detected faults from the FRONT of the undetected fault list */

    // if flist pi assignment detect other fault and use less pi, 
    // replace the other fault's pi assignment with flist's
    int assign_num_origin = 0;
    int assign_num_fault = 0;

    for( p = flist -> piassign; p; p = p -> pnext )
        assign_num_origin ++; 

    while( f_detected ){
        assign_num_fault = 0;
        for( p = f_detected -> piassign; p; p = p -> pnext )
            assign_num_fault ++; 
        if( assign_num_origin < assign_num_fault || !f_detected -> piassign )
        {
            for( plist = f_detected -> piassign; plist; plist = ptemp )
            {
                ptemp = plist -> pnext;
                cfree( plist ); 
            }

            f_detected -> piassign = NULL;
            p = f_detected -> piassign;

            for( plist = flist -> piassign; plist; plist = plist -> pnext )
            {
                ptemp = ALLOC(1, struct PIASSIGN);
                ptemp -> index_value = plist -> index_value;
                
                if( p )
                    p -> pnext = ptemp;
                else
                    f_detected -> piassign = ptemp;

                p = ptemp;
            }
        }

        f_detected -> sim_detect = 0;
        f_detected -> detect     = TRUE;
        f = f_detected -> pnext_detected;
        f_detected -> pnext_detected = NULL;
        f_detected  = f;
    }
    

    f_detected = NIL( struct FAULT );

}/* end of fault_sim_a_vecto */

int
fault_sim_a_vector_Moon(num)
    int *num;
{
    fptr f_detected, f,  fault_sim_Moon_int();
    f_detected = fault_sim_Moon_int();
/* the following two loops are both for fault dropping  */  
/* drop detected faults from the FRONT of the undetected fault list */
    //printf("detect: ");
    int no_use = TRUE;
    while( f_detected ) {
        //printf("%d ", f_detected -> fault_no);
        if( f_detected -> det_num != detection_num )
            no_use = FALSE;
        remove_fault( f_detected, 0 );

        f_detected -> sim_detect = 0;
        f = f_detected -> pnext_detected;
        f_detected -> pnext_detected = NIL(struct fault);
        f_detected  = f;
    }
    //printf("\n");

    /*
    for( i = 0; i < detection_num; i++ )
    {
        printf("%d:", i);
        for( f = det_flist[i]; f; f = f -> pnext )
            printf("%d ", f -> fault_no);
        printf("\n");
    
    }
   */
   return no_use;
}/* end of fault_sim_a_vecto */

int
fault_sim_a_vector_Moon_num(num)
    int *num;
{
    fptr f_detected, f,  fault_sim_Moon_int();
    f_detected = fault_sim_Moon_int();
/* the following two loops are both for fault dropping  */  
/* drop detected faults from the FRONT of the undetected fault list */
    //printf("detect: ");
    int total_Det = 0;
    while( f_detected ) {
        //printf("%d ", f_detected -> fault_no);
        ++total_Det;
        //remove_fault( f_detected, 0 );
        f_detected->det_num += 1; // XXX
        f_detected->detect_by[f_detected->det_num%detection_num] = (*num);

        f_detected -> sim_detect = 0;
        f = f_detected -> pnext_detected;
        f_detected -> pnext_detected = NIL(struct fault);
        f_detected  = f;
    }
    //printf("\n");

    /*
    for( i = 0; i < detection_num; i++ )
    {
        printf("%d:", i);
        for( f = det_flist[i]; f; f = f -> pnext )
            printf("%d ", f -> fault_no);
        printf("\n");
    
    }
   */
   return total_Det;
}/* end of fault_sim_a_vecto */

fptr
fault_sim_Moon_int()
{
        wptr w,faulty_wire,wtemp, wnext;
    /* array of 16 fptrs, which points to the 16 faults in a simulation packet  */
    fptr simulated_fault_list[num_of_pattern];
    fptr f,ftemp, f_detected;
    int fault_type;
    register int i,j,k,start_wire_index, l, m;
    register int num_of_fault; 
    pptr p, ptemp, plist;
    int fault_sim_evaluate();
    wptr get_faulty_wire();
    int sim();
    static int test_no = 0;
    void display_fault();

    test_no++;
    num_of_fault = 0; // counts the number of faults in a packet
    f_detected = NIL( struct FAULT);
    /* num_of_current_detect is used to keep track of the number of undetected
     * faults detected by this vector, initialize it to zero */

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
    //if (debug) { display_io(); }
    
    for (i = 0; i < ncktwire; i++) {
        if (i < ncktin) {
            sort_wlist[i] -> p1_flag |= CHANGED;
        }
        else {
            sort_wlist[i] -> p1_value = 2;
        }
    }
    sim_Moon();

    //for (i = 0; i < ncktwire; i++)
    //    printf("wirename: %s %d %d\n", sort_wlist[i] -> name, sort_wlist[i] -> value, sort_wlist[i] -> p1_value);


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
    for( l = 0; l < detection_num; l++ )
        if( f = det_flist[l] )
            break;

    while(f)
    {
        ftemp = f -> pnext;
        if( !ftemp )
        {
            for( l = f -> det_num + 1; l < detection_num; l++ )
                if( ftemp = det_flist[l] )
                    break;
        }

        /*
        if( ftemp )
            printf( " Sim falut next no: %d\n", ftemp-> fault_no );
        */

        //printf("falut nonnniniini: %d %d %d\n", f -> fault_no, sort_wlist[ f -> to_swlist ] -> value, sort_wlist[ f -> to_swlist] -> p1_value );

        if( f -> detect == REDUNDANT ) 
        {
            f = ftemp;
            //continue;
        }
        else
        {
            /* consider only active (aka. excited) fault
             * (sa1 with correct output of 0 or sa0 with correct output of 1) */
            if ( f->fault_type != sort_wlist[f->to_swlist]->value && 
                 2 != sort_wlist[f->to_swlist]->value && 
                 f->fault_type == sort_wlist[f->to_swlist]->p1_value ) {
                 //printf("falut nonnniniini: %d %d\n", f -> fault_no, f -> sim_detect );

                /* if f is a primary output or is directly connected to an primary output
                 * the fault is detected */
                if ((f->node->type == OUTPUT) || (f->io == GO && sort_wlist[f->to_swlist]->flag & OUTPUT)) {
                    //f->detect = TRUE;
                    f -> pnext_detected = f_detected;
                    f_detected = f;
                    f -> sim_detect = 1;
                    //printf("    detect: %d\n", f -> fault_no );
                        //continue;
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

                        //printf("Here fault no: %d\n", f -> fault_no );
                        /* add the fault to the simulated fault list and inject the fault */
                        //printf("INJECT: %x\n", sort_wlist[f -> to_swlist] -> wire_value2);
                        simulated_fault_list[num_of_fault] = f;
                        inject_fault_value(sort_wlist[f->to_swlist], num_of_fault,
                                f->fault_type); 
                        //printf("INJECT: %x\n", sort_wlist[f -> to_swlist] -> wire_value2);

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
                                //f->detect = TRUE;
                                f -> pnext_detected = f_detected;
                                f_detected = f;
                                f -> sim_detect = 1;
                                //printf("    detect: %d\n", f -> fault_no );
                            }

                            else {

                                /* if faulty_wire is not already marked as faulty, mark it as faulty
                                 * and add the wire to the list of faulty wires. */
                                //printf("Here fault no: %d\n", f -> fault_no);
                                if (!(faulty_wire->flag & FAULTY)) {
                                    faulty_wire->pnext = first_faulty_wire;
                                    first_faulty_wire = faulty_wire;
                                    first_faulty_wire->flag |= FAULTY;
                                }

                                /* add the fault to the simulated list and inject it */
                                simulated_fault_list[num_of_fault] = f;

                                inject_fault_value(faulty_wire, num_of_fault,
                                        fault_type );



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
            }
        }

        /*
         * fault simulation of a packet 
         */

        /* if this packet is full (16 faults)
         * or there is no more undetected faults remaining,
         * do the fault simulation */
        if ((num_of_fault == num_of_pattern) || !(ftemp)) {

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
                //printf( "wire name %s\n", w -> name );
                //printf("after  : %d\n", w->flag);
                if (w->flag & OUTPUT) { // if primary output 
                    for (i = 0; i < num_of_fault; i++) { // check every undetected fault     
                        //printf("falut no: %d\n", simulated_fault_list[i] -> fault_no );
                        if (!(simulated_fault_list[i]->detect) && !(simulated_fault_list[i] -> sim_detect)) {
                            if ((w->wire_value2 & Mask[i]) ^      // if value1 != value2
                                    (w->wire_value1 & Mask[i])) {
                                if (((w->wire_value2 & Mask[i]) ^ Unknown[i])&&  // and not unknowns
                                        ((w->wire_value1 & Mask[i]) ^ Unknown[i])){

                                        simulated_fault_list[i] -> sim_detect = 1;
                                        simulated_fault_list[i] -> pnext_detected = f_detected;
                                        f_detected = simulated_fault_list[i];
                                        //printf("    detect: %d\n", f_detected -> fault_no );
                                    //simulated_fault_list[i]->detect = TRUE;  // then the fault is detected
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

        f = ftemp;
    }
    return f_detected;
}

/* fault simulate a single test vector */
fault_sim_a_vector_frame01_X(num_of_current_detect)
    int *num_of_current_detect;
{
    wptr w,faulty_wire,wtemp, wnext;
    /* array of 16 fptrs, which points to the 16 faults in a simulation packet  */
    fptr simulated_fault_list[num_of_pattern];
    fptr f,ftemp, f_detected;
    int fault_type;
    register int i,j,k,start_wire_index, l, m;
    register int num_of_fault; 
    pptr p, ptemp, plist;
    int fault_sim_evaluate();
    wptr get_faulty_wire();
    int sim();
    static int test_no = 0;
    void display_fault();

    test_no++;
    num_of_fault = 0; // counts the number of faults in a packet
    f_detected = NIL( struct FAULT);
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
    //if (debug) { display_io(); }

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
    for( l = 0; l < detection_num; l++ )
        if( f = det_flist[l] )
            break;

    while(f)
    {
        ftemp = f -> pnext;
        if( !ftemp )
        {
            for( l = f -> det_num + 1; l < detection_num; l++ )
                if( ftemp = det_flist[l] )
                    break;
        }

        /*
        if( ftemp )
            printf( " Sim falut next no: %d\n", ftemp-> fault_no );
        */


        if( f -> detect == REDUNDANT ) 
        {
            f = ftemp;
            continue;
        }
        /* consider only active (aka. excited) fault
         * (sa1 with correct output of 0 or sa0 with correct output of 1) */
        if (f->fault_type != sort_wlist[f->to_swlist]->value) {

            /* if f is a primary output or is directly connected to an primary output
             * the fault is detected */
            if ((f->node->type == OUTPUT) || (f->io == GO && sort_wlist[f->to_swlist]->flag & OUTPUT)) {
                //f->detect = TRUE;
                w = sort_wlist[ f -> to_swlist ];
                if( w -> flag & INPUT)
                    w = w -> nxtpi;
                else
                    w = w -> inode[0] -> fnext -> owire[0];

                if( w -> value == f -> fault_type )
                {
                    f -> pnext_detected = f_detected;
                    f_detected = f;
                    f -> sim_detect = 1;
                    //continue;
                }
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
                            //f->detect = TRUE;
                            w = sort_wlist[ f -> to_swlist ];
                            if( w -> flag & INPUT)
                                w = w -> nxtpi;
                            else
                                w = w -> inode[0] -> fnext -> owire[0];

                            if( w -> value == f -> fault_type )
                            {
                                f -> pnext_detected = f_detected;
                                f_detected = f;
                                f -> sim_detect = 1;
                                //continue;
                            }


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
        }

        /*
         * fault simulation of a packet 
         */

        /* if this packet is full (16 faults)
         * or there is no more undetected faults remaining,
         * do the fault simulation */
        if ((num_of_fault == num_of_pattern) || !(ftemp)) {

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
                //printf( "wire name %s\n", w -> name );
                //printf("after  : %d\n", w->flag);
                if (w->flag & OUTPUT) { // if primary output 
                    for (i = 0; i < num_of_fault; i++) { // check every undetected fault
                        
                        if (!(simulated_fault_list[i]->detect) && !(simulated_fault_list[i] -> sim_detect)) {
                            if ((w->wire_value2 & Mask[i]) ^      // if value1 != value2
                                    (w->wire_value1 & Mask[i])) {
                                if (((w->wire_value2 & Mask[i]) ^ Unknown[i])&&  // and not unknowns
                                        ((w->wire_value1 & Mask[i]) ^ Unknown[i])){
        
                                    wnext = sort_wlist[ simulated_fault_list[ i ] -> to_swlist ];
                                    if( wnext -> flag & INPUT)
                                        wnext = wnext -> nxtpi;
                                    else
                                        wnext = wnext -> inode[0] -> fnext -> owire[0];
                                    /*
                                    if( simulated_fault_list[i] -> fault_no == 17 )
                                    {
                                        printf( "   wire name: %s \n", wnext -> name);
                                        printf( "   fault %d \n", simulated_fault_list[i] -> fault_type );
                                        printf( "   wnext %d \n", wnext -> wire_value1 & Mask[i] );
                                    }
                                    */
                                    if( (wnext -> wire_value2 & Mask[i] ^ Unknown[i]) && 
                                        ((wnext -> wire_value2 & Mask[i]) == (Mask[i] * simulated_fault_list[i] -> fault_type))) 
                                    {
                                        //remove_fault( simulated_fault_list[i]->detect, 0 ); //then the fault is detected
                                        simulated_fault_list[i] -> sim_detect = 1;
                                        simulated_fault_list[i] -> pnext_detected = f_detected;
                                        f_detected = simulated_fault_list[i];
                                            //printf("%d fault in detected list\n", simulated_fault_list[i] -> fault_no);
                                    }
                                    //simulated_fault_list[i]->detect = TRUE;  // then the fault is detected
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

        f = ftemp;
    }

/* the following two loops are both for fault dropping  */  
/* drop detected faults from the FRONT of the undetected fault list */
   
    //printf("detect: ");
    while( f_detected ) {
        //printf("%d ", f_detected -> fault_no);
        remove_fault( f_detected, 0 );
        (*num_of_current_detect) += f_detected -> eqv_fault_num;
        f_detected -> sim_detect = 0;
        f = f_detected -> pnext_detected;
        f_detected -> pnext_detected = NIL(struct fault);
        f_detected  = f;
    }
    //printf("\n");

    /*
    for( i = 0; i < detection_num; i++ )
    {
        printf("%d:", i);
        for( f = det_flist[i]; f; f = f -> pnext )
            printf("%d ", f -> fault_no);
        printf("\n");
    
    }
   */
   return;
}/* end of fault_sim_a_vecto */

/* fault simulate a single test vector */
    int
fault_sim_a_vector_frame01_Y(num_of_current_detect)
    int *num_of_current_detect;
{
    wptr w,faulty_wire,wtemp, wnext;
    /* array of 16 fptrs, which points to the 16 faults in a simulation packet  */
    fptr simulated_fault_list[num_of_pattern];
    fptr f,ftemp, f_detected;
    int fault_type;
    register int i,j,k,start_wire_index, l, m;
    register int num_of_fault; 
    pptr p, ptemp, plist;
    int fault_sim_evaluate();
    wptr get_faulty_wire();
    int sim();
    static int test_no = 0;
    void display_fault();

    test_no++;
    num_of_fault = 0; // counts the number of faults in a packet
    f_detected = NIL( struct FAULT);
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
    //if (debug) { display_io(); }

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
    for( l = 0; l < detection_num; l++ )
        if( f = det_flist[l] )
            break;

    while(f)
    {
        ftemp = f -> pnext;
        if( !ftemp )
        {
            for( l = f -> det_num + 1; l < detection_num; l++ )
                if( ftemp = det_flist[l] )
                    break;
        }

        /*
        if( ftemp )
            printf( " Sim falut next no: %d\n", ftemp-> fault_no );
        */


        if( f -> detect == REDUNDANT ) 
        {
            f = ftemp;
            continue;
        }

        //printf("Y falut sim: %d\n", f -> fault_no);
        /* consider only active (aka. excited) fault
         * (sa1 with correct output of 0 or sa0 with correct output of 1) */
        if (f->fault_type != sort_wlist[f->to_swlist]->value) {

            /* if f is a primary output or is directly connected to an primary output
             * the fault is detected */
            if ((f->node->type == OUTPUT) || (f->io == GO && sort_wlist[f->to_swlist]->flag & OUTPUT)) {
                //f->detect = TRUE;
                w = sort_wlist[ f -> to_swlist ];
                if( w -> flag & INPUT)
                    w = w -> nxtpi;
                else
                    w = w -> inode[0] -> fnext -> owire[0];

                if( w -> value == f -> fault_type )
                {
                    f -> pnext_detected = f_detected;
                    f_detected = f;
                    f -> sim_detect = 1;
                    //continue;
                }
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
                            //f->detect = TRUE;
                            w = sort_wlist[ f -> to_swlist ];
                            if( w -> flag & INPUT)
                                w = w -> nxtpi;
                            else
                                w = w -> inode[0] -> fnext -> owire[0];

                            if( w -> value == f -> fault_type )
                            {
                                f -> pnext_detected = f_detected;
                                f_detected = f;
                                f -> sim_detect = 1;
                                //continue;
                            }


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
        }

        /*
         * fault simulation of a packet 
         */

        /* if this packet is full (16 faults)
         * or there is no more undetected faults remaining,
         * do the fault simulation */
        if ((num_of_fault == num_of_pattern) || !(ftemp)) {

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
                //printf( "wire name %s\n", w -> name );
                //printf("after  : %d\n", w->flag);
                if (w->flag & OUTPUT) { // if primary output 
                    for (i = 0; i < num_of_fault; i++) { // check every undetected fault
                        
                        if (!(simulated_fault_list[i]->detect) && !(simulated_fault_list[i] -> sim_detect)) {
                            if ((w->wire_value2 & Mask[i]) ^      // if value1 != value2
                                    (w->wire_value1 & Mask[i])) {
                                if (((w->wire_value2 & Mask[i]) ^ Unknown[i])&&  // and not unknowns
                                        ((w->wire_value1 & Mask[i]) ^ Unknown[i])){
        
                                    wnext = sort_wlist[ simulated_fault_list[ i ] -> to_swlist ];
                                    if( wnext -> flag & INPUT)
                                        wnext = wnext -> nxtpi;
                                    else
                                        wnext = wnext -> inode[0] -> fnext -> owire[0];
                                    /*
                                    if( simulated_fault_list[i] -> fault_no == 17 )
                                    {
                                        printf( "   wire name: %s \n", wnext -> name);
                                        printf( "   fault %d \n", simulated_fault_list[i] -> fault_type );
                                        printf( "   wnext %d \n", wnext -> wire_value1 & Mask[i] );
                                    }
                                    */
                                    if( (wnext -> wire_value2 & Mask[i] ^ Unknown[i]) && 
                                        ((wnext -> wire_value2 & Mask[i]) == (Mask[i] * simulated_fault_list[i] -> fault_type))) 
                                    {
                                        //remove_fault( simulated_fault_list[i]->detect, 0 ); //then the fault is detected
                                        simulated_fault_list[i] -> sim_detect = 1;
                                        simulated_fault_list[i] -> pnext_detected = f_detected;
                                        f_detected = simulated_fault_list[i];
                                            //printf("%d fault in detected list\n", simulated_fault_list[i] -> fault_no);
                                    }
                                    //simulated_fault_list[i]->detect = TRUE;  // then the fault is detected
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

        f = ftemp;
    }

/* the following two loops are both for fault dropping  */  
/* drop detected faults from the FRONT of the undetected fault list */
 
    int no_use = TRUE;
    //printf("detect: ");
    //if (!f_detected)
    //    printf("excuse me??\n");
    while( f_detected ) {
        //printf("      %d\n", f_detected -> fault_no);
        
        if( f_detected -> det_num != detection_num )
            no_use = FALSE;
        remove_fault( f_detected, 0 );
        (*num_of_current_detect) += f_detected -> eqv_fault_num;
        f_detected -> sim_detect = 0;
        f = f_detected -> pnext_detected;
        f_detected -> pnext_detected = NIL(struct fault);
        f_detected  = f;
    }
    //printf("\n");

    /*
    for( i = 0; i < detection_num; i++ )
    {
        printf("%d:", i);
        for( f = det_flist[i]; f; f = f -> pnext )
            printf("%d ", f -> fault_no);
        printf("\n");
    
    }
   */
   return no_use;
}/* end of fault_sim_a_vecto */

fault_sim_a_vector_frame01_Z(num_of_current_detect)
    int *num_of_current_detect;
{
    wptr w,faulty_wire,wtemp, wnext;
    /* array of 16 fptrs, which points to the 16 faults in a simulation packet  */
    fptr simulated_fault_list[num_of_pattern];
    fptr f,ftemp, f_detected;
    int fault_type;
    register int i,j,k,start_wire_index, l;
    register int num_of_fault; 
    pptr p, ptemp, plist;
    int fault_sim_evaluate();
    wptr get_faulty_wire();
    int sim();
    static int test_no = 0;
    void display_fault();

    test_no++;
    num_of_fault = 0; // counts the number of faults in a packet
    f_detected = NIL( struct FAULT);
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
    for( l = 0; l < detection_num; l++ )
        if( f = det_flist[l] )
            break;

    while(f)
    {
        
        ftemp = f -> pnext;
        if( !ftemp )
        {
            for( l = f -> det_num + 1; l < detection_num; l++ )
                if( ftemp = det_flist[l] )
                    break;
        }

        /*
        if( ftemp )
            printf( " Sim falut next no: %d\n", ftemp-> fault_no );
        */


        if( f -> detect == REDUNDANT) 
        {
            f = ftemp;
            continue;
        }
        /* consider only active (aka. excited) fault
         * (sa1 with correct output of 0 or sa0 with correct output of 1) */
        if (f->fault_type != sort_wlist[f->to_swlist]->value) {

            /* if f is a primary output or is directly connected to an primary output
             * the fault is detected */
            if ((f->node->type == OUTPUT) || (f->io == GO && sort_wlist[f->to_swlist]->flag & OUTPUT)) {
                //f->detect = TRUE;
                w = sort_wlist[ f -> to_swlist ];
                if( w -> flag & INPUT)
                    w = w -> nxtpi;
                else
                    w = w -> inode[0] -> fnext -> owire[0];

                if( w -> value == f -> fault_type )
                {
                    f -> pnext_detected = f_detected;
                    f_detected = f;
                    f -> sim_detect = 1;
                }
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
                            //f->detect = TRUE;
                            w = sort_wlist[ f -> to_swlist ];
                            if( w -> flag & INPUT)
                                w = w -> nxtpi;
                            else
                                w = w -> inode[0] -> fnext -> owire[0];

                            if( w -> value == f -> fault_type )
                            {
                                f -> pnext_detected = f_detected;
                                f_detected = f;
                                f -> sim_detect = 1;
                            }


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
        }

        /*
         * fault simulation of a packet 
         */

        /* if this packet is full (16 faults)
         * or there is no more undetected faults remaining,
         * do the fault simulation */
        if ((num_of_fault == num_of_pattern) || !(ftemp)) {

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
                //printf( "wire name %s\n", w -> name );
                //printf("after  : %d\n", w->flag);
                if (w->flag & OUTPUT) { // if primary output 
                    for (i = 0; i < num_of_fault; i++) { // check every undetected fault
                        
                        if (!(simulated_fault_list[i]->detect) && !(simulated_fault_list[i] -> sim_detect)) {
                            if ((w->wire_value2 & Mask[i]) ^      // if value1 != value2
                                    (w->wire_value1 & Mask[i])) {
                                if (((w->wire_value2 & Mask[i]) ^ Unknown[i])&&  // and not unknowns
                                        ((w->wire_value1 & Mask[i]) ^ Unknown[i])){
        
                                    wnext = sort_wlist[ simulated_fault_list[ i ] -> to_swlist ];
                                    if( wnext -> flag & INPUT)
                                        wnext = wnext -> nxtpi;
                                    else
                                        wnext = wnext -> inode[0] -> fnext -> owire[0];
                                    /*
                                    if( simulated_fault_list[i] -> fault_no == 17 )
                                    {
                                        printf( "   wire name: %s \n", wnext -> name);
                                        printf( "   fault %d \n", simulated_fault_list[i] -> fault_type );
                                        printf( "   wnext %d \n", wnext -> wire_value1 & Mask[i] );
                                    }
                                    */
                                    if( (wnext -> wire_value2 & Mask[i] ^ Unknown[i]) && 
                                        ((wnext -> wire_value2 & Mask[i]) == (Mask[i] * simulated_fault_list[i] -> fault_type))) 
                                    {
                                        //remove_fault( simulated_fault_list[i]->detect, 0 ); //then the fault is detected
                                        simulated_fault_list[i] -> pnext_detected = f_detected;
                                        f_detected = simulated_fault_list[i];
                                        f_detected -> sim_detect = 1;
                                    }
                                    //simulated_fault_list[i]->detect = TRUE;  // then the fault is detected
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

        f = ftemp;
    }

/* the following two loops are both for fault dropping  */  
/* drop detected faults from the FRONT of the undetected fault list */

    // if flist pi assignment detect other fault and use less pi, 
    // replace the other fault's pi assignment with flist's

    while( f_detected ){

        f_detected -> sim_detect = 0;
        f_detected -> detect     = TRUE;
        f = f_detected -> pnext_detected;
        f_detected -> pnext_detected = NULL;
        f_detected  = f;
    }
    

    f_detected = NIL( struct FAULT );

}/* end of fault_sim_a_vecto */

/* fault simulate a single test vector */
fault_sim_a_vector_frame01_Sun( pat_id, num_of_current_detect)
    int pat_id;
    int *num_of_current_detect;
{
    wptr w,faulty_wire,wtemp, wnext;
    /* array of 16 fptrs, which points to the 16 faults in a simulation packet  */
    fptr simulated_fault_list[num_of_pattern];
    fptr f,ftemp, f_detected;
    int fault_type;
    register int i,j,k,start_wire_index, l, m;
    register int num_of_fault; 
    pptr p, ptemp, plist;
    int fault_sim_evaluate();
    wptr get_faulty_wire();
    int sim();
    static int test_no = 0;
    void display_fault();

    test_no++;
    num_of_fault = 0; // counts the number of faults in a packet
    f_detected = NIL( struct FAULT);
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
    //if (debug) { display_io(); }

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
    for( l = 0; l < detection_num; l++ )
        if( f = det_flist[l] )
            break;

    while(f)
    {
        ftemp = f -> pnext;
        if( !ftemp )
        {
            for( l = f -> det_num + 1; l < detection_num; l++ )
                if( ftemp = det_flist[l] )
                    break;
        }

        /*
        if( ftemp )
            printf( " Sim falut next no: %d\n", ftemp-> fault_no );
        */
        /*
        wnext = sort_wlist[ f -> to_swlist ];
        if( wnext -> flag & INPUT)
            wnext = wnext -> nxtpi;
        else
            wnext = wnext -> inode[0] -> fnext -> owire[0];
        */

        if( f -> detect == REDUNDANT )// || sort_wlist[f->to_swlist]->value == 2 || wnext -> value == 2) // XXX
        {
            f = ftemp;
            continue;
        }

        // XXX XXX
        
        /* consider only active (aka. excited) fault
         * (sa1 with correct output of 0 or sa0 with correct output of 1) */
        if (f->fault_type != sort_wlist[f->to_swlist]->value) {
            /* if f is a primary output or is directly connected to an primary output
             * the fault is detected */
            if ((f->node->type == OUTPUT) || (f->io == GO && sort_wlist[f->to_swlist]->flag & OUTPUT)) {
                //f->detect = TRUE;
                w = sort_wlist[ f -> to_swlist ];
                if( w -> flag & INPUT)
                    w = w -> nxtpi;
                else
                    w = w -> inode[0] -> fnext -> owire[0];

                if( w -> value == f -> fault_type )
                {
                    f -> pnext_detected = f_detected;
                    f_detected = f;
                    f -> sim_detect = 1;
                    //if( pat_id == 1 && f -> fault_no == 390 )
                    //    printf( "   Sun detect A\n");
                        
                    //continue;
                }
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
                            //f->detect = TRUE;
                            w = sort_wlist[ f -> to_swlist ];
                            if( w -> flag & INPUT)
                                w = w -> nxtpi;
                            else
                                w = w -> inode[0] -> fnext -> owire[0];

                            if( w -> value == f -> fault_type )
                            {
                                f -> pnext_detected = f_detected;
                                f_detected = f;
                                f -> sim_detect = 1;
                                //if( pat_id == 1 && f -> fault_no == 390 )
                                //    printf( "   Sun detect B\n");
                                //continue;
                            }


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
        }

        /*
         * fault simulation of a packet 
         */

        /* if this packet is full (16 faults)
         * or there is no more undetected faults remaining,
         * do the fault simulation */
        if ((num_of_fault == num_of_pattern) || !(ftemp)) {

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
                //printf( "wire name %s\n", w -> name );
                //printf("after  : %d\n", w->flag);
                if (w->flag & OUTPUT) { // if primary output 
                    for (i = 0; i < num_of_fault; i++) { // check every undetected fault
                        
                        if (!(simulated_fault_list[i]->detect) && !(simulated_fault_list[i] -> sim_detect)) {
                            if ((w->wire_value2 & Mask[i]) ^      // if value1 != value2
                                    (w->wire_value1 & Mask[i])) {
                                if (((w->wire_value2 & Mask[i]) ^ Unknown[i])&&  // and not unknowns
                                        ((w->wire_value1 & Mask[i]) ^ Unknown[i])){
        
                                    wnext = sort_wlist[ simulated_fault_list[ i ] -> to_swlist ];
                                    if( wnext -> flag & INPUT)
                                        wnext = wnext -> nxtpi;
                                    else
                                        wnext = wnext -> inode[0] -> fnext -> owire[0];
                                    /*
                                    if( simulated_fault_list[i] -> fault_no == 17 )
                                    {
                                        printf( "   wire name: %s \n", wnext -> name);
                                        printf( "   fault %d \n", simulated_fault_list[i] -> fault_type );
                                        printf( "   wnext %d \n", wnext -> wire_value1 & Mask[i] );
                                    }
                                    */
                                                                                      // XXX XXX
                                    if( (wnext -> wire_value2 & Mask[i] ^ Unknown[i]) &&  (wnext -> wire_value1 & Mask[i] ^ Unknown[i]) &&
                                        ((wnext -> wire_value2 & Mask[i]) == (Mask[i] * simulated_fault_list[i] -> fault_type))) 
                                    {
                                        //remove_fault( simulated_fault_list[i]->detect, 0 ); //then the fault is detected
                                        simulated_fault_list[i] -> sim_detect = 1;
                                        simulated_fault_list[i] -> pnext_detected = f_detected;
                                        f_detected = simulated_fault_list[i];
                                        //if( pat_id == 1 && f_detected -> fault_no == 390 )
                                           // printf( "   Sun detect C \n");
                                            //printf("%d fault in detected list\n", simulated_fault_list[i] -> fault_no);
                                    }
                                    //simulated_fault_list[i]->detect = TRUE;  // then the fault is detected
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

        f = ftemp;
    }

/* the following two loops are both for fault dropping  */  
/* drop detected faults from the FRONT of the undetected fault list */
 
    //printf("detect: ");
    //if (!f_detected)
    //    printf("excuse me??\n");
    while( f_detected ) {
        //printf("      %d\n", f_detected -> fault_no);
        // XXX
        /*
        if(sort_wlist[f_detected->to_swlist]->value == 2 ){
            f_detected -> sim_detect = 0;
            f = f_detected -> pnext_detected;
            f_detected -> pnext_detected = NIL(struct fault);
            f_detected  = f;
            continue;
        }
        */
        //if( pat_id == 0 )
        //    printf("      %d\n", f_detected -> fault_no);
        if( f_detected -> detect_by == -1 )
            f_detected -> detect_by = pat_id;
        //remove_fault( f_detected, 0 );
        (*num_of_current_detect) += f_detected -> eqv_fault_num;
        f_detected -> sim_detect = 0;
        f = f_detected -> pnext_detected;
        f_detected -> pnext_detected = NIL(struct fault);
        f_detected  = f;
    }
    //printf("\n");

}/* end of fault_sim_a_vecto */


    void
remove_fault( f , remove )
    fptr f;
    int remove;
{
    int i;
    fptr fprev;
    fptr fnext;

    fprev = f -> pprev;
    fnext = f -> pnext;

    if( fprev )
        fprev -> pnext = fnext;
    if( fnext )
        fnext -> pprev = fprev;

    if( det_flist[ f -> det_num ] == f )
        det_flist[ f -> det_num ] = fnext;

    if( remove == 1)
    {
        f -> pprev = NULL;
        f -> pnext = NULL;
        return;
    }

    f -> det_num ++;
    if( f -> det_num == detection_num )
    {
        f -> pprev = NULL;
        f -> pnext = NULL;
        f -> detect = TRUE;
        return;
    }


    if( det_flist[ f -> det_num ] )
    {
        det_flist[ f -> det_num ] -> pprev = f;
        f -> pnext = det_flist[ f -> det_num ];
        f -> pprev = NIL( struct FAULT );
        det_flist[ f -> det_num ] = f;
    }
    else
    {
        det_flist[ f -> det_num ] = f;
        f -> pprev = NIL( struct FAULT );
        f -> pnext = NIL( struct FAULT );
    }


}



check2()
{
    register int i;

    for (i = 0; i < ncktwire; i++) {
        if (sort_wlist[i]->flag & (FAULTY | FAULT_INJECTED | SCHEDULED) ||
                (sort_wlist[i]->fault_flag)){
            fprintf(stdout,"something is fishy(check2)...\n");
        }
    }
    return;
}/* end of check2 */


check()
{
    register int i;

    for (i = 0; i < ncktwire; i++) {
        if (sort_wlist[i]->wire_value1 != sort_wlist[i]->wire_value2) {
            fprintf(stdout,"something is fishy...\n");
        }
    }
    return;
}/* end of check */


/* evaluate wire w 
 * 1. update w->wire_value2 
 * 2. schedule new events if value2 != value1 */
fault_sim_evaluate(w)
    wptr w;
{
    unsigned int new_value;
    register nptr n;
    register int i;
    int combine();
    unsigned int PINV(),PEXOR(),PEQUIV();

    /*if (w->flag & INPUT) return;*/
    n = w->inode[0];
    switch(n->type) {
        /*break a multiple-input gate into multiple two-input gates */
        case AND:
        case BUF:
        case NAND:
            new_value = ALL_ONE;
            for (i = 0; i < n->nin; i++) {
                new_value &= n->iwire[i]->wire_value2;
            }
            if (n->type == NAND) {
                new_value = PINV(new_value);  // PINV is for three-valued inversion
            }
            break;
            /*  */
        case OR:
        case NOR:
            new_value = ALL_ZERO;
            for (i = 0; i < n->nin; i++) {
                new_value |= n->iwire[i]->wire_value2;
            }
            if (n->type == NOR) {
                new_value = PINV(new_value);
            }
            break;

        case NOT:
            new_value = PINV(n->iwire[0]->wire_value2);
            break;

        case XOR:
            new_value = PEXOR(n->iwire[0]->wire_value2,
                    n->iwire[1]->wire_value2);
            break;

        case EQV:
            new_value = PEQUIV(n->iwire[0]->wire_value2,
                    n->iwire[1]->wire_value2);
            break;
    }

    /* if the new_value is different than the wire_value1 (the good value),
     * save it */
    if (w->wire_value1 != new_value) {

        /* if this wire is faulty, make sure the fault remains injected */
        if (w->flag & FAULT_INJECTED) {
            combine(w,&new_value);
        }

        /* update wire_value2 */ 
        w->wire_value2 = new_value;

        /* insert wire w into the faulty_wire list */
        if (!(w->flag & FAULTY)) {
            w->flag |= FAULTY;
            w->pnext = first_faulty_wire;
            first_faulty_wire = w;
        }

        /* schedule new events */
        for (i = 0; i < w->nout; i++) {
            if (w->onode[i]->type != OUTPUT) {
                w->onode[i]->owire[0]->flag |= SCHEDULED;
            }
        }
    } // if new_value is differnt
    // if new_value is the same as the good value, do not schedule any new event
    return;
}/* end of fault_sim_evaluate */

/* Given a gate-input fault f, check if f is propagated to the gate output.
   If so, returns the gate output wire as the faulty_wire.
   Also returns the gate output fault type. 
   */
wptr get_faulty_wire(f,fault_type)
    fptr f;
    int *fault_type;
{
    register int i,is_faulty;
    is_faulty = TRUE;
    switch(f->node->type) {

        /* this case should not occur,
         * because we do not create fault in the NOT BUF gate input */
        case NOT:
        case BUF:
            fprintf(stdout,"something is fishy(get_faulty_net)...\n");
            break;

            /*check every gate input of AND 
              if any input is zero or unknown, then fault f is not propagated */ 
        case AND:
            for (i = 0; i < f->node->nin; i++) {
                if (f->node->iwire[i] != sort_wlist[f->to_swlist]) {
                    if (f->node->iwire[i]->value != 1) {
                        is_faulty = FALSE;  // not propagated
                    }
                }
            }
            /* AND gate input stuck-at one fault is propagated to 
               AND gate output stuck-at one fault */
            //*fault_type = STUCK1;  
            *fault_type = f -> fault_type;  
            break;

        case NAND:
            for (i = 0; i < f->node->nin; i++) {
                if (f->node->iwire[i] != sort_wlist[f->to_swlist]) {
                    if (f->node->iwire[i]->value != 1) {
                        is_faulty = FALSE;
                    }
                }
            }
            //*fault_type = STUCK0;
            *fault_type = f -> fault_type ^ 1;  
            break;
        case OR:
            for (i = 0; i < f->node->nin; i++) {
                if (f->node->iwire[i] != sort_wlist[f->to_swlist]) {
                    if (f->node->iwire[i]->value != 0) {
                        is_faulty = FALSE;
                    }
                }
            }
            //*fault_type = STUCK0;
            *fault_type = f -> fault_type;  
            break;
        case  NOR:
            for (i = 0; i < f->node->nin; i++) {
                if (f->node->iwire[i] != sort_wlist[f->to_swlist]) {
                    if (f->node->iwire[i]->value != 0) {
                        is_faulty = FALSE;
                    }
                }
            }
            //*fault_type = STUCK1;
            *fault_type = f -> fault_type ^ 1;  
            break;
        case XOR:
            for (i = 0; i < f->node->nin; i++) {
                if (f->node->iwire[i] != sort_wlist[f->to_swlist]) {
                    if (f->node->iwire[i]->value == 0) {
                        *fault_type = f->fault_type;
                    }
                    else {
                        *fault_type = f->fault_type ^ 1;
                    }
                }
            }
            break;
        case EQV:
            for (i = 0; i < f->node->nin; i++) {
                if (f->node->iwire[i] != sort_wlist[f->to_swlist]) {
                    if (f->node->iwire[i]->value == 0) {
                        *fault_type = f->fault_type ^ 1;
                    }
                    else {
                        *fault_type = f->fault_type;
                    }
                }
            }
            break;
    }
    if (is_faulty) {
        return(f->node->owire[0]);
    }
    return(NULL);
}/* end of get_faulty_wire */


inject_fault_value(faulty_wire,bit_position,fault)
    wptr faulty_wire;
    int bit_position,fault;
{
    if (fault) faulty_wire->wire_value2 |= Mask[bit_position];// SA1 fault
    else faulty_wire->wire_value2 &= ~Mask[bit_position]; // SA0 fault
    faulty_wire->fault_flag |= Mask[bit_position];// bit position of the fault 
    return;
}/* end of inject_fault_value */


/* For each fault in this packet, check
 * if wire w itself is the fault site, do not change its wire_value2.
 * (because the wire_value2 was already decided by the inject_fault_value fucntion) */
combine(w,new_value)
    wptr w;
    unsigned int *new_value;
{
    register int i;

    for (i = 0; i < num_of_pattern; i++) {
        if (w->fault_flag & Mask[i]) {
            *new_value &= ~Mask[i];
            *new_value |= (w->wire_value2 & Mask[i]);
        }
    }
    return;
} /* end of combine */


/* for three-valued logic inversion
 * Swap the odd bits and the even bits,
 * then do a inversion.
 * The purpose of swaping is to keep the unknown u=01 unchanged after inversion */  
    unsigned int
PINV(value)
    unsigned int value;
{
    return((((value & 0x55555555) << 1) ^ 0xaaaaaaaa) | 
            (((value & 0xaaaaaaaa) >> 1) ^ 0x55555555));
}/* end of PINV */


    unsigned int
PEXOR(value1,value2)
    unsigned int value1,value2;
{
    unsigned int PINV();
    return((value1 & PINV(value2)) | (PINV(value1) & value2));
}/* end of PEXOR */


    unsigned int
PEQUIV(value1,value2)
    unsigned int value1,value2;
{
    unsigned int PINV();
    return((value1 | PINV(value2)) & (PINV(value1) | value2));
}/* end of PEQUIV */
