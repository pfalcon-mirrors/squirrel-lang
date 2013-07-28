/*	see copyright notice in squirrel.h */
#ifndef _SQUSERDATA_H_
#define _SQUSERDATA_H_

struct SQUserData : CHAINABLE_OBJ 
{
	SQUserData(SQSharedState *ss){ _uiRef = 0; _delegate = 0; _hook = NULL; INIT_CHAIN(); ADD_TO_CHAIN(&_ss(this)->_gc_chain, this); }
	~SQUserData()
	{
		REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain, this);
		SetDelegate(NULL);
	}
	static SQUserData* Create(SQSharedState *ss, int size)
	{
		SQUserData* ud = (SQUserData*)SQ_MALLOC(sizeof(SQUserData)+(size-1));
		new (ud) SQUserData(ss);
		ud->_size = size - 1;
		return ud;
	}
#ifdef GARBAGE_COLLECTOR
	void Mark(SQCollectable **chain);
#endif
#if defined(CYCLIC_REF_SAFE) || defined(GARBAGE_COLLECTOR)
	void Clear(){SetDelegate(NULL);}
#endif
	void Release(){
		if (_hook) _hook(this->_val);
		int tsize = _size;
		this->~SQUserData();
		SQ_FREE(this, sizeof(SQUserData) + tsize);
	}
	void SetDelegate(SQTable *mt)
	{
		if (mt)	mt->_uiRef++;
		if (_delegate) {
			_delegate->_uiRef--;
			if (_delegate->_uiRef == 0)
				_delegate->Release();
		}
		_delegate = mt;
	}
	SQTable *_delegate;
	int _size;
	SQUSERDATARELEASE _hook;
	SQChar _val[1];
};

#endif //_SQUSERDATA_H_
