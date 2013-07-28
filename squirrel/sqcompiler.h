/*	see copyright notice in squirrel.h */
#ifndef _SQCOMPILER_H_
#define _SQCOMPILER_H_

struct SQVM;

#define	IDENTIFIER	258
#define	STRING_LITERAL	259
#define	INTEGER	260
#define	FLOAT	261
#define	DELEGATE	262
#define	DELETE	263
#define	EQ	264
#define	NE	265
#define	LE	266
#define	GE	267
#define	SWITCH	268
#define	ARROW	269
#define	AND	270
#define	OR	271
#define	IF	272
#define	ELSE	273
#define	WHILE	274
#define	BREAK	275
#define	FOR	276
#define	DO	277
#define	_NULL	278
#define	FOREACH	279
#define	IN	280
#define	NEWSLOT	281
#define	MODULO	282
#define	LOCAL	283
#define	CLONE	284
#define	FUNCTION	285
#define	RETURN	286
#define	TYPEOF	287
#define	UMINUS	288
#define	PLUSEQ	289
#define	MINUSEQ	290
#define	CONTINUE	291
#define YIELD 292
#define TRY 293
#define CATCH 294
#define THROW 295
#define SHIFTL 296
#define SHIFTR 297
#define RESUME 298
#define DOUBLE_COLON 299
#define CASE 300
#define DEFAULT 301
#define THIS 302
#define PLUSPLUS 303
#define MINUSMINUS 304

struct ParserException{SQChar *desc;ParserException(SQChar *err):desc(err){}};
bool Compile(SQVM *vm,SQLEXREADFUNC rg,SQUserPointer up,const SQChar *sourcename,SQObjectPtr &out,bool raiseerror,bool lineinfo);
#endif //_SQCOMPILER_H_
