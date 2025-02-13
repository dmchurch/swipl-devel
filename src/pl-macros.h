/* File: pl-macros.h
   
   This file contains a number of utility macros to make it easier (and
   more reliable) to do macro token manipulation. Nothing in this file
   is Prolog-specific. Macros with an underscore prefix are helpers
   internal to this file. Note: while the «, ## __VA_ARGS__» nonstandard
   comma-swallowing operator is in fact supported by all modern compilers,
   the (args...) extension to provide a descriptive name for __VA_ARGS__
   is not.
*/

#ifndef _PL_MACROS_H
#define _PL_MACROS_H

/* T_: constant tokens */
#define T_EMPTY(...)			/* empty */
#define T_COMMA(...)			,
#define T_SEMICOLON(...)		;
#define T_OPEN_PAREN(...)		(
#define T_CLOSE_PAREN(...)		)
#define T_PARENTHESES(...)		()

/* A_: Argument manipulation */
#define A_ARGN(n,...)			A_ARG ## n(__VA_ARGS__)
#define A_ARG0(a0,...)			a0
#define A_ARG1(a0,a1,...)		a1
#define A_ARG2(a0,a1,a2,...)		a2
#define A_ARG9(a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,...) a9
#define A_SHIFT1(a0, ...)		__VA_ARGS__
#define __A_COUNT(...)			A_ARG9(__VA_ARGS__)
#define _A_COUNT(...)			__A_COUNT(A_SHIFT1(~, ## __VA_ARGS__, 9,8,7,6,5,4,3,2,1,0))
#define A_COUNT(...)			_A_COUNT(__VA_ARGS__)
#define A_CALL(f,...)			f(__VA_ARGS__)
#define A_ECHO(...)			__VA_ARGS__
#define A_UNWRAP(list)			A_ECHO list
#define A_IGNORE(...)			/* empty */
#define A_STRINGIFY(...)		_A_STRINGIFY(__VA_ARGS__)
#define _A_STRINGIFY(...)		#__VA_ARGS__
#define A_PASTE(a0,a1)			_A_PASTE(a0,a1)
#define _A_PASTE(a0,a1)			a0 ## a1
#define A_LEADING_COMMA(...)		_A_LEADING_COMMA(__VA_ARGS__)
#define A_TRAILING_COMMA(...)		_A_TRAILING_COMMA(__VA_ARGS__)
#define _A_LEADING_COMMA(...)		, ## __VA_ARGS__
#define _A_TRAILING_COMMA(...)		A_SHIFT1(~, ## __VA_ARGS__,)
#define A_ISEMPTY(a)			A_ARGN(2, ~ A_LEADING_COMMA(a),0,1)
#define A_ISPRESENT(a)			A_ARGN(2, ~ A_LEADING_COMMA(a),1,0)

/* M_: Metaprogramming and control macros */
#define M_EMPTYIF0(a)			A_ARGN(1,A_PASTE(_M_EMPTYIF0_, a),a)
#define M_PRESENTIF0(a)			A_ARGN(2,A_PASTE(_M_EMPTYIF0_, a),a,)
#define _M_EMPTYIF0_0			~,
#define M_ISPAREN(a,...)		A_ARGN(2,_M_ISPAREN a, 1, 0)
#define M_ISBARE(a,...)			A_ARGN(2,_M_ISPAREN a, 0, 1)
#define _M_ISPAREN(...)			~,~
#define M_ISEMPTY(...)			A_ISEMPTY(A_ARGN(0,T_EMPTY A_LEADING_COMMA(__VA_ARGS__) A_TRAILING_COMMA(__VA_ARGS__) ()))
#define M_ISPRESENT(...)		A_ISPRESENT(A_ARGN(0,T_EMPTY A_LEADING_COMMA(__VA_ARGS__) A_TRAILING_COMMA(__VA_ARGS__) ()))

#define M_NOT(p)			M_ISEMPTY(M_EMPTYIF0(p))
#define M_BOOL(p)			M_ISPRESENT(M_EMPTYIF0(p))

#define M_IF(b)				A_PASTE(_M_IF, b)
#define _M_IF1(...)			__VA_ARGS__ _M_IF1b
#define _M_IF1b(...)			/* empty */
#define _M_IF0(...)			_M_IF0b
#define _M_IF0b(...)			__VA_ARGS__
#define M_IFEMPTY(...)			M_IF(M_ISEMPTY(__VA_ARGS__))
#define M_IFPRESENT(...)		M_IF(M_ISPRESENT(__VA_ARGS__))
#define M_IFPAREN(...)			M_IF(M_ISPAREN(__VA_ARGS__))

#define M_DEFAULT(...)			M_IFPRESENT(__VA_ARGS__)(__VA_ARGS__)

#define M_TEMPLATE(ctxarg, ...)		A_ECHO(_M_TEMPLATE0(ctxarg, __VA_ARGS__))
#define _M_TEMPLATE0(ctx, lit, f, ...)	lit _M_TPLEXPAND(ctx, f, 1, ## __VA_ARGS__) (ctx, ## __VA_ARGS__)
#define _M_TEMPLATE1(ctx, lit, f, ...)	lit _M_TPLEXPAND(ctx, f, 2, ## __VA_ARGS__) (ctx, ## __VA_ARGS__)
#define _M_TEMPLATE2(ctx, lit, f, ...)	lit _M_TPLEXPAND(ctx, f, 3, ## __VA_ARGS__) (ctx, ## __VA_ARGS__)
#define _M_TEMPLATE3(ctx, lit, f, ...)	lit _M_TPLEXPAND(ctx, f, 4, ## __VA_ARGS__) (ctx, ## __VA_ARGS__)
#define _M_TEMPLATE4(ctx, lit, f, ...)	lit _M_TPLEXPAND(ctx, f, 5, ## __VA_ARGS__) (ctx, ## __VA_ARGS__)
#define _M_TEMPLATE5(ctx, lit, f, ...)	lit _M_TPLEXPAND(ctx, f, 6, ## __VA_ARGS__) (ctx, ## __VA_ARGS__)
#define _M_TEMPLATE6(ctx, lit, f, ...)	lit _M_TPLEXPAND(ctx, f, 7, ## __VA_ARGS__) (ctx, ## __VA_ARGS__)
#define _M_TEMPLATE7(ctx, lit, f, ...)	lit _M_TPLEXPAND(ctx, f, 8, ## __VA_ARGS__) (ctx, ## __VA_ARGS__)
#define _M_TEMPLATE8(ctx, lit, f, ...)	lit _M_TPLEXPAND(ctx, f, 9, ## __VA_ARGS__) (ctx, ## __VA_ARGS__)
#define _M_TEMPLATE9(ctx, lit, f, ...)	lit _M_TPLEXPAND(ctx, f, 10, ## __VA_ARGS__) (ctx, ## __VA_ARGS__)
#define _M_TEMPLATE10(ctx, lit, f, ...)	lit _M_TPLEXPAND(ctx, f, 11, ## __VA_ARGS__) (ctx, ## __VA_ARGS__)

#define _M_TPLEXPAND(ctx, f, n, ...)	M_IFPAREN(f) (A_UNWRAP(f)) (_M_TPLCALL(f)(ctx)) M_IFEMPTY(__VA_ARGS__) (A_IGNORE) (_M_TPLNEXT(ctx, n, ## __VA_ARGS__))
#define _M_TPLCALL(f)			M_DEFAULT(f)(T_COMMA)
#define _M_TPLNEXT(ctx, n, lit, ...)	M_IFEMPTY(__VA_ARGS__) (lit A_IGNORE) (_M_TEMPLATE ## n)

#endif