/* namespace.c
 *
 * kevinhwm@gmail.com
 */

#include "namespace.h"

int ns_release(CINameSpace *_this);
int ns_output(CINameSpace *_this);


int GIns_init(CINameSpace *_this, const char *name, const char *db_args, int primary)
{
    if (_this == NULL) { return -1; }
    memset(_this, 0, sizeof(CINameSpace));

    strncpy(_this->name, name, sizeof(_this->name));
    _this->index_db = kcdbnew();
    if (kcdbopen(_this->index_db, db_args, KCOREADER|KCOWRITER|KCOCREATE|KCOTRYLOCK) == 0) {
	return -1;
    }

    /*
    if (primary) {
	GInsp_init(_this->prim);
    }
    else {
	GInss_init(_this->sec);
    }
    */

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

