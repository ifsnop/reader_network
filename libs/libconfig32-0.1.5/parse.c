
/*
 * Copyright (c) Abraham vd Merwe <abz@blio.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	  notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	  notice, this list of conditions and the following disclaimer in the
 *	  documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of other contributors
 *	  may be used to endorse or promote products derived from this software
 *	  without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>

#include <debug/memory.h>
#include <debug/log.h>

#include "scan.h"
#include "parse.h"

#define ERR		-1
#define FIN		0
#define OK		1

static stmt_t *stmt_init (char **keyword,int type)
{
   stmt_t *nw;

   if ((nw = (stmt_t *) mem_alloc (sizeof (stmt_t))) == NULL)
	 {
		log_printf (LOG_ERROR,"Out of memory\n");
		return (NULL);
	 }

   nw->name = *keyword;
   nw->type = type;
   if (nw->type == TOK_KEYWORD) nw->type = TOK_ENUM;
   nw->args = NULL;
   nw->n = 0;

   return (nw);
}

static void stmt_destroy (stmt_t **stmt)
{
   int i;

   mem_free ((*stmt)->name);

   for (i = 0; i < (*stmt)->n; i++)
	 if ((*stmt)->type == TOK_ENUM || (*stmt)->type == TOK_STRING)
	   mem_free ((*stmt)->args[i].string);

   if ((*stmt)->n) mem_free ((*stmt)->args);

   mem_free (*stmt);

   *stmt = NULL;
}

static int stmt_add (stmt_t *stmt,scan_t *sc)
{
   arg_t *tmp;

   if (stmt->args == NULL)
	 tmp = (arg_t *) mem_alloc (sizeof (arg_t));
   else
	 tmp = (arg_t *) mem_realloc (stmt->args,sizeof (arg_t) * (stmt->n + 1));

   if (tmp == NULL)
	 {
		log_printf (LOG_ERROR,"Out of memory\n");
		return (-1);
	 }

   stmt->args = tmp;

   switch (stmt->type)
	 {
	  case TOK_STRING:
		stmt->args[stmt->n].string = sc->token.string;
		break;
	  case TOK_INTEGER:
		stmt->args[stmt->n].integer = sc->token.integer;
		break;
	  case TOK_BOOLEAN:
		stmt->args[stmt->n].boolean = sc->token.boolean;
		break;
	  case TOK_ENUM:
		stmt->args[stmt->n].string = sc->token.keyword;
		break;
	 }

   (stmt->n)++;

   return (0);
}

static void stmt_save (section_t *section,stmt_t **stmt)
{
   if (section->stmt == NULL)
	 {
		section->stmt = *stmt;
		section->stmt->prev = section->stmt->next = section->stmt;
	 }
   else
	 {
		(*stmt)->prev = section->stmt->prev;
		section->stmt->prev->next = *stmt;
		section->stmt->prev = *stmt;
		(*stmt)->next = section->stmt;
	 }
}

/**************************************/

static void parse_error (scan_t *sc,const char *expect,int got)
{
   char *token,buf[128];
   switch (got)
	 {
	  case TOK_KEYWORD:
		token = "TOK_KEYWORD";
		mem_free (sc->token.keyword);
		break;
	  case TOK_STRING:
		token = "TOK_STRING";
		mem_free (sc->token.string);
		break;
	  case TOK_ENUM:
		token = "TOK_ENUM";
		mem_free (sc->token.keyword);
		break;
	  case TOK_INTEGER:
		token = "TOK_INTEGER";
		break;
	  case TOK_BOOLEAN:
		token = "TOK_BOOLEAN";
		break;
	  case 0:
		token = "EOF";
		break;
	  case '\n':
		token = "<nl>";
		break;
	  case '\t':
		token = "<tab>";
		break;
	  case '{':
	  case '}':
	  case '=':
	  case ',':
		buf[0] = '\'';
		buf[1] = got;
		buf[2] = '\'';
		buf[3] = '\0';
		token = buf;
		break;
	  default:
		sprintf (buf,"0x%2x",got);
		token = buf;
	 }
   log_printf (LOG_ERROR,"Parse error on line %d: Expected %s, got %s instead\n",sc->line,expect,token);
}

static const char *tok2str (int token)
{
#define ASSIGN(x) case x: return (#x)
   switch (token)
	 {
		ASSIGN(TOK_STRING);
		ASSIGN(TOK_INTEGER);
		ASSIGN(TOK_BOOLEAN);
		ASSIGN(TOK_ENUM);
	 }
   /* this is never reached unless my logic is screwed, but it keeps gcc happy */
   return (NULL);
}

static void type_mismatch (scan_t *sc,int expect,int got)
{
   const char *was = tok2str (expect),*is = tok2str (got);
   if (got == TOK_STRING)
	 mem_free (sc->token.string);
   else if (got == TOK_ENUM)
	 mem_free (sc->token.keyword);
   log_printf (LOG_ERROR,"Parse error on line %d: Type mismatch. Expected %s, got %s instead\n",sc->line,was,is);
}

/**************************************/

/*
 * free all memory occupied by the specified parse structure
 */
void parse_destroy (section_t **section)
{
   if (section != NULL && *section != NULL)
	 {
		int i;

		if ((*section)->name != NULL) mem_free ((*section)->name);

		if ((*section)->stmt != NULL)
		  {
			 stmt_t *tmp,*prev = (*section)->stmt;

			 do
			   {
				  tmp = (*section)->stmt->next;
				  stmt_destroy (&(*section)->stmt);
				  (*section)->stmt = tmp;
			   }
			 while (prev != (*section)->stmt);

			 (*section)->stmt = NULL;
		  }

		for (i = 0; i < (*section)->n; i++)
		  if ((*section)->child[i] != NULL)
			parse_destroy (&(*section)->child[i]);

		if ((*section)->n) mem_free ((*section)->child);
		mem_free (*section);

		*section = NULL;
	 }
}

#define SCAN() if ((result = scan (sc)) < 0) return (ERR)

/* forward declaration */
static int parse_section (section_t **,scan_t *,char **);

/*
 * value = TOK_STRING | TOK_INTEGER | TOK_BOOLEAN | TOK_KEYWORD
 * >> value (',' value)* '}'
 */
static int parse_array_or_section (section_t **section,scan_t *sc,char **keyword)
{
   int result,saved = 0;
   stmt_t *nw = NULL;

   SCAN();

   if (result == TOK_KEYWORD)
	 {
		char *tmp = sc->token.keyword;

		if ((result = scan (sc)) < 0)
		  {
			 mem_free (tmp);
			 return (ERR);
		  }

		saved = result;

		if (result == '=')
		  {
			 if ((*section)->child == NULL)
			   {
				  if (((*section)->child = (section_t **) mem_alloc (sizeof (section_t *))) == NULL)
					{
out_of_memory:
					   log_printf (LOG_ERROR,"Out of memory\n");
					   mem_free (tmp);
					   return (ERR);
					}

				  if (((*section)->child[0] = (section_t *) mem_alloc (sizeof (section_t))) == NULL)
					{
					   mem_free ((*section)->child);
					   (*section)->child = NULL;
					   goto out_of_memory;
					}
			   }
			 else
			   {
				  section_t **ptr;

				  if ((ptr = (section_t **) mem_realloc ((*section)->child,((*section)->n + 1) * sizeof (section_t *))) == NULL)
					goto out_of_memory;

				  (*section)->child = ptr;

				  if (((*section)->child[(*section)->n] = (section_t *) mem_alloc (sizeof (section_t))) == NULL)
					goto out_of_memory;
			   }

			 (*section)->child[(*section)->n]->n = 0;
			 (*section)->child[(*section)->n]->name = *keyword;
			 (*section)->child[(*section)->n]->stmt = NULL;
			 (*section)->child[(*section)->n]->child = NULL;

			 ((*section)->n)++;

			 return (parse_section (&(*section)->child[(*section)->n - 1],sc,&tmp));
		  }

		result = TOK_KEYWORD;
		sc->token.keyword = tmp;
	 }

   if (result != TOK_STRING && result != TOK_INTEGER && result != TOK_BOOLEAN && result != TOK_KEYWORD)
	 {
		mem_free (*keyword);
missing_value:
		parse_error (sc,"TOK_STRING, TOK_INTEGER, TOK_BOOLEAN, or TOK_ENUM",result);
		goto error;
	 }

   if ((nw = stmt_init (keyword,result)) == NULL)
	 {
unable_to_add:
		if (result == TOK_STRING)
		  mem_free (sc->token.string);
		else if (result == TOK_KEYWORD)
		  mem_free (sc->token.keyword);
		goto error;
	 }

   if (stmt_add (nw,sc) < 0)
	 goto unable_to_add;

   if (result == TOK_KEYWORD)
	 result = saved;
   else if ((result = scan (sc)) < 0)
	 goto error;

   while (result == ',')
	 {
		if ((result = scan (sc)) < 0) goto error;

		if (result != TOK_STRING && result != TOK_INTEGER && result != TOK_BOOLEAN && result != TOK_KEYWORD)
		  goto missing_value;

		if (result != nw->type && (result != TOK_KEYWORD || nw->type != TOK_ENUM))
		  {
			 type_mismatch (sc,nw->type,result);
			 goto error;
		  }

		if (stmt_add (nw,sc) < 0)
		  goto unable_to_add;

		if ((result = scan (sc)) < 0) goto error;
	 }

   if (result != '}')
	 {
		parse_error (sc,"'}'",result);
		goto error;
	 }

   stmt_save (*section,&nw);

   return (OK);

error:
   if (nw != NULL) stmt_destroy (&nw);
   return (ERR);
}

/* >> TOK_STRING | TOK_INTEGER | TOK_BOOLEAN | TOK_KEYWORD | '{' */
static int parse_operand (section_t **section,scan_t *sc,char **keyword)
{
   int result;

   SCAN();
   if (!result)
	 {
		parse_error (sc,"operand",0);
		return (ERR);
	 }

   if (result == '{') return (parse_array_or_section (section,sc,keyword));

   if (result == TOK_STRING || result == TOK_INTEGER || result == TOK_BOOLEAN || result == TOK_KEYWORD)
	 {
		stmt_t *nw;

		if ((nw = stmt_init (keyword,result)) == NULL)
		  return (ERR);

		if (stmt_add (nw,sc) < 0)
		  {
			 stmt_destroy (&nw);
			 return (ERR);
		  }

		stmt_save (*section,&nw);

		return (OK);
	 }

   parse_error (sc,"TOK_STRING, TOK_INTEGER, TOK_BOOLEAN, TOK_ENUM, or '{'",result);
   return (ERR);
}

#define SECTION_START	1
#define SECTION_END		2

/* >> TOK_KEYWORD '=' operand */
static int parse_assignment (section_t **section,scan_t *sc,char **lookahead,int why)
{
   int result;
   char *keyword;
   stmt_t *stmt = (*section)->stmt;

   /*
	* wtf is lookahead and why don't you parse TOK_KEYWORD '=' when it's not
	* NULL, I hear you say (;
	*
	* The reason for this hack is because if we parse a section (i.e. not the main
	* section), we've already looked at the keyword and the '=' sign (it's the only
	* way to distinguish between an array of enumerations and a section, so we need
	* to pass this information along and make this one exception.
	*
	* There is another instance where we need to peek at the next token and that is
	* to look for the '}' at the end of nested sections. We use why to distinguish
	* between the two cases.
	*
	* See parse_array_or_section() for more details.
	*/

   if (lookahead == NULL)
	 {
		SCAN();
		if (!result) return (FIN);
		if (result != TOK_KEYWORD)
		  {
			 parse_error (sc,"TOK_KEYWORD",result);
			 return (ERR);
		  }
		keyword = sc->token.keyword;
	 }
   else keyword = *lookahead;

   if (stmt != NULL)
	 do
	   {
		  if (!strcmp (stmt->name,keyword))
			{
			   log_printf (LOG_ERROR,"Parse error on line %d: Duplicate keyword: %s\n",sc->line,keyword);
			   mem_free (keyword);
			   return (ERR);
			}
		  stmt = stmt->next;
	   }
     while (stmt != (*section)->stmt);

   if (lookahead == NULL || why != SECTION_START)
	 {
		/* we can't use SCAN() here since we need to free keyword */
		if ((result = scan (sc)) < 0)
		  {
			 mem_free (keyword);
			 return (ERR);
		  }

		if (result != '=')
		  {
			 parse_error (sc,"'='",result);
			 mem_free (keyword);
			 return (ERR);
		  }
	 }

   result = parse_operand (section,sc,&keyword);

   if (result != OK) return (ERR);

   return (result);
}

static int parse_section (section_t **section,scan_t *sc,char **lookahead)
{
   int result = OK;
   if (lookahead != NULL)
	 {
		result = parse_assignment (section,sc,lookahead,SECTION_START);
		while (result == OK)
		  {
			 SCAN();
			 if (result == '}') return (OK);
			 if (result != TOK_KEYWORD)
			   {
				  parse_error (sc,"'}', or TOK_KEYWORD",result);
				  return (ERR);
			   }
			 result = parse_assignment (section,sc,&sc->token.keyword,SECTION_END);
			 if (result == FIN)
			   {
				  parse_error (sc,"'}'",result);
				  return (ERR);
			   }
		  }
	 }
   else while (result == OK) result = parse_assignment (section,sc,NULL,0);
   return (result);
}

/*
 * parse filename. return a linked list containing all the parsed
 * statements if successful, NULL if some error occurred. the
 * function automatically logs all errors
 */
section_t *parse (const char *filename)
{
   scan_t sc;
   section_t *section;
   int result;

   if ((section = (section_t *) mem_alloc (sizeof (section_t))) == NULL)
	 {
		log_printf (LOG_ERROR,"Out of memory\n");
		return (NULL);
	 }

   section->n = 0;
   section->name = NULL;
   section->stmt = NULL;
   section->child = NULL;

   log_printf (LOG_VERBOSE,"Opening configuration file: %s\n",filename);
   if (scan_open (&sc,filename) < 0)
	 {
		mem_free (section);
		return (NULL);
	 }

   result = parse_section (&section,&sc,NULL);
   if (result == FIN) log_printf (LOG_VERBOSE,"Successfully parsed configuration file\n");

   scan_close (&sc);
   log_printf (LOG_VERBOSE,"Closed configuration file\n");

   if (result == ERR) parse_destroy (&section);

   return (section);
}

