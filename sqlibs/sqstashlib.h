#ifndef _SQ_STASHLIB_H_
#define _SQ_STASHLIB_H_

#ifdef __cplusplus
extern "C" {
#endif

int sq_stashlib_register(HSQUIRRELVM v);
int sq_stashlib_close();

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
