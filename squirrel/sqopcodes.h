/*	see copyright notice in squirrel.h */
#ifndef _SQOPCODES_H_
#define _SQOPCODES_H_

enum SQOpcode
{
	_OP_LINE=				0x00,	//LINE				<<linenum>>
	_OP_LOAD=				0x01,	//<<target>> <<literal>> 
	_OP_LOADNULL=			0x02,	//<<target>>
	_OP_NEWSLOT=			0x03,	//<<target>> <<src>> <<key>> <<val>>
	_OP_SET=				0x04,	//<<target>> <<src>> <<key>> <<val>>
	_OP_GET=				0x05,	//<<target>> <<src>> <<key>>
	_OP_LOADROOTTABLE=		0x06,	//<<target>>
	_OP_PREPCALL=			0x07,	//<<closuretarget>> <<key>> <<src>> <<ttarget>>
	_OP_CALL=				0x08,	//<<target>> <<closure>> <<firstarg>> <<nargs>>
	_OP_MOVE=				0x09,	//<<target>> <<src>>
	_OP_ADD=				0x0A,	//<<terget>> <<op1>> <<op2>>
	_OP_SUB=				0x0B,	//like ADD
	_OP_MUL=				0x0C,	//like ADD
	_OP_DIV=				0x0D,	//like ADD
	_OP_MODULO=				0x0E,	//like ADD
	_OP_YIELD=				0x0F,	//like return
	_OP_EQ=					0x10,	//like ADD
	_OP_NE=					0x11,	//like ADD
	_OP_G=					0x12,	//like ADD
	_OP_GE=					0x13,	//like ADD
	_OP_L=					0x14,	//like ADD
	_OP_LE=					0x15,	//like ADD
	_OP_EXISTS=				0x16,	//like ADD
	_OP_NEWTABLE=			0x17,	//<<target>> <<size>>
	_OP_JMP=				0x18,	//<<pos>>
	_OP_JNZ=				0x19,	//<<src>> <<pos>>
	_OP_JZ=					0x1A,	//<<src>> <<pos>>
	_OP_RETURN=				0x1B,	//1(return) or -1(pushnull) <<retval>>
	_OP_CLOSURE=			0x1C,	// <<target>> <<index>> <<if 1 is a generator>>
	_OP_FOREACH=			0x1D,	//<<container>> <<first arg>> <<jmppos>>
	_OP_DELEGATE=			0x1E,	//<<target>> <<table>> <<delegate>>
	_OP_TYPEOF=				0x1F,	//<<target>> <<obj>>
	_OP_PUSHTRAP=			0x20,	//<<pos>>
	_OP_POPTRAP=			0x21,	//none
	_OP_THROW=				0x22,	//<<target>>
	_OP_NEWARRAY=			0x23,	//<<target>> <<size>>
	_OP_APPENDARRAY=		0x24,	//<<array>><<val>>
	_OP_AND=				0x25,	// <<target>> <<op1>> <<jmp>>
	_OP_OR=					0x26,	// <<target>> <<op1>> <<jmp>>
	_OP_NEG=				0x27,	//<<target>> <<src>>
	_OP_NOT=				0x28,	//<<target>> <<src>>
	_OP_DELETE=				0x29,	//<<target>> <<table>> <<key>>
	_OP_BWNOT=				0x2A,	//like UNM
	_OP_BWAND=				0x2B,	//like ADD
	_OP_BWOR=				0x2C,	//like ADD
	_OP_BWXOR=				0x2D,	//like ADD
	_OP_MINUSEQ=			0x2E,
	_OP_PLUSEQ=				0x2F,
	_OP_TAILCALL=			0x30,
	_OP_SHIFTL=				0x31,
	_OP_SHIFTR=				0x32,
	_OP_RESUME=				0x33,	//<<target>> <<generator>>
	_OP_CLONE=				0x34,	//<<target>> <<obj>>
	_OP_INC=				0x35,	//<<target>> <<src>> ++i
	_OP_DEC=				0x36,	//<<target>> <<src>> --i
	_OP_PINC=				0x37,	//<<target>> <<src>> i++
	_OP_PDEC=				0x38,	//<<target>> <<src>> i--
	//optimized stuff
	_OP_GETK=				0x39,
	_OP_PREPCALLK=			0x3A,
	_OP_DMOVE=				0x3B,
	_OP_GETPARENT=			0x3C,
	_OP_LOADNULLS=			0x3D,
	_OP_USHIFTR=			0x3E,
};

struct SQInstructionDesc {
	const SQChar *name;
	unsigned char psize[4];
};

struct SQInstruction 
{
	SQInstruction(){};
	SQInstruction(const SQInstruction &i)
	{
		op=i.op;
		_arg0=i._arg0;_arg1=i._arg1;
		_arg2=i._arg2;_arg3=i._arg3;
	}
	SQInstruction(SQOpcode _op,int a0=0,int a1=0,int a2=0,int a3=0)
	{	op=_op;
		_arg0=a0;_arg1=a1;
		_arg2=a2;_arg3=a3;
	}
	unsigned char op;
	unsigned char _arg0;
	unsigned short _arg1;
	unsigned char _arg2;
	unsigned char _arg3;
};

#include "squtils.h"
typedef sqvector<SQInstruction> SQInstructionVec;

#endif // _SQOPCODES_H_
