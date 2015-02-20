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

#include "language_list.h++"
#include <iostream>

static language_list _global;

language_list::language_list(void)
    : _languages()
{
}

void language_list::add(const language::ptr& lang)
{
    auto l = _languages.find(lang->name());
    if (l != _languages.end()) {
        std::cerr << "Language '"
                  << lang->name()
                  << "' added twice\n";
        abort();
    }

    _languages[lang->name()] = lang;
}

language::ptr language_list::search(const std::string& name)
{
    auto l = _languages.find(name);
    if (l == _languages.end())
        return NULL;

    return l->second;
}

void language_list::global_add(const language::ptr& lang)
{
    return _global.add(lang);
}

language::ptr language_list::global_search(const std::string& name)
{
    return _global.search(name);
}