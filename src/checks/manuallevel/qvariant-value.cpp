/*
    This file is part of the clazy static checker.

    Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
    Author: Sérgio Martins <sergio.martins@kdab.com>

    Copyright (C) 2015 Sergio Martins <smartins@kde.org>
    Copyright (C) 2020 Olivier Goffart <ogoffart@woboq.com>

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

#include "qvariant-value.h"
#include "TemplateUtils.h"
#include "StringUtils.h"
#include "SourceCompatibilityHelpers.h"
#include "clazy_stl.h"
#include "FixItUtils.h"
#include "Utils.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

#include <ctype.h>
#include <memory>
#include <vector>

class ClazyContext;

using namespace std;
using namespace clang;

QVariantValue::QVariantValue(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

void QVariantValue::VisitStmt(clang::Stmt *stm)
{
    auto callExpr = dyn_cast<CXXMemberCallExpr>(stm);
    if (!callExpr)
        return;

    CXXMethodDecl *methodDecl = callExpr->getMethodDecl();
    if (!methodDecl || clazy::name(methodDecl) != "value")
        return;

    CXXRecordDecl *decl = methodDecl->getParent();
    if (!decl || clazy::name(decl) != "QVariant")
        return;

    vector<QualType> typeList = clazy::getTemplateArgumentsTypes(methodDecl);
    const Type *t = typeList.empty() ? nullptr : typeList[0].getTypePtrOrNull();
    if (!t)
        return;

    string typeName = clazy::simpleTypeName(typeList[0], lo());
    std::string error = std::string("Use qvariant_cast instead of QVariant::value");

    std::vector<FixItHint> fixits;
    if (auto variant = callExpr->getImplicitObjectArgument()) {
        if (const auto *MemExpr = dyn_cast<MemberExpr>(callExpr->getCallee()->IgnoreParens())) {
            if (const auto tpl = MemExpr->getTemplateArgs()) {
                auto typeRange = tpl->getSourceRange();
                const char *begin = sm().getCharacterData(typeRange.getBegin());
                const char *end = sm().getCharacterData(Utils::locForNextToken(typeRange.getEnd(), sm(), lo()));
                fixits.push_back(clazy::createInsertion(clazy::getLocStart(variant),
                    "qvariant_cast<" + std::string(begin, end) + ">("));

                fixits.push_back(clazy::createReplacement(SourceRange(
                    Utils::locForNextToken(clazy::getLocEnd(MemExpr->getBase()), sm(), lo()),
                    clazy::getLocEnd(stm)),
                                                           ")"));
            }
        }
    }
    emitWarning(clazy::getLocStart(stm), error, fixits);
}
