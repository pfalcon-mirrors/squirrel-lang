#include <squirrel.h>
#include <sqstdstring.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#ifdef _UNICODE
#define scstrchr wcschr
#define scsnprintf wsnprintf
#define scatoi _wtoi
#else
#define scstrchr strchr
#define scsnprintf snprintf
#define scatoi atoi
#endif
#define MAX_FORMAT_LEN	20
#define MAX_WFORMAT_LEN	3
#define ADDITIONAL_FORMAT_SPACE (100*sizeof(SQChar))

static int validate_format(HSQUIRRELVM v, SQChar *fmt, const SQChar *src, int n,int &width)
{
	SQChar swidth[MAX_WFORMAT_LEN];
	int wc = 0;
	int start = n;
	fmt[0] = '%';
	while (scstrchr(_SC("-+ #0"), src[n])) n++;
	while (scisdigit(src[n])) {
		swidth[wc] = src[n];
		n++;
		wc++;
		if(wc>=MAX_WFORMAT_LEN)
			return sq_throwerror(v,_SC("width format too long"));
	}
	swidth[wc] = '\0';
	if(wc > 0) {
		width = scatoi(swidth);
	}
	else
		width = 0;
	if (src[n] == '.') {
	    n++;
    	
		wc = 0;
		while (scisdigit(src[n])) {
			swidth[wc] = src[n];
			n++;
			wc++;
			if(wc>=MAX_WFORMAT_LEN)
				return sq_throwerror(v,_SC("precision format too long"));
		}
		swidth[wc] = '\0';
		if(wc > 0) {
			width += scatoi(swidth);
		}
	}
	if (n-start > MAX_FORMAT_LEN )
		return sq_throwerror(v,_SC("format too long"));
	memcpy(&fmt[1],&src[start],((n-start)+1)*sizeof(SQChar));
	fmt[(n-start)+2] = '\0';
	return n;
}

static int _string_format(HSQUIRRELVM v)
{
	const SQChar *format;
	SQChar *dest;
	SQChar fmt[MAX_FORMAT_LEN];
	sq_getstring(v,2,&format);
	int allocated = (sq_getsize(v,2)+1)*sizeof(SQChar);
	dest = sq_getscratchpad(v,allocated);
	int n = 0,i = 0, nparam = 3, w;
	while(format[n] != '\0') {
		if(format[n] != '%') {
			dest[i++] = format[n];
			n++;
		}
		else if(format[++n] == '%') {
			dest[i++] = '%';
		}
		else {
			if( nparam > sq_gettop(v) )
				return sq_throwerror(v,_SC("not enough paramters for the given format string"));
			n = validate_format(v,fmt,format,n,w);
			if(n < 0) return -1;
			int addlen = 0;
			int valtype = 0;
			const SQChar *ts;
			SQInteger ti;
			SQFloat tf;
			switch(format[n]) {
			case 's':
				if(SQ_FAILED(sq_getstring(v,nparam,&ts))) 
					return sq_throwerror(v,_SC("string expected for the specified format"));
				addlen = (sq_getsize(v,nparam)*sizeof(SQChar))+(w*sizeof(SQChar));
				valtype = 's';
				break;
			case 'i': case 'd': case 'c':case 'o':  case 'u':  case 'x':  case 'X':
				if(SQ_FAILED(sq_getinteger(v,nparam,&ti))) 
					return sq_throwerror(v,_SC("integer expected for the specified format"));
				addlen = (ADDITIONAL_FORMAT_SPACE)+(w*sizeof(SQChar));
				valtype = 'i';
				break;
			case 'f': case 'g': case 'G': case 'e':  case 'E':
				if(SQ_FAILED(sq_getfloat(v,nparam,&tf))) 
					return sq_throwerror(v,_SC("float expected for the specified format"));
				addlen = (ADDITIONAL_FORMAT_SPACE)+(w*sizeof(SQChar));
				valtype = 'f';
				break;
			default:
				return sq_throwerror(v,_SC("invalid format"));
			}
			n++;
			if((allocated-i) < addlen)
			allocated += addlen - (allocated-i);
			dest = sq_getscratchpad(v,allocated);
			switch(valtype) {
			case 's': i += scsprintf(&dest[i],fmt,ts); break;
			case 'i': i += scsprintf(&dest[i],fmt,ti); break;
			case 'f': i += scsprintf(&dest[i],fmt,tf); break;
			};
			nparam ++;
		}
	}
	sq_pushstring(v,dest,i);
	return 1;
}


#define _DECL_FUNC(name,nparams,pmask) {_SC(#name),_string_##name,nparams,pmask}
static SQRegFunction stringlib_funcs[]={
	_DECL_FUNC(format,-2,_SC(".s")),
	{0,0}
};


int sqstd_register_stringlib(HSQUIRRELVM v)
{
	int i=0;
	while(stringlib_funcs[i].name!=0)
	{
		sq_pushstring(v,stringlib_funcs[i].name,-1);
		sq_newclosure(v,stringlib_funcs[i].f,0);
		sq_setparamscheck(v,stringlib_funcs[i].nparamscheck,stringlib_funcs[i].typemask);
		sq_createslot(v,-3);
		i++;
	}
	return 1;
}