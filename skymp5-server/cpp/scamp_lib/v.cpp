#include "v.h"

IError _const_none__; // inited later

byte* _v_malloc(int n) {
	if (n <= 0) {
		throw "cpp _v_malloc";
		exit(1);
	}
	byte* res = ((byte*)(0));
	{
		res = (byte*)malloc(n);
	}

	if (res == 0) {
		throw "cpp _v_malloc failed";
		exit(1);
	}
	return res;
}

byte* vcalloc(int n) {
	if (n < 0) {
		throw "cpp vcalloc";
		exit(1);
	}
	else if (n == 0) {
		return ((byte*)(0));
	}

	return (byte*)calloc(1, n);
}

void* memdup(void* src, int sz) {
	if (sz == 0) {
		return vcalloc(1);
	}
	{ // Unsafe block
		byte* mem = _v_malloc(sz);
		return memcpy(mem, src, sz);
	}
	return 0;
}

string CreateString(char* str) {
	string vstr;
	vstr.str = (byte*)str;
	vstr.is_lit = false;
	vstr.len = strlen(str);

	return vstr;
}

void opt_ok(void* data, Option* option, int size)
{
	*option = { 0, _const_none__ };
	memcpy(((byte*)(&option->err)) + sizeof(IError), data, size);
}