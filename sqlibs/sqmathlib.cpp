#include <squirrel.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "sqmathlib.h"

#define SINGLE_ARG_FUNC(_funcname) static int math_##_funcname(HSQUIRRELVM v){ \
	SQFloat f; \
	if(sq_gettop(v)!=2 || SQ_FAILED(sq_getfloat(v,2,&f)))return sq_throwerror(v,_SC("invalid param")); \
	sq_pushfloat(v,(SQFloat)_funcname(f)); \
	return 1; \
}

#define TWO_ARGS_FUNC(_funcname) static int math_##_funcname(HSQUIRRELVM v){ \
	SQFloat p1,p2; \
	if(sq_gettop(v)!=3 || SQ_FAILED(sq_getfloat(v,2,&p1)) || SQ_FAILED(sq_getfloat(v,3,&p2)))return sq_throwerror(v,_SC("invalid param")); \
	sq_pushfloat(v,(SQFloat)_funcname(p1,p2)); \
	return 1; \
}

static int math_srand(HSQUIRRELVM v)
{
	SQInteger i;
	if(!sq_getinteger(v,2,&i))return sq_throwerror(v,_SC("invalid param"));
	srand(i);
	return 0;
}

static int math_rand(HSQUIRRELVM v)
{
	rand();
	return 0;
}


SINGLE_ARG_FUNC(sqrt)
SINGLE_ARG_FUNC(sin)
SINGLE_ARG_FUNC(cos)
SINGLE_ARG_FUNC(asin)
SINGLE_ARG_FUNC(acos)
SINGLE_ARG_FUNC(log)
SINGLE_ARG_FUNC(log10)
SINGLE_ARG_FUNC(tan)
SINGLE_ARG_FUNC(atan)
TWO_ARGS_FUNC(atan2)
TWO_ARGS_FUNC(pow)
SINGLE_ARG_FUNC(floor)
SINGLE_ARG_FUNC(ceil)
SINGLE_ARG_FUNC(exp)

#define _DECL_FUNC(name,nparams) {_SC(#name),math_##name,nparams}
static SQRegFunction mathlib_funcs[]={
	_DECL_FUNC(sqrt,2),
	_DECL_FUNC(sin,2),
	_DECL_FUNC(cos,2),
	_DECL_FUNC(asin,2),
	_DECL_FUNC(acos,2),
	_DECL_FUNC(log,2),
	_DECL_FUNC(log10,2),
	_DECL_FUNC(tan,2),
	_DECL_FUNC(atan,2),
	_DECL_FUNC(atan2,3),
	_DECL_FUNC(pow,3),
	_DECL_FUNC(floor,2),
	_DECL_FUNC(ceil,2),
	_DECL_FUNC(exp,2),
	_DECL_FUNC(srand,2),
	_DECL_FUNC(rand,1),
	{0,0},
};

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

int sq_mathlib_register(HSQUIRRELVM v)
{
	int i=0;
	while(mathlib_funcs[i].name!=0)
	{
		sq_pushstring(v,mathlib_funcs[i].name,-1);
		sq_newclosure(v,mathlib_funcs[i].f,mathlib_funcs[i].nparamscheck,0);
		sq_createslot(v,-3);
		i++;
	}
	sq_pushstring(v,_SC("RAND_MAX"),-1);
	sq_pushinteger(v,RAND_MAX);
	sq_createslot(v,-3);
	sq_pushstring(v,_SC("PI"),-1);
	sq_pushfloat(v,(SQFloat)M_PI);
	sq_createslot(v,-3);
	return 1;
}
