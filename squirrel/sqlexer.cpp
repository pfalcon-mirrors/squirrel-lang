/*
	see copyright notice in squirrel.h
*/
#include "stdafx.h"
#include <ctype.h>
#include <stdlib.h>
#include "sqtable.h"
#include "sqstring.h"
#include "sqcompiler.h"
#include "sqlexer.h"

#define MAX_STRING 2024

#define CUR_CHAR (_currdata)
#define RETURN_TOKEN(t) {_prevtoken=_curtoken;_curtoken=t;return t;}
#define IS_EOB() (CUR_CHAR<=SQUIRREL_EOB)
#define NEXT() Next();_currentcolumn++
#define APPEND_CHAR(c) {_tempstring[size]=(c);size++;}
#define TERMINATE_BUFFER() _tempstring[size]=_SC('\0');
#define ADD_KEYWORD(key,id) _keywords->NewSlot( SQString::Create(#key) , SQInteger(id) )

SQLexer::SQLexer()
{
	_keywords=SQTable::Create(26);
	ADD_KEYWORD(while,WHILE);
	ADD_KEYWORD(do,DO);
	ADD_KEYWORD(if,IF);
	ADD_KEYWORD(else,ELSE);
	ADD_KEYWORD(break,BREAK);
	ADD_KEYWORD(continue,CONTINUE);
	ADD_KEYWORD(return,RETURN);
	ADD_KEYWORD(null,_NULL);
	ADD_KEYWORD(function,FUNCTION);
	ADD_KEYWORD(local,LOCAL);
	ADD_KEYWORD(for,FOR);
	ADD_KEYWORD(foreach,FOREACH);
	ADD_KEYWORD(in,IN);
	ADD_KEYWORD(typeof,TYPEOF);
	ADD_KEYWORD(delegate,DELEGATE);
	ADD_KEYWORD(delete,DELETE);
	ADD_KEYWORD(try,TRY);
	ADD_KEYWORD(catch,CATCH);
	ADD_KEYWORD(throw,THROW);
	ADD_KEYWORD(clone,CLONE);
	ADD_KEYWORD(yield,YIELD);
	ADD_KEYWORD(resume,RESUME);
	ADD_KEYWORD(switch,SWITCH);
	ADD_KEYWORD(case,CASE);
	ADD_KEYWORD(default,DEFAULT);
	ADD_KEYWORD(this,THIS);
}
SQLexer::~SQLexer()
{
	_keywords->Release();
}
void SQLexer::Init(SQREADFUNC rg,SQUserPointer up)
{
	_readf=rg;
	_up=up;
	_currentline=1;
	_currentchar=0;
	_currentcolumn=0;
	_prevtoken=-1;
	_lastline=_source;
	Next();
	
}

void SQLexer::Next()
{
	SQChar t;
	if(_readf(_up,&t,sizeof(SQChar))!=-1){
		_currdata=t;
		return;
	}
	_currdata=SQUIRREL_EOB;
}

SQObjectPtr SQLexer::Tok2Str(int tok)
{
	SQObjectPtr itr,key,val;
	int nitr;
	while((nitr=_keywords->Next(itr,key,val))!=-1){
		itr=(SQInteger)nitr;
		if(int(_integer(val))==tok)
			return key;
	}
	return SQObjectPtr();
}

void SQLexer::LexBlockComment()
{
	for(int nest = 1; nest>0;){
		switch(CUR_CHAR){
			case _SC('*'): {NEXT(); if(CUR_CHAR==_SC('/')) { nest--; NEXT(); }}; continue;
			case _SC('/'): {NEXT(); if(CUR_CHAR==_SC('*')) { nest++; NEXT(); }}; continue;
			case _SC('\n'): _currentline++; NEXT(); continue;
			case SQUIRREL_EOB: throw ParserException("missing \"*/\" in comment");
			default: NEXT();
		}
	}
}

int SQLexer::Lex()
{
	while(CUR_CHAR>SQUIRREL_EOB){
		switch(CUR_CHAR){
		case _SC('\t'): case _SC('\r'): case _SC(' '): NEXT(); continue;
		case _SC('\n'):
			_currentline++;
			_prevtoken=_curtoken;
			_curtoken='\n';
			NEXT();
			_lastline=&CUR_CHAR;
			_currentcolumn=1;
			continue;
		case _SC('/'):
			NEXT();
			switch(CUR_CHAR){
			case _SC('*'):
				NEXT();
				LexBlockComment();
				continue;	
			case _SC('/'):
				do { NEXT(); } while (CUR_CHAR != _SC('\n') && (!IS_EOB()));
				continue;
			default:
				RETURN_TOKEN('/');
			}
		case _SC('='):
			NEXT();
			if (CUR_CHAR != _SC('=')){ RETURN_TOKEN('=') }
			else { NEXT(); RETURN_TOKEN(EQ); }
		case _SC('<'):
			NEXT();
			if ( CUR_CHAR == _SC('=') ) { NEXT(); RETURN_TOKEN(LE) }
			else if ( CUR_CHAR == _SC('-') ) { NEXT(); RETURN_TOKEN(CREATE); }
			else if ( CUR_CHAR == _SC('<') ) { NEXT(); RETURN_TOKEN(SHIFTL); }
			else { RETURN_TOKEN('<') }
		case _SC('>'):
			NEXT();
			if (CUR_CHAR == _SC('=')){ NEXT(); RETURN_TOKEN(GE);}
			else if(CUR_CHAR == _SC('>')){ NEXT(); RETURN_TOKEN(SHIFTR);}
			else { RETURN_TOKEN('>') }
		case _SC('!'):
			NEXT();
			if (CUR_CHAR != _SC('=')){ RETURN_TOKEN('!')}
			else { NEXT(); RETURN_TOKEN(NE); }
		case _SC('"'):
		case _SC('\''):{
			int stype;
			if((stype=ReadString(CUR_CHAR))!=-1){
				RETURN_TOKEN(stype);
			}
			throw ParserException("error parsing the string");
			}
		case _SC('{'):case _SC('}'):case _SC('('):case _SC(')'):case _SC('['):case _SC(']'):
		case _SC(';'):case _SC(','):case _SC('%'):case _SC('?'):case _SC('^'):case _SC('~'):
		case _SC('*'):case _SC('.'):
			{int ret=CUR_CHAR;
			NEXT();RETURN_TOKEN(ret);}
		case _SC('&'):
			NEXT();
			if (CUR_CHAR != _SC('&')){ RETURN_TOKEN('&') }
			else { NEXT(); RETURN_TOKEN(AND); }
		case _SC('|'):
			NEXT();
			if (CUR_CHAR != _SC('|')){ RETURN_TOKEN('|') }
			else { NEXT(); RETURN_TOKEN(OR); }
		case _SC(':'):
			NEXT();
			if (CUR_CHAR != _SC(':')){ RETURN_TOKEN(':') }
			else { NEXT(); RETURN_TOKEN(DOUBLE_COLON); }
		case _SC('-'):
			NEXT();
			if (CUR_CHAR != _SC('=')){ RETURN_TOKEN('-')}
			else { NEXT(); RETURN_TOKEN(MINUSEQ) }
		case _SC('+'):
			NEXT();
			if (CUR_CHAR != _SC('=')){ RETURN_TOKEN('+')}
			else { NEXT(); RETURN_TOKEN(PLUSEQ) }
		case SQUIRREL_EOB:
			return 0;
		default:
			{
				if (isdigit(CUR_CHAR)) {
					int ret=ReadNumber();
					RETURN_TOKEN(ret);
				}
				else if (isalpha(CUR_CHAR) || CUR_CHAR == _SC('_')) {
					int t=ReadID();
					RETURN_TOKEN(t);
				}
				else {
					int c = CUR_CHAR;
					if (iscntrl(c))	throw ParserException("unexpected character(control)");
					NEXT();
					RETURN_TOKEN(c);  
				}
				RETURN_TOKEN(0);
			}
		}
	}

	return 0;    
}
	
int SQLexer::GetIDType(SQChar *s)
{
	SQObjectPtr t;
	if(_keywords->Get(SQString::Create(s),t)){
		return int(_integer(t));
	}
	return IDENTIFIER;
}
int SQLexer::ReadString(int ndelim)
{
	int size=0;
	NEXT();
	_tempstring[0]=0;
	
	if(IS_EOB())return -1;
		
	while(CUR_CHAR!=ndelim){
		switch(CUR_CHAR){
		case SQUIRREL_EOB:
			throw ParserException("unfinished string");
			return -1;
		case _SC('\\'):
			NEXT();
			switch(CUR_CHAR){
			case _SC('\n'): APPEND_CHAR(_SC('\n')); _currentline++; break;
			case _SC('t'): APPEND_CHAR(_SC('\t')); NEXT(); break;
			case _SC('a'): APPEND_CHAR(_SC('\a')); NEXT(); break;
			case _SC('b'): APPEND_CHAR(_SC('\b')); NEXT(); break;
			case _SC('n'): APPEND_CHAR(_SC('\n')); NEXT(); break;
			case _SC('r'): APPEND_CHAR(_SC('\r')); NEXT(); break;
			case _SC('v'): APPEND_CHAR(_SC('\v')); NEXT(); break;
			case _SC('f'): APPEND_CHAR(_SC('\f')); NEXT(); break;
			case _SC('\\'): APPEND_CHAR(_SC('\\')); NEXT(); break;
			case _SC('"'): APPEND_CHAR(_SC('"')); NEXT(); break;
			case _SC('\''): APPEND_CHAR(_SC('\'')); NEXT(); break;
			
			default:
				throw ParserException("unrecognised escaper char");
				break;
			}
			break;
		default:
			APPEND_CHAR(CUR_CHAR);
			NEXT();
		}
	}
	NEXT();
	TERMINATE_BUFFER();
	int len=strlen(_tempstring);
	if(ndelim==_SC('\'')){
		if(len==0)throw ParserException("empty constant");
		if(len>4)throw ParserException("constant too long");
		_nvalue=0;
		for(int i = 0; i<len; i++) 
			_nvalue = (_nvalue<<8)+_tempstring[i];
		return INTEGER;
	}
	_svalue = _tempstring;
	return STRING_LITERAL;
	
}

int SQLexer::ReadNumber()
{
#define TINT 1
#define TFLOAT 2
#define THEX 3
	int type=TINT,size=0,firstchar=CUR_CHAR;
	bool isfloat=false;
	SQChar *sTemp;
	NEXT();
	if(firstchar==_SC('0') && toupper(CUR_CHAR)==_SC('X')){
		NEXT();
		type=THEX;
		while(isxdigit(CUR_CHAR)){
			APPEND_CHAR(CUR_CHAR);
			NEXT();
		}
		if(size>8)throw ParserException("Hex number over 8 digits");
	}
	else{
		APPEND_CHAR(firstchar);
		while(CUR_CHAR==_SC('.') || isdigit(CUR_CHAR)){
            if(CUR_CHAR==_SC('.'))type=TFLOAT;
			APPEND_CHAR(CUR_CHAR);
			NEXT();
		}
	}
	
	TERMINATE_BUFFER();
	switch(type){
	case TFLOAT:
		_fvalue=(SQFloat)strtod(_tempstring,&sTemp);
		return FLOAT;
		break;
	case TINT:
		_nvalue=(SQInteger)atoi(_tempstring);
		return INTEGER;
		break;
	case THEX:
		*((unsigned long *)&_nvalue)=strtoul(_tempstring,&sTemp,16);
		return INTEGER;
		break;
	}
	return 0;
#undef TINT
#undef TFLOAT
#undef THEX
}

int SQLexer::ReadID()
{
	int res,size=0;
	do{
		APPEND_CHAR(CUR_CHAR);
		NEXT();
	}while (isalnum(CUR_CHAR) || CUR_CHAR == _SC('_'));
	TERMINATE_BUFFER();
	res=GetIDType(_tempstring);
	if(res==IDENTIFIER){
		_svalue = _tempstring;
	}
	return res;
}
