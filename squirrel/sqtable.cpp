/*
	see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
#include "sqvm.h"
#include "sqtable.h"
#include "sqfuncproto.h"
#include "sqclosure.h"

SQTable::SQTable(SQSharedState *ss,int nInitialSize)
{
	int pow2size=MINPOWER2;
	while(nInitialSize>pow2size)pow2size=pow2size<<1;
	AllocNodes(pow2size);
	_uiRef=0;
	_delegate=NULL;
	INIT_CHAIN();
	ADD_TO_CHAIN(&_sharedstate->_gc_chain,this);
}

void SQTable::Remove(const SQObjectPtr &key)
{
	unsigned long h = HashKey(key);
	_HashNode *n = &_nodes[h&(_numofnodes-1)];
	_HashNode *first = n;
	_HashNode *prev = NULL;
	do{
		if(type(n->key)==type(key) && _rawval(n->key)==_rawval(key)){
			if(n == first && n->next) {
				_HashNode *z = n->next;
				first->val = _null_;
				first->key = _null_;
				first->next = NULL;
					
				
				//rechain (fu&#s up the foreach)
				while(z) {
					_HashNode *main = &_nodes[HashKey(z->key)&(_numofnodes-1)];
					if(type(main->key) == OT_NULL) {
						main->key = z->key;
						main->val = z->val;
						main->next = NULL;
						z->key = z->val = _null_;
					}
					else {
						_HashNode *x = main;
						while(x->next)
							x = x->next;
						x->next = z;
					}
					_HashNode *t = z;
					z = z->next;
					t->next = NULL;
				}
				break;
			}
			else{
				if(prev)
					prev->next=n->next;
				n->val = n->key = _null_;
			}
			if(n > _firstfree)
				_firstfree=n;
			break;
		}
		prev=n;
		n=n->next;
	}while(n);
	Rehash(false);
}

void SQTable::AllocNodes(int nSize)
{
	_HashNode *nodes=(_HashNode *)SQ_MALLOC(sizeof(_HashNode)*nSize);
	for(int i=0;i<nSize;i++){
		new (&nodes[i]) _HashNode;
		nodes[i].next=NULL;
	}
	_numofnodes=nSize;
	_nodes=nodes;
	_firstfree=&_nodes[_numofnodes-1];
}

int SQTable::CountUsed()
{
	int n=0;
	for(int i=0;i<_numofnodes;i++){
		if(type(_nodes[i].key)!=OT_NULL) n++;
	}
	return n;
}

void SQTable::Rehash(bool force)
{
	int oldsize=_numofnodes;
	//prevent problems with the integer division
	if(oldsize<4)oldsize=4;
	_HashNode *nold=_nodes;
	int nelems=CountUsed();
	if (nelems >= oldsize-oldsize/4)  /* using more than 3/4? */
		AllocNodes(oldsize*2);
	else if (nelems <= oldsize/4 &&  /* less than 1/4? */
          oldsize > MINPOWER2)
		AllocNodes(oldsize/2);
	else if(force)
		AllocNodes(oldsize);
	else
		return;
	for (int i=0; i<oldsize; i++) {
		_HashNode *old = nold+i;
		if (type(old->key) != OT_NULL)
		NewSlot(old->key,old->val);
	}
	for(int k=0;k<oldsize;k++) 
		nold[k].~_HashNode();
	SQ_FREE(nold,oldsize*sizeof(_HashNode));
}

SQTable *SQTable::Clone()
{
	SQTable *nt=Create(_opt_ss(this),_numofnodes);
	SQInteger ridx=0;
	SQObjectPtr key,val;
	while((ridx=Next(ridx,key,val))!=-1){
		nt->NewSlot(key,val);
	}
	nt->SetDelegate(_delegate);
	return nt;
}
