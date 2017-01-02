
#include <stdio.h>
#include <stdlib.h>
#include "global.h"
#include "miscell.h"
#include <iostream>
#include <vector>
#include <climits>

#define U  2
#define D  3
#define B  4 // D_bar

using namespace std;

extern "C" void test_compression();


static void STC_noDict_naive( vector<pptr>* ); 

// Local helper funciton
static void getPatternList( vector<pptr>* );
static bool isCompatible( pptr, pptr );
static bool isSame(pptr, pptr);
static void delPattern(pptr);
static pptr mergePattern( pptr, pptr );
static void printPList( vector<pptr>* );
static void printPList2( vector<pptr>* );
static void setCktPiValue( pptr );   
    
void 
test_compression()
{
    // return;
    printf("test_compression()\n");
    // fptr * det_flist;
    fptr p1;
    // int detection_num;
    // det_flist.size() = detection_num;
/*
    for (p1 = det_flist[0]; p1; p1 = p1->pnext)
    {
        pptr p2;
        printf("fault_no:%d --> ", p1->fault_no);
        for(p2 = p1->piassign; p2; p2 = p2->pnext){
            printf("[%d]:%d ", p2->index_value/2, p2->index_value%2);
        }
        printf("\n");
    }
*/

    vector<pptr> pList;
    getPatternList( &pList );
    cout << "Initial Test Set\n";
    printPList2(&pList);

    STC_noDict_naive(&pList);
}

void
STC_noDict_naive( vector<pptr> *v )
{
    // remove identical patterns and free the memory
    vector<pptr> &V = (*v);
    for(int i = 0, n = v->size(); i < n -1; ++i){
        if(V[i] == 0) continue;
        for(int j = i+1; j < n; ++j){
            if(V[j] == 0) continue;
            if(isSame(V[i], V[j])){
                delPattern(V[j]);
                V[j] = 0;
            }
        }
    }
    
    // print all existing pattern
    printf("remove same\n");
    printPList2(v); 
    
    // iteratively merge patterns if they are compatible
    int mergeSuccess;
    int iterCntr = 0;
    do{
        mergeSuccess = 0;
        for(int i = 0, n = V.size(); i < n-1; ++i){
            if(!V[i]) continue;
            for(int j = i+1; j < n; ++j){
                if(!V[j]) continue;
                if(isCompatible(V[i],V[j])){
                    pptr merged = mergePattern(V[i], V[j]);
                    delPattern(V[i]);
                    delPattern(V[j]);
                    V[i] = merged;
                    V[j] = 0;
                    ++mergeSuccess;
                }
            }
        }
        printf(" %dth run, %d merged\n", iterCntr++, mergeSuccess);
    }while(mergeSuccess);
    
    // print all existing pattern
    printPList2(v);
    
    // TODO: remove NULL pattern
        
}


void
delPattern( pptr p )
{
    while(p){
        pptr pTmp = p;
        p = p->pnext;
        free(pTmp);
    }
}
void
getPatternList( vector<pptr> *v )
{
    for( int i = 0; i < detection_num; ++i ){
        for (fptr fp = det_flist[i]; fp; fp = fp->pnext){
            if(fp->piassign)
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

bool
isSame( pptr p1, pptr p2 )
{
    while(p1 || p2){
        if(!p1) return false;
        if(!p2) return false;
        if(p1->index_value != p2->index_value) 
            return false;
        p1 = p1->pnext;
        p2 = p2->pnext;
    }
    return true;
}

pptr
mergePattern( pptr p1, pptr p2 )
{
    pptr pHead = NULL;
    pptr pTmp  = NULL;
    pptr pNew  = NULL;
    vector<int> merged(ncktin+1, INT_MAX);
    for (pptr pp1 = p1; pp1; pp1 = pp1->pnext)
        merged[pp1->index_value/2] = pp1->index_value;
    for (pptr pp2 = p2; pp2; pp2 = pp2->pnext)
        merged[pp2->index_value/2] = pp2->index_value;
    
    for (int i = 0, n = merged.size(); i < n; ++i ){
        if (merged[i] == INT_MAX) continue;
        if(pHead == NULL){
            pHead = ALLOC( 1, struct PIASSIGN );
            pHead->index_value = merged[i];
            pTmp  = pHead;
        }
        else{
            pNew = ALLOC( 1, struct PIASSIGN );
            pNew->index_value = merged[i];
            pTmp->pnext = pNew;
            pTmp = pNew;
        }
    }
    return pHead;
}

void
printPList( vector<pptr>* v )
{
    // print all existing pattern
    vector<pptr> &V = (*v);
    int counter = 0; 
    for(int i = 0, n = V.size(); i < n; ++i){
        if(V[i] == 0) continue;
        printf("%d >> ", counter++);
        for (pptr p = V[i]; p; p=p->pnext)
            printf("[%d]:%d ", p->index_value/2, p->index_value%2);
        printf("\n");
    }
}
void
printPList2( vector<pptr>* v )
{
    // print all existing pattern
    vector<pptr> &V = (*v);
    int counter = 0; 
    for(int i = 0, n = V.size(); i < n; ++i){
        if(V[i] == 0) continue;
        printf("%d >> ", counter++);
        vector<char> vPattern(ncktin+1, 'x');
        for (pptr p = V[i]; p; p=p->pnext)
            vPattern[p->index_value/2] =  (p->index_value%2 ? '1':'0' );
        for (int j = 0; j < ncktin+1; ++j)
            cout << vPattern[j];
        printf("\n");
    }
}


void 
setCktPiValue( pptr p )
{
    // initialize all wire value -> Unknown
    for(int i = 0; i < ncktwire; ++i)
        sort_wlist[i]->value = U;
    // insert PIASSIGN value
    for(pptr pp = p; pp; pp = pp->pnext)
        sort_wlist[pp->index_value/2]->value = pp->index_value%2;
}


