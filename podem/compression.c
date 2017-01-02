
#include <stdio.h>
#include "global.h"
#include "miscell.h"


void
test_compression()
{
    printf("test_compression\n");
    // fptr * det_flist;
    fptr p1;
    // int detection_num;
    // det_flist.size() = detection_num;
    for (p1 = det_flist[0]; p1; p1 = p1->pnext)
    {
        pptr p2;
        printf("fault_no:%d\n", p1->fault_no);
        for(p2 = p1->piassign; p2; p2 = p2->pnext){
            printf("[%d]:%d", p2->index_value/2, p2->index_value%2);
        }
        printf("\n");
    }
}
