/*	see copyright notice in squirrel.h */
#ifndef _SQTABLE_H_
#define _SQTABLE_H_
/*
* The following code is based on Lua 4.0 (Copyright 1994-2002 Tecgraf, PUC-Rio.)
* http://www.lua.org/copyright.html#4
* http://www.lua.org/source/4.0.1/src_ltable.c.html
*/

#include "sqstring.h"


#define hashptr(p)  ((SQHash)(((SQInteger)p) >> 3))

inline SQHash HashObj(const SQObjectPtr &key)
{
	switch(type(key)) {
		case OT_STRING:		return _string(key)->_hash;
		case OT_FLOAT:		return (SQHash)((SQInteger)_float(key));
		case OT_BOOL: case OT_INTEGER:	return (SQHash)((SQInteger)_integer(key));
		default:			return hashptr(key._unVal.pRefCounted);
	}
}

struct SQTable : public SQDelegable 
{
private:
	struct _HashNode
	{
		_HashNode() { next = NULL; }
		SQObjectPtr val;
		SQObjectPtr key;
		_HashNode *next;
	};
	_HashNode *_firstfree;
	_HashNode *_nodes;
	SQInteger _numofnodes;
	SQInteger _usednodes;
	// Logarithm of minimal power-of-2 number >=_numofnodes
	// stored as log instead of number itself to save mem, assumes
	// target CPU has barrel shiffer, so (1 << pow) is quick.
	unsigned char pow;
	
///////////////////////////
	inline SQInteger TableHash(const SQObjectPtr &key)
	{
		//return HashObj(key) % _numofnodes;
		// To avoid expensive module operator, use
		// binary AND followed by oveflow. This definitely
		// leads to non-uniform index distribution, but
		// price of magic of allowing for non-power-of-2
		// hash sizes. For chaining method in this hash
		// implementation, it's not a big problem.
		SQInteger i = HashObj(key) & ((1 << pow) - 1);
		return i < _numofnodes ? i : i - _numofnodes;
	}
	void AllocNodes(SQInteger nSize);
	void Rehash(bool force);
	inline SQInteger IncreaseSize(SQInteger n)
	{
		// Grow by 50%
		return n + n / 2;
	}
	inline SQInteger DecreaseSize(SQInteger n)
	{
		// Shrink twice by 50%
		return n / 2;
	}
	SQTable(SQSharedState *ss, SQInteger nInitialSize);
	void _ClearNodes();
public:
	static SQTable* Create(SQSharedState *ss,SQInteger nInitialSize)
	{
		SQTable *newtable = (SQTable*)SQ_MALLOC(sizeof(SQTable));
		new (newtable) SQTable(ss, nInitialSize);
		newtable->_delegate = NULL;
		return newtable;
	}
	void Finalize();
	SQTable *Clone();
	~SQTable()
	{
		SetDelegate(NULL);
		REMOVE_FROM_CHAIN(&_sharedstate->_gc_chain, this);
		for (SQInteger i = 0; i < _numofnodes; i++) _nodes[i].~_HashNode();
		SQ_FREE(_nodes, _numofnodes * sizeof(_HashNode));
	}
#ifndef NO_GARBAGE_COLLECTOR 
	void Mark(SQCollectable **chain);
	SQObjectType GetType() {return OT_TABLE;}
#endif
	inline _HashNode *_Get(const SQObjectPtr &key,SQHash hash)
	{
		_HashNode *n = &_nodes[hash];
		do{
			if(_rawval(n->key) == _rawval(key) && type(n->key) == type(key)){
				return n;
			}
		}while((n = n->next));
		return NULL;
	}
	//for compiler use
	inline bool GetStr(const SQChar* key,SQInteger keylen,SQObjectPtr &val)
	{
		SQHash hash = _hashstr(key,keylen);
		_HashNode *n = &_nodes[hash & (_numofnodes - 1)];
		_HashNode *res = NULL;
		do{
			if(type(n->key) == OT_STRING && (scstrcmp(_stringval(n->key),key) == 0)){
				res = n;
				break;
			}
		}while((n = n->next));
		if (res) {
			val = _realval(res->val);
			return true;
		}
		return false;
	}
	bool Get(const SQObjectPtr &key,SQObjectPtr &val);
	void Remove(const SQObjectPtr &key);
	bool Set(const SQObjectPtr &key, const SQObjectPtr &val);
	//returns true if a new slot has been created false if it was already present
	bool NewSlot(const SQObjectPtr &key,const SQObjectPtr &val);
	SQInteger Next(bool getweakrefs,const SQObjectPtr &refpos, SQObjectPtr &outkey, SQObjectPtr &outval);
	
	SQInteger CountUsed(){ return _usednodes;}
	void Clear();
	void Release()
	{
		sq_delete(this, SQTable);
	}
	
};

#endif //_SQTABLE_H_
