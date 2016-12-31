/**********************************************************************/
/*           This is reachability control for atpg                    */
/*                                                                    */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include "atpg.h"


    void
rchctrl( n, value )
    nptr n;
    int  value;
{
    switch (n->type) {
        case OUTPUT:
        case     OR:
        case    NOR: 
        case    EQV:
        case    XOR:
        case    AND:
        case   NAND:break; 
    }


} /* end of unpack */

