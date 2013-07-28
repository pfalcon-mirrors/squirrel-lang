/*	see copyright notice in squirrel.h */
#ifndef _SQSTRING_H_
#define _SQSTRING_H_

inline unsigned int _hashstr (const SQChar *s, size_t l)
{
		unsigned int h = l;  /* seed */
		size_t step = (l>>5)|1;  /* if string is too long, don't hash all its chars */
		for (; l>=step; l-=step)
		h = h ^ ((h<<5)+(h>>2)+(unsigned char)*(s++));
		return h;
}

struct SQString : public SQRefCounted
{
	SQString(){}
	~SQString(){}
public:
	
	static SQString *Create(SQSharedState *ss,const SQChar *,int len=-1 );
	int Next(const SQObjectPtr &refpos,SQObjectPtr &outkey,SQObjectPtr &outval)
	{
		//first iteration
		int idx;
		switch(type(refpos))
		{
		case OT_NULL:
			idx=0;
			break;
		case OT_INTEGER:
			idx=(unsigned int)_integer(refpos);
			break;
		default:
			sqraise_str_error(_sharedstate,"critical vm error iterating a string with a non number idx");
			break;
		}
		
		while(idx<_len)
		{
			//first found
			outkey=(SQInteger)idx;
			outval=SQInteger(_val[idx]);
			//return idx for the next iteration
			return ++idx;
		}
		//nothing to iterate anymore
		return -1;
	}
	void Release();
	SQSharedState *_sharedstate;
	SQString *_next; //chain for the string table
	int _len;
	int _hash;
	SQChar _val[1];
};



#endif //_SQSTRING_H_
