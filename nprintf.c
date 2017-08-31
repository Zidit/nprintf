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


#include "ctype.h"
#include "nprintf.h"
#include <stdint.h>
#include <math.h>
#include <float.h>

#define FLAG_LEFT_JUST		(1 << 0)
#define FLAG_FORCE_SIGN		(1 << 1)
#define FLAG_BLANK_SIGN		(1 << 2)
#define FLAG_PRECEED		(1 << 3)
#define FLAG_PAD_ZERO		(1 << 4)

#define iszero(f) (((*(uint32_t*)&f) & 0x7FFFFFFF) == 0)

static struct stream std_strm = {
	(void*)0,
	0,
	0,
	&print_char
};

struct sub_specifiers {
	int f;
	int w;
	int p;
};

#define min(a,b) \
({ __typeof__ (a) _a = (a); \
  __typeof__ (b) _b = (b); \
  _a < _b ? _a : _b;})

#define abs(a) \
  ({ __typeof__ (a) _a = (a); \
	_a < 0 ? -_a : _a;})

void* nmemset(void* ptr, int value, int size)
{
	while(size--)
		*((unsigned char*)ptr + size) = value;
	return ptr;
}

char* strcpy(char *dest, const char *src)
{
	char* org = dest;
	while(*src)
		*(dest++) = *(src++);
	*(dest) = *(src);
	return org;
}

char* strapp(char *dest, const char *src)
{
	while(*src)
		*(dest++) = *(src++);
	return dest;
}

static int str_to_int(const char* str, int *value)
{
	int i = 0;
	*value = 0;

	while(str[i] >= '0' && str[i] <= '9') {
		*value *= 10;
		*value += str[i++] - '0';
	}

	return i;
}

static char* int_to_str(char* buf, unsigned int value, int base, int ucase)
{
	int i = 0;
	buf[i++] = '\0';

	if(value == 0) {
		buf[i++] = '0';
	} else {
		while(value) {
			int digit = (value % base);
			buf[i++] = digit < 10 ? ('0' + digit) : (ucase ? 'A' : 'a') + digit - 10;
			value /= base;
		}
	}
	i--;

	//reverse string
	int j = 0;
	while(j < i) {
		char c = buf[j];
		buf[j] = buf[i];
		buf[i] = c;
		i--;
		j++;
	}

	return buf;
}

static int _strlen(char* str)
{
	int i = 0;
	while(*(str++))
		i++;
	return i;
}

static int get_flags(const char* stream, int* chars_got)
{
	int flags = 0;
	*chars_got = 0;

	while(1) {
		switch(*(stream++)) {
			case '-':
				flags |= FLAG_LEFT_JUST;
				break;
			case '+':
				flags |= FLAG_FORCE_SIGN;
				break;
			case ' ':
				flags |= FLAG_BLANK_SIGN;
				break;
			case '#':
				flags |= FLAG_PRECEED;
				break;
			case '0':
				flags |= FLAG_PAD_ZERO;
				break;
			default:
				return flags;
		}

		(*chars_got)++;
	}
}

static int print_to_stream(int c, struct stream *strm)
{
	if(strm->buffer){
		if(strm->buffer_size > strm->pos)
			strm->buffer[strm->pos++] = c;
	}

	if(strm->printchar)
		strm->printchar(c);

	return 1;
}

static int printstr(char* str, struct stream *strm)
{
	int i;
	for(i = 0; *str; i++)
		print_to_stream(*str++, strm);
	return i;
}

static int printchars(char c, int len, struct stream *strm)
{
	if(len <= 0)
		return 0;
	int i;
	for(i = 0; i < len; i++)
		print_to_stream(c, strm);
	return len;
}


int printchar(int c, struct stream *strm)
{
	print_to_stream(c, strm);
	return 1;
}

static int print_with_padding(char* value, char* sign, struct sub_specifiers *ss, struct stream *strm)
{
	int chars_printed = 0;
	int left_pad = 0;
	if(!(ss->f & FLAG_LEFT_JUST)) {
		left_pad = ss->w - (ss->p > _strlen(value) ? ss->p : _strlen(value));
		if(sign[0])
			left_pad -= _strlen(sign);
		if(left_pad < 0)
			left_pad = 0;

		if(!(ss->f & FLAG_PAD_ZERO))
			chars_printed += printchars(' ', left_pad, strm);
	}

	chars_printed += printstr(sign, strm);

	if(ss->f & FLAG_PAD_ZERO)
		chars_printed += printchars('0', left_pad, strm);

	chars_printed += printchars('0', ss->p - _strlen(value), strm);
	chars_printed += printstr(value, strm);

	if(ss->f & FLAG_LEFT_JUST)
		chars_printed += printchars(' ', ss->w - chars_printed, strm);

	return chars_printed;
}


static int print_int(int value, struct sub_specifiers *ss, struct stream *strm)
{
	if(ss->p == 0 && value == 0)
		return 0;

	char buf[11];
	int_to_str(buf, abs(value), 10, 0);

	char* sign = "";
	if(value < 0) {
		sign = "-";
	} else if(ss->f & FLAG_FORCE_SIGN){
		sign = "+";
	} else if(ss->f & FLAG_BLANK_SIGN){
		sign = " ";
	}

	return print_with_padding(buf, sign, ss, strm);
}

static int print_uint(unsigned int value, struct sub_specifiers *ss, struct stream *strm)
{
	if(ss->p == 0 && value == 0)
		return 0;

	char buf[11];
	int_to_str(buf, value, 10, 0);
	char* sign = "";

	return print_with_padding(buf, sign, ss, strm);
}

static int print_octal(unsigned int value, struct sub_specifiers *ss, struct stream *strm)
{
	if(ss->p == 0 && value == 0)
		return 0;

	char buf[12];
	int_to_str(buf, value, 8, 0);
	char* sign = "";
	if((ss->f & FLAG_PRECEED) && value)
		sign = "0";

	return print_with_padding(buf, sign, ss, strm);
}

static int print_hex(unsigned int value, struct sub_specifiers *ss, struct stream *strm)
{
	if(ss->p == 0 && value == 0)
		return 0;

	char buf[9];
	int_to_str(buf, value, 16, 0);
	char* sign = "";
	if((ss->f & FLAG_PRECEED) && value)
		sign = "0x";

	return print_with_padding(buf, sign, ss, strm);
}

static int print_hex_upper(unsigned int value, struct sub_specifiers *ss, struct stream *strm)
{
	if(ss->p == 0 && value == 0)
		return 0;

	char buf[9];
	int_to_str(buf, value, 16, 1);
	char* sign = "";
	if(ss->f & FLAG_PRECEED)
		sign = "0X";

	return print_with_padding(buf, sign, ss, strm);
}

static int print_pointer(void* ptr, struct sub_specifiers *ss, struct stream *strm)
{
	char buf[9];
	int_to_str(buf, (intptr_t) ptr, 16, 0);
	char* sign = "";
	if(ss->f & FLAG_PRECEED)
		sign = "0X";

	ss->f &= ~FLAG_PAD_ZERO;
	ss->p = 8;
	return print_with_padding(buf, sign, ss, strm);
}

static int print_string(char* string, struct sub_specifiers *ss, struct stream *strm)
{
	if(string == (void*)0) {
		return printstr("(null)", strm);
	}

	int chars_printed = 0;
	int left_pad = 0;
	int string_len = _strlen(string);
	if(ss->p > 0)
		string_len = (string_len < ss->p) ? string_len : ss->p;

	if(!(ss->f & FLAG_LEFT_JUST)) {
		left_pad = ss->w - string_len;
		if(left_pad < 0)
			left_pad = 0;

		if(ss->f & FLAG_PAD_ZERO)
			chars_printed += printchars('0', left_pad, strm);
		else
			chars_printed += printchars(' ', left_pad, strm);
	}

	while(string_len--){
		print_to_stream(*(string++), strm);
		chars_printed++;
	}

	if(ss->f & FLAG_LEFT_JUST)
		chars_printed += printchars(' ', ss->w - chars_printed, strm);

	return chars_printed;
}

static int print_character(int c, struct sub_specifiers *ss, struct stream *strm)
{
	char buf[2];
	buf[0] = c;
	buf[1] = '\0';

	return print_with_padding(buf, "", ss, strm);
}

#ifdef NPRINT_FLOAT

static int float_to_str_integer(char* str, int str_size, uint32_t man, uint32_t exp)
{
	nmemset(str, '0', str_size);

	int digits = 0;
	int i;

	for (i = 0; i < exp + 1; i++) {
		int32_t j;
		uint32_t carry;

		carry = man & (1 << (23 - i)) ? 1 : 0;

		for (j = digits; j >= 0; j--) {
			str[j] += str[j] - '0' + carry;
			carry = (str[j] > '9') ? 1 : 0;

			if (carry) {
				str[j] -= 10;
				if(j == 0) {
					int k;
					for (k = str_size - 1; k >= 0; k--)
						str[k + 1] = str[k];

					str[0] = '1';
					digits++;
				}
			}
		}
	}

	str[str_size - 1] = '\0';
	digits++;
	return digits;
}

static int float_to_str_fraction_div2(char* str, int str_size, uint32_t digits)
{
	int32_t j;
	for (j = digits; j >= 0; j--) {
		if((str[j] & 0x1) && (j < str_size - 2)) { //is odd
			str[j + 1] += 5;
			if(j + 1 > digits)
				digits++;
		}
		str[j] /= 2;
	}

	return digits;
}

static int float_to_str_fraction(char* str, int str_size, uint32_t man, int32_t exp)
{
	nmemset(str, 0, str_size);

	int i;
	int digits = 0;
	int mantisa_len = 24;
	if(exp > -2)
		mantisa_len -= (exp + 2);

	if(mantisa_len < 0) {
		nmemset(str, '0', str_size);
		return 8;
	}

	if(man & 0x01)
		str[0] += 5;

	for (i = 0; i < mantisa_len ; i++) {
		digits = float_to_str_fraction_div2(str, str_size, digits);

		man >>= 1;
		if(man & 0x01)
			str[0] += 5;
	}

	if(exp < -2) {
		i = abs(exp + 2);
		while(i--) {
			digits = float_to_str_fraction_div2(str, str_size, digits);
		}
	}

	for(i = 0; i < str_size; i++)
		str[i] += '0';

	digits++;
	return digits;
}


static char* float_string(float value, char* buffer, struct sub_specifiers *ss, int ucase)
{
	char* buf = buffer;

	if(isnan(value))
		return strcpy(buf, ucase ? "NAN" : "nan");
	else if(isinf(value))
		return strcpy(buf, ucase ? "INF" : "inf");
	else if(iszero(value))
		return strcpy(buf, "0");

	uint32_t v = *(uint32_t*)&value;
	uint32_t sign = v >> 31;
	int32_t exp = ((v >> 23) & 0xff) - 127;
	uint32_t man = v & ((1 << 23) - 1);
	man |= (1 << 23);

	if(exp < 0) {
		if(ss->p == 0 && value >= 0.5f)
			buf = strapp(buf, "1");
		else
			buf = strapp(buf, "0.");
	} else {
		char str[17];
		int a = float_to_str_integer(str, sizeof(str), man, exp);

		int b10_man = min(a, sizeof(str) - 1);
		int b10_exp = a - b10_man;

		str[b10_man] = '\0';

		buf = strapp(buf, str);
		if(b10_exp > 0)
			while(b10_exp--)
				buf = strapp(buf, "0");
		buf = strapp(buf, ".");
	}

	char str[17];
	int precision = float_to_str_fraction(str, sizeof(str), man, exp);

	precision = ss->p;
	str[precision] = '\0';
	buf = strapp(buf, str);
	*buf = '\0';
	return buf;
}


#endif //NPRINT_FLOAT

static int print_float(float value, struct sub_specifiers *ss, struct stream *strm, char specifier)
{
#ifdef NPRINT_FLOAT
	char buffer[64] = {'\0'};

	if(ss->p < 0)
		ss->p = 6;

	if (specifier == 'f')
		float_string(value, buffer, ss, 0);
	else if (specifier == 'F')
		float_string(value, buffer, ss, 1);
	else
		return 0;

	char* sign = "";
	if(value < 0) {
		sign = "-";
	} else if(ss->f & FLAG_FORCE_SIGN){
		sign = "+";
	} else if(ss->f & FLAG_BLANK_SIGN){
		sign = " ";
	}

	return print_with_padding(buffer, sign, ss, strm);
#else //NPRINT_FLOAT
	return print_with_padding("NO FLOAT", "", ss, strm);
#endif //NPRINT_FLOAT
}

int vfnprintf(struct stream *strm, const char * format, va_list arg)
{
	char c;
	int tot = 0;

	for(;(c = *format); format++) {
		//Print normal character
		if(c != '%') {
			tot++;
			print_to_stream(c, strm);
			continue;
		}

		//Get specifiers and print variable
		struct sub_specifiers ss = {0};

		//Get flags
		while(1) {
			format++;
			if(*format == '-') ss.f |= FLAG_LEFT_JUST;
			else if(*format == '+') ss.f |= FLAG_FORCE_SIGN;
			else if(*format == ' ') ss.f |= FLAG_BLANK_SIGN;
			else if(*format == '#') ss.f |= FLAG_PRECEED;
			else if(*format == '0') ss.f |= FLAG_PAD_ZERO;
			else break;
		}

		//Get width
		if(*format == '*') {
			format++;
			ss.w = va_arg(arg, int);
		} else if(isdigit(*format)) {
			format += str_to_int(format, &ss.w);
		}

		if(ss.w < 0) {
			ss.w = -ss.w;
			ss.f |= FLAG_LEFT_JUST;
		}

		//Get precission
		if(*format == '.') {
			if(*(++format) == '*') {
				format++;
				ss.p = va_arg(arg, int);
			} else if(isdigit(*format)) {
				format += str_to_int(format, &ss.p);
			}
		} else {
			ss.p = -1;
		}

		//Select right print function
		switch(*format) {
			case 'd': case 'i':
				tot += print_int(va_arg(arg, int), &ss, strm);
				break;
			case 'u':
				tot += print_uint(va_arg(arg, unsigned int), &ss, strm);
				break;
			case 'o':
				tot += print_octal(va_arg(arg, unsigned int), &ss, strm);
				break;
			case 'x':
				tot += print_hex(va_arg(arg, unsigned int), &ss, strm);
				break;
			case 'X':
				tot += print_hex_upper(va_arg(arg, unsigned int), &ss, strm);
				break;
			case 'c':
				tot += print_character(va_arg(arg, int), &ss, strm);
				break;
			case 's':
				tot += print_string(va_arg(arg, char*), &ss, strm);
				break;
			case 'p':
				tot += print_pointer(va_arg(arg, void*), &ss, strm);
				break;
			case '%':
				tot += print_to_stream('%', strm);
				break;
			case 'f': case 'F':
				tot += print_float(va_arg(arg, double), &ss, strm, *format);
				break;
			case 'n':
				*((int*)va_arg(arg, void*)) = tot;
				break;
			case '\0':
				return tot;
	
			default:
				return 0;
		}
	}

	return tot;
}

int vnprintf(const char * format, va_list arg)
{
	struct stream strm = std_strm;
	return vfnprintf(&strm, format, arg);
}

int vsnnprintf(char *str, unsigned int size, const char *format, va_list arg)
{
	struct stream strm = {
		str,
		size - 1,
		0,
		(void*)0
	};

	int ret = vfnprintf(&strm, format, arg);
	strm.buffer[strm.pos] = '\0';
	return strm.pos;
}

int nprintf(const char * format, ...)
{
	va_list args;
	va_start(args, format);
	int ret = vnprintf(format, args);
	va_end(args);

	return ret;
}

int fnprintf(struct stream *strm, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	int ret = vfnprintf(strm, format, args);
	va_end(args);

	return ret;
}

int snnprintf(char *str, unsigned int size, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	int ret = vsnnprintf(str, size, format, args);
	va_end(args);

	return ret;
}
