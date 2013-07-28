/*
	see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
#include "sqfuncproto.h"
#include "sqstring.h"
#include "sqopcodes.h"
#include "sqfuncstate.h"

#ifdef _DEBUG
SQInstructionDesc g_InstrDesc[]={
	{_SC("_OP_LINE"),-1,2,0,0},
	{_SC("_OP_LOAD"),1,2,0,0},
	{_SC("_OP_LOADNULL"),0,0,0,0},
	{_SC("_OP_NEWSLOT"),1,1,1,1},
	{_SC("_OP_SET"),1,1,1,1},
	{_SC("_OP_GET"),1,1,1,0},
	{_SC("_OP_LOADROOTTABLE"),1,0,0,0},
	{_SC("_OP_PREPCALL"),1,1,1,1},
	{_SC("_OP_CALL"),1,1,1,1},
	{_SC("_OP_MOVE"),1,1,1,0},
	{_SC("_OP_ADD"),1,1,1,0},
	{_SC("_OP_SUB"),1,1,1,0},
	{_SC("_OP_MUL"),1,1,1,0},
	{_SC("_OP_DIV"),1,1,1,0},
	{_SC("_OP_MODULO"),1,1,1,0},
	{_SC("_OP_YIELD"),1,2,0,0},
	{_SC("_OP_EQ"),1,1,1,0},
	{_SC("_OP_NE"),1,1,1,0},
	{_SC("_OP_G"),1,1,1,0},
	{_SC("_OP_GE"),1,1,1,0},
	{_SC("_OP_L"),1,1,1,0},
	{_SC("_OP_LE"),1,1,1,0},
	{_SC("_OP_EXISTS"),1,1,1,0},
	{_SC("_OP_NEWTABLE"),1,2,0,0},
	{_SC("_OP_JMP"),-1,2,0,0},
	{_SC("_OP_JNZ"),1,2,0,0},
	{_SC("_OP_JZ"),1,2,0,0},
	{_SC("_OP_RETURN"),1,0,0,0},
	{_SC("_OP_CLOSURE"),1,2,1,0},
	{_SC("_OP_FOREACH"),1,1,1,1},
	{_SC("_OP_DELEGATE"),1,1,1,0},
	{_SC("_OP_TYPEOF"),1,1,0,0},
	{_SC("_OP_PUSHTRAP"),-1,2,0,0},
	{_SC("_OP_POPTRAP"),0,0,0,0},
	{_SC("_OP_THROW"),1,0,0,0},
	{_SC("_OP_NEWARRAY"),1,2,0,0},
	{_SC("_OP_APPENDARRAY"),1,1,0,0},
	{_SC("_OP_AND"),1,2,1,0},
	{_SC("_OP_OR"),1,2,1,0},
	{_SC("_OP_NEG"),1,1,0,0},
	{_SC("_OP_NOT"),1,1,0,0},
	{_SC("_OP_DELETE"),1,1,1,0},
	{_SC("_OP_BWNOT"),1,1,0,0},
	{_SC("_OP_BWAND"),1,1,1,0},
	{_SC("_OP_BWOR"),1,1,1,0},
	{_SC("_OP_BWXOR"),1,1,1,0},
	{_SC("_OP_MINUSEQ"),1,1,1,1},
	{_SC("_OP_PLUSEQ"),1,1,1,1},
	{_SC("_OP_TAILCALL"),1,1,1,1},
	{_SC("_OP_SHIFTL"),1,1,1,0},
	{_SC("_OP_SHIFTR"),1,1,1,0},
	{_SC("_OP_RESUME"),1,1,0,0},
	{_SC("_OP_CLONE"),1,1,0,0},
	//optimiz
	{_SC("_OP_GETK"),1,1,1,0},
	{_SC("_OP_PREPCALLK"),1,1,1,1},
};
#endif
void DumpLiteral(SQObjectPtr &o)
{
	switch(type(o)){
		case OT_STRING:	scprintf(_SC("\"%s\""),_stringval(o));break;
		case OT_FLOAT: scprintf(_SC("{%f}"),_float(o));break;
		case OT_INTEGER: scprintf(_SC("{%d}"),_integer(o));break;
	}
}
#ifdef _DEBUG_DUMP
void SQFuncState::Dump()
{
	unsigned int n=0,i;
	SQFunctionProto *func=_funcproto(_func);
	scprintf(_SC("SQInstruction sizeof %d\n"),sizeof(SQInstruction));
	scprintf(_SC("SQObject sizeof %d\n"),sizeof(SQObject));
	scprintf(_SC("--------------------------------------------------------------------\n"));
	scprintf(_SC("*****FUNCTION [%s]\n"),type(func->_name)==OT_STRING?_stringval(func->_name):_SC("unknown"));
	scprintf(_SC("-----LITERALS\n"));
	for(i=0;i<_literals.size();i++){
		scprintf(_SC("[%d] "),n);
		DumpLiteral(_literals[i]);
		scprintf(_SC("\n"));
		n++;
	}
	scprintf(_SC("-----PARAMS\n"));
	n=0;
	for(i=0;i<_parameters.size();i++){
		scprintf(_SC("[%d] "),n);
		DumpLiteral(_parameters[i]);
		scprintf(_SC("\n"));
		n++;
	}
	scprintf(_SC("-----LOCALS\n"));
	for(i=0;i<func->_localvarinfos.size();i++){
		SQLocalVarInfo lvi=func->_localvarinfos[i];
		scprintf(_SC("[%d] %s \t%d %d\n"),lvi._pos,_stringval(lvi._name),lvi._start_op,lvi._end_op);
		n++;
	}
	scprintf(_SC("-----LINE INFO\n"));
	for(i=0;i<_lineinfos.size();i++){
		SQLineInfo li=_lineinfos[i];
		scprintf(_SC("op [%d] line [%d] \n"),li._op,li._line);
		n++;
	}
	scprintf(_SC("-----dump\n"));
	n=0;
	for(i=0;i<_instructions.size();i++){
		SQInstruction &inst=_instructions[i];
		if(inst.op==_OP_LOAD || inst.op==_OP_PREPCALLK || inst.op==_OP_GETK
			|| ((inst.op==_OP_ADD || inst.op==_OP_SUB || inst.op==_OP_MUL || inst.op==_OP_DIV) 
				&& inst._arg3==0xFF)){
			
			scprintf(_SC("[%03d] %15s %d "),n,g_InstrDesc[inst.op].name,inst._arg0);
			if(inst._arg1==-1)
				scprintf(_SC("null"));
			else
				DumpLiteral(_literals[inst._arg1]);
			scprintf(_SC(" %d %d \n"),inst._arg2,inst._arg3);
		}
		else 
			scprintf(_SC("[%03d] %15s %d %d %d %d\n"),n,g_InstrDesc[inst.op].name,inst._arg0,inst._arg1,inst._arg2,inst._arg3);
		n++;
	}
	scprintf(_SC("-----\n"));
	scprintf(_SC("stack size[%d]\n"),func->_stacksize);
	scprintf(_SC("--------------------------------------------------------------------\n\n"));
}
#endif
int SQFuncState::GetStringConstant(const SQChar *cons)
{
	return GetConstant(SQString::Create(_sharedstate,cons));
}

int SQFuncState::GetNumericConstant(const SQInteger cons)
{
	return GetConstant(cons);
}

int SQFuncState::GetNumericConstant(const SQFloat cons)
{
	return GetConstant(cons);
}

int SQFuncState::GetConstant(SQObjectPtr cons)
{
	int n=0;
	for(unsigned int i=0;i<_literals.size();i++){
		if((type(cons)==type(_literals[i])) && 
			(_userpointer(cons)==_userpointer(_literals[i]))){
			return n;
		}
		n++;
	}
	_literals.push_back(cons);
	return _literals.size()-1;
}

void SQFuncState::SetIntructionParams(int pos,int arg0,int arg1,int arg2,int arg3)
{
	_instructions[pos]._arg0=*((unsigned int *)&arg0);
	_instructions[pos]._arg1=*((unsigned int *)&arg1);
	_instructions[pos]._arg2=*((unsigned int *)&arg2);
	_instructions[pos]._arg3=*((unsigned int *)&arg3);
}

void SQFuncState::SetIntructionParam(int pos,int arg,int val)
{
	switch(arg){
		case 0:_instructions[pos]._arg0=*((unsigned int *)&val);break;
		case 1:_instructions[pos]._arg1=*((unsigned int *)&val);break;
		case 2:_instructions[pos]._arg2=*((unsigned int *)&val);break;
		case 3:_instructions[pos]._arg3=*((unsigned int *)&val);break;
	};
}

int SQFuncState::AllocStackPos()
{
	int npos=_vlocals.size();
	_vlocals.push_back(SQLocalVarInfo());
	if(_vlocals.size()>((unsigned int)_stacksize))_stacksize=_vlocals.size();
	return npos;
}

int SQFuncState::FreeStackPos()
{
	assert(type(_vlocals.back()._name)==OT_NULL);
	_vlocals.pop_back();
	return _vlocals.size();
}

int SQFuncState::PushTarget(int n)
{
	if(n!=-1){
		_targetstack.push_back(n);
		return n;
	}
	n=AllocStackPos();
	_targetstack.push_back(n);
	return n;
}

int SQFuncState::TopTarget(){
	return _targetstack.back();
}
int SQFuncState::PopTarget()
{
	int npos=_targetstack.back();
	SQLocalVarInfo t=_vlocals[_targetstack.back()];
	if(type(t._name)==OT_NULL){
		_vlocals.pop_back();
	}
	_targetstack.pop_back();
	return npos;
}

int SQFuncState::GetStackSize()
{
	return _vlocals.size();
}

void SQFuncState::SetStackSize(int n)
{
	int size=_vlocals.size();
	while(size>n){
		size--;
		SQLocalVarInfo lvi=_vlocals.back();
		if(type(lvi._name)!=OT_NULL){
			lvi._end_op=GetCurrentPos();
			_localvarinfos.push_back(lvi);
		}
		_vlocals.pop_back();
	}
}

bool SQFuncState::IsLocal(unsigned int stkpos)
{
	if(stkpos>=_vlocals.size())return false;
	else if(type(_vlocals[stkpos]._name)!=OT_NULL)return true;
	return false;
}

int SQFuncState::PushLocalVariable(const SQObjectPtr &name)
{
	int pos=_vlocals.size();
	SQLocalVarInfo lvi;
	lvi._name=name;
	lvi._start_op=GetCurrentPos()+1;
	lvi._pos=_vlocals.size();
	_vlocals.push_back(lvi);
	if(_vlocals.size()>((unsigned int)_stacksize))_stacksize=_vlocals.size();
	
	return pos;
}

int SQFuncState::GetLocalVariable(const SQObjectPtr &name)
{
	int locals=_vlocals.size();
	while(locals>=1){
		if(type(_vlocals[locals-1]._name)==OT_STRING && _string(_vlocals[locals-1]._name)==_string(name)){
			return locals-1;
		}
		locals--;
	}
	return -1;
}

void SQFuncState::AddOuterValue(const SQObjectPtr &name)
{
	int pos=-1;
	if(_parent)pos=_parent->GetLocalVariable(name);
	if(pos!=-1)
		_outervalues.push_back(SQOuterVar(SQObjectPtr(SQInteger(pos)),true)); //local
	else
		_outervalues.push_back(SQOuterVar(name,false)); //global
}

void SQFuncState::AddParameter(const SQObjectPtr &name)
{
	PushLocalVariable(name);
	_parameters.push_back(name);
}

void SQFuncState::AddLineInfos(int line,bool lineop,bool force)
{
	if(_lastline!=line || force){
		SQLineInfo li;
		li._line=line;li._op=(GetCurrentPos()+1);
		_lineinfos.push_back(li);
		if(lineop)AddInstruction(_OP_LINE,0,line);
		_lastline=line;
	}
}

void SQFuncState::AddInstruction(SQInstruction &i)
{
	int size=_instructions.size();
	if(size>0 && (_optimization || i.op==_OP_RETURN)){ //simple optimizer
		SQInstruction &pi=_instructions[size-1];//previous intruction
		switch(i.op){
		case _OP_RETURN:
			if( _parent && i._arg0!=0xFF && pi.op==_OP_CALL){
				pi.op=_OP_TAILCALL;
			}
		break;
		case _OP_GET:
			if( pi.op==_OP_LOAD	&& pi._arg0==i._arg2 && (!IsLocal(pi._arg0))){
				pi._arg2=(unsigned char)i._arg1;
				pi.op=_OP_GETK;
				pi._arg0=i._arg0;
				return;
			}
		break;
		case _OP_PREPCALL:
			if( pi.op==_OP_LOAD	&& pi._arg0==i._arg1 && (!IsLocal(pi._arg0))){
				pi.op=_OP_PREPCALLK;
				pi._arg0=i._arg0;
				pi._arg2=i._arg2;
				pi._arg3=i._arg3;
				return;
			}
			break;
		case _OP_APPENDARRAY:
			if(pi.op==_OP_LOAD && pi._arg0==i._arg1	&& (!IsLocal(pi._arg0))){
				pi.op=_OP_APPENDARRAY;
				pi._arg0=i._arg0;
				pi._arg1=pi._arg1;
				pi._arg2=0xFF;
				return;
			}
			break;
		case _OP_MOVE: 
			if((pi.op==_OP_GET || pi.op==_OP_ADD || pi.op==_OP_SUB
				|| pi.op==_OP_MUL || pi.op==_OP_DIV || pi.op==_OP_SHIFTL
				|| pi.op==_OP_SHIFTR || pi.op==_OP_BWOR	|| pi.op==_OP_BWXOR
				|| pi.op==_OP_BWAND) && (pi._arg0==i._arg1))
			{
				pi._arg0=i._arg0;
				return;
			}
			break;
		case _OP_ADD:case _OP_SUB:case _OP_MUL:case _OP_DIV:
			if(pi.op==_OP_LOAD && pi._arg0==i._arg1	&& (!IsLocal(pi._arg0) ))
			{
				pi.op=i.op;
				pi._arg0=i._arg0;
				pi._arg2=i._arg2;
				pi._arg3=0xFF;
				return;
			}
			break;
		case _OP_LINE:
			if(pi.op==_OP_LINE)_instructions.pop_back();
			break;
		}
	}
	_optimization=true;
	_instructions.push_back(i);
}

void SQFuncState::Finalize()
{
	SQFunctionProto *f=_funcproto(_func);
	f->_literals.resize(_literals.size());
	f->_literals.copy(_literals);
	f->_functions.resize(_functions.size());
	f->_functions.copy(_functions);
	f->_parameters.resize(_parameters.size());
	f->_parameters.copy(_parameters);
	f->_outervalues.resize(_outervalues.size());
	f->_outervalues.copy(_outervalues);
	f->_instructions.resize(_instructions.size());
	f->_instructions.copy(_instructions);
	f->_localvarinfos.resize(_localvarinfos.size());
	f->_localvarinfos.copy(_localvarinfos);
	f->_lineinfos.resize(_lineinfos.size());
	f->_lineinfos.copy(_lineinfos);
}
