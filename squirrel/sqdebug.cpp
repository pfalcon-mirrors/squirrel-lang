/*
	see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
#include <stdarg.h>
#include "sqvm.h"
#include "sqfuncproto.h"
#include "sqclosure.h"
#include "sqstring.h"

SQRESULT sq_stackinfos(HSQUIRRELVM v, int level, SQStackInfos *si)
{
	int cssize = v->_callsstack.size();
	if (cssize > level) {
		memset(si, 0, sizeof(SQStackInfos));
		SQVM::CallInfo &ci = v->_callsstack[cssize-level-1];
		switch (type(ci._closure)) {
		case OT_CLOSURE:{
			SQFunctionProto *func = _funcproto(_closure(ci._closure)->_function);
			if (type(func->_name) == OT_STRING)
				si->funcname = _stringval(func->_name);
			if (type(func->_sourcename) == OT_STRING)
				si->source = _stringval(func->_sourcename);
			si->line = func->GetLine(ci._ip);
						}
			break;
		case OT_NATIVECLOSURE:
			si->source = si->funcname = _SC("NATIVE");
			si->line = -1;
			break;
		}
		return SQ_OK;
	}
	return SQ_ERROR;
}

void SQVM::RT_Error(const SQChar *s, ...)
{
	va_list vl;
	va_start(vl, s);
	scvsprintf(_sp(rsl(scstrlen(s)+(NUMBER_MAX_CHAR*2))), s, vl);
	va_end(vl);
	throw SQException(_ss(this), _spval);
}

SQString *SQVM::PrintObjVal(const SQObject &o)
{
	switch(type(o)) {
	case OT_STRING: return _string(o);
	case OT_INTEGER:
		scsprintf(_sp(rsl(NUMBER_MAX_CHAR+1)), _SC("%d"), _integer(o));
		return SQString::Create(_ss(this), _spval);
		break;
	case OT_FLOAT:
		scsprintf(_sp(rsl(NUMBER_MAX_CHAR+1)), _SC("%.14g"), _float(o));
		return SQString::Create(_ss(this), _spval);
		break;
	default:
		return SQString::Create(_ss(this), GetTypeName(o));
	}
}

void SQVM::IdxError(SQObject &o)
{
	SQObjectPtr oval = PrintObjVal(o);
	RT_Error(_SC("the index '%.50s' does not exists"), _stringval(oval));
}

void SQVM::CompareError(const SQObject &o1, const SQObject &o2)
{
	SQObjectPtr oval1 = PrintObjVal(o1),oval2 = PrintObjVal(o2);
	RT_Error(_SC("comparsion between '%.50s' and '%.50s'"), _stringval(oval1), _stringval(oval2));
}
