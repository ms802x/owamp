/*
 *      $Id$
 */
/************************************************************************
*									*
*			     Copyright (C)  2002			*
*				Internet2				*
*			     All Rights Reserved			*
*									*
************************************************************************/
/*
 *	File:		arithm.c
 *
 *	Author:		Anatoly Karp
 *			Internet2
 *
 *	Date:		Sun Jun 02 12:29:20 MDT 2002
 *
 *	Description:	
 */
/*
** Implementation of extended-precision arithmetic.
*/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "arithm.h"

#define T OWPTime

/*
** This implementation uses eight 18-bit quantities to represent
** and manipulate Owamp's [64].[64] space of timestamps.
*/

/*
** Constructor function. More significant digits go first.
*/
T OWPTime_new(unsigned short a, 
	      unsigned short b, 
	      unsigned short c, 
	      unsigned short d,
	      unsigned short e, 
	      unsigned short f,
	      unsigned short g, 
	      unsigned short h,
	      int set_flag
	      )
{
	T x = malloc(sizeof(*x));
	if (x == NULL){
		perror("malloc");
		return NULL;
	}

	if (!set_flag)
		return x;

	x->digits[0] = h;
	x->digits[1] = g;
	x->digits[2] = f;
	x->digits[3] = e;
	x->digits[4] = d;
	x->digits[5] = c;
	x->digits[6] = b;
	x->digits[7] = a;

	return x;
}

#define BASE 0x10000 /* i.e., arithmetic is done modulo 2^16 */

/* 
** This function embeds the unsigned 32-bit integers into the
** OWPTime space.
*/
T OWPTime_from_ulong(unsigned long a)
{
	unsigned short tmp1, tmp2;

	tmp1 = (unsigned short)(a/BASE);
	tmp2 = (unsigned short)(a%BASE);

	return OWPTime_new(0, 0, tmp1, tmp2, 0, 0, 0, 0, 1);
}

/*
** This function is used primarily for debugging.
*/
void
OWPTime_print(T x)
{
	int i;
	assert(x);
	
	for (i = NUM_DIGITS - 1; i >= 0; i--)
		fprintf(stderr, "%hu ", x->digits[i]);
	fprintf(stderr, "\n");
}

/*
** Destructor function.
*/
void
OWPTime_destroy(T x)
{
	free(x);
}

static
int owp_overflow_happened(unsigned short a, unsigned short b, unsigned short c)
{
	return (c < a || c < b);
}

/*
** The next two functions implement the basic arithmetic
** operation in the OWPTime space. The result is saved
** in the variable z.
*/
void OWPTime_add(T x, T y, T z)
{
	int i;
	unsigned short carry = 0;

	assert(x); assert(y); assert(z);
	for (i = 0; i < NUM_DIGITS; i++){
		fprintf(stderr, "DEBUG: adding digits %lu and %lu\n",
			x->digits[i], y->digits[i]);
		/* carry = 0 or 1 */
		z->digits[i] = x->digits[i] + y->digits[i];

		if (owp_overflow_happened(x->digits[i], y->digits[i], 
					  z->digits[i])){
			z->digits[i] += carry;
			carry = 1;
		} else {
			if (!carry)
				continue;
			z->digits[i] += carry;

			/* now only need to update carry */
			if (z->digits[i] != 0)
				carry = 0;
		}

		fprintf(stderr, "DEBUG: getting z =  %lu\n", z->digits[i]);
	}
}

void 
OWPTime_mul(T x, T y, T z)
{
	int i, j;
	unsigned short tmp[(2*NUM_DIGITS)];
	
	assert(x); assert(y); assert(z);
	for (i = 0; i < (2*NUM_DIGITS); i++)
		tmp[i] = 0UL;

	for (i = 0; i < NUM_DIGITS; i++){
		unsigned short int carry = 0;
		for (j = 0; j < NUM_DIGITS; j++){
			carry += x->digits[i]*y->digits[j] + tmp[i+j];
			tmp[i+j] = carry%BASE;
			carry /= BASE;
		}

		for ( ; j < (2*NUM_DIGITS) - i; j++){
			carry += tmp[i+j];
			tmp[i+j] = carry%BASE;
			carry /= BASE;
		}
	}

	/* Need to shift by NUM_DIGITS/2 digits now */
	for (i = 0; i < NUM_DIGITS; i++)
		z->digits[i] = tmp[i+(NUM_DIGITS/2)];
}

/*
** Discussion: abcd.efgh = A.B, where LHS is in base 2^16,
** 0 <= A <= 2^32 - 1, 0 <= B <= 2^24 - 1, then
** A = c*(2^16) + d, and (multiplying fractional parts by 2^24)
** B = e*(2^8) + f/(2^8) [commit to always rounding down - shifting]
*/
void
OWPTime2Formatted(T from, OWPFormattedTime to)
{
	to->t[0] = (unsigned long)(from->digits[5]) << 16 + from->digits[4];
	to->t[1] = (unsigned long)(from->digits[3]) << 8 
		+ (unsigned long)(from->digits[2])>> 8;
}

void
OWPFormatted2Time(OWPFormattedTime from, T to)
{
	to->digits[7] = to->digits[6] = 0;
	to->digits[5] = (unsigned short)(from->t[0]/BASE);
	to->digits[4] = (unsigned short)(from->t[0]%BASE);

	/* the fractional part has been left-shifted by 8 bits already */
	to->digits[3] = (unsigned short)(from->t[1]/BASE);
	to->digits[2] = (unsigned short)(from->t[1]%BASE);
	to->digits[1] = to->digits[0] = 0;
}

void OWPTime2timeval(T from, struct timeval *to)
{
	/* first convert the fractional part */
	unsigned short a = from->digits[3];
	unsigned short b = from->digits[2];
	T tmp1 = OWPTime_new(0, 0, 0, 0, a, b, 0, 0, 1);
	T tmp2 = OWPTime_from_ulong(1000000);
	T res = OWPTime_new(0, 0, 0, 0, 0, 0, 0, 0, 0);

	OWPTime_mul(tmp1, tmp2, res);
	to->tv_usec = res->digits[4];

	/* now the integer part */
	to->tv_sec = (unsigned long)(from->digits[5]) << 16 + from->digits[4];

	OWPTime_destroy(tmp1);
	OWPTime_destroy(tmp2);
	OWPTime_destroy(res);
}

void OWPtimeval2Time(struct timeval *from, T to)
{

}
