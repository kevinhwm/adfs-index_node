/* namespace.c
 *
 * kevinhwm@gmail.com
 */

#include "namespace.h"

int ns_release(CINameSpace *_this);
int ns_output(CINameSpace *_this);


int GIns_init(CINameSpace *_this, const char *name, const char *db_args, int primary)
{


    _this->release = ns_release;
    _this->output = ns_output;
    return 0;
}

int ns_release(CINameSpace *_this)
{
    if (_this->index_db) { 
	kcfree(_this->index_db); 
	_this->index_db = NULL; 
    }
    return 0;
}

int ns_output(CINameSpace *_this)
{
    return 0;
}

