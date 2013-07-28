#include <squirrel.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "sqmathlib.h"

#define SINGLE_ARG_FUNC(_funcname) static int math_##_funcname(HSQUIRRELVM v){ \
	SQFloat f; \
	if(sq_gettop(v)!=2 || SQ_FAILED(sq_getfloat(v,2,&f)))return sq_throwerror(v,"invalid param"); \
	sq_pushfloat(v,(SQFloat)_funcname(f)); \
	return 1; \
}

#define TWO_ARGS_FUNC(_funcname) static int math_##_funcname(HSQUIRRELVM v){ \
	SQFloat p1,p2; \
	if(sq_gettop(v)!=3 || SQ_FAILED(sq_getfloat(v,2,&p1)) || SQ_FAILED(sq_getfloat(v,3,&p2)))return sq_throwerror(v,"invalid param"); \
	sq_pushfloat(v,(SQFloat)_funcname(p1,p2)); \
	return 1; \
}

static int math_srand(HSQUIRRELVM v)
{
	SQInteger i;
	if(!sq_getinteger(v,1,&i))return sq_throwerror(v,"invalid param");
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

#define _DECL_FUNC(name) {#name,math_##name}
static SQRegFunction mathlib_funcs[]={
	_DECL_FUNC(sqrt),
	_DECL_FUNC(sin),
	_DECL_FUNC(cos),
	_DECL_FUNC(asin),
	_DECL_FUNC(acos),
	_DECL_FUNC(log),
	_DECL_FUNC(log10),
	_DECL_FUNC(tan),
	_DECL_FUNC(atan),
	_DECL_FUNC(atan2),
	_DECL_FUNC(pow),
	_DECL_FUNC(floor),
	_DECL_FUNC(ceil),
	_DECL_FUNC(exp),
	_DECL_FUNC(srand),
	_DECL_FUNC(rand),
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
		sq_newclosure(v,mathlib_funcs[i].f,0);
		sq_createslot(v,-3);
		i++;
	}
	sq_pushstring(v,"RAND_MAX",-1);
	sq_pushinteger(v,RAND_MAX);
	sq_createslot(v,-3);
	sq_pushstring(v,"PI",-1);
	sq_pushfloat(v,(SQFloat)M_PI);
	sq_createslot(v,-3);
	return 1;
}
