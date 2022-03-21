#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>
#ifndef WIN32
# include <err.h>
#endif
#include "json.h"
#include "base64.h"

/*
 * Copyright (C) 2016-2019 Jan-Piet Mens <jp@mens.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#define SPACER		"   "
#define FLAG_ARRAY	0x01
#define FLAG_PRETTY	0x02
#define FLAG_NOBOOL	0x04
#define FLAG_BOOLEAN	0x08
#define FLAG_NOSTDIN	0x10
#define FLAG_SKIPNULLS	0x20
#define FLAG_MASK	(FLAG_ARRAY | FLAG_PRETTY | FLAG_NOBOOL | FLAG_BOOLEAN | FLAG_NOSTDIN | FLAG_SKIPNULLS)

/* Size of buffer blocks for pipe slurping */
#define SLURP_BLOCK_SIZE 4096

static JsonNode *pile;		/* pile of nested objects/arrays */

#ifdef _WIN32
# define err(n, s)	{ fprintf(stderr, s); exit(n); }
# define errx(n, f, a)	{ fprintf(stderr, f, a); exit(n); }
# define fseeko	fseek
# define ftello	ftell
#endif

#define TAG_TO_FLAGS(tag) ((FLAG_MASK + 1) * (tag))
#define TAG_FLAG_BOOL     (TAG_TO_FLAGS(JSON_BOOL))
#define TAG_FLAG_STRING   (TAG_TO_FLAGS(JSON_STRING))
#define TAG_FLAG_NUMBER   (TAG_TO_FLAGS(JSON_NUMBER))
#define COERCE_MASK       (TAG_FLAG_BOOL | TAG_FLAG_STRING | TAG_FLAG_NUMBER)

JsonTag flags_to_tag(int flags) {
	return flags / (FLAG_MASK + 1);
}

void json_copy_to_object(JsonNode * obj, JsonNode * object_or_array, int clobber)
{
	JsonNode *node, *node_child, *obj_child;

	if (obj->tag != JSON_OBJECT && obj->tag != JSON_ARRAY)
		return;

	json_foreach(node, object_or_array) {
		if (!clobber & (json_find_member(obj, node->key) != NULL))
			continue;	/* Don't clobber existing keys */
		if (obj->tag == JSON_OBJECT) {
			if (node->tag == JSON_STRING)
				json_append_member(obj, node->key, json_mkstring(node->string_));
			else if (node->tag == JSON_NUMBER)
				json_append_member(obj, node->key, json_mknumber(node->number_));
			else if (node->tag == JSON_BOOL)
				json_append_member(obj, node->key, json_mkbool(node->bool_));
			else if (node->tag == JSON_NULL)
				json_append_member(obj, node->key, json_mknull());
			else if (node->tag == JSON_OBJECT) {
				/* Deep-copy existing object to new object */
				json_append_member(obj, node->key, (obj_child = json_mkobject()));
				json_foreach(node_child, node) {
					json_copy_to_object(obj_child, node_child, clobber);
				}
			} else
				fprintf(stderr, "PANIC: unhandled JSON type %d\n", node->tag);
		} else if (obj->tag == JSON_ARRAY) {
			if (node->tag == JSON_STRING)
				json_append_element(obj, json_mkstring(node->string_));
			if (node->tag == JSON_NUMBER)
				json_append_element(obj, json_mknumber(node->number_));
			if (node->tag == JSON_BOOL)
				json_append_element(obj, json_mkbool(node->bool_));
			if (node->tag == JSON_NULL)
				json_append_element(obj, json_mknull());
		}
	}
}

char *slurp_file(const char* filename, size_t *out_len, bool fold_newlines)
{
	char *buf;
	int i = 0;
	int ch;
	off_t buffer_len;
	FILE *fp;
	bool use_stdin = strcmp(filename, "-") == 0;

	if (use_stdin) fp = stdin;
	else if ((fp = fopen(filename, "r")) == NULL) {
		perror(filename);
		errx(1, "Cannot open %s for reading", filename);
	}
	if (fseeko(fp, 0, SEEK_END) != 0) {
		/* If we cannot seek, we're operating off a pipe and
		   need to dynamically grow the buffer that we're
		   reading into */
		buffer_len = SLURP_BLOCK_SIZE;
	} else {
		buffer_len = ftello(fp) + 1;
		fseeko(fp, 0, SEEK_SET);
	}

	if ((buf = malloc(buffer_len)) == NULL) {
		errx(1, "File %s is too large to be read into memory", filename);
	}
	while ((ch = fgetc(fp)) != EOF) {
		if (i == (buffer_len - 1)) {
			buffer_len += SLURP_BLOCK_SIZE;
			if ((buf = realloc(buf, buffer_len)) == NULL) {
				errx(1, "File %s is too large to be read into memory", filename);
			}
		}
		if (ch != '\n' || !fold_newlines) {
			buf[i++] = ch;
		}
	}
	if (!use_stdin) fclose(fp);
	buf[i] = 0;
	*out_len = i;
	return (buf);
}

JsonNode *jo_mknull(JsonTag type) {
	switch (type) {
		case JSON_STRING:
			return json_mkstring("");
			break;
		case JSON_NUMBER:
			return json_mknumber(0);
			break;
		case JSON_BOOL:
			return json_mkbool(false);
			break;
		default:
			return json_mknull();
			break;
	}
}

JsonNode *jo_mkbool(bool b, JsonTag type) {
	switch (type) {
		case JSON_STRING:
			return json_mkstring(b ? "true" : "false");
			break;
		case JSON_NUMBER:
			return json_mknumber(b ? 1 : 0);
			break;
		default:
			return json_mkbool(b);
			break;
	}
}

JsonNode *jo_mkstring(char *str, JsonTag type) {
	switch (type) {
		case JSON_NUMBER:
			/* Length of string */
			return json_mknumber(strlen(str));
			break;
		case JSON_BOOL:
			/* True if not empty */
			return json_mkbool(strlen(str) > 0);
			break;
		default:
			return json_mkstring(str);
			break;
	}
}

JsonNode *jo_mknumber(char *str, JsonTag type) {
	/* ASSUMPTION: str already tested as valid number */
	double n = strtod(str, NULL);

	switch (type) {
		case JSON_STRING:
			/* Just return the original representation */
			return json_mkstring(str);
			break;
		case JSON_BOOL:
			return json_mkbool(n != 0);
			break;
		default:
			/* ASSUMPTION: str already tested as valid number */
			return json_mknumber(n);
			break;
	}
}

/*
 * Attempt to "sniff" the type of data in `str' and return
 * a JsonNode of the correct JSON type.
 */

JsonNode *vnode(char *str, int flags)
{
	JsonTag type = flags_to_tag(flags);

	if (strlen(str) == 0) {
		return (flags & FLAG_SKIPNULLS) ? (JsonNode *)NULL : jo_mknull(type);
	}

	/* If str begins with a double quote, keep it a string */

	if (*str == '"') {
#if 0
		char *bp = str + strlen(str) - 1;

		if (bp > str && *bp == '"')
			*bp = 0;		/* Chop closing double quote */
		return json_mkstring(str + 1);
#endif
		return jo_mkstring(str, type);
	}

	char *endptr;
	double num = strtod(str, &endptr);

	if (!*endptr && isfinite(num)) {
		return jo_mknumber(str, type);
	}

	if (!(flags & FLAG_NOBOOL)) {
		if (strcmp(str, "true") == 0) {
			return jo_mkbool(true, type);
		} else if (strcmp(str, "false") == 0) {
			return jo_mkbool(false, type);
		} else if (strcmp(str, "null") == 0) {
			return jo_mknull(type);
		}
	}

	if (*str == '\\') {
		++str;
	} else {
		if (*str == '@' || *str == '%' || *str == ':') {
			char *filename = str + 1, *content;
			bool binmode = (*str == '%');
			bool jsonmode = (*str == ':');
			size_t len = 0;
			JsonNode *j = NULL;
	
			if ((content = slurp_file(filename, &len, false)) == NULL) {
				errx(1, "Error reading file %s", filename);
			}
	
			if (binmode) {
				char *encoded;
	
				if ((encoded = base64_encode(content, len)) == NULL) {
					errx(1, "Cannot base64-encode file %s", filename);
				}
	
				j = json_mkstring(encoded);
				free(encoded);
			} else if (jsonmode) {
				j = json_decode(content);
				if (j == NULL) {
					errx(1, "Cannot decode JSON in file %s", filename);
				}
			}
	
			// If it got this far without valid JSON, just consider it a string
			if (j == NULL) {
				char *bp = content + strlen(content) - 1;
	
				if (*bp == '\n') *bp-- = 0;
				if (*bp == '\r') *bp = 0;
				j = json_mkstring(content);
			}
			free(content);
			return (j);
		}
	}

	if (*str == '{' || *str == '[') {
		if (type == JSON_STRING) {
			return json_mkstring(str);
		}
		JsonNode *obj = json_decode(str);

		if (obj == NULL) {
			/* JSON cannot be decoded; return the string */
			// fprintf(stderr, "Cannot decode JSON from %s\n", str);

			obj = json_mkstring(str);
		}

		return (obj);
	}

	return jo_mkstring(str, type);
}

/*
 * Attempt to sniff `str' into a boolean; return a
 * corresponding JsonNode for it.
 */

JsonNode *boolnode(char *str)
{
	if (strlen(str) == 0) {
		return json_mknull();
	}

	if (tolower((unsigned char) *str) == 't') {
		return json_mkbool(1);
	}

	return json_mkbool(atoi(str));
}

int usage(char *prog)
{
	fprintf(stderr, "Usage: %s [-a] [-B] [-D] [-d keydelim] [-p] [-e] [-n] [-v] [-V] [-f file] [--] [-s|-n|-b] [word...]\n", prog);
	fprintf(stderr, "\tword is key=value or key@value\n");
	fprintf(stderr, "\t-a creates an array of words\n");
	fprintf(stderr, "\t-B disable boolean true/false/null detection\n");
	fprintf(stderr, "\t-D deduplicate object keys\n");
	fprintf(stderr, "\t-d key will be object path separated by keydelim\n");
	fprintf(stderr, "\t-f load file as JSON object or array\n");
	fprintf(stderr, "\t-p pretty-prints JSON on output\n");
	fprintf(stderr, "\t-e quit if stdin is empty do not wait for input\n");
	fprintf(stderr, "\t-s coerce type guessing to string\n");
	fprintf(stderr, "\t-b coerce type guessing to bool\n");
	fprintf(stderr, "\t-n coerce type guessing to number\n");
	fprintf(stderr, "\t-v show version\n");
	fprintf(stderr, "\t-V show version in JSON\n");

	return (-1);
}

/*
 * Check whether we're being given nested arrays or objects.
 * `kv' contains the "key" such as "number" or "point[]" or
 * "geo[lat]". `value' the actual value for that element.
 *
 * Returns true if nesting is completely handled, otherwise:
 *   *keyp   -> remaining key for caller to insert "value"
 *   *baseop -> object node in which caller should insert "value"
 */

bool resolve_nested(int flags, char **keyp, char key_delim, JsonNode *value, JsonNode **baseop)
{
	char *member = NULL, *bo, *bc, *so;		/* bracket open, close, sub-object */
	JsonNode *op;
	int found = false;

	(void)flags;

	if (key_delim) {
		/* First construct nested object */
		while ((so = strchr(*keyp, key_delim)) != NULL) {
			*so = 0;
			if ((op = json_find_member(*baseop, *keyp)) == NULL) {
				/* Add a nested object node */
				op = json_mkobject();
				json_append_member(*baseop, *keyp, op);
			}
			*baseop = op;
			*keyp = so + 1;
		}
	}

	/* Now check for trailing geo[] or geo[lat] */
	if ((bo = strchr(*keyp, '[')) != NULL) {
		if (*(bo+1) == ']') {
			*bo = 0;
		} else if ((bc = strchr(bo + 1, ']')) == NULL) {
			fprintf(stderr, "missing closing bracket on %s\n", *keyp);
			return (false);
		} else {
			*bo = *bc = 0;
			member = bo + 1;
		}

		/*
		 * *keyp is now `geo' for both `geo[]` and `geo[lat]`
		 * member is null for the former and "lat" for the latter.
		 * Find an existing object in *baseop for this member name
		 * or create a new one if we don't have it.
		 */

		if ((op = json_find_member(*baseop, *keyp)) != NULL) {
			found = true;
		} else {
			op = (member == NULL) ? json_mkarray() : json_mkobject();
		}

		if (member == NULL) {		/* we're doing an array */
			json_append_element(op, value);
		} else {			/* we're doing an object */
			json_append_member(op, member, value);
		}

		if (!found) {
			json_append_member(*baseop, *keyp, op);
		}

		return (true);
	}
	return (false);
}

int member_to_object(JsonNode *object, int flags, char key_delim, char *kv)
{
	/* we expect key=value or key:value (boolean on last) */
	char *p = strchr(kv, '=');
	char *q = strchr(kv, '@');
	char *r = strchr(kv, ':');

	if ((r && *(r+1) == '=') && !q) {
		char *filename = p + 1;
		char *content;
		size_t len;

		if ((content = slurp_file(filename, &len, false)) == NULL) {
			errx(1, "Error reading file %s", filename);
		}

		JsonNode *o = json_decode(content);
		free(content);

		if (o == NULL) {
			errx(1, "Cannot decode JSON in file %s", filename);
		}

		*r = 0;		/* Chop at ":=" */
		if (!resolve_nested(flags, &kv, key_delim, o, &object))
			json_append_member(object, kv, o);
		return (0);
	}

	if (!p && !q && !r) {
		return (-1);
	}

	JsonNode *val;
	if (p) {
		*p = 0;
		val = vnode(p+1, flags);

		if (!resolve_nested(flags, &kv, key_delim, val, &object))
			json_append_member(object, kv, val);
	} else {
		if (q) {
			*q = 0;
			val = boolnode(q+1);

			if (!resolve_nested(flags | FLAG_BOOLEAN, &kv, key_delim, val, &object))
				json_append_member(object, kv, val);
		}
	}
	return (0);
}

/*
 * Append kv to the array or object.
 */

void append_kv(JsonNode *object_or_array, int flags, char key_delim, char *kv)
{
	if (flags & FLAG_ARRAY) {
		json_append_element(object_or_array, vnode(kv, flags));
	} else {
		if (member_to_object(object_or_array, flags, key_delim, kv) == -1) {
			fprintf(stderr, "Argument `%s' is neither k=v nor k@v\n", kv);
		}
	}
}

#ifdef _WIN32
#include <windows.h>
char* utf8_from_locale(const char *str, size_t len)
{
	wchar_t* wcsp;
	char* mbsp;
	size_t mbssize, wcssize;

	if (len == 0) {
		return strdup("");
	}
	if (len == -1) {
		len = strlen(str);
	}
	wcssize = MultiByteToWideChar(GetACP(), 0, str, len,  NULL, 0);
	wcsp = (wchar_t*) malloc((wcssize + 1) * sizeof(wchar_t));
	if (!wcsp) {
		return NULL;
	}
	wcssize = MultiByteToWideChar(GetACP(), 0, str, len, wcsp, wcssize + 1);
	wcsp[wcssize] = 0;

	mbssize = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR) wcsp, -1, NULL, 0, NULL, NULL);
	mbsp = (char*) malloc((mbssize + 1));
	if (!mbsp) {
		free(wcsp);
		return NULL;
	}
	mbssize = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR) wcsp, -1, mbsp, mbssize, NULL, NULL);
	mbsp[mbssize] = 0;
	free(wcsp);
	return mbsp;
}
# define utf8_free(p) free(p)

char* locale_from_utf8(const char *utf8, size_t len)
{
	wchar_t* wcsp;
	char* mbsp;
	size_t mbssize, wcssize;

	if (len == 0) {
		return strdup("");
	}
	if (len == -1) {
		len = strlen(utf8);
	}
	wcssize = MultiByteToWideChar(CP_UTF8, 0, utf8, len,  NULL, 0);
	wcsp = (wchar_t*) malloc((wcssize + 1) * sizeof(wchar_t));
	if (!wcsp) {
		return NULL;
	}
	wcssize = MultiByteToWideChar(CP_UTF8, 0, utf8, len, wcsp, wcssize + 1);
	wcsp[wcssize] = 0;
	mbssize = WideCharToMultiByte(GetACP(), 0, (LPCWSTR) wcsp, -1, NULL, 0, NULL, NULL);
	mbsp = (char*) malloc((mbssize + 1));
	if (!mbsp) {
		free(wcsp);
		return NULL;
	}
	mbssize = WideCharToMultiByte(GetACP(), 0, (LPCWSTR) wcsp, -1, mbsp, mbssize, NULL, NULL);
	mbsp[mbssize] = 0;
	free(wcsp);
	return mbsp;
}
# define locale_free(p) free(p)
#else
# define utf8_from_locale(p, l) (p)
# define utf8_free(p) do {} while (0)
# define locale_from_utf8(p, l) (p)
# define locale_free(p) do {} while (0)
#endif

char *stringify(JsonNode *json, int flags)
{
	int pretty = flags & FLAG_PRETTY;

	return json_stringify(json, (pretty) ? SPACER : NULL);
}

int version(int flags)
{
	JsonNode *json = json_mkobject();
	char *js;

	json_append_member(json, "program", json_mkstring("jo"));
	json_append_member(json, "author", json_mkstring("Jan-Piet Mens"));
	json_append_member(json, "repo", json_mkstring("https://github.com/jpmens/jo"));
	json_append_member(json, "version", json_mkstring(PACKAGE_VERSION));

	if ((js = stringify(json, flags)) != NULL) {
		printf("%s\n", js);
		free(js);
	}
	json_delete(json);
	return (0);
}

int main(int argc, char **argv)
{
	int c, key_delim = 0;
	bool showversion = false;
	char *kv, *js_string, *progname, buf[BUFSIZ], *p;
	char *in_file = NULL, *in_str;
	size_t in_len = 0;
	int ttyin = isatty(fileno(stdin)), ttyout = isatty(fileno(stdout));
	int flags = 0;
	JsonNode *json, *op;

#if HAVE_PLEDGE
	if (pledge("stdio rpath", NULL) == -1) {
		err(1, "pledge");
	}
#endif

	progname = (progname = strrchr(*argv, '/')) ? progname + 1 : *argv;

	while ((c = getopt(argc, argv, "aBDd:f:hpenvV")) != EOF) {
		switch (c) {
			case 'a':
				flags |= FLAG_ARRAY;
				break;
			case 'B':
				flags |= FLAG_NOBOOL;
				break;
			case 'D':
				json_dedup_members(true);
				break;
			case 'd':
				key_delim = optarg[0];
				break;
			case 'f':
				in_file = optarg;
				break;
			case 'h':
				usage(progname);
				return (0);
			case 'p':
				flags |= FLAG_PRETTY;
				break;
			case 'e':
				flags |= FLAG_NOSTDIN;
				break;
			case 'n':
				flags |= FLAG_SKIPNULLS;
				break;
			case 'v':
				printf("jo %s\n", PACKAGE_VERSION);
				exit(0);
			case 'V':
				showversion = true;
				break;
			default:
				exit(usage(progname));
		}
	}

	if (showversion) {
		return(version(flags));
	}

	argc -= optind;
	argv += optind;

	pile = json_mkobject();
	if (in_file != NULL) {
		if ((in_str = slurp_file(in_file, &in_len, false)) == NULL) {
			errx(1, "Error reading file %s", in_file);
		}
		json = json_decode(in_str);
		if (json) {
			switch (json->tag) {
				case JSON_ARRAY:
					flags |= FLAG_ARRAY;
					break;
				case JSON_OBJECT:
					break;
				default:
					errx(1, "Input JSON not an array or object: %s", stringify(json, flags));
			}
		} else
			json = (flags & FLAG_ARRAY) ? json_mkarray() : json_mkobject();
	} else {
		json = (flags & FLAG_ARRAY) ? json_mkarray() : json_mkobject();
	}

	if (argc == 0) {
		if (flags & FLAG_NOSTDIN) {
			return(0);
		}
		while (fgets(buf, sizeof(buf), stdin) != NULL) {
			if (buf[strlen(buf) - 1] == '\n')
				buf[strlen(buf) - 1] = 0;
			p = ttyin ? utf8_from_locale(buf, -1) : buf;
			append_kv(json, flags, key_delim, p);
			if (ttyin) utf8_free(p);
		}
	} else {
		while ((kv = *argv++)) {
			if (kv[0] == '-' && !(flags & COERCE_MASK)) {
				/* Set one-shot coerce flag */
				switch (kv[1]) {
					case 'b':
						flags |= TAG_FLAG_BOOL;
						break;
					case 's':
						flags |= TAG_FLAG_STRING;
						break;
					case 'n':
						flags |= TAG_FLAG_NUMBER;
						break;
					default:
						/* Treat as normal input */
						p = utf8_from_locale(kv, -1);
						append_kv(json, flags, key_delim, p);
						utf8_free(p);
						/* Reset any one-shot coerce flags */
						flags &= ~(COERCE_MASK);
				}
			} else {
				p = utf8_from_locale(kv, -1);
				append_kv(json, flags, key_delim, p);
				utf8_free(p);
				/* Reset any one-shot coerce flags */
				flags &= ~(COERCE_MASK);
			}
		}
	}

	/*
	 * See if we have any nested objects or arrays in the pile,
	 * and copy these into our main object if so.
	 */

	json_foreach(op, pile) {
		JsonNode *o;

		if (op->tag == JSON_ARRAY) {
			o = json_mkarray();
		} else if (op->tag == JSON_OBJECT) {
			o = json_mkobject();
		} else {
			continue;
		}
		json_copy_to_object(o, op, 0);
		json_append_member(json, op->key, o);
	}


	if ((js_string = stringify(json, flags)) == NULL) {
		fprintf(stderr, "Invalid JSON\n");
		exit(2);
	}

	p = ttyout ? locale_from_utf8(js_string, -1) : js_string;
	printf("%s\n", p);
	if (ttyout) locale_free(p);
	free(js_string);
	json_delete(json);
	json_delete(pile);
	return (0);
}
