/*
	see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
#include <ctype.h>
#include <stdlib.h>
#include "sqtable.h"
#include "sqstring.h"
#include "sqcompiler.h"
#include "sqlexer.h"

#define CUR_CHAR (_currdata)
#define RETURN_TOKEN(t) { _prevtoken = _curtoken; _curtoken = t; return t;}
#define IS_EOB() (CUR_CHAR <= SQUIRREL_EOB)
#define NEXT() Next();_currentcolumn++
#define APPEND_CHAR(c) { _tempstring[size] = (c); size++; }
#define TERMINATE_BUFFER() _tempstring[size] = _SC('\0');
#define ADD_KEYWORD(key,id) _keywords->NewSlot( SQString::Create(ss, _SC(#key)) ,SQInteger(id))

SQLexer::SQLexer(){}
SQLexer::~SQLexer()
{
	_keywords->Release();
}

void SQLexer::Init(SQSharedState *ss, SQLEXREADFUNC rg, SQUserPointer up)
{
	_sharedstate = ss;
	_keywords = SQTable::Create(ss, 26);
	ADD_KEYWORD(while, TK_WHILE);
	ADD_KEYWORD(do, TK_DO);
	ADD_KEYWORD(if, TK_IF);
	ADD_KEYWORD(else, TK_ELSE);
	ADD_KEYWORD(break, TK_BREAK);
	ADD_KEYWORD(continue, TK_CONTINUE);
	ADD_KEYWORD(return, TK_RETURN);
	ADD_KEYWORD(null, TK_NULL);
	ADD_KEYWORD(function, TK_FUNCTION);
	ADD_KEYWORD(local, TK_LOCAL);
	ADD_KEYWORD(for, TK_FOR);
	ADD_KEYWORD(foreach, TK_FOREACH);
	ADD_KEYWORD(in, TK_IN);
	ADD_KEYWORD(typeof, TK_TYPEOF);
	ADD_KEYWORD(delegate, TK_DELEGATE);
	ADD_KEYWORD(delete, TK_DELETE);
	ADD_KEYWORD(try, TK_TRY);
	ADD_KEYWORD(catch, TK_CATCH);
	ADD_KEYWORD(throw, TK_THROW);
	ADD_KEYWORD(clone, TK_CLONE);
	ADD_KEYWORD(yield, TK_YIELD);
	ADD_KEYWORD(resume, TK_RESUME);
	ADD_KEYWORD(switch, TK_SWITCH);
	ADD_KEYWORD(case, TK_CASE);
	ADD_KEYWORD(default, TK_DEFAULT);
	ADD_KEYWORD(this, TK_THIS);

	_readf = rg;
	_up = up;
	_lasttokenline = _currentline = 1;
	_currentcolumn = 0;
	_prevtoken = -1;
	Next();
}

void SQLexer::Next()
{
	SQInteger t = _readf(_up);
	if(t > MAX_CHAR) throw ParserException(_SC("Invalid character"));
	if(t != 0) {
		_currdata = t;
		return;
	}
	_currdata = SQUIRREL_EOB;
}

SQObjectPtr SQLexer::Tok2Str(int tok)
{
	SQObjectPtr itr, key, val;
	int nitr;
	while((nitr = _keywords->Next(itr, key, val)) != -1) {
		itr = (SQInteger)nitr;
		if(((int)_integer(val)) == tok)
			return key;
	}
	return SQObjectPtr();
}

void SQLexer::LexBlockComment()
{
	for(int nest = 1; nest > 0;) {
		switch(CUR_CHAR) {
			case _SC('*'): { NEXT(); if(CUR_CHAR == _SC('/')) { nest--; NEXT(); }}; continue;
			case _SC('/'): { NEXT(); if(CUR_CHAR == _SC('*')) { nest++; NEXT(); }}; continue;
			case _SC('\n'): _currentline++; NEXT(); continue;
			case SQUIRREL_EOB: throw ParserException(_SC("missing \"*/\" in comment"));
			default: NEXT();
		}
	}
}

int SQLexer::Lex()
{
	_lasttokenline = _currentline;
	while(CUR_CHAR>SQUIRREL_EOB) {
		switch(CUR_CHAR){
		case _SC('\t'): case _SC('\r'): case _SC(' '): NEXT(); continue;
		case _SC('\n'):
			_currentline++;
			_prevtoken=_curtoken;
			_curtoken=_SC('\n');
			NEXT();
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
			else { NEXT(); RETURN_TOKEN(TK_EQ); }
		case _SC('<'):
			NEXT();
			if ( CUR_CHAR == _SC('=') ) { NEXT(); RETURN_TOKEN(TK_LE) }
			else if ( CUR_CHAR == _SC('-') ) { NEXT(); RETURN_TOKEN(TK_NEWSLOT); }
			else if ( CUR_CHAR == _SC('<') ) { NEXT(); RETURN_TOKEN(TK_SHIFTL); }
			else if ( CUR_CHAR == _SC('[') ) { NEXT(); ReadMultilineString(); RETURN_TOKEN(TK_STRING_LITERAL); }
			else { RETURN_TOKEN('<') }
		case _SC('>'):
			NEXT();
			if (CUR_CHAR == _SC('=')){ NEXT(); RETURN_TOKEN(TK_GE);}
			else if(CUR_CHAR == _SC('>')){ NEXT(); RETURN_TOKEN(TK_SHIFTR);}
			else { RETURN_TOKEN('>') }
		case _SC('!'):
			NEXT();
			if (CUR_CHAR != _SC('=')){ RETURN_TOKEN('!')}
			else { NEXT(); RETURN_TOKEN(TK_NE); }
		case _SC('"'):
		case _SC('\''):{
			int stype;
			if((stype=ReadString(CUR_CHAR))!=-1){
				RETURN_TOKEN(stype);
			}
			throw ParserException(_SC("error parsing the string"));
			}
		case _SC('{'): case _SC('}'): case _SC('('): case _SC(')'): case _SC('['): case _SC(']'):
		case _SC(';'): case _SC(','): case _SC('%'): case _SC('?'): case _SC('^'): case _SC('~'):
		case _SC('*'): case _SC('.'):
			{int ret = CUR_CHAR;
			NEXT(); RETURN_TOKEN(ret); }
		case _SC('&'):
			NEXT();
			if (CUR_CHAR != _SC('&')){ RETURN_TOKEN('&') }
			else { NEXT(); RETURN_TOKEN(TK_AND); }
		case _SC('|'):
			NEXT();
			if (CUR_CHAR != _SC('|')){ RETURN_TOKEN('|') }
			else { NEXT(); RETURN_TOKEN(TK_OR); }
		case _SC(':'):
			NEXT();
			if (CUR_CHAR != _SC(':')){ RETURN_TOKEN(':') }
			else { NEXT(); RETURN_TOKEN(TK_DOUBLE_COLON); }
		case _SC('-'):
			NEXT();
			if (CUR_CHAR == _SC('=')){ NEXT(); RETURN_TOKEN(TK_MINUSEQ);}
			else if  (CUR_CHAR == _SC('-')){ NEXT(); RETURN_TOKEN(TK_MINUSMINUS);}
			else RETURN_TOKEN('-');
		case _SC('+'):
			NEXT();
			if (CUR_CHAR == _SC('=')){ NEXT(); RETURN_TOKEN(TK_PLUSEQ);}
			else if (CUR_CHAR == _SC('+')){ NEXT(); RETURN_TOKEN(TK_PLUSPLUS);}
			else RETURN_TOKEN('+');
		case SQUIRREL_EOB:
			return 0;
		default:{
				if (scisdigit(CUR_CHAR)) {
					int ret = ReadNumber();
					RETURN_TOKEN(ret);
				}
				else if (scisalpha(CUR_CHAR) || CUR_CHAR == _SC('_')) {
					int t = ReadID();
					RETURN_TOKEN(t);
				}
				else {
					int c = CUR_CHAR;
					if (sciscntrl(c)) throw ParserException(_SC("unexpected character(control)"));
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
	if(_keywords->Get(SQString::Create(_sharedstate, s), t)) {
		return int(_integer(t));
	}
	return TK_IDENTIFIER;
}


int SQLexer::ReadMultilineString()
{
	_longstr.resize(0);
	if(CUR_CHAR == _SC('\n')) { NEXT(); }
	for(int nest = 1; nest > 0;) {
		switch(CUR_CHAR) {
			case _SC(']'): {
				NEXT(); 
				if(CUR_CHAR == _SC('>')) {
					nest--; NEXT(); 
					if(nest > 0) {
						_longstr.push_back(_SC(']')); _longstr.push_back(_SC('>'));
					}
				}
				else { _longstr.push_back(_SC(']')); }
				continue;	} 
			case _SC('<'): { 
				_longstr.push_back(CUR_CHAR); NEXT(); 
				if(CUR_CHAR == _SC('[')) { 
					_longstr.push_back(CUR_CHAR); 
					nest++; NEXT(); 
				}
				continue;	} 
			case _SC('\\'): {
				NEXT();
				switch(CUR_CHAR) {
				case '<': case '>': { _longstr.push_back(CUR_CHAR); NEXT(); } continue;
				default: { _longstr.push_back(_SC('\\')); _longstr.push_back(CUR_CHAR); NEXT(); } continue;
				}
			}
			case SQUIRREL_EOB: throw ParserException(_SC("missing \"]>\" in a long string"));
			default: { _longstr.push_back(CUR_CHAR); NEXT(); }
		}
	}
	_longstr.push_back(_SC('\0'));
	_svalue = &_longstr[0];
	return TK_STRING_LITERAL;
}
int SQLexer::ReadString(int ndelim)
{
	int size = 0;
	NEXT();
	_tempstring[0] = 0;
	
	if(IS_EOB()) return -1;
		
	while(CUR_CHAR != ndelim) {
		if(size >= MAX_STRING) throw ParserException(_SC("string too long"));
		switch(CUR_CHAR) {
		case SQUIRREL_EOB:
			throw ParserException(_SC("unfinished string"));
			return -1;
		case _SC('\n'): throw ParserException(_SC("newline in a constant"));
		case _SC('\\'):
			NEXT();
			switch(CUR_CHAR) {
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
				throw ParserException(_SC("unrecognised escaper char"));
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
	int len = scstrlen(_tempstring);
	if(ndelim == _SC('\'')) {
		if(len == 0) throw ParserException(_SC("empty constant"));
		if(len > 4) throw ParserException(_SC("constant too long"));
		_nvalue = 0;
		for(int i = 0; i < len; i++) 
			_nvalue = (_nvalue<<sizeof(SQChar)) + _tempstring[i];
		return TK_INTEGER;
	}
	_svalue = _tempstring;
	return TK_STRING_LITERAL;
}

int SQLexer::ReadNumber()
{
#define TINT 1
#define TFLOAT 2
#define THEX 3
	int type = TINT, size = 0, firstchar = CUR_CHAR;
	bool isfloat = false;
	SQChar *sTemp;
	NEXT();
	if(firstchar == _SC('0') && toupper(CUR_CHAR) == _SC('X')) {
		NEXT();
		type = THEX;
		while(isxdigit(CUR_CHAR)) {
			APPEND_CHAR(CUR_CHAR);
			NEXT();
		}
		if(size > 8) throw ParserException(_SC("Hex number over 8 digits"));
	}
	else {
		APPEND_CHAR(firstchar);
		while (CUR_CHAR == _SC('.') || scisdigit(CUR_CHAR)) {
            if(CUR_CHAR == _SC('.')) type = TFLOAT;
			APPEND_CHAR(CUR_CHAR);
			NEXT();
		}
	}
	TERMINATE_BUFFER();
	switch(type) {
	case TFLOAT:
		_fvalue = (SQFloat)scstrtod(_tempstring,&sTemp);
		return TK_FLOAT;
	case TINT:
		_nvalue = (SQInteger)scatoi(_tempstring);
		return TK_INTEGER;
	case THEX:
		*((unsigned long *)&_nvalue) = scstrtoul(_tempstring,&sTemp,16);
		return TK_INTEGER;
	}
	return 0;
}

int SQLexer::ReadID()
{
	int res, size = 0;
	do {
		APPEND_CHAR(CUR_CHAR);
		NEXT();
	} while(scisalnum(CUR_CHAR) || CUR_CHAR == _SC('_'));
	TERMINATE_BUFFER();
	res = GetIDType(_tempstring);
	if(res == TK_IDENTIFIER) {
		_svalue = _tempstring;
	}
	return res;
}
