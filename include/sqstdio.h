#ifndef _SQSTDIO_H_
#define _SQSTDIO_H_

#ifdef __cplusplus
extern "C" {
#endif

#define SQ_SEEK_CUR 0
#define SQ_SEEK_END 1
#define SQ_SEEK_SET 2

typedef void* SQFILE;

SQUIRREL_API SQFILE sqstd_fopen(const SQChar *,const SQChar *);
SQUIRREL_API SQInteger sqstd_fread(SQUserPointer, SQInteger, SQInteger, SQFILE);
SQUIRREL_API SQInteger sqstd_fwrite(const SQUserPointer, SQInteger, SQInteger, SQFILE);
SQUIRREL_API int sqstd_fseek(SQFILE , long , int);
SQUIRREL_API long sqstd_ftell(SQFILE);
SQUIRREL_API int sqstd_fflush(SQFILE);
SQUIRREL_API int sqstd_fclose(SQFILE);
SQUIRREL_API int sqstd_feof(SQFILE);

SQUIRREL_API SQRESULT sqstd_createfile(HSQUIRRELVM v, SQFILE file,int own);
SQUIRREL_API SQRESULT sqstd_getfile(HSQUIRRELVM v, int idx, SQFILE *file);

SQUIRREL_API SQRESULT sqstd_register_iolib(HSQUIRRELVM v);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*_SQSTDIO_H_*/

