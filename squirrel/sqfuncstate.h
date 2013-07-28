/*	see copyright notice in squirrel.h */
#ifndef _SQFUNCSTATE_H_
#define _SQFUNCSTATE_H_
///////////////////////////////////
#include "squtils.h"

typedef sqvector<int> IntVec;

#define MAX_FUNC_STACKSIZE 0xFF
#define MAX_LITERALS 0xFFFF

struct SQFuncState
{
	SQFuncState(SQSharedState *ss,SQFunctionProto *func,SQFuncState *parent);
#ifdef _DEBUG_DUMP
	void Dump();
#endif
	void AddInstruction(SQOpcode _op,int arg0=0,int arg1=0,int arg2=0,int arg3=0){SQInstruction i(_op,arg0,arg1,arg2,arg3);AddInstruction(i);}
	void AddInstruction(SQInstruction &i);
	void SetIntructionParams(int pos,int arg0,int arg1,int arg2=0,int arg3=0);
	void SetIntructionParam(int pos,int arg,int val);
	SQInstruction &GetInstruction(int pos){return _instructions[pos];}
	void PopInstructions(int size){for(int i=0;i<size;i++)_instructions.pop_back();}
	void SetStackSize(int n);
	void SnoozeOpt(){_optimization=false;}
	int GetCurrentPos(){return _instructions.size()-1;}
	int GetStringConstant(const SQChar *cons);
	int GetNumericConstant(const SQInteger cons);
	int GetNumericConstant(const SQFloat cons);
	int PushLocalVariable(const SQObjectPtr &name);
	void AddParameter(const SQObjectPtr &name);
	void AddOuterValue(const SQObjectPtr &name);
	int GetLocalVariable(const SQObjectPtr &name);
	int GenerateCode();
	int GetStackSize();
	int CalcStackFrameSize();
	void AddLineInfos(int line,bool lineop,bool force=false);
	void Finalize();
	int AllocStackPos();
	int FreeStackPos();
	int PushTarget(int n=-1);
	int PopTarget();
	int TopTarget();
	bool IsLocal(unsigned int stkpos);
	
	SQLocalVarInfoVec _vlocals;
	IntVec _targetstack;
	int _stacksize;
	IntVec _unresolvedbreaks;
	IntVec _unresolvedcontinues;
	SQObjectPtrVec _functions;
	SQObjectPtrVec _parameters;
	SQOuterVarVec _outervalues;
	SQInstructionVec _instructions;
	SQLocalVarInfoVec _localvarinfos;
	//SQObjectPtrVec _literals;
	SQObjectPtr _literals;
	SQInteger _nliterals;
	SQLineInfoVec _lineinfos;
	SQObjectPtr _func;
	SQFuncState *_parent;
	IntVec _breaktargets; //contains number of nested exception traps
	IntVec _continuetargets;
	int _lastline;
	bool _optimization;
	SQSharedState *_sharedstate;
private:
	int GetConstant(SQObjectPtr cons);
};


#endif //_SQFUNCSTATE_H_

