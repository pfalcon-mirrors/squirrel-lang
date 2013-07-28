#include "sqpcheader.h"
#include "sqvm.h"
#include "sqstring.h"
#include "sqtable.h"
#include "sqarray.h"
#include "sqfuncproto.h"
#include "sqclosure.h"
#include "squserdata.h"
#include "sqfuncstate.h"
#include "sqcompiler.h"

SQObjectPtr &sq_aux_gettypedarg(HSQUIRRELVM v,int idx,SQObjectType type)
{
	SQObjectPtr &o=stack_get(v,idx);
	if(type(o)!=type)sqraise_str_error(_ss(v),_SC("wrong argument type"));
	return o;
}
void sq_aux_paramscheck(HSQUIRRELVM v,int count)
{
	if(sq_gettop(v)<count)sqraise_str_error(_ss(v),_SC("not enough params in the stack"));
}		

int sq_aux_throwobject(HSQUIRRELVM v,SQException &e)
{
	v->_lasterror=e._description;
	return SQ_ERROR;
}

int sq_aux_invalidtype(HSQUIRRELVM v,SQObjectType type)
{
	scsprintf(_ss(v)->GetScratchPad(100),_SC("unexpected type %s"),GetTypeName(type));
	return sq_throwerror(v,_ss(v)->GetScratchPad(-1));
}

HSQUIRRELVM sq_newvm(HSQUIRRELVM friendvm,int initialstacksize)
{
	SQSharedState *ss;
	SQVM *v;
	if(!friendvm){
		sq_new(ss,SQSharedState);
		ss->Init();
	}else{
		ss=_ss(friendvm);
	}
	v=(SQVM *)SQ_MALLOC(sizeof(SQVM));
	new (v) SQVM(ss);
	
	if(v->Init(initialstacksize)){
		return v;
	}else{
		sq_delete(v,SQVM);
		return NULL;
	}
}

void sq_seterrorhandler(HSQUIRRELVM v)
{
	SQObjectPtr o=stack_get(v,-1);
	if(sq_isclosure(o) || sq_isnativeclosure(o) || sq_isnull(o)){
		v->_errorhandler=o;
		v->Pop();
	}
}

void sq_setdebughook(HSQUIRRELVM v)
{
	SQObjectPtr o=stack_get(v,-1);
	if(sq_isclosure(o) || sq_isnativeclosure(o) || sq_isnull(o)){
		v->_debughook=o;
		v->Pop();
	}
}

void sq_releasevm(HSQUIRRELVM v)
{
	SQSharedState *ss=_ss(v);
	sq_delete(v,SQVM);
	if(!ss->_vms_chain)
		sq_delete(ss,SQSharedState);
}

SQRESULT sq_compile(HSQUIRRELVM v,SQLEXREADFUNC read,SQUserPointer p,const SQChar *sourcename,int raiseerror,int lineinfo)
{
	SQObjectPtr o;
	if(Compile(v,read,p,sourcename,o,raiseerror>0?true:false,lineinfo>0?true:false)){
		o=SQClosure::Create(_ss(v),_funcproto(o));
		v->Push(o);
		return SQ_OK;
	}
	return SQ_ERROR;
}

void sq_addref(HSQUIRRELVM v,HSQOBJECT *po)
{
	SQObjectPtr refs;
	if(type(*po)==OT_NULL)return;
	if(_table(v->_refs_table)->Get(*po,refs)){
		refs=_integer(refs)+1;
	}
	else{
		refs=1;
	}
	_table(v->_refs_table)->NewSlot(*po,refs);
}

void sq_release(HSQUIRRELVM v,HSQOBJECT *po)
{
	SQObjectPtr refs;
	if(type(*po)==OT_NULL)return;
	if(_table(v->_refs_table)->Get(*po,refs)){
		int n=_integer(refs)-1;
		if(n<=0){
			_table(v->_refs_table)->Remove(*po);
			sq_resetobject(po);
		}
		else {
			refs=n;_table(v->_refs_table)->Set(*po,refs);
		}
	}
}

void sq_pushnull(HSQUIRRELVM v)
{
	v->Push(_null_);
}

void sq_pushstring(HSQUIRRELVM v,const SQChar *s,int len)
{
	if(s)
		v->Push(SQObjectPtr(SQString::Create(_ss(v),s,len)));
	else v->Push(_null_);
}

void sq_pushinteger(HSQUIRRELVM v,SQInteger n)
{
	v->Push(SQObjectPtr(n));
}

void sq_pushfloat(HSQUIRRELVM v,SQFloat n)
{
	v->Push(SQObjectPtr(n));
}

void sq_pushuserpointer(HSQUIRRELVM v,SQUserPointer p)
{
	v->Push(SQObjectPtr(p));
}

SQUserPointer sq_newuserdata(HSQUIRRELVM v,unsigned int size)
{
	SQUserData *ud=SQUserData::Create(_ss(v),size);
	v->Push(SQObjectPtr(ud));
	return ud->_val;
}

void sq_newtable(HSQUIRRELVM v)
{
	v->Push(SQObjectPtr(SQTable::Create(_ss(v),0)));	
}

void sq_newarray(HSQUIRRELVM v,int size)
{
	v->Push(SQObjectPtr(SQArray::Create(_ss(v),size)));	
}


SQRESULT sq_arrayappend(HSQUIRRELVM v,int idx)
{
	try{
		sq_aux_paramscheck(v,2);
		SQObjectPtr &arr=sq_aux_gettypedarg(v,idx,OT_ARRAY);
		_array(arr)->Append(v->GetUp(-1));
		v->Pop(1);
	}
	catch(SQException &e){return sq_aux_throwobject(v,e);}
	return SQ_OK;
}

SQRESULT sq_arraypop(HSQUIRRELVM v,int idx,int pushval)
{
	try{
		sq_aux_paramscheck(v,1);
		SQObjectPtr &arr=sq_aux_gettypedarg(v,idx,OT_ARRAY);
		if(_array(arr)->Size()>0){
            if(pushval!=0){v->Push(_array(arr)->Top());_array(arr)->Pop();return 1;}
			return 0;
		}
		return sq_throwerror(v,_SC("empty array"));
	}
	catch(SQException &e){return sq_aux_throwobject(v,e);}
}

SQRESULT sq_arrayresize(HSQUIRRELVM v,int idx,int newsize)
{
	try{
		sq_aux_paramscheck(v,2);
		SQObjectPtr &arr=sq_aux_gettypedarg(v,idx,OT_ARRAY);
		if(_array(arr)->Size()>0){
			_array(arr)->Resize(newsize);
			return SQ_OK;
		}
		return sq_throwerror(v,_SC("empty array"));
	}
	catch(SQException &e){return sq_aux_throwobject(v,e);}
}

SQRESULT sq_arrayreverse(HSQUIRRELVM v,int idx)
{
	try{
		sq_aux_paramscheck(v,1);
		SQArray *arr=_array(sq_aux_gettypedarg(v,idx,OT_ARRAY));
		if(arr->Size()>0){
			SQObjectPtr t;
			int size=arr->Size();
			int n=size>>1; size-=1;
			for(int i=0;i<n;i++){
				t=arr->_values[i];
				arr->_values[i]=arr->_values[size-i];
				arr->_values[size-i]=t;
			}
			return SQ_OK;
		}
		return sq_throwerror(v,_SC("empty array"));
	}
	catch(SQException &e){return sq_aux_throwobject(v,e);}

}

void sq_newclosure(HSQUIRRELVM v,SQFUNCTION func,unsigned int nfreevars)
{
	SQNativeClosure *nc=SQNativeClosure::Create(_ss(v),func);
	for(unsigned int i=0;i<nfreevars;i++){
		nc->_outervalues.push_back(v->Top());
		v->Pop();
	}
	v->Push(SQObjectPtr(nc));	
}

void sq_pushroottable(HSQUIRRELVM v)
{
	v->Push(v->_roottable);
}

int sq_setroottable(HSQUIRRELVM v)
{
	SQObjectPtr o=stack_get(v,-1);
	if(sq_istable(o) || sq_isnull(o)){
		v->_roottable=o;
		v->Pop();
		return 0;
	}
	return sq_throwerror(v,_SC("ivalid type"));
}

void sq_setforeignptr(HSQUIRRELVM v,SQUserPointer p)
{
	v->_foreignptr=p;
}

SQUserPointer sq_getforeignptr(HSQUIRRELVM v)
{
	return v->_foreignptr;
}

void sq_push(HSQUIRRELVM v,int idx)
{
	v->Push(stack_get(v,idx));
}

SQObjectType sq_gettype(HSQUIRRELVM v,int idx)
{
	return type(stack_get(v,idx));
}

SQRESULT sq_getinteger(HSQUIRRELVM v,int idx,SQInteger *i)
{
	SQObjectPtr &o=stack_get(v,idx);
	if(sq_isnumeric(o)){
		*i=tointeger(o);
		return SQ_OK;
	}
	return SQ_ERROR;
}

SQRESULT sq_getfloat(HSQUIRRELVM v,int idx,SQFloat *f)
{
	SQObjectPtr &o=stack_get(v,idx);
	if(sq_isnumeric(o)){
		*f=tofloat(o);
		return SQ_OK;
	}
	return SQ_ERROR;
}

SQRESULT sq_getstring(HSQUIRRELVM v,int idx,const SQChar **c)
{
	try{
		SQObjectPtr &o=sq_aux_gettypedarg(v,idx,OT_STRING);
		*c=_stringval(o);
		return SQ_OK;
	}
	catch(SQException &e){ sq_aux_throwobject(v,e); return SQ_ERROR;}
}

SQRESULT sq_clone(HSQUIRRELVM v,int idx)
{
	SQObjectPtr &o=stack_get(v,idx);
	v->Push(_null_);
	if(!v->Clone(o,stack_get(v,-1))){
		v->Pop();
		return sq_aux_invalidtype(v,type(o));
	}
	return 1;
}

SQInteger sq_getsize(HSQUIRRELVM v,int idx)
{
	SQObjectPtr &o=stack_get(v,idx);
	SQObjectType type=type(o);
	switch(type)
	{
	case OT_STRING:return _string(o)->_len;
	case OT_TABLE:return _table(o)->CountUsed();
	case OT_ARRAY:return _array(o)->Size();
	default:
		return sq_aux_invalidtype(v,type);
	}
}

SQRESULT sq_getuserdata(HSQUIRRELVM v,int idx,SQUserPointer *p)
{
	try{
		SQObjectPtr &o=sq_aux_gettypedarg(v,idx,OT_USERDATA);
		(*p)=_userdataval(o);
	}catch(SQException &e){sq_aux_throwobject(v,e); return SQ_ERROR;}
	return SQ_OK;
}

SQRESULT sq_getuserpointer(HSQUIRRELVM v,int idx,SQUserPointer *p)
{
	try{
		SQObjectPtr &o=sq_aux_gettypedarg(v,idx,OT_USERPOINTER);
		(*p)=_userpointer(o);
	}catch(SQException &e){sq_aux_throwobject(v,e); return SQ_ERROR;}
	return SQ_OK;
}

int sq_gettop(HSQUIRRELVM v)
{
	return (v->_top)-v->_stackbase;
}

void sq_settop(HSQUIRRELVM v,int newtop)
{
	int top=sq_gettop(v);
	if(top>newtop)
		sq_pop(v,top-newtop);
	else while(top<newtop)sq_pushnull(v);
}

void sq_pop(HSQUIRRELVM v,int nelemstopop)
{
	v->Pop(nelemstopop);
}

void sq_remove(HSQUIRRELVM v,int idx)
{
	v->Remove(idx);
}

int sq_cmp(HSQUIRRELVM v)
{
	return v->ObjCmp(stack_get(v,-1),stack_get(v,-2));
}

SQRESULT sq_createslot(HSQUIRRELVM v,int idx)
{
	try{
		sq_aux_paramscheck(v,3);
		SQObjectPtr &self=sq_aux_gettypedarg(v,idx,OT_TABLE);
		SQObjectPtr &key=v->GetUp(-2);
		if(type(key)==OT_NULL)return 0;
		v->NewSlot(self,key,v->GetUp(-1));
		v->Pop(2);
		return SQ_OK;
	}
	catch(SQException &e){return sq_aux_throwobject(v,e);}
}

SQRESULT sq_set(HSQUIRRELVM v,int idx)
{
	SQObjectPtr &self=stack_get(v,idx);
	try{
		if(v->Set(self,v->GetUp(-2),v->GetUp(-1))){
			v->Pop(2);
			return SQ_OK;
		}
		v->IdxError(v->GetUp(-2));return SQ_ERROR;
	}
	catch(SQException &e){return sq_aux_throwobject(v,e);}
	
}

SQRESULT sq_rawset(HSQUIRRELVM v,int idx)
{
	try{
		SQObjectPtr &self=sq_aux_gettypedarg(v,idx,OT_TABLE);
		if(type(v->GetUp(-1))==OT_NULL) return sq_throwerror(v,_SC("null key"));
			_table(self)->NewSlot(v->GetUp(-2),v->GetUp(-1));
		v->Pop(2);
		return SQ_OK;
		v->IdxError(v->GetUp(-2));return SQ_ERROR;
	}
	catch(SQException &e){return sq_aux_throwobject(v,e);}
}

SQRESULT sq_setdelegate(HSQUIRRELVM v,int idx)
{
	SQObjectPtr &self=stack_get(v,idx);
	SQObjectPtr &mt=v->GetUp(-1);
	SQObjectType type=type(self);
	switch(type){
	case OT_TABLE:
		if(type(mt)==OT_TABLE){
			_table(self)->SetDelegate(_table(mt));v->Pop();}
		else if(type(mt)==OT_NULL){
			_table(self)->SetDelegate(NULL);v->Pop();}
		else return sq_aux_invalidtype(v,type);
		break;
	case OT_USERDATA:
		if(type(mt)==OT_TABLE){
			_userdata(self)->SetDelegate(_table(mt));v->Pop();}
		else if(type(mt)==OT_NULL){
			_userdata(self)->SetDelegate(NULL);v->Pop();}
		else return sq_aux_invalidtype(v,type);
		break;
	default:
			return sq_aux_invalidtype(v,type);
		break;
	}
	return SQ_OK;
}

SQRESULT sq_getdelegate(HSQUIRRELVM v,int idx)
{
	SQObjectPtr &self=stack_get(v,idx);
	switch(type(self)){
	case OT_TABLE:
		if(!_table(self)->_delegate)break;
		v->Push(SQObjectPtr(_table(self)->_delegate));
		return 1;
		break;
	case OT_USERDATA:
		if(!_userdata(self)->_delegate)break;
		v->Push(SQObjectPtr(_table(self)->_delegate));
		return 1;
		break;
	}
	return sq_throwerror(v,_SC("wrong type"));
}

SQRESULT sq_get(HSQUIRRELVM v,int idx)
{
	SQObjectPtr ret;
	SQObjectPtr &self=stack_get(v,idx);
	if(v->Get(self,v->GetUp(-1),v->GetUp(-1),false))
		return 1;
	v->Pop(1);
	return sq_throwerror(v,_SC("the index doesn't exist"));
}

SQRESULT sq_rawget(HSQUIRRELVM v,int idx)
{
	SQObjectPtr ret;
	SQObjectPtr &self=sq_aux_gettypedarg(v,idx,OT_TABLE);
	if(_table(self)->Get(v->GetUp(-1),v->GetUp(-1)))
		return 1;
	v->Pop(1);
	return sq_throwerror(v,_SC("the index doesn't exist"));
}

SQRESULT sq_getstackobj(HSQUIRRELVM v,int idx,HSQOBJECT *po)
{
	*po=stack_get(v,idx);
	return SQ_OK;
}

const SQChar *sq_getlocal(HSQUIRRELVM v,unsigned int level,unsigned int idx)
{
	unsigned int cstksize=v->_callsstack.size();
	unsigned int lvl=(cstksize-level)-1;
	int stackbase=v->_stackbase;
	if(lvl<cstksize){
		for(unsigned int i=0;i<level;i++){
			SQVM::CallInfo &ci=v->_callsstack[(cstksize-i)-1];
			stackbase-=ci._prevstkbase;
		}
		SQVM::CallInfo &ci=v->_callsstack[lvl];
		if(type(ci._closure)!=OT_CLOSURE)
			return NULL;
		SQClosure *c=_closure(ci._closure);
		SQFunctionProto *func=_funcproto(c->_function);
		return func->GetLocal(v,stackbase,idx,(ci._ip-func->_instructions._vals)-1);
	}
	return NULL;
}

void sq_pushobject(HSQUIRRELVM v,HSQOBJECT obj)
{
	v->Push(SQObjectPtr(obj));
}

void sq_resetobject(HSQOBJECT *po)
{
	po->_type=OT_NULL;
	po->_unVal.pUserPointer=NULL;
}

int sq_throwerror(HSQUIRRELVM v,const SQChar *err)
{
	v->ThrowError(err);
	return -1;
}

void sq_getlasterror(HSQUIRRELVM v)
{
	v->Push(v->_lasterror);
}

SQRESULT sq_resume(HSQUIRRELVM v,int retval)
{
	try{
		if(type(v->GetUp(-1))==OT_GENERATOR){
			v->Push(_null_); //retval
			v->GetUp(-1)=v->Execute(v->GetUp(-2),v->_top,0,v->_top,true);
			if(!retval)
				v->Pop();
			return SQ_OK;
		}
		return sq_throwerror(v,_SC("only generators can be resumed"));
	}
	catch(SQException &e){return sq_aux_throwobject(v,e);}
}

SQRESULT sq_call(HSQUIRRELVM v,int params,int retval)
{
	SQObjectPtr res;
	try{
		if(v->Call(v->GetUp(-(params+1)),params,v->_top-params,res)){
			v->Pop(params+1);//pop closure and args
			if(retval){
				v->Push(res);return 1;
			}
			return SQ_OK;
		}
		return sq_throwerror(v,_SC("call failed"));
	}
	catch(SQException &e){return sq_aux_throwobject(v,e);}
}

void sq_setreleasehook(HSQUIRRELVM v,int idx,SQUSERDATARELEASE hook)
{
	if(sq_gettop(v)>=2){
		SQObjectPtr &ud=stack_get(v,idx);
		if(type(ud)!=OT_USERDATA)return;
		_userdata(ud)->_hook=hook;
	}
}

void sq_setcompilererrorhandler(HSQUIRRELVM v,SQCOMPILERERROR f)
{
	v->_compilererrorhandler=f;
}

SQRESULT sq_writeclosure(HSQUIRRELVM v,SQWRITEFUNC w,SQUserPointer up)
{
	try{
		SQObjectPtr o=sq_aux_gettypedarg(v,-1,OT_CLOSURE);
		SQClosure *c=_closure(o);
		_closure(o)->Save(v,up,w);
	}
	catch(SQException &e){return sq_aux_throwobject(v,e);}
	return SQ_OK;
}

SQRESULT sq_readclosure(HSQUIRRELVM v,SQREADFUNC r,SQUserPointer up)
{
	SQObjectPtr func=SQFunctionProto::Create();
	_funcproto(func)->Load(v,up,r);
	SQObjectPtr closure=SQClosure::Create(_ss(v),_funcproto(func));
	v->Push(closure);
	return 1;
}

SQChar *sq_getscratchpad(HSQUIRRELVM v,int minsize)
{
	return _ss(v)->GetScratchPad(minsize);
}

SQRESULT sq_setfreevariable(HSQUIRRELVM v,int idx,unsigned int nval)
{
	SQObjectPtr &self=stack_get(v,idx);
	switch(type(self))
	{
	case OT_CLOSURE:
		if(_closure(self)->_outervalues.size()<nval){
			_closure(self)->_outervalues[nval]=stack_get(v,-1);
		}
		else return sq_throwerror(v,_SC("invalid free var index"));
	case OT_NATIVECLOSURE:
		if(_nativeclosure(self)->_outervalues.size()<nval){
			_nativeclosure(self)->_outervalues[nval]=stack_get(v,-1);
		}
		else return sq_throwerror(v,_SC("invalid free var index"));
	default:
		return sq_aux_invalidtype(v,type(self));
	}
	v->Pop(1);
	return SQ_OK;
}

SQRESULT sq_next(HSQUIRRELVM v,int idx)
{
	SQObjectPtr o=stack_get(v,idx),key=stack_get(v,-1),realkey,val;
	int nrefidx;
	switch(type(stack_get(v,idx))){
		case OT_TABLE:
			if((nrefidx=_table(o)->Next(key,realkey,val))==-1)return SQ_ERROR;
			break;
		case OT_ARRAY:
			if((nrefidx=_array(o)->Next(key,realkey,val))==-1)return SQ_ERROR;
			break;
		case OT_STRING:
			if((nrefidx=_string(o)->Next(key,realkey,val))==-1)return SQ_ERROR;
			break;
		default:
			return sq_aux_invalidtype(v,type(o));
	}
	stack_get(v,-1)=(SQInteger)nrefidx;
	v->Push(realkey);v->Push(val);
	return SQ_OK;
}

void *sq_malloc(unsigned int size)
{
	return SQ_MALLOC(size);
}

void *sq_realloc(void* p,unsigned int oldsize,unsigned int newsize)
{
	return SQ_REALLOC(p,oldsize,newsize);
}
void sq_free(void *p,unsigned int size)
{
	SQ_FREE(p,size);
}
