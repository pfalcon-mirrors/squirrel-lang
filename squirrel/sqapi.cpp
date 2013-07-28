/*
	see copyright notice in squirrel.h
*/
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
	SQObjectPtr &o = stack_get(v,idx);
	if(type(o) != type){
		SQObjectPtr oval = v->PrintObjVal(o);
		v->RT_Error(_SC("wrong argument type, expected '%s' got '%.50s'"),GetTypeName(type),_stringval(oval));
	}
	return o;
}
void sq_aux_paramscheck(HSQUIRRELVM v,int count)
{
	if(sq_gettop(v) < count) v->RT_Error(_SC("not enough params in the stack"));
}		

int sq_aux_throwobject(HSQUIRRELVM v,SQException &e)
{
	v->_lasterror = e._description;
	return SQ_ERROR;
}

int sq_aux_invalidtype(HSQUIRRELVM v,SQObjectType type)
{
	scsprintf(_ss(v)->GetScratchPad(100), _SC("unexpected type %s"), GetTypeName(type));
	return sq_throwerror(v, _ss(v)->GetScratchPad(-1));
}

HSQUIRRELVM sq_open(int initialstacksize)
{
	SQSharedState *ss;
	SQVM *v;
	sq_new(ss, SQSharedState);
	ss->Init();
	v = (SQVM *)SQ_MALLOC(sizeof(SQVM));
	new (v) SQVM(ss);
	ss->_root_vm = v;
	if(v->Init(NULL, initialstacksize)) {
		return v;
	} else {
		sq_delete(v, SQVM);
		return NULL;
	}
	return v;
}

HSQUIRRELVM sq_newthread(HSQUIRRELVM friendvm, int initialstacksize)
{
	SQSharedState *ss;
	SQVM *v;
	ss=_ss(friendvm);
	
	v= (SQVM *)SQ_MALLOC(sizeof(SQVM));
	new (v) SQVM(ss);
	
	if(v->Init(friendvm, initialstacksize)) {
		friendvm->Push(v);
		return v;
	} else {
		sq_delete(v, SQVM);
		return NULL;
	}
}

int sq_getvmstate(HSQUIRRELVM v)
{
	if(v->_suspended)
		return SQ_VMSTATE_SUSPENDED;
	else { 
		if(v->_callsstack.size() != 0) return SQ_VMSTATE_RUNNING;
		else return SQ_VMSTATE_IDLE;
	}
}

void sq_seterrorhandler(HSQUIRRELVM v)
{
	SQObject o = stack_get(v, -1);
	if(sq_isclosure(o) || sq_isnativeclosure(o) || sq_isnull(o)) {
		v->_errorhandler = o;
		v->Pop();
	}
}

void sq_setdebughook(HSQUIRRELVM v)
{
	SQObject o = stack_get(v,-1);
	if(sq_isclosure(o) || sq_isnativeclosure(o) || sq_isnull(o)) {
		v->_debughook = o;
		v->Pop();
	}
}

void sq_close(HSQUIRRELVM v)
{
	SQSharedState *ss = _ss(v);
	_thread(ss->_root_vm)->Finalize();
	sq_delete(ss, SQSharedState);
}

SQRESULT sq_compile(HSQUIRRELVM v,SQLEXREADFUNC read,SQUserPointer p,const SQChar *sourcename,int raiseerror)
{
#ifndef SQ_VM_ONLY
	SQObjectPtr o;
	if(Compile(v, read, p, sourcename, o, raiseerror>0?true:false, _ss(v)->_debuginfo)) {
		v->Push(SQClosure::Create(_ss(v), _funcproto(o)));
		return SQ_OK;
	}
	return SQ_ERROR;
#else
	return sq_throwerror(v,_SC("squirrel has been compiled with SQ_VM_ONLY"));
#endif
}

void sq_enabledebuginfo(HSQUIRRELVM v, int debuginfo)
{
	_ss(v)->_debuginfo = debuginfo!=0?true:false;
}

void sq_addref(HSQUIRRELVM v,HSQOBJECT *po)
{
	SQObjectPtr refs;
	if(!ISREFCOUNTED(type(*po))) return;
	if(_table(_ss(v)->_refs_table)->Get(*po, refs)) {
		refs = _integer(refs) + 1;
	}
	else{
		refs = 1;
	}
	_table(_ss(v)->_refs_table)->NewSlot(*po, refs);
}

void sq_release(HSQUIRRELVM v,HSQOBJECT *po)
{
	SQObjectPtr refs;
	if(!ISREFCOUNTED(type(*po))) return;
	if(_table(_ss(v)->_refs_table)->Get(*po, refs)) {
		int n = _integer(refs) - 1;
		if(n <= 0) {
			_table(_ss(v)->_refs_table)->Remove(*po);
			sq_resetobject(po);
		}
		else {
			refs = n;_table(_ss(v)->_refs_table)->Set(*po, refs);
		}
	}
}

const SQChar *sq_objtostring(HSQOBJECT *o) 
{
	if(sq_type(*o) == OT_STRING) {
		return o->_unVal.pString->_val;
	}
	return NULL;
}

SQInteger sq_objtointeger(HSQOBJECT *o) 
{
	if(sq_isnumeric(*o)) {
		return tointeger(*o);
	}
	return 0;
}

SQFloat sq_objtofloat(HSQOBJECT *o) 
{
	if(sq_isnumeric(*o)) {
		return tofloat(*o);
	}
	return 0;
}

void sq_pushnull(HSQUIRRELVM v)
{
	v->Push(_null_);
}

void sq_pushstring(HSQUIRRELVM v,const SQChar *s,int len)
{
	if(s)
		v->Push(SQObjectPtr(SQString::Create(_ss(v), s, len)));
	else v->Push(_null_);
}

void sq_pushinteger(HSQUIRRELVM v,SQInteger n)
{
	v->Push(n);
}

void sq_pushfloat(HSQUIRRELVM v,SQFloat n)
{
	v->Push(n);
}

void sq_pushuserpointer(HSQUIRRELVM v,SQUserPointer p)
{
	v->Push(p);
}

SQUserPointer sq_newuserdata(HSQUIRRELVM v,unsigned int size)
{
	SQUserData *ud = SQUserData::Create(_ss(v), size);
	v->Push(ud);
	return ud->_val;
}

void sq_newtable(HSQUIRRELVM v)
{
	v->Push(SQTable::Create(_ss(v), 0));	
}

void sq_newarray(HSQUIRRELVM v,int size)
{
	v->Push(SQArray::Create(_ss(v), size));	
}


SQRESULT sq_arrayappend(HSQUIRRELVM v,int idx)
{
	SQ_TRY {
		sq_aux_paramscheck(v,2);
		SQObjectPtr &arr = sq_aux_gettypedarg(v, idx, OT_ARRAY);
		_array(arr)->Append(v->GetUp(-1));
		v->Pop(1);
	}
	SQ_CATCH(SQException, e) { return sq_aux_throwobject(v,e); }
	return SQ_OK;
}

SQRESULT sq_arraypop(HSQUIRRELVM v,int idx,int pushval)
{
	SQ_TRY {
		sq_aux_paramscheck(v, 1);
		SQObjectPtr &arr = sq_aux_gettypedarg(v, idx, OT_ARRAY);
		if(_array(arr)->Size() > 0) {
            if(pushval != 0){ v->Push(_array(arr)->Top()); }
			_array(arr)->Pop();
			return SQ_OK;
		}
		return sq_throwerror(v, _SC("empty array"));
	}
	SQ_CATCH(SQException, e){ return sq_aux_throwobject(v, e); }
}

SQRESULT sq_arrayresize(HSQUIRRELVM v,int idx,int newsize)
{
	SQ_TRY {
		sq_aux_paramscheck(v,1);
		SQObjectPtr &arr = sq_aux_gettypedarg(v, idx, OT_ARRAY);
		if(_array(arr)->Size() > 0) {
			_array(arr)->Resize(newsize);
			return SQ_OK;
		}
		return sq_throwerror(v, _SC("empty array"));
	}
	SQ_CATCH(SQException, e){ return sq_aux_throwobject(v, e); }
}

SQRESULT sq_arrayreverse(HSQUIRRELVM v,int idx)
{
	SQ_TRY {
		sq_aux_paramscheck(v, 1);
		SQArray *arr = _array(sq_aux_gettypedarg(v, idx, OT_ARRAY));
		if(arr->Size() > 0) {
			SQObjectPtr t;
			int size = arr->Size();
			int n = size >> 1; size -= 1;
			for(int i = 0; i < n; i++) {
				t = arr->_values[i];
				arr->_values[i] = arr->_values[size-i];
				arr->_values[size-i] = t;
			}
			return SQ_OK;
		}
		return sq_throwerror(v, _SC("empty array"));
	}
	SQ_CATCH(SQException, e) { return sq_aux_throwobject(v,e); }

}

void sq_newclosure(HSQUIRRELVM v,SQFUNCTION func,unsigned int nfreevars)
{
	SQNativeClosure *nc = SQNativeClosure::Create(_ss(v), func);
	nc->_nparamscheck = 0;
	for(unsigned int i = 0; i < nfreevars; i++) {
		nc->_outervalues.push_back(v->Top());
		v->Pop();
	}
	v->Push(SQObjectPtr(nc));	
}

SQRESULT sq_getclosureinfo(HSQUIRRELVM v,int idx,unsigned int *nparams,unsigned int *nfreevars)
{
	SQObject o = stack_get(v, idx);
	if(sq_isclosure(o)) {
		SQClosure *c = _closure(o);
		SQFunctionProto *proto = _funcproto(c->_function);
		*nparams = (unsigned int)proto->_parameters.size();
        *nfreevars = (unsigned int)c->_outervalues.size();
		return SQ_OK;
	}
	return sq_throwerror(v,_SC("the object is not a closure"));
}

SQRESULT sq_setnativeclosurename(HSQUIRRELVM v,int idx,const SQChar *name)
{
	SQObject o = stack_get(v, idx);
	if(sq_isnativeclosure(o)) {
		SQNativeClosure *nc = _nativeclosure(o);
		nc->_name = SQString::Create(_ss(v),name);
		return SQ_OK;
	}
	return sq_throwerror(v,_SC("the object is not a nativeclosure"));
}

SQRESULT sq_setparamscheck(HSQUIRRELVM v,int nparamscheck,const SQChar *typemask)
{
	SQObject o = stack_get(v, -1);
	if(!sq_isnativeclosure(o))
		return sq_throwerror(v, _SC("native closure expected"));
	SQNativeClosure *nc = _nativeclosure(o);
	nc->_nparamscheck = nparamscheck;
	if(typemask) {
		SQIntVec res;
		if(!CompileTypemask(res, typemask))
			return sq_throwerror(v, _SC("invalid typemask"));
		nc->_typecheck.copy(res);
	}
	else {
		nc->_typecheck.resize(0);
	}
	return SQ_OK;
}

void sq_pushroottable(HSQUIRRELVM v)
{
	v->Push(v->_roottable);
}

void sq_pushregistrytable(HSQUIRRELVM v)
{
	v->Push(_ss(v)->_registry);
}

SQRESULT sq_setroottable(HSQUIRRELVM v)
{
	SQObject o = stack_get(v, -1);
	if(sq_istable(o) || sq_isnull(o)) {
		v->_roottable = o;
		v->Pop();
		return SQ_OK;
	}
	return sq_throwerror(v, _SC("ivalid type"));
}

void sq_setforeignptr(HSQUIRRELVM v,SQUserPointer p)
{
	v->_foreignptr = p;
}

SQUserPointer sq_getforeignptr(HSQUIRRELVM v)
{
	return v->_foreignptr;
}

void sq_push(HSQUIRRELVM v,int idx)
{
	v->Push(stack_get(v, idx));
}

SQObjectType sq_gettype(HSQUIRRELVM v,int idx)
{
	return type(stack_get(v, idx));
}

SQRESULT sq_getinteger(HSQUIRRELVM v,int idx,SQInteger *i)
{
	SQObjectPtr &o = stack_get(v, idx);
	if(sq_isnumeric(o)) {
		*i = tointeger(o);
		return SQ_OK;
	}
	return SQ_ERROR;
}

SQRESULT sq_getfloat(HSQUIRRELVM v,int idx,SQFloat *f)
{
	SQObjectPtr &o = stack_get(v, idx);
	if(sq_isnumeric(o)) {
		*f = tofloat(o);
		return SQ_OK;
	}
	return SQ_ERROR;
}

SQRESULT sq_getstring(HSQUIRRELVM v,int idx,const SQChar **c)
{
	SQ_TRY {
		SQObjectPtr &o = sq_aux_gettypedarg(v, idx, OT_STRING);
		*c = _stringval(o);
		return SQ_OK;
	}
	SQ_CATCH(SQException,e){ sq_aux_throwobject(v,e); return SQ_ERROR;}
}

SQRESULT sq_getthread(HSQUIRRELVM v,int idx,HSQUIRRELVM *thread)
{
	SQ_TRY {
		SQObjectPtr &o = sq_aux_gettypedarg(v, idx, OT_THREAD);
		*thread = _thread(o);
		return SQ_OK;
	}
	SQ_CATCH(SQException,e){ sq_aux_throwobject(v,e); return SQ_ERROR;}
}

SQRESULT sq_clone(HSQUIRRELVM v,int idx)
{
	SQObjectPtr &o = stack_get(v,idx);
	v->Push(_null_);
	if(!v->Clone(o, stack_get(v, -1))){
		v->Pop();
		return sq_aux_invalidtype(v, type(o));
	}
	return SQ_OK;
}

SQInteger sq_getsize(HSQUIRRELVM v, int idx)
{
	SQObjectPtr &o = stack_get(v, idx);
	SQObjectType type = type(o);
	switch(type) {
	case OT_STRING:		return _string(o)->_len;
	case OT_TABLE:		return _table(o)->CountUsed();
	case OT_ARRAY:		return _array(o)->Size();
	case OT_USERDATA:	return _userdata(o)->_size;
	default:
		return sq_aux_invalidtype(v, type);
	}
}

SQRESULT sq_getuserdata(HSQUIRRELVM v,int idx,SQUserPointer *p,unsigned int *typetag)
{
	SQ_TRY {
		SQObjectPtr &o = sq_aux_gettypedarg(v, idx, OT_USERDATA);
		(*p) = _userdataval(o);
		if(typetag) *typetag = _userdata(o)->_typetag;
	} SQ_CATCH(SQException,e) { sq_aux_throwobject(v, e); return SQ_ERROR; }
	return SQ_OK;
}

SQRESULT sq_settypetag(HSQUIRRELVM v,int idx,unsigned int typetag)
{
	SQ_TRY {
		SQObjectPtr &o = sq_aux_gettypedarg(v, idx, OT_USERDATA);
		_userdata(o)->_typetag = typetag;
	} SQ_CATCH(SQException,e) { sq_aux_throwobject(v, e); return SQ_ERROR; }
	return SQ_OK;
}

SQRESULT sq_getuserpointer(HSQUIRRELVM v, int idx, SQUserPointer *p)
{
	SQ_TRY {
		SQObjectPtr &o = sq_aux_gettypedarg(v, idx, OT_USERPOINTER);
		(*p) = _userpointer(o);
	} SQ_CATCH(SQException,e) { sq_aux_throwobject(v, e); return SQ_ERROR; }
	return SQ_OK;
}

int sq_gettop(HSQUIRRELVM v)
{
	return (v->_top) - v->_stackbase;
}

void sq_settop(HSQUIRRELVM v, int newtop)
{
	int top = sq_gettop(v);
	if(top > newtop)
		sq_pop(v, top - newtop);
	else
		while(top < newtop) sq_pushnull(v);
}

void sq_pop(HSQUIRRELVM v, int nelemstopop)
{
	assert(v->_top >= nelemstopop);
	v->Pop(nelemstopop);
}

void sq_remove(HSQUIRRELVM v, int idx)
{
	v->Remove(idx);
}

int sq_cmp(HSQUIRRELVM v)
{
	return v->ObjCmp(stack_get(v, -1), stack_get(v, -2));
}

SQRESULT sq_createslot(HSQUIRRELVM v, int idx)
{
	SQ_TRY {
		sq_aux_paramscheck(v, 3);
		SQObjectPtr &self = sq_aux_gettypedarg(v, idx, OT_TABLE);
		SQObjectPtr &key = v->GetUp(-2);
		if(type(key) == OT_NULL) return sq_throwerror(v, _SC("null is not a valid key"));
		v->NewSlot(self, key, v->GetUp(-1));
		v->Pop(2);
		return SQ_OK;
	} SQ_CATCH(SQException, e) { return sq_aux_throwobject(v, e); }
}

SQRESULT sq_deleteslot(HSQUIRRELVM v,int idx,int pushval)
{
	SQ_TRY {
		sq_aux_paramscheck(v, 2);
		SQObjectPtr &self = sq_aux_gettypedarg(v, idx, OT_TABLE);
		SQObjectPtr &key = v->GetUp(-1);
		if(type(key) == OT_NULL) return sq_throwerror(v, _SC("null is not a valid key"));
		SQObjectPtr res;
		v->DeleteSlot(self, key, res);
		if(pushval)	v->GetUp(-1) = res;
		else v->Pop(1);
		return SQ_OK;
	} SQ_CATCH(SQException, e) { return sq_aux_throwobject(v, e); }
}

SQRESULT sq_set(HSQUIRRELVM v,int idx)
{
	SQObjectPtr &self = stack_get(v, idx);
	SQ_TRY {
		if(v->Set(self, v->GetUp(-2), v->GetUp(-1))) {
			v->Pop(2);
			return SQ_OK;
		}
		v->IdxError(v->GetUp(-2));return SQ_ERROR;
	}
	SQ_CATCH(SQException, e){ return sq_aux_throwobject(v, e); }
}

SQRESULT sq_rawset(HSQUIRRELVM v,int idx)
{
	SQ_TRY {
		SQObjectPtr &self = stack_get(v, idx);
		switch(type(self)) {
		case OT_TABLE:
			if(type(v->GetUp(-2)) == OT_NULL) return sq_throwerror(v, _SC("null key"));
				_table(self)->NewSlot(v->GetUp(-2), v->GetUp(-1));
			v->Pop(2);
			return SQ_OK;
		break;
		case OT_ARRAY:
			if(v->Set(self, v->GetUp(-2), v->GetUp(-1))) {
				v->Pop(2);
				return SQ_OK;
			}
		break;
		default:
			v->Pop(2);
			return sq_throwerror(v, _SC("rawset works only on arrays and tables"));
		}
		v->IdxError(v->GetUp(-2));return SQ_ERROR;
	}
	SQ_CATCH(SQException, e){ return sq_aux_throwobject(v, e); }
}

SQRESULT sq_setdelegate(HSQUIRRELVM v,int idx)
{
	SQObjectPtr &self = stack_get(v, idx);
	SQObjectPtr &mt = v->GetUp(-1);
	SQObjectType type = type(self);
	switch(type) {
	case OT_TABLE:
		if(type(mt) == OT_TABLE) {
			if(!_table(self)->SetDelegate(_table(mt))) return sq_throwerror(v, _SC("delagate cycle")); v->Pop();}
		else if(type(mt)==OT_NULL) {
			_table(self)->SetDelegate(NULL); v->Pop(); }
		else return sq_aux_invalidtype(v,type);
		break;
	case OT_USERDATA:
		if(type(mt)==OT_TABLE) {
			_userdata(self)->SetDelegate(_table(mt)); v->Pop(); }
		else if(type(mt)==OT_NULL) {
			_userdata(self)->SetDelegate(NULL); v->Pop(); }
		else return sq_aux_invalidtype(v, type);
		break;
	default:
			return sq_aux_invalidtype(v, type);
		break;
	}
	return SQ_OK;
}

SQRESULT sq_rawdeleteslot(HSQUIRRELVM v,int idx,int pushval)
{
	SQ_TRY {
		sq_aux_paramscheck(v, 2);
		SQObjectPtr &self = sq_aux_gettypedarg(v, idx, OT_TABLE);
		SQObjectPtr &key = v->GetUp(-1);
		SQObjectPtr t;
		if(_table(self)->Get(key,t)) {
			_table(self)->Remove(key);
		}
		if(pushval != 0)
			if(pushval)	v->GetUp(-1) = t;
		else
			v->Pop(1);
		return SQ_OK;
	}
	SQ_CATCH(SQException, e){ return sq_aux_throwobject(v, e); }
}

SQRESULT sq_getdelegate(HSQUIRRELVM v,int idx)
{
	SQObjectPtr &self=stack_get(v,idx);
	switch(type(self)){
	case OT_TABLE:
		if(!_table(self)->_delegate)break;
		v->Push(SQObjectPtr(_table(self)->_delegate));
		return SQ_OK;
		break;
	case OT_USERDATA:
		if(!_userdata(self)->_delegate)break;
		v->Push(SQObjectPtr(_userdata(self)->_delegate));
		return SQ_OK;
		break;
	}
	return sq_throwerror(v,_SC("wrong type"));
}

SQRESULT sq_get(HSQUIRRELVM v,int idx)
{
	SQObjectPtr &self=stack_get(v,idx);
	if(v->Get(self,v->GetUp(-1),v->GetUp(-1),false))
		return SQ_OK;
	v->Pop(1);
	return sq_throwerror(v,_SC("the index doesn't exist"));
}

SQRESULT sq_rawget(HSQUIRRELVM v,int idx)
{
	SQObjectPtr &self=stack_get(v,idx);
	switch(type(self)) {
	case OT_TABLE:
		if(_table(self)->Get(v->GetUp(-1),v->GetUp(-1)))
			return SQ_OK;
		break;
	case OT_ARRAY:
		if(v->Get(self,v->GetUp(-1),v->GetUp(-1),false))
			return SQ_OK;
		break;
	default:
		v->Pop(1);
		return sq_throwerror(v,_SC("rawget works only on arrays and tables"));
	}	
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
	po->_unVal.pUserPointer=NULL;po->_type=OT_NULL;
}

int sq_throwerror(HSQUIRRELVM v,const SQChar *err)
{
	v->_lasterror=SQString::Create(_ss(v),err);
	return -1;
}

void sq_getlasterror(HSQUIRRELVM v)
{
	v->Push(v->_lasterror);
}

void sq_reservestack(HSQUIRRELVM v,int nsize)
{
	if (((unsigned int)v->_top + nsize) > v->_stack.size()) {
		v->_stack.resize(v->_stack.size() + ((v->_top + nsize) - v->_stack.size()));
	}
}

SQRESULT sq_resume(HSQUIRRELVM v,int retval)
{
	SQ_TRY {
		if(type(v->GetUp(-1))==OT_GENERATOR){
			v->Push(_null_); //retval
			v->GetUp(-1)=v->Execute(v->GetUp(-2),v->_top,0,v->_top,SQVM::ET_RESUME_GENERATOR);
			if(!retval)
				v->Pop();
			return SQ_OK;
		}
		return sq_throwerror(v,_SC("only generators can be resumed"));
	}
	SQ_CATCH(SQException,e){return sq_aux_throwobject(v,e);}
}

SQRESULT sq_call(HSQUIRRELVM v,int params,int retval)
{
	SQObjectPtr res;
	int top = v->_top;
	SQ_TRY {
		if(v->Call(v->GetUp(-(params+1)),params,v->_top-params,res,SQVM::ET_CALL)){
			v->Pop(params);//pop args
			if(retval){
				v->Push(res); return SQ_OK;
			}
			return SQ_OK;
		}
		if(!v->_suspended)
			v->Pop(params);
		return sq_throwerror(v,_SC("call failed"));
	}
	SQ_CATCH(SQException,e) {
		v->Pop(params); 
		return sq_aux_throwobject(v,e);
	}
}

SQRESULT sq_suspendvm(HSQUIRRELVM v)
{
	return v->Suspend();
}

SQRESULT sq_wakeupvm(HSQUIRRELVM v,int wakeupret,int retval)
{
	SQ_TRY {
		SQObjectPtr ret;
		if(!v->_suspended)
			return sq_throwerror(v,_SC("cannot resume a vm that is not running any code"));
		if(wakeupret){
			v->GetAt(v->_stackbase+v->_suspended_target)=v->GetUp(-1); //retval
			v->Pop();
		}else v->GetAt(v->_stackbase+v->_suspended_target)=_null_;
		ret=v->Execute(_null_,v->_top,-1,-1,SQVM::ET_RESUME_VM);
		if(sq_getvmstate(v) == SQ_VMSTATE_IDLE) {
			while (v->_top > 1) v->_stack[--v->_top] = _null_;
		}
		if(retval)
			v->Push(ret);
		return SQ_OK;
	}
	SQ_CATCH(SQException,e){return sq_aux_throwobject(v,e);}
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
	_ss(v)->_compilererrorhandler=f;
}

SQRESULT sq_writeclosure(HSQUIRRELVM v,SQWRITEFUNC w,SQUserPointer up)
{
	SQ_TRY {
		SQObjectPtr o=sq_aux_gettypedarg(v,-1,OT_CLOSURE);
		SQClosure *c=_closure(o);
		unsigned short tag = SQ_BYTECODE_STREAM_TAG;
		if(w(up,&tag,2) != 2)
			return sq_throwerror(v,_SC("io error"));
		_closure(o)->Save(v,up,w);
	}
	SQ_CATCH(SQException,e){return sq_aux_throwobject(v,e);}
	return SQ_OK;
}

SQRESULT sq_readclosure(HSQUIRRELVM v,SQREADFUNC r,SQUserPointer up)
{
	SQ_TRY {
		SQObjectPtr func=SQFunctionProto::Create();
		SQObjectPtr closure=SQClosure::Create(_ss(v),_funcproto(func));
		unsigned short tag;
		if(r(up,&tag,2) != 2)
			return sq_throwerror(v,_SC("io error"));
		if(tag != SQ_BYTECODE_STREAM_TAG)
			return sq_throwerror(v,_SC("invalid stream"));
		_closure(closure)->Load(v,up,r);
		v->Push(closure);
	}
	SQ_CATCH(SQException, e) { return sq_aux_throwobject(v, e); }
	return SQ_OK;
}

SQChar *sq_getscratchpad(HSQUIRRELVM v,int minsize)
{
	return _ss(v)->GetScratchPad(minsize);
}

int sq_collectgarbage(HSQUIRRELVM v)
{
#ifndef NO_GARBAGE_COLLECTOR
	return _ss(v)->CollectGarbage(v);
#else
	return -1;
#endif
}

SQRESULT sq_setfreevariable(HSQUIRRELVM v,int idx,unsigned int nval)
{
	SQObjectPtr &self=stack_get(v,idx);
	switch(type(self))
	{
	case OT_CLOSURE:
		if(_closure(self)->_outervalues.size()>nval){
			_closure(self)->_outervalues[nval]=stack_get(v,-1);
		}
		else return sq_throwerror(v,_SC("invalid free var index"));
		break;
	case OT_NATIVECLOSURE:
		if(_nativeclosure(self)->_outervalues.size()>nval){
			_nativeclosure(self)->_outervalues[nval]=stack_get(v,-1);
		}
		else return sq_throwerror(v,_SC("invalid free var index"));
		break;
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

struct BufState{
	const SQChar *buf;
	int ptr;
	int size;
};

SQInteger buf_lexfeed(SQUserPointer file)
{
	BufState *buf=(BufState*)file;
	if(buf->size<(buf->ptr+1))
		return 0;
	return buf->buf[buf->ptr++];
}

SQRESULT sq_compilebuffer(HSQUIRRELVM v,const SQChar *s,int size,const SQChar *sourcename,int raiseerror) {
	BufState buf;
	buf.buf = s;
	buf.size = size;
	buf.ptr = 0;
	return sq_compile(v, buf_lexfeed, &buf, sourcename, raiseerror);
}

void sq_move(HSQUIRRELVM dest,HSQUIRRELVM src,int idx)
{
	dest->Push(stack_get(src,idx));
}

void sq_setprintfunc(HSQUIRRELVM v, SQPRINTFUNCTION printfunc)
{
	_ss(v)->_printfunc = printfunc;
}

SQPRINTFUNCTION sq_getprintfunc(HSQUIRRELVM v)
{
	return _ss(v)->_printfunc;
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
