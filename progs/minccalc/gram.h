/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     NAN = 258,
     IN = 259,
     TO = 260,
     IDENT = 261,
     REAL = 262,
     AVG = 263,
     PROD = 264,
     SUM = 265,
     LET = 266,
     NEG = 267,
     LEN = 268,
     MAX = 269,
     MIN = 270,
     IMAX = 271,
     IMIN = 272,
     ISNAN = 273,
     SQRT = 274,
     ABS = 275,
     EXP = 276,
     LOG = 277,
     SIN = 278,
     COS = 279,
     TAN = 280,
     ASIN = 281,
     ACOS = 282,
     ATAN = 283,
     CLAMP = 284,
     SEGMENT = 285,
     LT = 286,
     LE = 287,
     GT = 288,
     GE = 289,
     EQ = 290,
     NE = 291,
     NOT = 292,
     AND = 293,
     OR = 294,
     IF = 295,
     ELSE = 296,
     FOR = 297
   };
#endif
/* Tokens.  */
#define NAN 258
#define IN 259
#define TO 260
#define IDENT 261
#define REAL 262
#define AVG 263
#define PROD 264
#define SUM 265
#define LET 266
#define NEG 267
#define LEN 268
#define MAX 269
#define MIN 270
#define IMAX 271
#define IMIN 272
#define ISNAN 273
#define SQRT 274
#define ABS 275
#define EXP 276
#define LOG 277
#define SIN 278
#define COS 279
#define TAN 280
#define ASIN 281
#define ACOS 282
#define ATAN 283
#define CLAMP 284
#define SEGMENT 285
#define LT 286
#define LE 287
#define GT 288
#define GE 289
#define EQ 290
#define NE 291
#define NOT 292
#define AND 293
#define OR 294
#define IF 295
#define ELSE 296
#define FOR 297




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 15 "gram.y"
{
int      pos;
node_t   node;
double   real;
ident_t  ident;
}
/* Line 1529 of yacc.c.  */
#line 140 "gram.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

