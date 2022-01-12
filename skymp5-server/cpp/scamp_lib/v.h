#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t byte;

byte* _v_malloc(int n);
byte* vcalloc(int n);
void* memdup(void* src, int sz);

struct string {
	byte* str;
	int len;
	int is_lit;
};

string CreateString(char* str);

#define _SLIT(s) string { (byte*)("" s), (sizeof(s) - 1), 1 }

struct None__ {
	string msg;
	int code;
};

struct VError {
	string msg;
	int code;
};

struct IError {
	union {
		void* _object;
		VError* _Error;
		None__* _None__;
	};
	int _typ;
	string* msg;
	int* code;
};

struct Option {
	byte state;
	IError err;
};

void opt_ok(void* data, Option* option, int size);

#ifndef __offsetof_ptr
#define __offsetof_ptr(ptr,PTYPE,FIELDNAME) ((size_t)((byte *)&((PTYPE *)ptr)->FIELDNAME - (byte *)ptr))
#endif

const int _IError_Error_index = 1;

inline IError I_Error_to_Interface_IError(VError* x) {
	return IError{
		x,
		_IError_Error_index,
		(string*)((char*)x + __offsetof_ptr(x, VError, msg)),
		(int*)((char*)x + __offsetof_ptr(x, VError, code)),
	};
}

inline IError _v_error(string message) {
	auto err = VError{ message, 0 };
	return I_Error_to_Interface_IError((VError*)memdup(&err, sizeof(VError)));
}

struct Option_void {
	byte state;
	IError err;
	byte data[sizeof(byte)];
};