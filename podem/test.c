#include <stdio.h>
#include "global.h"
#include "miscell.h"

#define U  2
// dededededededd


extern int total_attempt_num;

fptr
choose_primary_fault()
{
    fptr p;
    int  i;
    for( i = 0; i < detection_num; i++ )
    {
        if( p = det_flist[i] )
            break;
    }
    return p;
}

fptr
choose_second_fault( s_fault )
    fptr s_fault;
{
    fptr p;
    int  i;

    if( p = s_fault -> pnext )
        return p;
    else
    {
        for( i = s_fault -> det_num + 1; i < detection_num; i++ )
        {
            if( p = det_flist[i] )
                break;
        }
    }
    return p;
}

// if x number in pi is lower than a threshold return TRUE
// else return FALSE
static double threshold = 0.5;

int 
pi_x_num()
{
    int x_num = 0;
    int i;
    for( i = 0; i < ncktin; i++ )
    {
        if( sort_wlist[i] -> value == U ) 
            x_num ++;
    }

    if( (double)x_num / (double) ncktin < threshold )
    {
        /// assin all unknown PI and save this pattern in plist
        for( i = 0; i < ncktin; i++ )
        {
            if( sort_wlist[i] -> value == U ) 
            {
                sort_wlist[i] -> value = rand()&01;
                if( sort_wlist[i] -> pvspi != NULL )
                    sort_wlist[i] -> pvspi -> p1_value = sort_wlist[i] -> value;
            }
            
            if( i == ncktin - 1 && sort_wlist[i] -> p1_value == U ) 
                sort_wlist[i] -> p1_value = rand()&01;
        }
        return 1;
    }
    else
        return 0;

    return 1;
}

int 
pi_x_num_static()
{
    int x_num = 0;
    int i;
    for( i = 0; i < ncktin; i++ )
    {
        if( sort_wlist[i] -> value == U ) 
            x_num ++;
    }

    if( (double)x_num / (double) ncktin < threshold )
    {
        add_pat_ini_test_set();
        return 1;
    }
    else
        return 0;

    return 1;
}

void
set_backtrack_limit()
{
    //backtrack_limit = 400;
    backtrack_limit = ncktin * 5/2;

}

test()
{
    register fptr undetect_fault;
    register fptr f,fault_under_test, ftemp;
    fptr fault_sim_a_vector(), fault_simulate_vectors();
    fptr transition_delay_fault_simulation();
    int podem();
    int current_detect_num,total_detect_num, i;
    int total_no_of_backtracks = 0;  // accumulative number of backtracks
    int current_backtracks;
    register int no_of_aborted_faults = 0, no_of_redundant_faults = 0;
    register int no_of_calls = 0;
    int display_undetect();
    void display_circuit();
    void display_fault();
    fptr choose_primary_fault();
    fptr choose_second_fault();
    int  pi_x_num();
    int  pi_x_num_static();
    void fault_sim_a_vector_frame01_X();
    int fault_sim_a_vector_Moon();

    in_vector_no = 0;
    total_detect_num = 0;
    undetect_fault = first_fault;
    fault_under_test = first_fault;
    set_backtrack_limit();

    /* Fsim only mode */
    if(fsim_only)
    {
        undetect_fault=fault_simulate_vectors(vectors,sim_vectors,undetect_fault,&total_detect_num);
        fault_under_test = undetect_fault;
        in_vector_no+=sim_vectors;
        display_undetect(undetect_fault);
        fprintf(stdout,"\n");
        return;
    }// if fsim only

    /* tdFsim only mode */
    if(tdfsim_only)
    {
        transition_delay_fault_simulation(vectors,sim_vectors,undetect_fault,&total_detect_num);
        printf("end of test.c\n");
        return;
    }
    
    // only dynamic
    if( tdfatpg_only && compression )
    {
        int PrimaryFault = 0;
        int SecondFault = 0;
        fptr fault_detect;
        int i;

        fault_under_test = choose_primary_fault();
        
        while( fault_under_test )
        {
            if( fault_under_test -> test_tried )
            {
                fault_under_test = choose_second_fault( fault_under_test );
                continue;
            }
            //printf( "PrimaryFault %d \n", fault_under_test -> fault_no);

            PrimaryFault = 0;
            switch(podem_Moon(fault_under_test,&current_backtracks, 1)) {
                case TRUE:
                    //printf( "PrimaryFault %d \n", fault_under_test -> fault_no);
                    fault_under_test = choose_primary_fault();
                    PrimaryFault = 1;
                    break;
                case FALSE:
                    //printf("    remove %d \n", fault_under_test -> fault_no);
                    fault_under_test->detect = REDUNDANT;
                    fault_under_test->test_tried = TRUE; // deal later
                    remove_fault( fault_under_test, 1 );
                    fault_under_test = choose_primary_fault();
                    PrimaryFault = 0;
                    break;
                case MAYBE:
                    //printf("    maybe %d \n", fault_under_test -> fault_no);
                    fault_under_test->test_tried = TRUE; // deal later
                    fault_under_test = choose_second_fault( fault_under_test );
                    PrimaryFault = 0;
                    break;
            }
            
            //display_io_Moon();
            
            if( PrimaryFault )
            {
                if( pi_x_num() )
                {
                    // fault sim and fault drop 
                    
                    fault_sim_a_vector_Moon(&current_detect_num);
                    //display_io_Moon();
                    add_pat_ini_test_set_Moon();
                    in_vector_no++;
                    continue;
                }

            
            }
            else
                continue;

            /*
            for( i = 0; i < ncktin; i++ )
                printf( "%d", sort_wlist[i] -> value );
            printf("\n");
            */

            //second pattern
            SecondFault = 0;
            while( fault_under_test )
            {
                //printf( "SecondFault %d \n", fault_under_test -> fault_no);
                switch( podem_Moon(fault_under_test,&current_backtracks, 0) )
                {
                    case TRUE: 
                        //printf( "SecondFault %d \n", fault_under_test -> fault_no);
                        break;
                    case FALSE:
                        //printf( "   False SecondFault %d \n", fault_under_test -> fault_no);
                        break;
                    case MAYBE:
                        //printf( "   Maybe SecondFault %d \n", fault_under_test -> fault_no);
                        break;
                
                }

                //display_io_Moon();

                if( pi_x_num() )
                {
                    // fault sim and fault drop 
                    fault_sim_a_vector_Moon(&current_detect_num);
                    //display_io_Moon();
                    add_pat_ini_test_set_Moon();
                    in_vector_no++;
                    SecondFault = 1;
                    break;
                }

                fault_under_test = choose_second_fault( fault_under_test );
            
            
            }

            if( !SecondFault )
            {
                for( i = 0; i < ncktin; i++ )
                {
                    if( sort_wlist[i] -> value == U ) 
                    {
                        sort_wlist[i] -> value = rand()&01;
                        if( sort_wlist[i] -> pvspi != NULL )
                            sort_wlist[i] -> pvspi -> p1_value = sort_wlist[i] -> value;
                    }
                    
                    if( i == ncktin - 1 && sort_wlist[i] -> p1_value == U ) 
                        sort_wlist[i] -> p1_value = rand()&01;
                }

                fault_sim_a_vector_Moon(&current_detect_num);
                //display_io_Moon();
                add_pat_ini_test_set_Moon();
                in_vector_no++;
            }

            /*
            for( i = 0; i < detection_num; i++ )
                if( det_flist[i] )
                    printf( "%d ", det_flist[i] -> fault_no );
                else
                    printf( "x ");
            printf("\n");
            */
            fault_under_test = choose_primary_fault();

        }
        printf("NEW Dynamic Only  pattern num:%d\n", in_vector_no);
        return;
    }


    // true  tdfatpg_only  w/o compression 
    if( tdfatpg_only && !compression )
    {
        int PrimaryFault = 0;
        int SecondFault = 0;
        fptr fault_detect;
        int i;

        fault_under_test = choose_primary_fault();
        while( fault_under_test )
        {
            if( fault_under_test -> test_tried )
            //if( fault_under_test -> fault_no != 3390 )
            {
                fault_under_test = choose_second_fault( fault_under_test );
                continue;
            }
            PrimaryFault = 0;
            //printf( "\n\nSSSSSSSSSSSSSSSSSSSSS fault no: %d  SSSSSSSSSSSSSSSSSS\n", fault_under_test -> fault_no );
            //display_fault( fault_under_test );
            switch(podem_Moon(fault_under_test,&current_backtracks, 1)) {
                case TRUE:
                    //printf( "PrimaryFault %d \n", fault_under_test -> fault_no);
                    PrimaryFault = 1;
                    break;
                case FALSE:
                    //printf("    remove %d \n", fault_under_test -> fault_no);
                    fault_under_test->detect = REDUNDANT;
                    fault_under_test->test_tried = TRUE; // deal later
                    remove_fault( fault_under_test, 1 );
                    fault_under_test = choose_primary_fault();
                    PrimaryFault = 0;
                    break;
                case MAYBE:
                    //printf("    MAYBE %d \n", fault_under_test -> fault_no);
                    fault_under_test->test_tried = TRUE; // deal later
                    fault_under_test = choose_second_fault( fault_under_test );
                    PrimaryFault = 0;
                    break;
            }
            //display_io_Moon();
            
            if( PrimaryFault )
            {
                
                for( i = 0; i < ncktin; i++ )
                {
                    if( sort_wlist[i] -> value == U ) 
                    {
                        sort_wlist[i] -> value = rand()&01;
                        if( sort_wlist[i] -> pvspi != NULL )
                            sort_wlist[i] -> pvspi -> p1_value = sort_wlist[i] -> value;
                    }
                    
                    if( i == ncktin - 1 && sort_wlist[i] -> p1_value == U ) 
                        sort_wlist[i] -> p1_value = rand()&01;
                }
                

                /*
                for( i = 0; i < detection_num; i++ )
                {
                    printf("det{%d}:", i);
                    for( f = det_flist[i]; f; f = f -> pnext)
                        printf("%d  ", f -> fault_no );
                    printf("\n");
                }
                */
                fault_sim_a_vector_Moon(&current_detect_num);
                //display_io_Moon();
                add_pat_ini_test_set_Moon();
                in_vector_no++;
                fault_under_test = choose_primary_fault();
            }
            //fault_under_test = choose_second_fault(fault_under_test);
        }   
        printf("NEW pattern num:%d\n", in_vector_no);
        return;
    }




    /* ATPG mode */
    /* Figure 5 in the PODEM paper */
    while(fault_under_test) {
        switch(podem(fault_under_test,&current_backtracks)) {
            case TRUE:
                /*by defect, we want only one pattern per fault */
                /*run a fault simulation, drop ALL detected faults */
                if (total_attempt_num == 1) {
                    undetect_fault = fault_sim_a_vector(undetect_fault,&current_detect_num);
                    total_detect_num += current_detect_num;
                }
                /* If we want mutiple petterns per fault, 
                 * NO fault simulation.  drop ONLY the fault under test */ 
                else {
                    fault_under_test->detect = TRUE;
                    /* walk through the undetected fault list */
                    for (f = undetect_fault; f; f = f->pnext_undetect) {
                        if (f == fault_under_test) {
                            /* drop fault_under_test */
                            if (f == undetect_fault)
                                undetect_fault = undetect_fault->pnext_undetect;
                            else {  
                                ftemp->pnext_undetect = f->pnext_undetect;
                            }
                            break;
                        }
                        ftemp = f;
                    }
                }
                in_vector_no++;
                break;
            case FALSE:  // 
                fault_under_test->detect = REDUNDANT;
                no_of_redundant_faults++;
                break;

            case MAYBE:  //
                no_of_aborted_faults++;
                break;
        }
        fault_under_test->test_tried = TRUE;
        fault_under_test = NULL;
        for (f = undetect_fault; f; f = f->pnext_undetect) {
            if (!f->test_tried) {
                fault_under_test = f;
                break;
            }


        }
        total_no_of_backtracks += current_backtracks; // accumulate number of backtracks
        no_of_calls++;
    }

    display_undetect(undetect_fault);
    fprintf(stdout,"\n");
    fprintf(stdout,"#number of aborted faults = %d\n",no_of_aborted_faults);
    fprintf(stdout,"\n");
    fprintf(stdout,"#number of redundant faults = %d\n",no_of_redundant_faults);
    fprintf(stdout,"\n");
    fprintf(stdout,"#number of calling podem1 = %d\n",no_of_calls);
    fprintf(stdout,"\n");
    fprintf(stdout,"#total number of backtracks = %d\n",total_no_of_backtracks);
    return;
}/* end of test */



