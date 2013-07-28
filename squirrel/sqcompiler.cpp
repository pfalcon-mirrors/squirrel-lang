/*
	see copyright notice in squirrel.h
*/
#include "stdafx.h"
#include <stdarg.h>
#include "sqopcodes.h"
#include "sqstring.h"
#include "sqfuncproto.h"
#include "sqfuncstate.h"
#include "sqcompiler.h"
#include "sqlexer.h"
#include "sqvm.h"

#define DEREF_NO_DEREF -1
#define DEREF_FIELD		-2

#define MAX_KEYS_PER_APPEND 100

struct ExpState
{
	ExpState()
	{
		_deref=DEREF_NO_DEREF;
		_delete=false;
		_funcarg=false;
	}
	bool _delete;
	bool _funcarg;
	int _deref;
};


typedef sqvector<ExpState> ExpStateVec;

#define _exst (_expstates.top())

#define BEGIN_BREAKBLE_BLOCK()	int __nbreaks__=_fs->_unresolvedbreaks.size(); \
							int __ncontinues__=_fs->_unresolvedcontinues.size(); \
							_fs->_breaktargets++;_fs->_continuetargets++;

#define END_BREAKBLE_BLOCK(continue_target) {__nbreaks__=_fs->_unresolvedbreaks.size()-__nbreaks__; \
					__ncontinues__=_fs->_unresolvedcontinues.size()-__ncontinues__; \
					if(__ncontinues__>0)ResolveContinues(_fs,__ncontinues__,continue_target); \
					if(__nbreaks__>0)ResolveBreaks(_fs,__nbreaks__); \
					_fs->_breaktargets--;_fs->_continuetargets--;}

class SQCompiler
{
public:
	SQCompiler(SQREADFUNC rg,SQUserPointer up,const SQChar* sourcename,bool raiseerror,bool lineinfo)
	{
		_lex.Init(rg,up);
		_sourcename=SQString::Create(sourcename);
		_lineinfo=lineinfo;_raiseerror=raiseerror;
	}
	void Error(const SQChar *s,...)
	{
		static SQChar temp[256];
		va_list vl;
		va_start(vl, s);
		vsprintf(temp, s, vl);
		va_end(vl);
		throw ParserException(temp);
	}
	void Lex(){	_token=_lex.Lex();}
	void PushExpState(){ _expstates.push_back(ExpState()); }
	bool IsDerefToken(int tok)
	{
		switch(tok){
		case '=':case '(':case CREATE:
		case MINUSEQ:case PLUSEQ:return true;
		}
		return false;
	}
	ExpState PopExpState(){
		ExpState ret=_expstates.top();
		_expstates.pop_back();
		return ret;
	}
	SQObjectPtr Expect(int tok)
	{
		SQObjectPtr ret;
		if(_token!=tok){
			if(tok>255){
				switch(tok)
				{
				case IDENTIFIER:
					ret=SQString::Create("IDENTIFIER");
					break;
				case STRING_LITERAL:
					ret=SQString::Create("STRING_LITERAL");
					break;
				case INTEGER:
					ret=SQString::Create("INTEGER");
					break;
				case FLOAT:
					ret=SQString::Create("FLOAT");
					break;
				default:
					ret=_lex.Tok2Str(tok);
				}
				Error("expected '%s'",_stringval(ret));
			}
			Error("expected '%c'",tok);
		}
		switch(tok)
		{
		case IDENTIFIER:
			ret=SQString::Create(_lex._svalue);
			break;
		case STRING_LITERAL:
			ret=SQString::Create(_lex._svalue);
			break;
		case INTEGER:
			ret=_lex._nvalue;
			break;
		case FLOAT:
			ret=_lex._fvalue;
			break;
		}
		Lex();
		return ret;
	}
	bool IsEndOfStatement(){return ((_lex._prevtoken=='\n') || (_token==SQUIRREL_EOB) || (_token=='}') || (_token==';')); }
	void OptionalSemicolon()
	{
		if(_token==';'){ Lex(); return; }
		if(!IsEndOfStatement()) {
			Error("end of statement expected (; or lf)");
		}
	}
	bool Compile(SQVM *vm,SQObjectPtr &o)
	{
		try{
			_debugline=1;
			_debugop=0;
			Lex();
			SQFuncState funcstate(SQFunctionProto::Create(),NULL);
			_funcproto(funcstate._func)->_name=SQString::Create("main");
			_fs=&funcstate;
			_fs->AddParameter(SQString::Create("this"));
			_funcproto(_fs->_func)->_sourcename=_sourcename;
			int stacksize=_fs->GetStackSize();
			while(_token>0){
				Statement();
				if(_lex._prevtoken!='}')OptionalSemicolon();
			}
			CleanStack(stacksize);
			_fs->AddLineInfos(_lex._currentline,_lineinfo,true);
			_fs->AddInstruction(_OP_RETURN,-1);
			_funcproto(_fs->_func)->_stacksize=_fs->_stacksize;
			_fs->SetStackSize(0);
			_fs->Finalize();
			o=_fs->_func;
#ifdef _DEBUG_DUMP
			_fs->Dump();
#endif
		}
		catch(ParserException &ex){
			if(_raiseerror && vm->_compilererrorhandler){
				SQObjectPtr ret;
				vm->_compilererrorhandler(ex.desc,type(_sourcename)==OT_STRING?_stringval(_sourcename):"unknown",
					_lex._currentline,_lex._currentcolumn);
			}
			return false;
		}
		return true;
	}
	void Statements()
	{
		while(_token!='}' && _token!=DEFAULT && _token!=CASE){
			Statement();
			if(_lex._prevtoken!='}' && _lex._prevtoken!=';')OptionalSemicolon();
		}
	}
	void Statement()
	{
		_fs->AddLineInfos(_lex._currentline,_lineinfo);
		switch(_token){
		case ';':
			Lex();
			break;
		case IF:
			IfStatement();
			break;
		case WHILE:
			WhileStatement();
			break;
		case DO:
			DoWhileStatement();
			break;
		case FOR:
			ForStatement();
			break;
		case FOREACH:
			ForEachStatement();
			break;
		case SWITCH:
			SwitchStatement();
			break;
		case LOCAL:
			LocalDeclStatement();
			break;
		case RETURN:
		case YIELD:{
			SQOpcode op;
			if(_token==RETURN){
				op=_OP_RETURN;
			}
			else{
				op=_OP_YIELD;
				_funcproto(_fs->_func)->_bgenerator=true;
			}
			Lex();
			if(!IsEndOfStatement()){
				Expression();
				_fs->AddInstruction(op,1,_fs->PopTarget());
			}
			else{ _fs->AddInstruction(op,-1); }
			break;}
		case BREAK:
			if(_fs->_breaktargets<=0)Error("'break' has to be in a loop block");
			_fs->AddInstruction(_OP_JMP,0,-1234);
			_fs->_unresolvedbreaks.push_back(_fs->GetCurrentPos());
			Lex();
			break;
		case CONTINUE:
			if(_fs->_continuetargets<=0)Error("'continue' has to be in a loop block");
			_fs->AddInstruction(_OP_JMP,0,-1234);
			_fs->_unresolvedcontinues.push_back(_fs->GetCurrentPos());
			Lex();
			break;
		case FUNCTION:
			FunctionStatement(_token);
			break;
		case '{':{
				int stacksize=_fs->GetStackSize();
				Lex();
				Statements();
				Expect('}');
				_fs->SetStackSize(stacksize);
			}
			break;
		case TRY:
			TryCatchStatement();
			break;
		case THROW:
			Lex();
			Expression();
			_fs->AddInstruction(_OP_THROW,_fs->PopTarget());
			break;
		default:
			Expression();
			_fs->PopTarget();
			break;
		}
		_fs->SnoozeOpt();
	}
	void EmitDerefOp(SQOpcode op)
	{
		int val=_fs->PopTarget();
		int key=_fs->PopTarget();
		int src=_fs->PopTarget();
        _fs->AddInstruction(op,_fs->PushTarget(),src,key,val);
	}
	void Emit2ArgsOP(SQOpcode op,int p3=0)
	{
		int p2=_fs->PopTarget(); //src in OP_GET
		int p1=_fs->PopTarget(); //key in OP_GET
		_fs->AddInstruction(op,_fs->PushTarget(),p1,p2,p3);
	}
	ExpState Expression(bool funcarg=false,bool bdelete=false)
	{
		PushExpState();
		_exst._delete=bdelete;
		_exst._funcarg=funcarg;
		LogicalOrExp();
		switch(_token){
		case '=':
		case CREATE:
		case MINUSEQ:
		case PLUSEQ:{
				int op=_token;
				int ds=_exst._deref;
				if(ds==DEREF_NO_DEREF)Error("can't assign expression");
				Lex(); Expression();

				switch(op){
				case CREATE:
					if(ds==DEREF_FIELD)
						EmitDerefOp(_OP_NEWSLOT);
					else //if _derefstate != DEREF_NO_DEREF && DEREF_FIELD so is the index of a local
						Error("can't 'create' a local slot");
					break;
				case '=': //ASSIGN
					if(ds==DEREF_FIELD)
						EmitDerefOp(_OP_SET);
					else {//if _derefstate != DEREF_NO_DEREF && DEREF_FIELD so is the index of a local
						int p2=_fs->PopTarget(); //src in OP_GET
						int p1=_fs->TopTarget(); //key in OP_GET
						_fs->AddInstruction(_OP_MOVE,p1,p2);
					}
					break;
				case MINUSEQ: 
					if(ds==DEREF_FIELD)
						EmitDerefOp(_OP_MINUSEQ);
					else {//if _derefstate != DEREF_NO_DEREF && DEREF_FIELD so is the index of a local
						Emit2ArgsOP(_OP_MINUSEQ,-1);
					}
					break;
				case PLUSEQ: 
					if(ds==DEREF_FIELD)
						EmitDerefOp(_OP_PLUSEQ);
					else {//if _derefstate != DEREF_NO_DEREF && DEREF_FIELD so is the index of a local
						Emit2ArgsOP(_OP_PLUSEQ,-1);
					}				}
			}
			break;
		case '?':{
			Lex();
			_fs->AddInstruction(_OP_JZ,_fs->PopTarget());
			int jzpos=_fs->GetCurrentPos();
			int trg=_fs->PushTarget();
			Expression();
			int first_exp=_fs->PopTarget();
			if(trg!=first_exp)_fs->AddInstruction(_OP_MOVE,trg,first_exp);
			int endfirstexp=_fs->GetCurrentPos();
			_fs->AddInstruction(_OP_JMP,0,0);
			Expect(':');
			int jmppos=_fs->GetCurrentPos();
			Expression();
			int second_exp=_fs->PopTarget();
			if(trg!=second_exp)_fs->AddInstruction(_OP_MOVE,trg,second_exp);
			_fs->SetIntructionParam(jmppos,1,_fs->GetCurrentPos()-jmppos);
			_fs->SetIntructionParam(jzpos,1,endfirstexp-jzpos+1);
			_fs->SnoozeOpt();
			}
			break;
		}
		return PopExpState();
	}
	void BIN_EXP(SQOpcode op,void (SQCompiler::*f)(void))
	{
		Lex(); (this->*f)();
		int op1=_fs->PopTarget();int op2=_fs->PopTarget();
		_fs->AddInstruction(op,_fs->PushTarget(),op1,op2);
	}
	void LogicalOrExp()
	{
		LogicalAndExp();
		for(;;)if(_token==OR){
			int first_exp=_fs->PopTarget();
			int trg=_fs->PushTarget();
			_fs->AddInstruction(_OP_OR,trg,0,first_exp,0);
			int jpos=_fs->GetCurrentPos();
			if(trg!=first_exp)_fs->AddInstruction(_OP_MOVE,trg,first_exp);
			Lex(); LogicalOrExp();
			int second_exp=_fs->PopTarget();
			if(trg!=second_exp)_fs->AddInstruction(_OP_MOVE,trg,second_exp);
			_fs->SnoozeOpt();
			_fs->SetIntructionParam(jpos,1,(_fs->GetCurrentPos()-jpos));

			break;
		}else return;
	}
	void LogicalAndExp()
	{
		BitwiseOrExp();
		for(;;)switch(_token){
		case AND: {
			int op1=_fs->PopTarget();
			_fs->AddInstruction(_OP_AND,_fs->PushTarget(),0,op1,0);
			_fs->PopTarget();
			int jpos=_fs->GetCurrentPos();
			Lex(); LogicalAndExp();
			_fs->SnoozeOpt();
			_fs->SetIntructionParam(jpos,1,_fs->GetCurrentPos()-jpos);
			break;
			}
		case IN: BIN_EXP(_OP_EXISTS,&SQCompiler::BitwiseOrExp);break;
		default:
			return;
		}
	}
	void BitwiseOrExp()
	{
		BitwiseXorExp();
		for(;;)if(_token=='|')
		{BIN_EXP(_OP_BWOR,&SQCompiler::BitwiseXorExp);
		}else return;
	}
	void BitwiseXorExp()
	{
		BitwiseAndExp();
		for(;;)if(_token=='^')
		{BIN_EXP(_OP_BWXOR,&SQCompiler::BitwiseAndExp);
		}else return;
	}
	void BitwiseAndExp()
	{
		CompExp();
		for(;;)if(_token=='&')
		{BIN_EXP(_OP_BWAND,&SQCompiler::CompExp);
		}else return;
	}
	void CompExp()
	{
		ShiftExp();
		for(;;)switch(_token){
		case EQ: BIN_EXP(_OP_EQ,&SQCompiler::ShiftExp);break;
		case '>': BIN_EXP(_OP_G,&SQCompiler::ShiftExp);break;
		case '<': BIN_EXP(_OP_L,&SQCompiler::ShiftExp);break;
		case GE: BIN_EXP(_OP_GE,&SQCompiler::ShiftExp);break;
		case LE: BIN_EXP(_OP_LE,&SQCompiler::ShiftExp);break;
		case NE: BIN_EXP(_OP_NE,&SQCompiler::ShiftExp);break;
		default:return;	
		}
	}
	void ShiftExp()
	{
		PlusExp();
		for(;;)switch(_token){
		case SHIFTL: BIN_EXP(_OP_SHIFTL,&SQCompiler::PlusExp);break;
		case SHIFTR: BIN_EXP(_OP_SHIFTR,&SQCompiler::PlusExp);break;
		default:return;	
		}
	}
	void PlusExp()
	{
		MultExp();
		for(;;)switch(_token){
		case '+':BIN_EXP(_OP_ADD,&SQCompiler::MultExp);break;
		case '-':BIN_EXP(_OP_SUB,&SQCompiler::MultExp);break;
		default:return;
		}
	}
	
	void MultExp()
	{
		Factor();
		for(;;)switch(_token){
		case '*':BIN_EXP(_OP_MUL,&SQCompiler::Factor);break;
		case '/':BIN_EXP(_OP_DIV,&SQCompiler::Factor);break;
		case '%': BIN_EXP(_OP_MODULO,&SQCompiler::Factor);break;
		default:return;
		}
	}
	void Factor()
	{
		switch(_token)
		{
		case STRING_LITERAL:{
				SQObjectPtr id(SQString::Create(_lex._svalue));
				_fs->AddInstruction(_OP_LOAD,_fs->PushTarget(),_fs->GetStringConstant(_stringval(id)));
				Lex(); 
			}
			break;
		case IDENTIFIER:
		case THIS:{
			SQObjectPtr id(_token==IDENTIFIER?SQString::Create(_lex._svalue):SQString::Create("this"));
				int pos=-1;
				Lex();
				if((pos=_fs->GetLocalVariable(id))==-1){
					_fs->PushTarget(0);
					_fs->AddInstruction(_OP_LOAD,_fs->PushTarget(),_fs->GetStringConstant(_stringval(id)));
					if(NeedGet())Emit2ArgsOP(_OP_GET);
					_exst._deref=DEREF_FIELD;
				}
				else{
					if(!IsDerefToken(_token) && ((!_exst._delete) || (_exst._delete && !IsEndOfStatement())) ){
						_fs->PushTarget(pos);
					}else _fs->PushTarget(pos);
					_exst._deref=pos;
				}
				PrefixedExpr(pos);
			}
			break;
		case DOUBLE_COLON:  // "::"
			_fs->AddInstruction(_OP_LOADROOTTABLE,_fs->PushTarget());
			_exst._deref=DEREF_FIELD;
			_token='.'; //hack
			PrefixedExpr(-1);
			break;
		case _NULL: 
			_fs->AddInstruction(_OP_LOADNULL,_fs->PushTarget());
			Lex();
			break;
		case INTEGER: 
			_fs->AddInstruction(_OP_LOAD,_fs->PushTarget(),_fs->GetNumericConstant(_lex._nvalue));
			Lex();
			break;
		case FLOAT: 
			_fs->AddInstruction(_OP_LOAD,_fs->PushTarget(),_fs->GetNumericConstant(_lex._fvalue));
			Lex();
			break;
		case '[':{
				_fs->AddInstruction(_OP_NEWARRAY,_fs->PushTarget());
				int apos=_fs->GetCurrentPos(),key=0;
				Lex();
				while(_token!=']'){
                    Expression(); 
					if(_token==',')Lex();
					int val=_fs->PopTarget();
					int array=_fs->TopTarget();
					_fs->AddInstruction(_OP_APPENDARRAY,array,val);
					key++;
				}
				_fs->SetIntructionParam(apos,1,key);
				Lex();
			}
			break;
		case '{':{
			_fs->AddInstruction(_OP_NEWTABLE,_fs->PushTarget());
			int tpos=_fs->GetCurrentPos(),nkeys=0;
			Lex();
			while(_token!='}'){
				if(_token=='['){Lex();Expression();Expect(']');}
				else{
					_fs->AddInstruction(_OP_LOAD,_fs->PushTarget(),_fs->GetStringConstant(_stringval(Expect(IDENTIFIER))));
				}
				Expect('=');Expression();
				if(_token==',')Lex();//optional comma
				
				nkeys++;
				int val=_fs->PopTarget();
				int key=_fs->PopTarget();
				int table=_fs->TopTarget(); //<<BECAUSE OF THIS NO COMMON EMIT FUNC IS POSSIBLE
				_fs->AddInstruction(_OP_NEWSLOT,_fs->PushTarget(),table,key,val);
				_fs->PopTarget();
			}
			_fs->SetIntructionParam(tpos,1,nkeys);
			Lex();
				 }
			break;
		case FUNCTION: FunctionExp(_token);break;
		case '-': UnaryOP(_OP_NEG); break;
		case '!': UnaryOP(_OP_NOT); break;
		case '~': UnaryOP(_OP_BWNOT); break;
		case TYPEOF : UnaryOP(_OP_TYPEOF); break;
		case RESUME : UnaryOP(_OP_RESUME); break;
		case CLONE : UnaryOP(_OP_CLONE); break;
		case DELETE : DeleteExpr(); break;
		case DELEGATE : DelegateExpr(); break;
		case '(': Lex(); Expression(); Expect(')'); PrefixedExpr(); 
			break;
		default: Error("expression expected");
		}
	}
	void UnaryOP(SQOpcode op)
	{
		Lex();Factor();
		int src=_fs->PopTarget();
		_fs->AddInstruction(op,_fs->PushTarget(),src);
	}
	bool NeedGet()
	{
		return _token!='=' && _token!=PLUSEQ && _token!=MINUSEQ && _token!='(' && _token!=CREATE && ((!_exst._delete) || (_exst._delete && (_token=='.' || _token=='[')));
	}
	//if 'pos' != -1 the previous variable is a local variable
	void PrefixedExpr(int pos=-1)
	{
		bool bexit=false;
		while(!bexit){
			switch(_token){
			case '.':{
				pos=-1;
				SQObjectPtr idx;
				Lex(); idx=Expect(IDENTIFIER); 
				_fs->AddInstruction(_OP_LOAD,_fs->PushTarget(),_fs->GetStringConstant(_stringval(idx)));
				if(NeedGet() )Emit2ArgsOP(_OP_GET);
				_exst._deref=DEREF_FIELD;
				}
				break;
			case '[': 
				Lex(); Expression(); Expect(']'); 
				pos=-1;
				if(NeedGet())Emit2ArgsOP(_OP_GET);
				_exst._deref=DEREF_FIELD;
				break;
			case '(': {
				if(_exst._deref!=DEREF_NO_DEREF){
					if(pos==-1){
						int key=_fs->PopTarget(); //key
						int table=_fs->PopTarget(); //table etc...
						int closure=_fs->PushTarget();
						int ttarget=_fs->PushTarget();
						_fs->AddInstruction(_OP_PREPCALL,closure,key,table,ttarget);
					}
					else{
						_fs->AddInstruction(_OP_MOVE,_fs->PushTarget(),0);
					}
				}
				else
					_fs->AddInstruction(_OP_MOVE,_fs->PushTarget(),0);
				_exst._deref=DEREF_NO_DEREF;
				Lex();
				FunctionCallArgs();
				 }
				break;
			default: return;
			}
		}
	}
	void FunctionCallArgs()
	{
		int nargs=1;//this
		 while(_token!=')'){
			 Expression(true);
			 int pos=_fs->PopTarget();
			 int trg=_fs->PushTarget(); 
			 if(trg!=pos)_fs->AddInstruction(_OP_MOVE,trg,pos);
			 nargs++;if(_token==',')Lex();
		 }Lex();
		 for(int i=0;i<(nargs-1);i++)_fs->PopTarget();
		 int stackbase=_fs->PopTarget();
		 int closure=_fs->PopTarget();
         _fs->AddInstruction(_OP_CALL,_fs->PushTarget(),closure,stackbase,nargs);
	}
	void LocalDeclStatement()
	{
		SQObjectPtr varname;
		do{
			Lex();varname=Expect(IDENTIFIER);
			if(_token=='='){
				Lex();Expression();
				int src=_fs->PopTarget();
				int dest=_fs->PushTarget();
				if(dest!=src)_fs->AddInstruction(_OP_MOVE,dest,src);
			}
			else{
				_fs->AddInstruction(_OP_LOADNULL,_fs->PushTarget());
			}
			_fs->PopTarget();
			_fs->PushLocalVariable(varname);
		
		}while(_token==',');
	}
	void IfStatement()
	{
		int jmppos;
		bool haselse=false;
		Lex();Expect('(');Expression();Expect(')');
		_fs->AddInstruction(_OP_JZ,_fs->PopTarget());
		int jnepos=_fs->GetCurrentPos();
		int stacksize=_fs->GetStackSize();
		
		Statement();
		//
		if(_token!='}' && _token!=ELSE)OptionalSemicolon();
		
		CleanStack(stacksize);
		int endifblock=_fs->GetCurrentPos();
		if(_token==ELSE){
			haselse=true;
			stacksize=_fs->GetStackSize();
			_fs->AddInstruction(_OP_JMP);
			jmppos=_fs->GetCurrentPos();
			Lex();
			Statement();OptionalSemicolon();
			CleanStack(stacksize);
			_fs->SetIntructionParam(jmppos,1,_fs->GetCurrentPos()-jmppos);
		}
		_fs->SetIntructionParam(jnepos,1,endifblock-jnepos+(haselse?1:0));
	}
	void WhileStatement()
	{
		int jzpos,jmppos;
		int stacksize=_fs->GetStackSize();
		jmppos=_fs->GetCurrentPos();
		Lex();Expect('(');Expression();Expect(')');
		
		BEGIN_BREAKBLE_BLOCK();
		_fs->AddInstruction(_OP_JZ,_fs->PopTarget());
		jzpos=_fs->GetCurrentPos();
		stacksize=_fs->GetStackSize();
		
		Statement();
		
		CleanStack(stacksize);
		_fs->AddInstruction(_OP_JMP,0,jmppos-_fs->GetCurrentPos()-1);
		_fs->SetIntructionParam(jzpos,1,_fs->GetCurrentPos()-jzpos);
		
		END_BREAKBLE_BLOCK(jmppos);
	}
	void DoWhileStatement()
	{
		Lex();
		int jzpos=_fs->GetCurrentPos();
		int stacksize=_fs->GetStackSize();
		BEGIN_BREAKBLE_BLOCK()
		Statement();
		CleanStack(stacksize);
		Expect(WHILE);
		int continuetrg=_fs->GetCurrentPos()+1;
		Expect('(');Expression();Expect(')');
		_fs->AddInstruction(_OP_JNZ,_fs->PopTarget(),jzpos-_fs->GetCurrentPos()-1);
		END_BREAKBLE_BLOCK(continuetrg);
	}
	void ForStatement()
	{
		Lex();
		int stacksize=_fs->GetStackSize();
		Expect('(');
		if(_token==LOCAL)LocalDeclStatement();
		else if(_token!=';'){
			Expression();
			_fs->PopTarget();
		}
		Expect(';');
		int jmppos=_fs->GetCurrentPos();
		int jzpos=-1;
		if(_token!=';'){Expression();_fs->AddInstruction(_OP_JZ,_fs->PopTarget()); jzpos=_fs->GetCurrentPos();}
		Expect(';');
		int expstart=_fs->GetCurrentPos()+1;
		if(_token!=')'){
			Expression();
			_fs->PopTarget();
		}
		Expect(')');
		int expend=_fs->GetCurrentPos();
		int expsize=(expend-expstart)+1;
		SQInstructionVec exp;
		if(expsize>0){
			for(int i=0;i<expsize;i++)
				exp.push_back(_fs->GetInstruction(expstart+i));
			_fs->PopInstructions(expsize);
		}
		BEGIN_BREAKBLE_BLOCK()
		Statement();
		int continuetrg=_fs->GetCurrentPos()+1;
		if(expsize>0){
			for(int i=0;i<expsize;i++)
				_fs->AddInstruction(exp[i]);
		}
		_fs->AddInstruction(_OP_JMP,0,jmppos-_fs->GetCurrentPos()-1,0);
		if(jzpos>0)_fs->SetIntructionParam(jzpos,1,_fs->GetCurrentPos()-jzpos);
		CleanStack(stacksize);
		
		END_BREAKBLE_BLOCK(continuetrg);
	}
	void ForEachStatement()
	{
		SQObjectPtr idxname,valname;
		Lex();Expect('(');valname=Expect(IDENTIFIER);
		if(_token==','){
			idxname=valname;
			Lex();valname=Expect(IDENTIFIER);
		}
		else{
			idxname=SQString::Create("@INDEX@");
		}
		Expect(IN);
		
		//save the stack size
		int stacksize=_fs->GetStackSize();
		//put the table in the stack(evaluate the table expression)
		Expression();Expect(')');
		int container=_fs->TopTarget();
		//push the index local var
		int indexpos=_fs->PushLocalVariable(idxname);
		_fs->AddInstruction(_OP_LOADNULL,indexpos);
		//push the value local var
		int valuepos=_fs->PushLocalVariable(valname);
		_fs->AddInstruction(_OP_LOADNULL,valuepos);
		//push reference index
		int itrpos=_fs->PushLocalVariable(SQString::Create("@ITERATOR@")); //use invalid id to make it inaccessible
		_fs->AddInstruction(_OP_LOADNULL,itrpos);
		int jmppos=_fs->GetCurrentPos();
		_fs->AddInstruction(_OP_FOREACH,container,0,indexpos);
		int foreachpos=_fs->GetCurrentPos();
		//generate the statement code
		BEGIN_BREAKBLE_BLOCK()
		Statement();
		_fs->AddInstruction(_OP_JMP,0,jmppos-_fs->GetCurrentPos()-1);
		_fs->SetIntructionParam(foreachpos,1,_fs->GetCurrentPos()-foreachpos);
		//restore the local variable stack(remove index,val and ref idx)
		CleanStack(stacksize);
		END_BREAKBLE_BLOCK(foreachpos);
	}
	void SwitchStatement()
	{
		IntVec casesj,stats_sizes,breaks,continues;
		Lex();Expect('(');Expression();Expect(')');
		Expect('{');
		int expr=_fs->TopTarget();
		_fs->_breaktargets++;
		int nbreaks=_fs->_unresolvedbreaks.size();
		int ncontinues=_fs->_unresolvedcontinues.size();
		int jzpos=-1;
		int stats_total=0;
		SQInstructionVec stats;
		while(_token==CASE)
		{
			Lex();Expression();Expect(':');
			int trg=_fs->PopTarget();
			_fs->AddInstruction(_OP_EQ,trg,trg,expr);
			_fs->AddInstruction(_OP_JNZ,trg,0);
			casesj.push_back(_fs->GetCurrentPos());
			int statstart=_fs->GetCurrentPos()+1;
			int nbreaks=_fs->_unresolvedbreaks.size();
			int ncontinues=_fs->_unresolvedcontinues.size();
			int stacksize=_fs->GetStackSize();
			Statements();
			_fs->SetStackSize(stacksize);
			breaks.push_back(_fs->_unresolvedbreaks.size()-nbreaks);
			continues.push_back(_fs->_unresolvedcontinues.size()-ncontinues);
			int statend=_fs->GetCurrentPos();
			int statsize=statend-statstart+1;
			stats_sizes.push_back(statsize);
			//pops move the statements in the temp array
			if(statsize>0){
				for(int i=0;i<statsize;i++)
					stats.push_back(_fs->GetInstruction(statstart+i));
				_fs->PopInstructions(statsize);
			}
			stats_total+=statsize;
		}
		_fs->AddInstruction(_OP_JMP,0,0);
		casesj.push_back(_fs->GetCurrentPos());
		stats_sizes.push_back(0);
		breaks.push_back(0);
		continues.push_back(0);
		//copys the statements back in the funcstate
		int stat_base=_fs->GetCurrentPos();
		if(stats_total>0){
			for(int i=0;i<stats_total;i++)
				_fs->AddInstruction(stats[i]);
		}
		//relocates breaks and continues
		int breaksidx=_fs->_unresolvedbreaks.size()-(_fs->_unresolvedbreaks.size()-nbreaks);
		int continuesidx=_fs->_unresolvedcontinues.size()-(_fs->_unresolvedcontinues.size()-ncontinues);
		for(unsigned int i=0;i<casesj.size();i++){
			int offs=stat_base-casesj[i],k;
			_fs->SetIntructionParam(casesj[i],1,stat_base-casesj[i]);
			for(k=0;k<breaks[i];k++){
				_fs->_unresolvedbreaks[breaksidx]+=offs;
				breaksidx++;
			}
			for(k=0;k<continues[i];k++){
				_fs->_unresolvedcontinues[continuesidx]+=offs;
				continuesidx++;
			}
			stat_base+=stats_sizes[i];
		}
		nbreaks=_fs->_unresolvedbreaks.size()-nbreaks;
		if(nbreaks>0)ResolveBreaks(_fs,nbreaks);
		nbreaks=_fs->_unresolvedbreaks.size();
		if(_token==DEFAULT){
			Lex();Expect(':');
			Statements();
		}
		_fs->_breaktargets--;
		Expect('}');
		nbreaks=_fs->_unresolvedbreaks.size()-nbreaks;
		if(nbreaks>0)ResolveBreaks(_fs,nbreaks);
		_fs->PopTarget(); //pops expr
	}
	void FunctionStatement(int ftype)
	{
		SQObjectPtr id;
		Lex();id=Expect(IDENTIFIER);
		_fs->PushTarget(0);
		_fs->AddInstruction(_OP_LOAD,_fs->PushTarget(),_fs->GetStringConstant(_stringval(id)));
		if(_token==DOUBLE_COLON)Emit2ArgsOP(_OP_GET);
		
		while(_token==DOUBLE_COLON){
			Lex();
			id=Expect(IDENTIFIER);
			_fs->AddInstruction(_OP_LOAD,_fs->PushTarget(),_fs->GetStringConstant(_stringval(id) ));
			if(_token==DOUBLE_COLON)Emit2ArgsOP(_OP_GET);
		}
		Expect('(');
		CreateFunction(id);
		_fs->AddInstruction(_OP_CLOSURE,_fs->PushTarget(),_fs->_functions.size()-1,ftype==FUNCTION?0:1);
		EmitDerefOp(_OP_NEWSLOT);
		_fs->PopTarget();
	}
	void TryCatchStatement()
	{
		SQObjectPtr exid;
		Lex();
		_fs->AddInstruction(_OP_PUSHTRAP,0,0);
		int trappos=_fs->GetCurrentPos();
		Statement();
		_fs->AddInstruction(_OP_POPTRAP,0,0);
		_fs->AddInstruction(_OP_JMP,0,0);
		int jmppos=_fs->GetCurrentPos();
		_fs->SetIntructionParam(trappos,1,(_fs->GetCurrentPos()-trappos));
		Expect(CATCH);Expect('(');exid=Expect(IDENTIFIER);Expect(')');
		int stacksize=_fs->GetStackSize();
		int ex_target=_fs->PushLocalVariable(exid);
		_fs->SetIntructionParam(trappos,0,ex_target);
		Statement();
		_fs->SetIntructionParams(jmppos,0,(_fs->GetCurrentPos()-jmppos),0);
		CleanStack(stacksize);
	}
	void FunctionExp(int ftype)
	{
		Lex();Expect('(');
		CreateFunction(_null_);
		_fs->AddInstruction(_OP_CLOSURE,_fs->PushTarget(),_fs->_functions.size()-1,ftype==FUNCTION?0:1);
	}
	void DelegateExpr()
	{
		Lex();Expression();
		Expect(':');
		Expression();
		int table=_fs->PopTarget();int delegate=_fs->PopTarget();
		_fs->AddInstruction(_OP_DELEGATE,_fs->PushTarget(),table,delegate);
	}
	void DeleteExpr()
	{
		ExpState es;
		Lex();es=Expression(false,true);
		int ds=es._deref;
		if(ds==DEREF_NO_DEREF)Error("can't delete an expression");

		if(ds==DEREF_FIELD)Emit2ArgsOP(_OP_DELETE);
		else Error("cannot delete a local");
	}
	void CreateFunction(SQObjectPtr name)
	{
		SQFuncState funcstate(SQFunctionProto::Create(),_fs);
		_funcproto(funcstate._func)->_name=name;
		SQObjectPtr paramname;
		funcstate.AddParameter(SQString::Create("this"));
		_funcproto(funcstate._func)->_sourcename=_sourcename;
		while(_token!=')'){
			paramname=Expect(IDENTIFIER);
			funcstate.AddParameter(paramname);
			if(_token==',')Lex();
			else if(_token!=')')Error("expected ')' or ','");
		}
		Expect(')');
		//outer values
		if(_token==':'){
			Lex();Expect('(');
			while(_token!=')'){
				paramname=Expect(IDENTIFIER);
				//outers are treated as implicit local variables
				funcstate.AddParameter(paramname);
				funcstate.AddOuterValue(paramname);
				if(_token==',')Lex();
				else if(_token!=')')Error("expected ')' or ','");
			}
			Lex();
		}
		
		SQFuncState *currchunk=_fs;
		_fs=&funcstate;
		Statement();
		funcstate.AddLineInfos(_lex._currentline,_lineinfo,true);
        funcstate.AddInstruction(_OP_RETURN,-1);
		funcstate.SetStackSize(0);
		_funcproto(_fs->_func)->_stacksize=_fs->_stacksize;
		funcstate.Finalize();
#ifdef _DEBUG_DUMP
		funcstate.Dump();
#endif
		_fs=currchunk;
		_fs->_functions.push_back(funcstate._func);
	}
	void CleanStack(int stacksize)
	{
		if(_fs->GetStackSize()!=stacksize){
			_fs->SetStackSize(stacksize);
		}
	}
	void ResolveBreaks(SQFuncState *funcstate,int ntoresolve)
	{
		while(ntoresolve>0){
			int pos=funcstate->_unresolvedbreaks.back();
			funcstate->_unresolvedbreaks.pop_back();
			//set the jmp instruction
			funcstate->SetIntructionParams(pos,0,funcstate->GetCurrentPos()-pos,0);
			ntoresolve--;
		}
	}
	void ResolveContinues(SQFuncState *funcstate,int ntoresolve,int targetpos)
	{
		while(ntoresolve>0){
			int pos=funcstate->_unresolvedcontinues.back();
			funcstate->_unresolvedcontinues.pop_back();
			//set the jmp instruction
			funcstate->SetIntructionParams(pos,0,targetpos-pos-1,0);
			ntoresolve--;
		}
	}
private:
	int _token;
	SQFuncState *_fs;
	SQObjectPtr _sourcename;
	SQLexer _lex;
	bool _lineinfo;
	bool _raiseerror;
	int _debugline;
	int _debugop;
	ExpStateVec _expstates;
};

bool Compile(SQVM *vm,SQREADFUNC rg,SQUserPointer up,const SQChar *sourcename,SQObjectPtr &out,bool raiseerror,bool lineinfo)
{
	SQCompiler p(rg,up,sourcename,raiseerror,lineinfo);
	return p.Compile(vm,out);
}
