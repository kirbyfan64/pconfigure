/*
 * Copyright (C) 2015 Palmer Dabbelt
 *   <palmer@dabbelt.com>
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

#ifndef LANGUAGE_HXX
#define LANGUAGE_HXX

#include "context.h++"
#include "opts_target.h++"
#include <libmakefile/target.h++>
#include <memory>
#include <regex>

/* Contains a single language.  Languages take contexts (which the
 * Configfile parser understands) and turn them into targets (which
 * the Makefile generator understands) in a way that's specific to
 * each source language. */
class language: public opts_target {
public:
    typedef std::shared_ptr<language> ptr;

private:
    /* These implement "opts_target" */
    std::vector<std::string> _compile_opts;
    std::vector<std::string> _link_opts;

public:
    language(void);

public:
    /* Returns the name of this language, which is used as a unique
     * key when users refer to it from Configfiles. */
    virtual std::string name(void) const = 0;

    /* Returns a deep copy of this language, such that modifications
     * of the returned language will not effect this language.  Note
     * that this has to return a regular pointer (and not a
     * shared_ptr) because C++11 doesn't support covariant return
     * types. */
    virtual language* clone(void) const = 0;

    /* Returns TRUE if this language can process the given context. */
    virtual bool can_process(const context::ptr& ctx) const = 0;

    /* Returns an arbitrary integer.  When multiple languages are
     * capable of processing a context then the one of largest
     * priority will be picked. */
    virtual int priority(void) const { return 0; }

    /* Returns the targets that this context needs in order to build,
     * as a flattened list. */
    virtual
    std::vector<makefile::target::ptr> targets(const context::ptr& ctx) const;

    /* Virtual methods from opts_target. */
    virtual void add_compileopt(const std::string& data);
    virtual void add_linkopt(const std::string& data);

    /* Lists both the compile and link options, for languages that
     * don't discriminate -- the hope here is that compilers can
     * optimize when they're available for inlining... */
    virtual std::vector<std::string> clopts(void) const
        {
            auto opt = std::vector<std::string>();
            opt.insert(opt.end(), _compile_opts.begin(), _compile_opts.end());
            opt.insert(opt.end(), _link_opts.begin(), _link_opts.end());
            return opt;
        }

    virtual std::vector<std::string> clopts(const context::ptr& ctx) const
        {
            auto opt = clopts();
            auto lopt = ctx->clopts();
            opt.insert(opt.end(), lopt.begin(), lopt.end());
            return opt;
        }

protected:
    /* Returns TRUE if every source that's a direct child of the given
     * context has a name that matches any one of the given regular
     * expressions.  Essentially this is a helper function that allows
     * a language to check if it can build the given codebase. */
    static bool all_sources_match(const context::ptr& ctx,
                                  const std::vector<std::regex>& rxs);
};

#endif
