/*	see copyright notice in squirrel.h */
#ifndef _SQLEXER_H_
#define _SQLEXER_H_

#define MAX_STRING 2024

struct SQLexer
{
	SQLexer();
	~SQLexer();
	void Init(SQSharedState *ss,SQLEXREADFUNC rg,SQUserPointer up);
	int Lex();
	SQObjectPtr Tok2Str(int tok);
private:
	int GetIDType(SQChar *s);
	int ReadString(int ndelim);
	int ReadNumber();
	void LexBlockComment();
	int ReadID();
	void Next();
	const SQChar *_source;
	const SQChar *_lastline;
	int _curtoken;
	SQChar _tempstring[MAX_STRING];
	SQTable *_keywords;
public:
	int _prevtoken;
	int _currentchar;
	int _currentline;
	int _currentcolumn;
	const SQChar *_svalue;
	SQInteger _nvalue;
	SQFloat _fvalue;
	SQLEXREADFUNC _readf;
	SQUserPointer _up;
	SQChar _currdata;
	SQSharedState *_sharedstate;
};

#endif
