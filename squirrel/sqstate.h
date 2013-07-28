/*	see copyright notice in squirrel.h */
#ifndef _SQSTATE_H_
#define _SQSTATE_H_

#include "squtils.h"
#include "sqobject.h"
struct SQString;
struct SQTable;
//max number of character for a printed number
#define NUMBER_MAX_CHAR 50

struct StringTable
{
	StringTable();
	~StringTable();
	//return a string obj if exists
	//so when there is a table query, if the string doesn't exists in the global state
	//it cannot be in a table so the result will be always null
	//SQString *get(const SQChar *news);
	SQString *Add(const SQChar *,int len);
	void Remove(SQString *);
private:
	void Resize(int size);
	void AllocNodes(int size);
	SQString **_strings;
	unsigned int _numofslots;
	unsigned int _slotused;
};

#define ADD_STRING(str,len) GS->_stringtable->Add(str,len)
#define REMOVE_STRING(bstr) GS->_stringtable->Remove(bstr)

struct SQObjectPtr;
typedef sqvector<SQObjectPtr> SQObjectPtrVec;

struct SQGlobalState
{
	SQGlobalState();
	~SQGlobalState();
	void Init(SQUIRREL_MALLOC _malloc,SQUIRREL_REALLOC _realloc,SQUIRREL_FREE _free);
public:
	SQChar* GetScratchPad(int size);
#ifdef GARBAGE_COLLECTOR
	int CollectGarbage(SQVM *vm); 
	static void MarkObject(SQObjectPtr &o,SQCollectable **chain);
#endif
	SQUIRREL_MALLOC _sq_malloc;
	SQUIRREL_REALLOC _sq_realloc;
	SQUIRREL_FREE _sq_free;

	SQObjectPtrVec *_metamethods;
	SQObjectPtrVec *_systemstrings;
	SQObjectPtrVec *_types;
	StringTable *_stringtable;
#if defined(CYCLIC_REF_SAFE) || defined(GARBAGE_COLLECTOR)
	SQCollectable *_gc_chain;
	SQCollectable *_vms_chain;
#endif
	SQObjectPtr _null;
	SQObjectPtr _notnull;

	SQObjectPtr _table_default_delegate;
	static SQRegFunction _table_default_delegate_funcz[];
	SQObjectPtr _array_default_delegate;
	static SQRegFunction _array_default_delegate_funcz[];
	SQObjectPtr _string_default_delegate;
	static SQRegFunction _string_default_delegate_funcz[];
	SQObjectPtr _number_default_delegate;
	static SQRegFunction _number_default_delegate_funcz[];
	SQObjectPtr _generator_default_delegate;
	static SQRegFunction _generator_default_delegate_funcz[];
	SQObjectPtr _closure_default_delegate;
	static SQRegFunction _closure_default_delegate_funcz[];
private:
	SQChar *_scratchpad;
	int _scratchpadsize;
};

//#define _null_ (GS->_null)
//#define _notnull_ (GS->_notnull)
#define _sp(s) (GS->GetScratchPad(s))
#define _spval (GS->GetScratchPad(-1))

#define _table_ddel		_table(GS->_table_default_delegate) 
#define _array_ddel		_table(GS->_array_default_delegate) 
#define _string_ddel	_table(GS->_string_default_delegate) 
#define _number_ddel	_table(GS->_number_default_delegate) 
#define _generator_ddel	_table(GS->_generator_default_delegate) 
#define _closure_ddel	_table(GS->_closure_default_delegate) 

extern SQGlobalState *GS;
extern SQObjectPtr _null_;
extern SQObjectPtr _notnull_;
#endif //_SQSTATE_H_
