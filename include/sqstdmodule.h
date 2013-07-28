#ifndef _SQSTD_MODULE_H_
#define _SQSTD_MODULE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef void* SQSTDHMODULE;

typedef SQRESULT (*p_sqstd_moduleapi_openmodule)(HSQUIRRELVM,const SQChar* /*modulename*/ ,SQSTDHMODULE*);
typedef void *(*p_sqstd_moduleapi_getsymbol)(HSQUIRRELVM,SQSTDHMODULE,const SQChar* /*symbol*/);
typedef void (*p_sqstd_moduleapi_closemodule)(SQSTDHMODULE);

typedef struct tagSQSTDUserModuleAPI {
	p_sqstd_moduleapi_openmodule _sqstd_moduleapi_openmodule;
	p_sqstd_moduleapi_getsymbol _sqstd_moduleapi_getsymbol;
	p_sqstd_moduleapi_closemodule _sqstd_moduleapi_closemodule;
} SQSTDUserModuleAPI;

SQUIRREL_API SQRESULT sqstd_importmodule(HSQUIRRELVM v, const SQChar* module);
SQUIRREL_API SQRESULT sqstd_register_modulelib(HSQUIRRELVM v,SQSTDUserModuleAPI *api);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*_SQSTD_MODULE_H_*/
