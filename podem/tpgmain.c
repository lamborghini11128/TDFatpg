
#define MAIN

#include <stdio.h>
#include "global.h"
#include "miscell.h"

extern char *filename;
int backtrack_limit = 400;       /* default value */
int total_attempt_num = 1;      /* default value */

main(argc,argv)
int argc;
char *argv[];
{
    FILE *in;
    void level_circuit(), rearrange_gate_inputs(), input();
    char inpFile[200], vetFile[200];
    int i, j;

    vetFile[0]='0';
    fsim_only=FALSE;
	tdfsim_only=FALSE;
    timer(stdout,"START", filename);

    strcpy(inpFile, "xxxx");
    i = 1;
    detection_num = 1;
/* parse the input switches & arguments */
    while(i< argc) {
        if (strcmp(argv[i],"-anum") == 0) {
            total_attempt_num = atoi(argv[i+1]);
            i+=2;
        }
        else if (strcmp(argv[i],"-bt") == 0) {
            backtrack_limit = atoi(argv[i+1]);
            i+=2;
        }
        else if (strcmp(argv[i],"-fsim") == 0) {
            strcpy(vetFile, argv[i+1]);
            fsim_only=TRUE;
            i+=2;
        }
		else if (strcmp(argv[i],"-tdfsim") == 0) {
			strcpy(vetFile, argv[i+1]);
            tdfsim_only=TRUE;
            i+=2;
		}
	    else if (strcmp(argv[i],"-tdfatpg") == 0) {
            tdfatpg_only=TRUE;
            i+=1;
		}
        else if (strcmp(argv[i],"-compression") == 0) {
            compression = TRUE;
            i+=1;
		}
	    else if (strcmp(argv[i],"-ndet") == 0) {
            //compression = TRUE;
			detection_num = atoi( argv[i+1]);
            i+=2;
	    }

        else if (argv[i][0] == '-') {
            j = 1;
            while (argv[i][j] != '\0') {
                if (argv[i][j] == 'd') {
                    j++ ;
                }
                else {
                    fprintf(stderr, "atpg: unknown option\n");
                    usage();
                }
            }
            i++ ;
        }
        else {
            (void)strcpy(inpFile,argv[i]);
            i++ ;
        }
    }

/* an input file was not specified, so describe the proper usage */
    if (strcmp(inpFile, "xxxx") == 0) { usage(); }

/* read in and parse the input file */
    if( tdfatpg_only )
        input_frame01(inpFile); // input.c
    else
        input(inpFile); // input.c

    printf("%d detection\n", detection_num);
    printf("Finish input\n");
/* if vector file is provided, read it */
    if(vetFile[0] != '0') { read_vectors(vetFile); }
    //timer(stdout,"for reading in circuit",filename);
    //printf("Finish read vectors\n");

    level_circuit();  // level.c
    printf("Finish level\n");
    //timer(stdout,"for levelling circuit",filename);

    rearrange_gate_inputs();  //level.c
    printf("Finish rearrange gate \n");
    //timer(stdout,"for rearranging gate inputs",filename);

    create_dummy_gate(); //init_flist.c

    printf("Finish create_dummy_gate \n");
    //timer(stdout,"for creating dummy nodes",filename);

    if( tdfatpg_only )
        generate_fault_list_Moon(); //init_flist.c
    else
        generate_fault_list(); //init_flist.c

    printf("Finish generate fault list \n");
    //timer(stdout,"for generating fault list",filename);

    initialize_vars();
    test(); //test.c
	
    //test_compression();
    //if(!tdfsim_only){
//		compute_fault_coverage(); //init_flist.c
	//}
    
    //timer(stdout,"for test pattern generation",filename);
    exit(0);
}

usage()
{

   fprintf(stderr, "usage: atpg [options] infile\n");
   fprintf(stderr, "Options\n");
   fprintf(stderr, "      -fsim <filename>: fault simulation only; filename provides vectors\n");
   fprintf(stderr, "      -anum <num>: <num> specifies number of vectors per fault\n");
   fprintf(stderr, "      -bt <num>: <num> specifies number of backtracks\n");
   exit(-1);

} /* end of usage() */


read_vectors(vetFile)
char vetFile[];
{
    FILE *fopen(),*cPtr;
    char t[2000];
    int i,inx,iny;
 
    sim_vectors=0;
    if((cPtr=fopen(vetFile, "r")) == NULL) {
        fprintf(stderr,"File %s could not be opened\n",vetFile);
        exit(-1);
    }
    while(fgets(t,2000,cPtr) != NULL) {
        if(t[0] != 'T') continue;
        sim_vectors++;
    }
    vectors=ALLOC(sim_vectors+1,string);
    for(i=0; i<sim_vectors; i++) { vectors[i]=ALLOC(ncktin+2,char); }
    (void) rewind(cPtr);
    sim_vectors=0;
    while(fgets(t,2000,cPtr) != NULL) {
        if(t[0] != 'T') continue;
        inx=2;
        iny=0;
repeat:
        while(t[inx] != '\0' && iny < ncktin+1) {
			if(t[inx] == ' '){
				inx++;
				continue;
			}
				
            vectors[sim_vectors][iny]=t[inx];
            inx++;
            iny++;
        }
        if(iny == ncktin+1) { vectors[sim_vectors][iny] = '\0'; }
        if(iny < ncktin+1) {
            fgets(t,2000,cPtr);
            inx=0;
            goto repeat;
        }
        sim_vectors++;
    }
	
}

