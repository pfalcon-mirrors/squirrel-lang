/*
	see copyright notice in squirrel.h
*/
#include "stdafx.h"
#include "sqtable.h"
#include "sqvm.h"
#include "sqfuncproto.h"
#include "sqclosure.h"

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

void SQTable::Rehash()
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
	else
		AllocNodes(oldsize);
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
	SQTable *nt=Create(_numofnodes);
	SQInteger ridx=0;
	SQObjectPtr key,val;
	while((ridx=Next(ridx,key,val))!=-1){
		nt->NewSlot(key,val);
	}
	nt->SetDelegate(_delegate);
	return nt;
}
