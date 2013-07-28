#ifndef _SQ_BLOBLIB_H_
#define _SQ_BLOBLIB_H_

#ifdef __cplusplus
extern "C" {
#endif

void sq_blob_createdelegate(HSQUIRRELVM v);
SQUserPointer sq_blob_newblob(HSQUIRRELVM v,int size,SQUIRREL_MALLOC __malloc,SQUIRREL_FREE __free);
SQRESULT sq_blob_getblob(HSQUIRRELVM v,int idx,SQUserPointer *p);
SQInteger sq_blob_getblobsize(HSQUIRRELVM v,int idx);
void sq_blob_register(HSQUIRRELVM v,SQUIRREL_MALLOC _default_malloc,SQUIRREL_FREE _default_free);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif //_SQ_BLOBLIB_H_ 
