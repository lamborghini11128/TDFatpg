
#include <stdio.h>
#include "global.h"
#include "miscell.h"
#include <iostream>
#include <vector>

using namespace std;
extern "C" void test_compression();


static void getPatternList( vector<pptr>* );
static bool isCompatible( pptr, pptr );
void 
test_compression()
{
    return;
    cout << "cout_OK\n";
    printf("test_compression\n");
    // fptr * det_flist;
    fptr p1;
    // int detection_num;
    // det_flist.size() = detection_num;
    for (p1 = det_flist[0]; p1; p1 = p1->pnext)
    {
        pptr p2;
        printf("fault_no:%d --> ", p1->fault_no);
        for(p2 = p1->piassign; p2; p2 = p2->pnext){
            printf("[%d]:%d ", p2->index_value/2, p2->index_value%2);
        }
        printf("\n");
    }


    vector<pptr> pList;
    getPatternList( &pList );
    cout << "222\n";
    for( int i = 0, n = pList.size(); i < n; ++i ){
        cout << i << " -> ";
        for( pptr p2 = pList[i]; p2; p2 = p2->pnext )
            printf("[%d]:%d ", p2->index_value/2, p2->index_value%2);
        cout << "\n";
    }

    for (int i = 0, n = pList.size(); i < n-1; ++i){
        for( int j = i+1; j < n; ++j ){
            if(isCompatible(pList[i], pList[j]))
                printf("[%d][%d] v\n", i, j);
        }
    }
}


void
getPatternList( vector<pptr> *v )
{
    for( int i = 0; i < detection_num; ++i ){
        for (fptr fp = det_flist[i]; fp; fp = fp->pnext){
            v->push_back(fp->piassign);
        }
    }
}

bool
isCompatible( pptr p1, pptr p2 )
{
    if( !p1 || !p2 ) return false;
    
    while(p1 && p2){
        int id1 = p1->index_value/2;
        int id2 = p2->index_value/2;
        if (id1 < id2)          
            p1 = p1->pnext;
        else if (id1 > id2 )
            p2 = p2->pnext;
        else{
            if ( p1->index_value%2 != p2->index_value%2 )
                return false;
            p1 = p1->pnext;
            p2 = p2->pnext;
        }
    }
    return true;
}
