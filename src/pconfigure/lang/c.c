
/*
 * Copyright (C) 2011 Daniel Dabbelt
 *   <palmem@comcast.net>
 *
 * This file is part of pconfigure.
 * 
 * pconfigure is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * pconfigure is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with pconfigure.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "c.h"
#include "../lambda.h"
#include <talloc.h>
#include <string.h>
#include <clang-c/Index.h>
#include <unistd.h>

static struct language *language_c_search(struct language *l_uncast,
                                          struct language *parent,
                                          const char *path);
static const char *language_c_objname(struct language *l_uncast,
                                      void *context, struct context *c);
static void language_c_deps(struct language *l_uncast, struct context *c,
                            void (*func) (const char *, ...));
static void language_c_build(struct language *l_uncast, struct context *c,
                             void (*func) (bool, const char *, ...));
static void language_c_link(struct language *l_uncast, struct context *c,
                            void (*func) (bool, const char *, ...));
static void language_c_extras(struct language *l_uncast, struct context *c,
                              void *context, void (*func) (const char *));

static char *string_strip(const char *in, void *context);

struct language *language_c_new(struct clopts *o, const char *name)
{
    struct language_c *l;

    if (strcmp(name, "c") != 0)
        return NULL;

    l = talloc(o, struct language_c);
    if (l == NULL)
        return NULL;

    language_init(&(l->l));
    l->l.name = talloc_strdup(l, "c");
    l->l.link_name = talloc_strdup(l, "c");
    l->l.compiled = true;
    l->l.compile_str = talloc_strdup(l, "CC");
    l->l.compile_cmd = talloc_strdup(l, "gcc");
    l->l.link_str = talloc_strdup(l, "LD");
    l->l.link_cmd = talloc_strdup(l, "gcc");
    l->l.search = &language_c_search;
    l->l.objname = &language_c_objname;
    l->l.deps = &language_c_deps;
    l->l.build = &language_c_build;
    l->l.link = &language_c_link;
    l->l.extras = &language_c_extras;

    return &(l->l);
}

struct language *language_c_search(struct language *l_uncast,
                                   struct language *parent, const char *path)
{
    struct language_c *l;

    l = talloc_get_type(l_uncast, struct language_c);
    if (l == NULL)
        return NULL;

    if (strcmp(path + strlen(path) - 2, ".c") != 0)
        return NULL;

    if (parent == NULL)
        return l_uncast;

    if (strcmp(parent->link_name, l_uncast->link_name) != 0)
        return NULL;

    return l_uncast;
}

const char *language_c_objname(struct language *l_uncast, void *context,
                               struct context *c)
{
    char *o;
    const char *compileopts_hash, *langopts_hash;
    struct language_c *l;
    void *subcontext;

    l = talloc_get_type(l_uncast, struct language_c);
    if (l == NULL)
        return NULL;

    subcontext = talloc_new(NULL);
    compileopts_hash = stringlist_hashcode(c->compile_opts, subcontext);
    langopts_hash = stringlist_hashcode(l->l.compile_opts, subcontext);

    /* This should be checked higher up in the stack, but just make sure */
    assert(c->full_path[strlen(c->src_dir)] == '/');
    o = talloc_asprintf(context, "%s/%s/%s-%s.o",
                        c->obj_dir,
                        c->full_path + strlen(c->src_dir) + 1,
                        compileopts_hash, langopts_hash);

    TALLOC_FREE(subcontext);
    return o;
}

void language_c_deps(struct language *l_uncast, struct context *c,
                     void (*func) (const char *, ...))
{
    void *context;
    struct language_c *l;
    int clang_argc;
    char **clang_argv;
    int i;
    CXIndex index;
    CXTranslationUnit tu;

    l = talloc_get_type(l_uncast, struct language_c);
    if (l == NULL)
        return;

    context = talloc_new(NULL);

    /* Creates the argc/argv for a call to clang that will determine which
     * includes are used by the file in question. */
    clang_argc = stringlist_size(l->l.compile_opts)
        + stringlist_size(c->compile_opts) + 1;
    clang_argv = talloc_array(context, char *, clang_argc + 1);
    for (i = 0; i <= clang_argc; i++)
        clang_argv[i] = NULL;

    clang_argv[0] = talloc_strdup(clang_argv, c->full_path);
    i = 1;
    /* *INDENT-OFF* */
    stringlist_each(l->l.compile_opts,
		    lambda(int, (const char *str),
			   {
			       clang_argv[i] = talloc_strdup(clang_argv, str);
			       i++;
			       return 0;
			   }
                    ));
    stringlist_each(c->compile_opts,
		    lambda(int, (const char *str),
			   {
			       clang_argv[i] = talloc_strdup(clang_argv, str);
			       i++;
			       return 0;
			   }
                    ));
    /* *INDENT-ON* */

    /* Asks libclang for the list of includes */
    index = clang_createIndex(0, 0);
    /* *INDENT-OFF* */
    tu = clang_parseTranslationUnit(index, 0,
                                    (const char *const *)clang_argv,
                                    clang_argc, 0, 0, CXTranslationUnit_None);
    clang_getInclusions(tu,
			lambda(void,
			       (CXFile included_file,
				CXSourceLocation * inclusion_stack,
				unsigned include_len, void *unused),
			       {
                                   CXString fn;
                                   const char *fn_cstr;
                                   fn = clang_getFileName(included_file);
                                   fn_cstr = clang_getCString(fn);
                                   if (fn_cstr[0] != '/')
				       func("%s",
					    string_strip(fn_cstr, context));
                                   clang_disposeString(fn);
			       }
			    ), NULL);
    /* *INDENT-ON* */
    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);

    TALLOC_FREE(context);
}

void language_c_build(struct language *l_uncast, struct context *c,
                      void (*func) (bool, const char *, ...))
{
    struct language_c *l;
    void *context;
    const char *obj_path;

    l = talloc_get_type(l_uncast, struct language_c);
    if (l == NULL)
        return;

    context = talloc_new(NULL);
    obj_path = language_objname(l_uncast, context, c);

    func(true, "echo -e \"%s\\t%s\"",
         l->l.compile_str, c->full_path + strlen(c->src_dir) + 1);

    func(false, "mkdir -p `dirname %s` >& /dev/null || true", obj_path);

    func(false, "%s\\", l->l.compile_cmd);
    /* *INDENT-OFF* */
    stringlist_each(l->l.compile_opts,
		    lambda(int, (const char *opt),
			   {
			       func(false, "\\ %s", opt);
			       return 0;
			   }
                    ));
    stringlist_each(c->compile_opts,
		    lambda(int, (const char *opt),
			   {
			       func(false, "\\ %s", opt);
			       return 0;
			   }
                    ));
    /* *INDENT-ON* */
    func(false, "\\ -c %s -o %s\n", c->full_path, obj_path);

    TALLOC_FREE(context);
}

void language_c_link(struct language *l_uncast, struct context *c,
                     void (*func) (bool, const char *, ...))
{
    struct language_c *l;
    void *context;

    l = talloc_get_type(l_uncast, struct language_c);
    if (l == NULL)
        return;

    context = talloc_new(NULL);

    func(true, "echo -e \"%s\\t%s\"",
         l->l.link_str, c->full_path + strlen(c->bin_dir) + 1);

    func(false, "mkdir -p `dirname %s` >& /dev/null || true", c->full_path);

    func(false, "\\\t@%s", l->l.link_cmd);
    /* *INDENT-OFF* */
    stringlist_each(l->l.link_opts,
		    lambda(int, (const char *opt),
			   {
			       func(false, "\\ %s", opt);
			       return 0;
			   }
                    ));
    stringlist_each(c->link_opts,
		    lambda(int, (const char *opt),
			   {
			       func(false, "\\ %s", opt);
			       return 0;
			   }
                    ));
    stringlist_each(c->objects,
		    lambda(int, (const char *opt),
			   {
			       func(false, "\\ %s", opt);
			       return 0;
			   }
                    ));
    /* *INDENT-ON* */
    func(false, "\\ -o %s\n", c->full_path);

    TALLOC_FREE(context);
}

void language_c_extras(struct language *l_uncast, struct context *c,
                       void *context, void (*func) (const char *))
{
    /* *INDENT-OFF* */
    language_deps(l_uncast, c, 
		  lambda(void, (const char *format, ...),
			 {
			     va_list args;
			     char *cfile;

			     va_start(args, NULL);
			     cfile = talloc_vasprintf(context, format, args);
			     cfile[strlen(cfile)-1] = 'c';
			     if (access(cfile, R_OK) == 0)
				 func(cfile);
			     
			     va_end(args);
			 }
		      ));
    /* *INDENT-ON* */
}

char *string_strip(const char *filename_cstr, void *context)
{
    char *source_name;

    source_name = talloc_strdup(context, filename_cstr);

    {
        int last_dir, pprev_dir, prev_dir, i, o;

        pprev_dir = -1;
        prev_dir = -1;
        last_dir = -1;
        i = 0;
        o = 0;
        while (i < strlen(filename_cstr))
        {
            source_name[o] = filename_cstr[i];

            if ((o > 0) && (filename_cstr[i] == '/'))
            {
                pprev_dir = prev_dir;
                prev_dir = last_dir;
                last_dir = o;
            }

            if (filename_cstr[i - 1] == '.' && filename_cstr[i - 2] == '.')
            {
                if (pprev_dir > 0)
                {
                    o = pprev_dir;
                    pprev_dir = -1;
                    prev_dir = -1;
                    last_dir = -1;
                }
            }

            source_name[o] = filename_cstr[i];

            i++;
            o++;
        }
        source_name[o] = '\0';
    }

    return source_name;
}