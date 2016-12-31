
#include <stdio.h>
#include "global.h"
#include "miscell.h"
#include <iostream>
using namespace std;
extern "C" void test_compression();

void
test_compression()
{
    cout << "cout_OK\n";
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
