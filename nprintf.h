/*
Copyright (c) 2017 Mika Terhokoski

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */


#ifndef NPRINTF_H_
#define NPRINTF_H_

#include <stdarg.h>

#define NPRINT_FLOAT

struct stream {
	char* buffer;
	unsigned int buffer_size;
	unsigned int pos;
	void (*printchar)(int c);
};

int vnprintf(const char *format, va_list arg);
int vfnprintf(struct stream *strm, const char * format, va_list arg);
int vsnnprintf(char *str, unsigned int size, const char *format, va_list arg);

int nprintf(const char *format, ...)	__attribute__ ((format (printf, 1, 2)));
int fnprintf(struct stream *strm, const char *format, ...) __attribute__ ((format (printf, 2, 3)));
int snnprintf(char *str, unsigned int size, const char *format, ...) __attribute__ ((format (printf, 3, 4)));

extern void print_char(int c);
/*
#define printf(...) (nprintf(__VA_ARGS__))
#define fprintf(...) (fnprintf(__VA_ARGS__))
#define snprintf(...) (snnprintf(__VA_ARGS__))

#define vprintf(...) (vnprintf(__VA_ARGS__))
#define vfprintf(...) (vfnprintf(__VA_ARGS__))
#define vsnprintf(...) (vsnnprintf(__VA_ARGS__))
*/
#endif /* NPRINTF_H_ */
