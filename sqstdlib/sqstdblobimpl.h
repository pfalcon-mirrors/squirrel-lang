/*	see copyright notice in squirrel.h */
#ifndef _SQSTD_BLOBIMPL_H_
#define _SQSTD_BLOBIMPL_H_

struct SQBlob : public SQStream
{
	SQBlob(void *buf, SQInteger size) {
		_buf = (unsigned char *)buf;
		_size = size;
		// _allocated == 0 means readonly buffer, with
		// memory owned by something else. The intention
		// is to provide (possibly sliced) view inside
		// other object. Few issues: for this to work
		// reliably, target object must be referenced
		// by such blob, but this is low-level type
		// not concerned with Squirrel objects. So,
		// it would need to be wrapped in higher-level
		// object keeping such reference. Also: should
		// read-only, own vs borrowed memory buffer,
		// growability all be orthogonal properties?
		_allocated = 0;
		_ptr = 0;
	}
	SQBlob(SQInteger size) {
		_size = size;
		_allocated = size;
		_buf = (unsigned char *)sq_malloc(size);
		memset(_buf, 0, _size);
		_ptr = 0;
	}
	virtual ~SQBlob() {
		if (_allocated)
			sq_free(_buf, _allocated);
	}
	SQInteger Write(void *buffer, SQInteger size) {
		if (!_allocated) return 0;
		if(!CanAdvance(size)) {
			GrowBufOf(_ptr + size - _size);
		}
		memcpy(&_buf[_ptr], buffer, size);
		_ptr += size;
		return size;
	}
	SQInteger Read(void *buffer,SQInteger size) {
		SQInteger n = size;
		if(!CanAdvance(size)) {
			if((_size - _ptr) > 0)
				n = _size - _ptr;
			else return 0;
		}
		memcpy(buffer, &_buf[_ptr], n);
		_ptr += n;
		return n;
	}
	SQInteger Readline(void *buffer,SQInteger size) { return -1; }
	bool Resize(SQInteger n) {
		if (!_allocated) return false;
		if(n != _allocated) {
			unsigned char *newbuf = (unsigned char *)sq_malloc(n);
			memset(newbuf,0,n);
			if(_size > n)
				memcpy(newbuf,_buf,n);
			else
				memcpy(newbuf,_buf,_size);
			sq_free(_buf,_allocated);
			_buf=newbuf;
			_allocated = n;
			if(_size > _allocated)
				_size = _allocated;
			if(_ptr > _allocated)
				_ptr = _allocated;
		}
		return true;
	}
	bool GrowBufOf(SQInteger n)
	{
		bool ret = true;
		if(_size + n > _allocated) {
			if(_size + n > _size * 2)
				ret = Resize(_size + n);
			else
				ret = Resize(_size * 2);
		}
		_size = _size + n;
		return ret;
	}
	bool CanAdvance(SQInteger n) {
		if(_ptr+n>_size)return false;
		return true;
	}
	SQInteger Seek(SQInteger offset, SQInteger origin) {
		switch(origin) {
			case SQ_SEEK_SET:
				if(offset > _size || offset < 0) return -1;
				_ptr = offset;
				break;
			case SQ_SEEK_CUR:
				if(_ptr + offset > _size || _ptr + offset < 0) return -1;
				_ptr += offset;
				break;
			case SQ_SEEK_END:
				if(_size + offset > _size || _size + offset < 0) return -1;
				_ptr = _size + offset;
				break;
			default: return -1;
		}
		return 0;
	}
	bool IsValid() {
		return _buf?true:false;
	}
	bool EOS() {
		return _ptr == _size;
	}
	SQInteger Flush() { return 0; }
	SQInteger Tell() { return _ptr; }
	SQInteger Len() { return _size; }
	SQUserPointer GetBuf(){ return _buf; }
private:
	SQInteger _size;
	SQInteger _allocated;
	SQInteger _ptr;
	unsigned char *_buf;
};

#endif //_SQSTD_BLOBIMPL_H_
