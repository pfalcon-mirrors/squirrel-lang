/*
	see copyright notice in squirrel.h
*/
#include "stdafx.h"
#include "sqvm.h"
#include "sqfuncproto.h"
#include "sqclosure.h"
#include "sqstring.h"

int sq_stackinfos(HSQUIRRELVM v,int level,SQStackInfos *si)
{
	int cssize=v->_callsstack.size();
	if(cssize>level){
		memset(si,0,sizeof(SQStackInfos));
		SQVM::CallInfo &ci=v->_callsstack[cssize-level-1];
		switch(type(ci._closure)){
		case OT_CLOSURE:{
			SQFunctionProto *func=_funcproto(_closure(ci._closure)->_function);
			if(type(func->_name)==OT_STRING)
				si->funcname=_stringval(func->_name);
			if(type(func->_sourcename)==OT_STRING)
				si->source=_stringval(func->_sourcename);
			si->line=func->GetLine(ci._ip);
						}
			break;
		case OT_NATIVECLOSURE:
			si->source=si->funcname="NATIVE";
			si->line=-1;
			break;
		}
		
		return 1;
	}
	return 0;
}
