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

#include "qvariant-deprecated.h"
#include "TemplateUtils.h"
#include "StringUtils.h"
#include "SourceCompatibilityHelpers.h"
#include "clazy_stl.h"
#include "ClazyContext.h"
#include "ContextUtils.h"
#include "FixItUtils.h"

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
#include <map>

class ClazyContext;

using namespace std;
using namespace clang;

QVariantDeprecated::QVariantDeprecated(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

static StringRef variantTypeToMetaType(StringRef name)
{
    static const std::map<StringRef, StringRef> values = {
        { "Invalid", "UnknownType" },
        { "Bool", "Bool" },
        { "Int", "Int" },
        { "UInt", "UInt" },
        { "LongLong", "LongLong" },
        { "ULongLong", "ULongLong" },
        { "Double", "Double" },
        { "Char", "QChar" },
        { "Map", "QVariantMap" },
        { "List", "QVariantList" },
        { "String", "QString" },
        { "StringList", "QStringList" },
        { "ByteArray", "QByteArray" },
        { "BitArray", "QBitArray" },
        { "Date", "QDate" },
        { "Time", "QTime" },
        { "DateTime", "QDateTime" },
        { "Url", "QUrl" },
        { "Locale", "QLocale" },
        { "Rect", "QRect" },
        { "RectF", "QRectF" },
        { "Size", "QSize" },
        { "SizeF", "QSizeF" },
        { "Line", "QLine" },
        { "LineF", "QLineF" },
        { "Point", "QPoint" },
        { "PointF", "QPointF" },
        { "RegExp", "QRegExp" },
        { "RegularExpression", "QRegularExpression" },
        { "Hash", "QVariantHash" },
        { "EasingCurve", "QEasingCurve" },
        { "Uuid", "QUuid" },
        { "ModelIndex", "QModelIndex" },
        { "PersistentModelIndex", "QPersistentModelIndex" },
        { "LastCoreType", "LastCoreType" },
        { "Font", "QFont" },
        { "Pixmap", "QPixmap" },
        { "Brush", "QBrush" },
        { "Color", "QColor" },
        { "Palette", "QPalette" },
        { "Image", "QImage" },
        { "Polygon", "QPolygon" },
        { "Region", "QRegion" },
        { "Bitmap", "QBitmap" },
        { "Cursor", "QCursor" },
        { "KeySequence", "QKeySequence" },
        { "Pen", "QPen" },
        { "TextLength", "QTextLength" },
        { "TextFormat", "QTextFormat" },
        { "Matrix", "QMatrix" },
        { "Transform", "QTransform" },
        { "Matrix4x4", "QMatrix4x4" },
        { "Vector2D", "QVector2D" },
        { "Vector3D", "QVector3D" },
        { "Vector4D", "QVector4D" },
        { "Quaternion", "QQuaternion" },
        { "PolygonF", "QPolygonF" },
        { "Icon", "QIcon" },
        { "LastGuiType", "LastGuiType" },
        { "SizePolicy", "QSizePolicy" },
        { "UserType", "User" },
    };
    auto it = values.find(name);
    if (it != values.end())
        return it->second;
    return "";
}

void QVariantDeprecated::checkTypeEnum(clang::Stmt* stm)
{
    auto refExpr = dyn_cast<DeclRefExpr>(stm);
    if (!refExpr)
        return;
    auto type = refExpr->getType();
    auto val = dyn_cast<EnumConstantDecl>(refExpr->getDecl());
    if (!val || clazy::simpleTypeName(type, m_context->ci.getLangOpts()) != "QVariant::Type")
        return;
    auto replacement = variantTypeToMetaType(clazy::name(val));
    if (replacement.empty())
        return;
    if (auto rec = clazy::firstContextOfType<CXXRecordDecl>(clazy::contextForDecl(m_context->lastDecl))) {
        auto name = clazy::name(rec);
        if (name == "QVariant" || name == /* QVariant:: */ "Private")
            return; // skip the QVariant class itself
    }

    std::string error = ("QVariant::" + clazy::name(val) + " is deprecated, use QMetaType::" + replacement).str();
    auto fixit = clazy::createReplacement(refExpr->getSourceRange(), ("QMetaType::" + replacement).str());
    emitWarning(clazy::getLocStart(refExpr), std::move(error), {fixit});
}

void QVariantDeprecated::checkTypeFn(clang::Stmt* stm)
{
    auto callExpr = dyn_cast<CXXMemberCallExpr>(stm);
    if (!callExpr)
        return;

    CXXMethodDecl *methodDecl = callExpr->getMethodDecl();
    if (!methodDecl || clazy::name(methodDecl) != "type")
        return;

    CXXRecordDecl *decl = methodDecl->getParent();
    if (!decl)
        return;

    auto className = clazy::name(decl);
    if (className != "QVariant")
        return;

    auto callee = dyn_cast<MemberExpr>(callExpr->getCallee());
    if (!callee)
        return;
    auto fixit = clazy::createReplacement(callee->getMemberLoc(), "userType");
    emitWarning(callee->getMemberLoc(), ("Use " + className + "::userType() instead of " + className + "::type()").str(), { fixit });
}


void QVariantDeprecated::VisitStmt(clang::Stmt *stm)
{
    checkTypeEnum(stm);
    checkTypeFn(stm);
}
