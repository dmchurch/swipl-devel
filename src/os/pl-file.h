/*  Part of SWI-Prolog

    Author:        Jan Wielemaker
    E-mail:        J.Wielemaker@vu.nl
    WWW:           http://www.swi-prolog.org
    Copyright (c)  2011-2020, University of Amsterdam
			      CWI, Amsterdam
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in
       the documentation and/or other materials provided with the
       distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
    ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef PL_FILE_H_INCLUDED
#define PL_FILE_H_INCLUDED

typedef enum
{ ST_FALSE = -1,			/* Do not check stream types */
  ST_LOOSE = 0,				/* Default: accept latin-1 for binary */
  ST_TRUE  = 1				/* Strict checking */
} st_check;

typedef enum iri_op
{ IRI_OPEN,		/* const char *how, term_t options -> IOSTREAM **s */
  IRI_ACCESS,		/* const char *mode -> bool */
  IRI_SIZE,		/* -> int64_t *sz */
  IRI_TIME		/* -> double *time */
} iri_op;

/* pl-file.c */
COMMON(void)		initIO(void);
COMMON(void)		dieIO(void);
COMMON(void)		closeFiles(int all);
COMMON(int)		openFileDescriptors(unsigned char *buf, int size);
COMMON(void)		protocol(const char *s, size_t n);
COMMON(int)		getTextInputStream__LD(term_t t, IOSTREAM **s ARG_LD);
COMMON(int)		getBinaryInputStream__LD(term_t t, IOSTREAM **s ARG_LD);
COMMON(int)		getTextOutputStream__LD(term_t t, IOSTREAM **s ARG_LD);
COMMON(int)		getBinaryOutputStream__LD(term_t t, IOSTREAM **s ARG_LD);
COMMON(int)	        reportStreamError(IOSTREAM *s);
COMMON(int)		streamStatus(IOSTREAM *s);
COMMON(int)		setFileNameStream(IOSTREAM *s, atom_t name);
COMMON(atom_t)		fileNameStream(IOSTREAM *s);
COMMON(int)		getSingleChar(IOSTREAM *s, int signals);
COMMON(int)		readLine(IOSTREAM *in, IOSTREAM *out, char *buffer);
COMMON(int)		LockStream(void);
COMMON(int)		UnlockStream(void);
COMMON(IOSTREAM *)	PL_current_input(void);
COMMON(IOSTREAM *)	PL_current_output(void);
COMMON(int)		pl_see(term_t f);
COMMON(int)		pl_seen(void);
COMMON(int)		seeString(const char *s);
COMMON(int)		seeingString(void);
COMMON(int)		seenString(void);
COMMON(int)		tellString(char **s, size_t *size, IOENC enc);
COMMON(int)		toldString(void);
COMMON(void)		prompt1(atom_t prompt);
COMMON(atom_t)		PrologPrompt(void);
COMMON(int)		streamNo(term_t spec, int mode);
COMMON(void)		release_stream_handle(term_t spec);
COMMON(int)		unifyTime(term_t t, time_t time);
#ifdef __WINDOWS__
COMMON(word)		pl_make_fat_filemap(term_t dir);
#endif
COMMON(int)		PL_unify_stream_or_alias(term_t t, IOSTREAM *s);
COMMON(void)		pushOutputContext(void);
COMMON(void)		popOutputContext(void);
COMMON(IOENC)		atom_to_encoding(atom_t a);
COMMON(atom_t)		encoding_to_atom(IOENC enc);
COMMON(int)		setupOutputRedirect(term_t to,
					    redir_context *ctx,
					    int redir);
COMMON(int)		closeOutputRedirect(redir_context *ctx);
COMMON(void)		discardOutputRedirect(redir_context *ctx);
COMMON(int)		push_input_context(atom_t type);
COMMON(int)		pop_input_context(void);
COMMON(int)		stream_encoding_options(atom_t type, atom_t encoding,
						int *bom, IOENC *enc);
COMMON(int)		file_name_is_iri(const char *path);
COMMON(int)		iri_hook(const char *url, iri_op op, ...);
COMMON(atom_t)		file_name_to_atom(const char *fn);

		 /*******************************
		 *	LD-USING FUNCTIONS	*
		 *******************************/

#define getTextInputStream(t, s)	getTextInputStream__LD(t, s PASS_LD)
#define getBinaryInputStream(t, s)	getBinaryInputStream__LD(t, s PASS_LD)
#define getTextOutputStream(t, s)	getTextOutputStream__LD(t, s PASS_LD)
#define getBinaryOutputStream(t, s)	getBinaryOutputStream__LD(t, s PASS_LD)

#endif /*PL_FILE_H_INCLUDED*/
