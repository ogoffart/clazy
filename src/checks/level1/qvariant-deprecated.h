/*
    This file is part of the clazy static checker.

    Copyright (C) 2019 Olivier Goffart <ogoffart@woboq.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#pragma once
#include "checkbase.h"

#include <string>

class ClazyContext;
namespace clang {
class Stmt;
}  // namespace clang

/**
 * XXX
 */
class QVariantDeprecated
    : public CheckBase
{
    void checkTypeEnum(clang::Stmt *stm);
    void checkTypeFn(clang::Stmt *stm);
public:
    QVariantDeprecated(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stm) override;
};

