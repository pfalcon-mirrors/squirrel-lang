/*	see copyright notice in squirrel.h */
#ifndef _SQARRAY_H_
#define _SQARRAY_H_

struct SQArray : public CHAINABLE_OBJ
{
private:
	SQArray(int nsize){_values.resize(nsize);_uiRef=0;}
	~SQArray()
	{
		REMOVE_FROM_CHAIN(&GS->_gc_chain,this);
	}
public:
	static SQArray* Create(int nInitialSize)
	{
		SQArray *newarray=(SQArray*)SQ_MALLOC(sizeof(SQArray));
		new (newarray) SQArray(nInitialSize);
		ADD_TO_CHAIN(&GS->_gc_chain,newarray);
		return newarray;
	}
#ifdef GARBAGE_COLLECTOR
	void Mark(SQCollectable **chain);
#endif
	void Clear(){_values.resize(0);}
	bool Get(const int nidx,SQObjectPtr &val)
	{
		if(nidx>=0 && nidx<(int)_values.size()){
			val=_values[nidx];
			return true;
		}
		else return false;
	}
	bool Set(const int nidx,const SQObjectPtr &val)
	{
		if(nidx>=0 && nidx<(int)_values.size()){
			_values[nidx]=val;
			return true;
		}
		else return false;
	}
	int Next(const SQObjectPtr &refpos,SQObjectPtr &outkey,SQObjectPtr &outval)
	{
		//first iteration
		unsigned int idx;
		switch(type(refpos)){
		case OT_NULL:
			idx=0;
			break;
		case OT_INTEGER:
			idx=(unsigned int)_integer(refpos);
			break;
		default:
			sqraiseerror("critical vm error iterating a table with a non number idx");
			break;
		}
		
		while(idx<_values.size()){
			//first found
			outkey=(SQInteger)idx;
			outval=_values[idx];
			//return idx for the next iteration
			return ++idx;
		}
		//nothing to iterate anymore
		return -1;
	}
	SQArray *Clone(){SQArray *anew=Create(Size()); anew->_values.copy(_values); return anew; }
	int Size(){return _values.size();}
	void Resize(int size) { _values.resize(size); }
	void Reserve(int size) { _values.reserve(size); }
	void Append(const SQObject &o){_values.push_back(o);}
	SQObjectPtr &Top(){return _values.top();}
	void Pop(){_values.pop_back();}
	inline void Insert(const SQObject& idx,const SQObject &val){_values.insert((unsigned int)tointeger(idx),val);}
	inline void Remove(const SQObject& idx){_values.remove((unsigned int)_integer(idx));}
	void Release()
	{
		sq_delete(this,SQArray);
	}
	SQObjectPtrVec _values;
};
#endif //_SQARRAY_H_
