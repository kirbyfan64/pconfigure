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

#include "context.h++"
#include <sstream>

context::context(void)
    : type(context_type::DEFAULT),
      prefix("/usr/local"),
      gen_dir("obj/proc"),
      bin_dir("bin"),
      lib_dir("lib"),
      libexec_dir("libexec"),
      src_dir("src"),
      cmd(NULL),
      children()
{
}

context::context(const context_type& _type,
                 const std::string& _prefix,
                 const std::string& _gen_dir,
                 const std::string& _bin_dir,
                 const std::string& _lib_dir,
                 const std::string& _libexec_dir,
                 const std::string& _src_dir,
                 const command::ptr& _cmd,
                 const std::vector<ptr>& _children)
    : type(_type),
      prefix(_prefix),
      gen_dir(_gen_dir),
      bin_dir(_bin_dir),
      lib_dir(_lib_dir),
      libexec_dir(_libexec_dir),
      src_dir(_src_dir),
      cmd(_cmd),
      children(_children)
{
}

context::ptr context::dup(const context_type& type,
                          const command::ptr& cmd,
                          const std::vector<ptr>& children)
{
    return std::make_shared<context>(type,
                                     this->prefix,
                                     this->gen_dir,
                                     this->bin_dir,
                                     this->lib_dir,
                                     this->libexec_dir,
                                     this->src_dir,
                                     cmd,
                                     children);
}

bool context::check_type(const std::vector<context_type>& types)
{
    for (const auto& type: types)
        if (this->type == type)
            return true;

    return false;
}

std::string context::as_tree_string(const std::string prefix) const
{
    std::stringstream ss;

    ss << prefix
       << "[" << std::to_string(type) << "]" << " "
       << cmd->data()
       << "\n";

    for (const auto& child: children)
        ss << child->as_tree_string(prefix + "  ");

    return ss.str();
}

void context::add_compileopt(const std::string& data)
{
    compile_opts.push_back(data);
}

void context::add_linkopt(const std::string& data)
{
    link_opts.push_back(data);
}
