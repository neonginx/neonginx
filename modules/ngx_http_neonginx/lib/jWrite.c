//
// jWrite.c		version 1v2
//
// A *really* simple JSON writer in C
//
// see: jWrite.h for info
//
// TonyWilk, Mar 2015
//
#define _CRT_SECURE_NO_WARNINGS		// stop complaining about deprecated functions

#include <stddef.h>
#include <stdio.h>
#include <string.h>	// memset()

//
// jWrite.h
//
// A *really* simple JSON writer in C  (C89)
// - a collection of functions to generate JSON semi-automatically
//
// The idea is to simplify writing native C values into a JSON string and
// to provide some error trapping to ensure that the result is valid JSON.
//
// Example:
//		jwOpen( buffer, buflen, JW_OBJECT, JW_PRETTY );		// open root node as object
//		jwObj_string( "key", "value" );
//		jwObj_int( "int", 1 );
//		jwObj_array( "anArray");
//			jwArr_int( 0 );
//			jwArr_int( 1 );
//			jwArr_int( 2 );
//		jwEnd();
//		err= jwClose();								// close root object
//
// results in:
//
//		{
//		    "key": "value",
//		    "int": 1,
//		    "anArray": [
//		        0,
//		        1,
//		        2
//		    ]
//		}
//
// Note that jWrite handles string quoting and getting commas in the right place.
// If the sequence of calls is incorrect
// e.g.
//		jwOpen( buffer, buflen, JW_OBJECT, 1 );
//		jwObj_string( "key", "value" );
//			jwArr_int( 0 );
//      ...
//
// then the error code returned from jwClose() would indicate that you attempted to
// put an array element into an object (instead of a key:value pair)
// To locate the error, the supplied buffer has the JSON created upto the error point
// and a call to jwErrorPos() would return the function call at which the error occurred
// - in this case 3, the 3rd function call "jwArr_int(0)" is not correct at this point.
//
// The root JSON type can be JW_OBJECT or JW_ARRAY.
//
// For more information on each function, see the prototypes below.
//
//
// GLOBAL vs. Application-Supplied Control Structure
// -------------------------------------------------
// jWrite requires a jWriteControl structure to save the internal state.
// For many applications it is much simpler for this to be a global variable as
// used by the above examples.
//
// To use multiple instances of jWrite, an application has to supply unique instances
// of jWriteControl structures. 
//
// This feature is enabled by commenting out the definition of JW_GLOBAL_CONTROL_STRUCT
//
// All the jWrite functions then take an additional parameter: a ptr to the structure
// e.g.
//		struct jWriteControl jwc;
//
//		jwOpen( &jwc, buffer, buflen, JW_OBJECT, 1 );
//		jwObj_string( &jwc, "key", "value" );
//		jwObj_int( &jwc, "int", 1 );
//		jwObj_array( &jwc, "anArray");
//			jwArr_int( &jwc, 0 );
//			jwArr_int( &jwc, 1 );
//			jwArr_int( &jwc, 2 );
//		jwEnd( &jwc );
//		err= jwClose( &jwc );
//
// - which is more flexible, but a pain to type in !
//
// TonyWilk, Mar 2015
//
// 
// #define JW_GLOBAL_CONTROL_STRUCT	// <--- comment this out to use applic-supplied jWriteControl

#define JWRITE_STACK_DEPTH 32			// max nesting depth of objects/arrays

#define JW_COMPACT	0					// output string control for jwOpen()
#define JW_PRETTY	1					// pretty adds \n and indentation

enum jwNodeType{
	JW_OBJECT= 1,
	JW_ARRAY
};

struct jwNodeStack{
	enum jwNodeType nodeType;
	int elementNo;
};

struct jWriteControl{
	char *buffer;						// pointer to application's buffer
	unsigned int buflen;				// length of buffer
	char *bufp;							// current write position in buffer
	char tmpbuf[32];					// local buffer for int/double convertions
	int error;							// error code
	int callNo;							// API call on which error occurred
	struct jwNodeStack nodeStack[JWRITE_STACK_DEPTH];	// stack of array/object nodes
	int stackpos;
	int isPretty;						// 1= pretty output (inserts \n and spaces)
};

// Error Codes
// -----------
#define JWRITE_OK           0
#define JWRITE_BUF_FULL     1		// output buffer full
#define JWRITE_NOT_ARRAY	2		// tried to write Array value into Object
#define JWRITE_NOT_OBJECT	3		// tried to write Object key/value into Array
#define JWRITE_STACK_FULL	4		// array/object nesting > JWRITE_STACK_DEPTH
#define JWRITE_STACK_EMPTY	5		// stack underflow error (too many 'end's)
#define JWRITE_NEST_ERROR	6		// nesting error, not all objects closed when jwClose() called


// API functions
// -------------

// Returns '\0'-termianted string describing the error (as returned by jwClose())
//
char *jwErrorToString( int err );


#ifdef JW_GLOBAL_CONTROL_STRUCT		/* USING GLOBAL g_jWriteControl */

// jwOpen
// - initialises jWrite with the application supplied 'buffer' of length 'buflen'
//   in operation, the buffer will always contain a valid '\0'-terminated string
// - jWrite will not overrun the buffer (it returns an "output buffer full" error)
// - rootType is the base JSON type: JW_OBJECT or JW_ARRAY
// - isPretty controls 'prettifying' the output: JW_PRETTY or JW_COMPACT
void jwOpen( char *buffer, unsigned int buflen, enum jwNodeType rootType, int isPretty );

// jwClose
// - closes the element opened by jwOpen()
// - returns error code (0 = JWRITE_OK)
// - after an error, all following jWrite calls are skipped internally
//   so the error code is for the first error detected
int jwClose( );

// jwErrorPos
// - if jwClose returned an error, this function returns the number of the jWrite function call
//   which caused that error.
int jwErrorPos( );

// Object insertion functions
// - used to insert "key":"value" pairs into an object
//
void jwObj_string( char *key, char *value, size_t len );
void jwObj_int( char *key, unsigned long value );
void jwObj_double( char *key, double value );
void jwObj_bool( char *key, int oneOrZero );
void jwObj_null( char *key );
void jwObj_object( char *key );
void jwObj_array( char *key );

// Array insertion functions
// - used to insert "value" elements into an array
//
void jwArr_string( char *value, size_t len );
void jwArr_int( unsigned long value );
void jwArr_double( double value );
void jwArr_bool( int oneOrZero );
void jwArr_null( );
void jwArr_object( );
void jwArr_array( );

// jwEnd
// - defines the end of an Object or Array definition
int jwEnd( );


// these 'raw' routines write the JSON value as the contents of rawtext
// i.e. enclosing quotes are not added
// - use if your app. supplies its own value->string functions
//
void jwObj_raw( char *key, char *rawtext );
void jwArr_raw( char *rawtext );

#else  /* JW_GLOBAL_CONTROL_STRUCT not defined */
// Same API functions with app-supplied control struct option
//
void jwOpen( struct jWriteControl *jwc, char *buffer, unsigned int buflen, enum jwNodeType rootType, int isPretty );
int jwClose( struct jWriteControl *jwc );
int jwErrorPos( struct jWriteControl *jwc );
void jwObj_string( struct jWriteControl *jwc, char *key, char *value, size_t len );
void jwObj_int( struct jWriteControl *jwc, char *key, unsigned long value );
void jwObj_double( struct jWriteControl *jwc, char *key, double value );
void jwObj_bool( struct jWriteControl *jwc, char *key, int oneOrZero );
void jwObj_null( struct jWriteControl *jwc, char *key );
void jwObj_object( struct jWriteControl *jwc, char *key );
void jwObj_array( struct jWriteControl *jwc, char *key );
void jwArr_string( struct jWriteControl *jwc, char *value, size_t len );
void jwArr_int( struct jWriteControl *jwc, unsigned long value );
void jwArr_double( struct jWriteControl *jwc, double value );
void jwArr_bool( struct jWriteControl *jwc, int oneOrZero );
void jwArr_null( struct jWriteControl *jwc );
void jwArr_object( struct jWriteControl *jwc );
void jwArr_array( struct jWriteControl *jwc );
int jwEnd( struct jWriteControl *jwc );
void jwObj_raw( struct jWriteControl *jwc, char *key, char *rawtext );
void jwArr_raw( struct jWriteControl *jwc, char *rawtext );

#endif /* JW_GLOBAL_CONTROL_STRUCT */


/* end of jWrite.h */



//#include <stdint.h>			// definintion of uint32_t, int32_t
typedef unsigned int uint32_t;
typedef int int32_t;


// the jWrite functions take the above jWriteControl structure pointer
// to maintain state while writing a JSON string.
//
// You can opt to use a single global instance of a jWriteControl structure
// which simplifies the function parameters or to supply your own structure
//
#ifdef JW_GLOBAL_CONTROL_STRUCT
struct jWriteControl g_jWriteControl;			// global control struct
#define JWC_DECL								// function parameter decl is empty
#define JWC_DECL0
#define JWC(x) g_jWriteControl.x				// functions access global
#define JWC_PARAM								// pointer to struct is empty
#define JWC_PARAM0
#else
#define JWC_DECL struct jWriteControl *jwc,		// function parameter is ptr to control struct
#define JWC_DECL0 struct jWriteControl *jwc		// function parameter, no params
#define JWC(x) jwc->x							// functions use pointer
#define JWC_PARAM jwc,							// pointer to stuct
#define JWC_PARAM0 jwc							// pointer to stuct, no params
#endif

//------------------------------------------
// Internal functions
//
void jwPutch( JWC_DECL char c );
void jwPutstr( JWC_DECL char *str, size_t len, unsigned int length_check);
void jwPutraw( JWC_DECL char *str );
void modp_itoa10(unsigned long value, char* str);
void modp_dtoa2(double value, char* str, int prec);
void jwPretty( JWC_DECL0 );
enum jwNodeType jwPop( JWC_DECL0 );
void jwPush( JWC_DECL enum jwNodeType nodeType );


//------------------------------------------
// jwOpen
// - open writing of JSON starting with rootType = JW_OBJECT or JW_ARRAY
// - initialise with user string buffer of length buflen
// - isPretty=JW_PRETTY adds \n and spaces to prettify output (else JW_COMPACT)
//
void jwOpen( JWC_DECL char *buffer, unsigned int buflen, 
				   enum jwNodeType rootType, int isPretty )
{
	memset( buffer, 0, buflen );	// zap the whole destination buffer
	JWC(buffer)= buffer;
	JWC(buflen)= buflen;
	JWC(bufp)= buffer;
	JWC(nodeStack)[0].nodeType= rootType;
	JWC(nodeStack)[0].elementNo= 0;
	JWC(stackpos)=0;
	JWC(error)= JWRITE_OK;
	JWC(callNo)= 1;
	JWC(isPretty)= isPretty;
	jwPutch( JWC_PARAM (rootType==JW_OBJECT) ? '{' : '[' );
}

//------------------------------------------
// jwClose
// - closes the root JSON object started by jwOpen()
// - returns error code
//
int jwClose( JWC_DECL0 )
{
	if( JWC(error) == JWRITE_OK )
	{
		if( JWC(stackpos) == 0 )
		{
			enum jwNodeType node= JWC(nodeStack)[0].nodeType;
			if( JWC(isPretty) )
				jwPutch( JWC_PARAM '\n' );
			jwPutch( JWC_PARAM (node == JW_OBJECT) ? '}' : ']');
		}else{
			JWC(error)= JWRITE_NEST_ERROR;	// nesting error, not all objects closed when jwClose() called
		}
	}
	return JWC(error);
}

//------------------------------------------
// End the current array/object
//
int jwEnd( JWC_DECL0 )
{
	if( JWC(error) == JWRITE_OK )
	{
		enum jwNodeType node;
		int lastElemNo= JWC(nodeStack)[JWC(stackpos)].elementNo;
		node= jwPop( JWC_PARAM0 );
		if( lastElemNo > 0 )
			jwPretty( JWC_PARAM0 );
		jwPutch( JWC_PARAM (node == JW_OBJECT) ? '}' : ']');
	}
	return JWC(error);
}


//------------------------------------------
// jwErrorPos
// - Returns position of error: the nth call to a jWrite function
//
int jwErrorPos( JWC_DECL0 )
{
	return JWC(callNo);
}


//------------------------------------------
// Object insert functions
//
int _jwObj( JWC_DECL char *key, size_t len );

// put raw string to object (i.e. contents of rawtext without quotes)
//
void jwObj_raw( JWC_DECL char *key, char *rawtext )
{
	if(_jwObj( JWC_PARAM key, 0 ) == JWRITE_OK)
		jwPutraw( JWC_PARAM rawtext);
}

// put "quoted" string to object
//
void jwObj_string( JWC_DECL char *key, char *value, size_t len )
{
	if(_jwObj( JWC_PARAM key, len ) == JWRITE_OK)
		jwPutstr( JWC_PARAM value, len, 1);
}

void jwObj_int( JWC_DECL char *key, unsigned long value )
{
	modp_itoa10( value, JWC(tmpbuf) );
	jwObj_raw( JWC_PARAM key, JWC(tmpbuf) );
}

void jwObj_double( JWC_DECL char *key, double value )
{
	modp_dtoa2( value, JWC(tmpbuf), 6 );
	jwObj_raw( JWC_PARAM key, JWC(tmpbuf) );
}

void jwObj_bool( JWC_DECL char *key, int oneOrZero )
{
	jwObj_raw( JWC_PARAM key, (oneOrZero) ? "true" : "false" );
}

void jwObj_null( JWC_DECL char *key )
{
	jwObj_raw( JWC_PARAM key, "null" );
}

// put Object in Object
//
void jwObj_object( JWC_DECL char *key )
{
	if(_jwObj( JWC_PARAM key, 0) == JWRITE_OK)
	{
		jwPutch( JWC_PARAM '{' );
		jwPush( JWC_PARAM JW_OBJECT );
	}
}

// put Array in Object
//
void jwObj_array( JWC_DECL char *key )
{
	if(_jwObj( JWC_PARAM key, 0) == JWRITE_OK)
	{
		jwPutch( JWC_PARAM '[' );
		jwPush( JWC_PARAM JW_ARRAY );
	}
}

//------------------------------------------
// Array insert functions
//
int _jwArr( JWC_DECL0 );

// put raw string to array (i.e. contents of rawtext without quotes)
//
void jwArr_raw( JWC_DECL char *rawtext )
{
	if(_jwArr( JWC_PARAM0 ) == JWRITE_OK)
		jwPutraw( JWC_PARAM rawtext);
}

// put "quoted" string to array
//
void jwArr_string( JWC_DECL char *value, size_t len )
{
	if(_jwArr( JWC_PARAM0 ) == JWRITE_OK)
		jwPutstr( JWC_PARAM value, len, 1);
}

void jwArr_int( JWC_DECL unsigned long value )
{
	modp_itoa10( value, JWC(tmpbuf) );
	jwArr_raw( JWC_PARAM JWC(tmpbuf) );
}

void jwArr_double( JWC_DECL double value )
{
	modp_dtoa2( value, JWC(tmpbuf), 6 );
	jwArr_raw( JWC_PARAM JWC(tmpbuf) );
}

void jwArr_bool( JWC_DECL int oneOrZero )
{
	jwArr_raw( JWC_PARAM  (oneOrZero) ? "true" : "false" );
}

void jwArr_null( JWC_DECL0 )
{
	jwArr_raw( JWC_PARAM  "null" );
}

void jwArr_object( JWC_DECL0 )
{
	if(_jwArr( JWC_PARAM0 ) == JWRITE_OK)
	{
		jwPutch( JWC_PARAM '{' );
		jwPush( JWC_PARAM JW_OBJECT );
	}
}

void jwArr_array( JWC_DECL0 )
{
	if(_jwArr( JWC_PARAM0 ) == JWRITE_OK)
	{
		jwPutch( JWC_PARAM '[' );
		jwPush( JWC_PARAM JW_ARRAY );
	}
}


//------------------------------------------
// jwErrorToString
// - returns string describing error code
//
char *jwErrorToString( int err )
{
	switch( err )
	{
	case JWRITE_OK:         return "OK"; 
	case JWRITE_BUF_FULL:   return "output buffer full";
	case JWRITE_NOT_ARRAY:	return "tried to write Array value into Object";
	case JWRITE_NOT_OBJECT:	return "tried to write Object key/value into Array";
	case JWRITE_STACK_FULL:	return "array/object nesting > JWRITE_STACK_DEPTH";
	case JWRITE_STACK_EMPTY:return "stack underflow error (too many 'end's)";
	case JWRITE_NEST_ERROR:	return "nesting error, not all objects closed when jwClose() called";
	}
	return "Unknown error";
}

//============================================================================
// Internal functions
//
void jwPretty( JWC_DECL0 )
{
	int i;
	if( JWC(isPretty) )
	{
		jwPutch( JWC_PARAM '\n' );
		for( i=0; i<JWC(stackpos)+1; i++ )
			jwPutraw( JWC_PARAM "    " );
	}
}

// Push / Pop node stack
//
void jwPush( JWC_DECL enum jwNodeType nodeType )
{
	if( (JWC(stackpos)+1) >= JWRITE_STACK_DEPTH )
		JWC(error)= JWRITE_STACK_FULL;		// array/object nesting > JWRITE_STACK_DEPTH
	else
	{
		JWC(nodeStack[++JWC(stackpos)]).nodeType= nodeType;
		JWC(nodeStack[JWC(stackpos)]).elementNo= 0;
	}
}

enum jwNodeType jwPop( JWC_DECL0 )
{
	enum jwNodeType retval= JWC(nodeStack[JWC(stackpos)]).nodeType;
	if( JWC(stackpos) == 0 )
		JWC(error)= JWRITE_STACK_EMPTY;		// stack underflow error (too many 'end's)
	else
		JWC(stackpos)--;
	return retval;
}

void jwPutch( JWC_DECL char c )
{
	if( (unsigned int)(JWC(bufp) - JWC(buffer)) >= JWC(buflen) )
	{
		JWC(error)= JWRITE_BUF_FULL;
	}else{
		*JWC(bufp)++ = c;
	}
}

// put string enclosed in quotes
//
void jwPutstr( JWC_DECL char *str, size_t len, unsigned int length_check )
{
	size_t contatore = 0;
	jwPutch( JWC_PARAM '\"' );
	while( *str != '\0' && (contatore < len || !length_check)){
		jwPutch( JWC_PARAM *str++ );
		contatore++;
	}
	jwPutch( JWC_PARAM '\"' );
}

// put raw string
//
void jwPutraw( JWC_DECL char *str )
{
	while( *str != '\0' )
		jwPutch( JWC_PARAM *str++ );
}


// *common Object function*
// - checks error
// - checks current node is OBJECT
// - adds comma if reqd
// - adds "key" :
//
int _jwObj( JWC_DECL char *key, size_t len )
{
	if(JWC(error) == JWRITE_OK)
	{
		JWC(callNo)++;
		if( JWC(nodeStack)[JWC(stackpos)].nodeType != JW_OBJECT )
			JWC(error)= JWRITE_NOT_OBJECT;			// tried to write Object key/value into Array
		else if( JWC(nodeStack)[JWC(stackpos)].elementNo++ > 0 )
			jwPutch( JWC_PARAM ',' );
		jwPretty( JWC_PARAM0 );
		jwPutstr( JWC_PARAM key, len, 0);
		jwPutch( JWC_PARAM ':' );
		if( JWC(isPretty) )
			jwPutch( JWC_PARAM ' ' );
	}
	return JWC(error);
}

// *common Array function*
// - checks error
// - checks current node is ARRAY
// - adds comma if reqd
//
int _jwArr( JWC_DECL0 )
{
	if(JWC(error) == JWRITE_OK)
	{
		JWC(callNo)++;
		if( JWC(nodeStack)[JWC(stackpos)].nodeType != JW_ARRAY )
			JWC(error)= JWRITE_NOT_ARRAY;			// tried to write array value into Object
		else if( JWC(nodeStack)[JWC(stackpos)].elementNo++ > 0 )
			jwPutch( JWC_PARAM ',' );
		jwPretty( JWC_PARAM0 );
	}
	return JWC(error);
}

//=================================================================
//
// modp value-to-string functions
// - modified for C89
//
// We use these functions as they are a lot faster than sprintf()
//
// Origin of these routines:
/*
 * <pre>
 * Copyright &copy; 2007, Nick Galbreath -- nickg [at] modp [dot] com
 * All rights reserved.
 * http://code.google.com/p/stringencoders/
 * Released under the bsd license.
 * </pre>
 */

static void strreverse(char* begin, char* end)
{
    char aux;
    while (end > begin)
        aux = *end, *end-- = *begin, *begin++ = aux;
}

/** \brief convert an signed integer to char buffer
 *
 * \param[in] value
 * \param[out] buf the output buffer.  Should be 16 chars or more.
 */
void modp_itoa10(unsigned long value, char* str)
{
    char* wstr=str;
    // Take care of sign
    unsigned long uvalue = (value < 0) ? -value : value;
    // Conversion. Number is reversed.
    do *wstr++ = (char)(48 + (uvalue % 10)); while(uvalue /= 10);
    if (value < 0) *wstr++ = '-';
    *wstr='\0';

    // Reverse string
    strreverse(str,wstr-1);
}

/**
 * Powers of 10
 * 10^0 to 10^9
 */
static const double pow10[] = {1, 10, 100, 1000, 10000, 100000, 1000000,
                               10000000, 100000000, 1000000000};

/** \brief convert a floating point number to char buffer with a
 *         variable-precision format, and no trailing zeros
 *
 * This is similar to "%.[0-9]f" in the printf style, except it will
 * NOT include trailing zeros after the decimal point.  This type
 * of format oddly does not exists with printf.
 *
 * If the input value is greater than 1<<31, then the output format
 * will be switched exponential format.
 *
 * \param[in] value
 * \param[out] buf  The allocated output buffer.  Should be 32 chars or more.
 * \param[in] precision  Number of digits to the right of the decimal point.
 *    Can only be 0-9.
 */
void modp_dtoa2(double value, char* str, int prec)
{
    /* if input is larger than thres_max, revert to exponential */
    const double thres_max = (double)(0x7FFFFFFF);
    int count;
    double diff = 0.0;
    char* wstr = str;
	int neg= 0;
	int whole;
	double tmp;
	uint32_t frac;

    /* Hacky test for NaN
     * under -fast-math this won't work, but then you also won't
     * have correct nan values anyways.  The alternative is
     * to link with libmath (bad) or hack IEEE double bits (bad)
     */
    if (! (value == value)) {
        str[0] = 'n'; str[1] = 'a'; str[2] = 'n'; str[3] = '\0';
        return;
    }

    if (prec < 0) {
        prec = 0;
    } else if (prec > 9) {
        /* precision of >= 10 can lead to overflow errors */
        prec = 9;
    }

    /* we'll work in positive values and deal with the
       negative sign issue later */
    if (value < 0) {
        neg = 1;
        value = -value;
    }


    whole = (int) value;
    tmp = (value - whole) * pow10[prec];
    frac = (uint32_t)(tmp);
    diff = tmp - frac;

    if (diff > 0.5) {
        ++frac;
        /* handle rollover, e.g.  case 0.99 with prec 1 is 1.0  */
        if (frac >= pow10[prec]) {
            frac = 0;
            ++whole;
        }
    } else if (diff == 0.5 && ((frac == 0) || (frac & 1))) {
        /* if halfway, round up if odd, OR
           if last digit is 0.  That last part is strange */
        ++frac;
    }

    /* for very large numbers switch back to native sprintf for exponentials.
       anyone want to write code to replace this? */
    /*
      normal printf behavior is to print EVERY whole number digit
      which can be 100s of characters overflowing your buffers == bad
    */
    if (value > thres_max) {
        sprintf(str, "%e", neg ? -value : value);
        return;
    }

    if (prec == 0) {
        diff = value - whole;
        if (diff > 0.5) {
            /* greater than 0.5, round up, e.g. 1.6 -> 2 */
            ++whole;
        } else if (diff == 0.5 && (whole & 1)) {
            /* exactly 0.5 and ODD, then round up */
            /* 1.5 -> 2, but 2.5 -> 2 */
            ++whole;
        }

        //vvvvvvvvvvvvvvvvvvv  Diff from modp_dto2
    } else if (frac) {
        count = prec;
        // now do fractional part, as an unsigned number
        // we know it is not 0 but we can have leading zeros, these
        // should be removed
        while (!(frac % 10)) {
            --count;
            frac /= 10;
        }
        //^^^^^^^^^^^^^^^^^^^  Diff from modp_dto2

        // now do fractional part, as an unsigned number
        do {
            --count;
            *wstr++ = (char)(48 + (frac % 10));
        } while (frac /= 10);
        // add extra 0s
        while (count-- > 0) *wstr++ = '0';
        // add decimal
        *wstr++ = '.';
    }

    // do whole part
    // Take care of sign
    // Conversion. Number is reversed.
    do *wstr++ = (char)(48 + (whole % 10)); while (whole /= 10);
    if (neg) {
        *wstr++ = '-';
    }
    *wstr='\0';
    strreverse(str, wstr-1);
}
//=================================================================

/* end of jWrite.c */
