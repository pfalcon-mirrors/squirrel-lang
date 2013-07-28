/*	see copyright notice in squirrel.h */
#ifndef _SQOPCODES_H_
#define _SQOPCODES_H_

enum BitWiseOP {
	BW_AND = 0,
	BW_OR = 2,	//like ADD
	BW_XOR = 3,
	BW_SHIFTL = 4,
	BW_SHIFTR = 5,
	BW_USHIFTR = 6
};
enum SQOpcode
{
	_OP_LINE=				0x00,	
	_OP_LOAD=				0x01,	
	_OP_TAILCALL=			0x02,	
	_OP_CALL=				0x03,	
	_OP_PREPCALL=			0x04,	
	_OP_PREPCALLK=			0x05,	
	_OP_GETK=				0x06,	
	_OP_MOVE=				0x07,	
	_OP_NEWSLOT=			0x08,	
	_OP_DELETE=				0x09,	
	_OP_SET=				0x0A,	
	_OP_GET=				0x0B,
	_OP_EQ=					0x0C,
	_OP_NE=					0x0D,
	_OP_ARITH=				0x0E,
	_OP_BITW=				0x0F,
	_OP_RETURN=				0x10,	
	_OP_LOADNULL=			0x11,	
	_OP_LOADNULLS=			0x12,	
	_OP_LOADROOTTABLE=		0x13,	
	_OP_DMOVE=				0x14,	
	_OP_JMP=				0x15,	
	_OP_JNZ=				0x16,	
	_OP_JZ=					0x17,	
	_OP_LOADFREEVAR=		0x18,	
	_OP_VARGC=				0x19,	
	_OP_GETVARGV=			0x1A,	
	_OP_NEWTABLE=			0x1B,	
	_OP_NEWARRAY=			0x1C,	
	_OP_APPENDARRAY=		0x1D,	
	_OP_GETPARENT=			0x1E,	
	_OP_MINUSEQ=			0x1F,	
	_OP_PLUSEQ=				0x20,	
	_OP_INC=				0x21,	
	_OP_INCL=				0x22,	
	_OP_PINC=				0x23,	
	_OP_PINCL=				0x24,	
	_OP_G=					0x25,	
	_OP_GE=					0x26,	
	_OP_L=					0x27,	
	_OP_LE=					0x28,	
	_OP_EXISTS=				0x29,	
	_OP_INSTANCEOF=			0x2A,	
	_OP_AND=				0x2B,	
	_OP_OR=					0x2C,	
	_OP_NEG=				0x2D,	
	_OP_NOT=				0x2E,	
	_OP_BWNOT=				0x2F,	
	_OP_CLOSURE=			0x30,	
	_OP_YIELD=				0x31,	
	_OP_RESUME=				0x32,
	_OP_FOREACH=			0x33,
	_OP_DELEGATE=			0x34,
	_OP_CLONE=				0x35,
	_OP_TYPEOF=				0x36,
	_OP_PUSHTRAP=			0x37,
	_OP_POPTRAP=			0x38,
	_OP_THROW=				0x39,
	_OP_CLASS=				0x3A,
	_OP_NEWSLOTA=			0x3B
};

struct SQInstructionDesc {
	const SQChar *name;
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
