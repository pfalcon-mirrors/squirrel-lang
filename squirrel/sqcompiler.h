/*	see copyright notice in squirrel.h */
#ifndef _SQCOMPILER_H_
#define _SQCOMPILER_H_

struct SQVM;

#define	TK_IDENTIFIER	258
#define	TK_STRING_LITERAL	259
#define	TK_INTEGER	260
#define	TK_FLOAT	261
#define	TK_DELEGATE	262
#define	TK_DELETE	263
#define	TK_EQ	264
#define	TK_NE	265
#define	TK_LE	266
#define	TK_GE	267
#define	TK_SWITCH	268
#define	TK_ARROW	269
#define	TK_AND	270
#define	TK_OR	271
#define	TK_IF	272
#define	TK_ELSE	273
#define	TK_WHILE	274
#define	TK_BREAK	275
#define	TK_FOR	276
#define	TK_DO	277
#define	TK_NULL	278
#define	TK_FOREACH	279
#define	TK_IN	280
#define	TK_NEWSLOT	281
#define	TK_MODULO	282
#define	TK_LOCAL	283
#define	TK_CLONE	284
#define	TK_FUNCTION	285
#define	TK_RETURN	286
#define	TK_TYPEOF	287
#define	TK_UMINUS	288
#define	TK_PLUSEQ	289
#define	TK_MINUSEQ	290
#define	TK_CONTINUE	291
#define TK_YIELD 292
#define TK_TRY 293
#define TK_CATCH 294
#define TK_THROW 295
#define TK_SHIFTL 296
#define TK_SHIFTR 297
#define TK_RESUME 298
#define TK_DOUBLE_COLON 299
#define TK_CASE 300
#define TK_DEFAULT 301
#define TK_THIS 302
#define TK_PLUSPLUS 303
#define TK_MINUSMINUS 304
#define TK_PARENT 305

struct ParserException{ SQChar *desc; ParserException(SQChar *err):desc(err) {} };
bool Compile(SQVM *vm, SQLEXREADFUNC rg, SQUserPointer up, const SQChar *sourcename, SQObjectPtr &out, bool raiseerror, bool lineinfo);
#endif //_SQCOMPILER_H_
