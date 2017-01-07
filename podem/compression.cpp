
#include <stdio.h>
#include <stdlib.h>
extern "C" {
#include "global.h"
#include "miscell.h"
}
#include <iostream>
#include <vector>
#include <climits>
#include <string>
#include <algorithm>
#include "util.h"
#define U  2
#define D  3
#define B  4 // D_bar

using namespace std;
typedef std::string String;

extern "C" void test_compression();
extern "C" void add_pat_ini_test_set();
extern "C" void add_pat_ini_test_set_Moon();
extern "C" void initialize_vars();

static void STC_ReverseOrderFaultSim(vector<String> &);
static void STC_SortedFaultSim(vector<String> &);

// Local helper funciton
static vector<pptr> IniTestSet;
static vector<String> TestSet_Moon;
static void setPiValue_Moon( const String &);
static void printPList_Moon( const vector<String> &);
static void delete_fault(fptr);

void 
test_compression()
{
    
    // version Moon
    //STC_ReverseOrderFaultSim( TestSet_Moon );
    STC_SortedFaultSim( TestSet_Moon );
    cout << "after FS:\n";
    printPList_Moon(TestSet_Moon);
}

void
add_pat_ini_test_set()
{
    //static vector<pptr> IniTestSet;
    pptr p, ptemp;
    p = NULL;
    for( int i = 0; i < ncktin; i++ )
    {
        if( sort_wlist[i] -> value != 2 ) // 2 is unknown
        {
            ptemp = ALLOC( 1, struct PIASSIGN );
            ptemp -> index_value = i * 2 + sort_wlist[i] -> value;
            if( p == NULL ){
                (IniTestSet).push_back( ptemp );
            }
            else
                p -> pnext = ptemp;
            p = ptemp;
        }
    }
}

void
add_pat_ini_test_set_Moon()
{
    //static vector<pptr> IniTestSet;
    pptr p, ptemp;
    p = NULL;
    String pattern;
    for( int i = 0; i < ncktin; i++ )
        pattern += ( sort_wlist[i] -> p1_value == 1)? "1" : "0";

    pattern += ( sort_wlist[0] -> value == 1)? "1" : "0";
    TestSet_Moon.push_back( pattern );


}

void
STC_ReverseOrderFaultSim( vector<String> & testSet )
{
    int dummyInt;
    // reset fault list first
    generate_fault_list_Moon();
    
    for( int i = testSet.size()-1; i >=0; --i){
       if( testSet[i].empty() ) continue;

        setPiValue_Moon( testSet[i] );
        int no_use = fault_sim_a_vector_Moon(&dummyInt);
        if ( no_use ){
            testSet[i] = "";
            printf(">> #%d is no use.\n", i);
        }
    }    
 
}

void
STC_SortedFaultSim( vector<String> & testSet )
{
    int dummyInt;
    ///////////////////
    generate_fault_list_Moon();
    for (fptr f1 = det_flist[0]; f1; f1 = f1->pnext){
        f1->detect_by = ALLOC( detection_num, int ); 
    }
    for( int i = testSet.size()-1; i >=0; --i){
        if( testSet[i].empty() ) continue;
        setPiValue_Moon( testSet[i] );
        fault_sim_a_vector_Moon_num(&i);
    }
    // collecting essential patterns
    vector<bool> chooseEss( testSet.size(),false );
    fptr ftmp;
    for ( fptr f1 = det_flist[0]; f1; f1 = ftmp ){
        ftmp = f1->pnext;
        if(f1->det_num > detection_num ) continue;
        if(f1->det_num < detection_num ){
            delete_fault(f1);
            //printf("#%d is too hard to detect, drop. \n", f1->fault_no);
            continue;
        }
        delete_fault(f1);
        //printf("#%d is essential\n", f1->fault_no);
        for( int j = 0; j < detection_num; ++j )
            chooseEss[f1->detect_by[j]] = true;
    }
    vector<String>            EssPat;
    vector<PatternForSort> nonEssPat;
    for(int i = 0, n = testSet.size(); i < n; ++i){
        if(testSet[i].empty()) continue;
        if(chooseEss[i])
            EssPat.push_back(testSet[i]);
        else
            nonEssPat.push_back(PatternForSort(testSet[i], 0));
    }
    for(int i = 0, n=nonEssPat.size(); i < n; ++i){
        setPiValue_Moon( nonEssPat[i].pattern );
        int dNum = fault_sim_a_vector_Moon_num(&dummyInt);
        nonEssPat[i].detNum = dNum;        
    }
    sort(nonEssPat.begin(), nonEssPat.end(), pat_comp);
    // reset all fault_detNum
    for (fptr f1 = det_flist[0]; f1; f1 = f1->pnext){
        f1->det_num = 0; 
    }
    for (int i = 0, n = EssPat.size(); i < n; ++i){
         setPiValue_Moon( EssPat[i] );
         fault_sim_a_vector_Moon(&dummyInt);
    }
    for(int i = 0, n=nonEssPat.size(); i < n; ++i){
        setPiValue_Moon( nonEssPat[i].pattern );
        int no_use = fault_sim_a_vector_Moon(&dummyInt);
        if(!no_use)
            EssPat.push_back(nonEssPat[i].pattern);
    }
    testSet.clear();
    testSet = EssPat;
}

void printPList_Moon(const vector<String> &testSet)
{
    int cntr = 0;
    for (int i = 0, n = testSet.size(); i < n; i++){
        if(testSet[i].empty()) continue;
        ++cntr;
        cout << "T'";
        for (int j = 0, n2 = testSet[i].size(); j < n2-1; ++j){
            cout << testSet[i][j];
        }
        cout << " " << testSet[i][testSet[i].size()-1] <<"'\n";
    }
    cout << "# total pattern: " << cntr << endl;
}

void
setPiValue_Moon( const String & pat )
{
    for (int i = 0; i < ncktin; ++i){
        sort_wlist[i]->p1_value = ( pat[i] == '1' ? 1:0 );
        sort_wlist[i+1]->value  = ( pat[i] == '1' ? 1:0 );
    }
    sort_wlist[0]->value = ( pat[ncktin] == '1' ? 1:0 );
}

void
delete_fault( fptr f )
{
    fptr fprev;
    fptr fnext;

    fprev = f -> pprev;
    fnext = f -> pnext;

    if( fprev )
        fprev -> pnext = fnext;
    if( fnext )
        fnext -> pprev = fprev;

    if( det_flist[0] == f )
        det_flist[0] = fnext;

    f -> pprev = NULL;
    f -> pnext = NULL;
    return;
}
