#ifndef MAKEFILE_H
#define MAKEFILE_H

#include "error.h"
#include <stdio.h>
#include "string_list.h"

enum makefile_state
{
    MAKEFILE_STATE_NONE = 0
};

struct makefile
{
    /* The current state of the makefile */
    enum makefile_state state;

    /* The file pointer used to write to this makefile.  Don't use this
     * directly, but instead use the methods below */
    FILE *file;

    /* The list of targets to be build when one types "make all" */
    struct string_list *targets_all;

};

/* Call this (only once) before calling any other methods that use makefile */
enum error makefile_boot(void);

/* Opens (or closes) a makefile */
enum error makefile_init(struct makefile *m);
enum error makefile_clear(struct makefile *m);

/* Creates a new target */
enum error makefile_create_target(struct makefile *m, const char *name);

/* Adds a new dependency to a target (_start just changes state, for use with
   raw fd writes) */
enum error makefile_start_deps(struct makefile *m);
enum error makefile_add_dep(struct makefile *m, const char *dep);

/* Adds a new comand for building a target (_start as above) */
enum error makefile_start_cmds(struct makefile *m);
enum error makefile_add_cmd(struct makefile *m, const char *cmd);

#endif
