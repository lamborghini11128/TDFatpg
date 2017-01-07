
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

#define U  2
#define D  3
#define B  4 // D_bar

using namespace std;
typedef std::string String;

extern "C" void test_compression();
extern "C" void add_pat_ini_test_set();
extern "C" void add_pat_ini_test_set_Moon();
extern "C" void initialize_vars();

static void STC_noDict_naive( vector<pptr>* ); 
static void STC_fault_sim( vector<pptr>* ); 
static void STC_fault_sim2( vector<pptr>* ); 

static void STC_ReverseOrderFaultSim(vector<String> &);

// Local helper funciton
static void getPatternList( vector<pptr>* );
static bool isCompatible( pptr, pptr );
static bool isSame(pptr, pptr);
static void delPattern(pptr);
static pptr mergePattern( pptr, pptr );
static void printPList( pptr );
static void printPList2( vector<pptr>* );
static void setCktPiValue( String );   
static void setCktPiValue_partial( pptr );   
static int  fault_undropped_num();   
static vector<pptr> IniTestSet;
static vector<String> TestSet_Moon;
static void setPiValue_Moon( const String &);
static void printPList_Moon( const vector<String> &);
void initialize_vars()
{
    return;
    //IniTestSet = new vector<pptr>();
    //cout<<IniTestSet->size()<<endl;
    //IniTestSet->push_back( NULL );
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
test_compression()
{
    
    /*
    // version XYZ.
    if(!compression)     return;
    printf("test_compression()\n");
    vector<pptr> &pList = IniTestSet; 
    //getPatternList( &pList );
    cout << "Initial Test Size :" << IniTestSet.size() << endl;

    STC_noDict_naive(&pList);
    cout << "Stage I Test Size :" << IniTestSet.size() << endl;
    STC_fault_sim2(&pList);
    //STC_fault_sim(&pList);
    */

    // version Moon
    STC_ReverseOrderFaultSim( TestSet_Moon );
    cout << "after ROFS:\n";
    printPList_Moon(TestSet_Moon);
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
STC_noDict_naive( vector<pptr> *v )
{
    // remove identical patterns and free the memory
    
    vector<pptr> &V = (*v);
   /*
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
    */
    // iteratively merge patterns if they are compatible
    int mergeSuccess;
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
    }while(mergeSuccess);
    
    // TODO: remove NULL pattern
    // Let it be.
        
}

void
STC_fault_sim( vector<pptr> *v )
{
    vector<pptr>   &V = (*v);
    vector<String> patVec;
    int dummyInt       = 0; 
    int counter        = 0;
    int use_num        = INT_MAX;
    int pattern_num    = 0;
    // count non-NULL pattern number 
    for (int i = 0, n = V.size(); i < n; ++i)
        if(V[i]) ++pattern_num;
    
    const double STC_thre = 0.4; 
    while( use_num > pattern_num * STC_thre ){
        use_num = 0;
        for (int i = 0, n = V.size(); i < n; ++i){
            if(!V[i]) continue;
            String s = "";
            // randomly assign initial pattern;
            for(int j = 0; j < ncktin; ++j){
                s += ( rand()&01 ? "1" : "0" );
            }
        
            for(pptr p1 = V[i]; p1; p1 = p1->pnext){
                s[p1->index_value/2] = ( p1->index_value%2 ? '1' : '0'); 
            }
        
            setCktPiValue(s);
            if( !fault_sim_a_vector_frame01_Y(&dummyInt) ){
                patVec.push_back(s);
                ++use_num;
            }
        }
        cout << "iteration: " << counter++ <<" patSize:"<< patVec.size()<<endl;
    }
    
    // print ALL pattern out
    for(int i = 0, n = patVec.size(); i < n; ++i){
        cout<< "T'"; 
        for (int j = 1; j < ncktin; ++j){
            cout << patVec[i][j];
        }
        cout << " " << patVec[i][0] << "'\n";
    }
    cout << "# pattern size = " << patVec.size() << endl;
}

void
STC_fault_sim2( vector<pptr> *v )
{
    vector<pptr>   &V = (*v);
    vector<String> patVec;
    int dummyInt       = 0; 
    int counter        = 0;
    int use_num        = INT_MAX;
    int pattern_num    = 0;
    // count non-NULL pattern number
    

    for (int i = 0, n = V.size(); i < n; ++i){
        if(V[i]){
            ++pattern_num;
            setCktPiValue_partial(V[i]);
            fault_sim_a_vector_frame01_Sun(i, &dummyInt);
        }
    }
    //return; 
        fptr f1 = choose_primary_fault();
        while(f1){
            if(f1 -> detect_by == -1 ){
                f1 = choose_second_fault(f1);
                continue;
            }
            String s = "";
            // randomly assign initial pattern;
            for(int j = 0; j < ncktin; ++j){
                s += ( rand()&01 ? "1" : "0" );
            }
        
            for(pptr p1 = V[f1->detect_by]; p1; p1 = p1->pnext){
                s[p1->index_value/2] = ( p1->index_value%2 ? '1' : '0'); 
            }
            setCktPiValue(s);
            /*
            if(f1->fault_no == 1392){
                for( int j = 0; j < ncktin; ++j )
                    cout<< sort_wlist[j]->value;
                cout<<endl;
            }
            */
            //use_num = 0;
            if( !fault_sim_a_vector_frame01_Y(&dummyInt) ){
                patVec.push_back(s);
                //++use_num;
            }

            f1 = choose_primary_fault();
        }
    // print ALL pattern out
    for(int i = 0, n = patVec.size(); i < n; ++i){
        cout<< "T'"; 
        for (int j = 1; j < ncktin; ++j){
            cout << patVec[i][j];
        }
        cout << " " << patVec[i][0] << "'\n";
    }
    cout << "# pattern size = " << patVec.size() << endl;
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
    vector<int> merged(ncktin, INT_MAX);
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
printPList( pptr p1 )
{    
    if (!p1) return;
    vector<char> vPattern(ncktin, 'x');
    for (pptr p = p1; p; p=p->pnext)
        vPattern[p->index_value/2] =  (p->index_value%2 ? '1':'0' );
    for (int j = 0; j < ncktin; ++j)
        cout << vPattern[j];
    printf("\n");
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
        vector<char> vPattern(ncktin, 'x');
        for (pptr p = V[i]; p; p=p->pnext)
            vPattern[p->index_value/2] =  (p->index_value%2 ? '1':'0' );
        for (int j = 0; j < ncktin; ++j)
            cout << vPattern[j];
        printf("\n");
    }
}

void 
setCktPiValue_partial( pptr p )
{
    // initialize all wire value -> Unknown
    for(int i = 0; i < ncktwire; ++i)
        sort_wlist[i]->value = U;
    // insert PIASSIGN value, and assign unspecified PI = 0
    for(int i = 0; i < ncktin; ++i)
        sort_wlist[i]->value = 0;
    for(pptr pp = p; pp; pp = pp->pnext)
        sort_wlist[pp->index_value/2]->value = pp->index_value%2;
}



void 
setCktPiValue( String sPat )
{
    // initialize all wire value -> Unknown
    for(int i = 0; i < ncktwire; ++i)
        sort_wlist[i]->value = U;
    // insert PIASSIGN value
    for(int i = 0; i < ncktin; ++i)
        sort_wlist[i]->value = (sPat[i] == '1' ? 1 : 0 );
    //for(pptr pp = p; pp; pp = pp->pnext)
    //    sort_wlist[pp->index_value/2]->value = pp->index_value%2;
}

int fault_undropped_num()
{
    int cntr = 0;
    for(int i = 0; i < detection_num; ++i){
        for( fptr fp = det_flist[i]; fp; fp = fp->pnext ){
            ++cntr;
        }
    }
    return cntr;
}
