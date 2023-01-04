//===--- NoreturnpositionmakesharedCheck.cpp - clang-tidy -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "NoReturnPositionMakeSharedCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Tooling/Transformer/RewriteRule.h"
#include "clang/Tooling/Transformer/Stencil.h"
#include "llvm/ADT/StringRef.h"

using namespace ::clang::ast_matchers;
using namespace ::clang::transformer;

namespace clang {
namespace tidy {
namespace tfg {

RewriteRuleWith<std::string> preferUniquePtrOverSharedPtr() {
  auto msg = cat("Prefer ``make_unique`` over ``make_shared``");

  auto MakeSharedCallExpr =
    callExpr(
      callee(
        functionDecl(hasName("::std::make_shared"))),
      hasDescendant(
        declRefExpr()
          .bind("make_shared_invocation")));
  
  auto HasSharedPtrType =
    hasType(
      cxxRecordDecl(
        hasName("::std::shared_ptr")));
  
  auto SharedPtrTypeLoc =
    allOf(
      HasSharedPtrType,
      hasTypeLoc(
        typeLoc().bind("varTypeSpec")));
  
  auto DeclReturn = 
    returnStmt(
      hasReturnValue(
        declRefExpr(
          to(decl().bind("returnedDecl")))));

  auto ReturnedVarInitWithMakeShared = 
    varDecl(
      hasInitializer(MakeSharedCallExpr),
      optionally(SharedPtrTypeLoc),
      equalsBoundNode("returnedDecl"))
    .bind("makeSharedVarInit");

  auto UntemplatedRPMS = makeRule(
    traverse(TK_IgnoreUnlessSpelledInSource,
      returnStmt(
        hasReturnValue(MakeSharedCallExpr))),
    changeTo(name("make_shared_invocation"), cat("make_unique")),
    msg
  );

  auto UntemplatedReturnSharedPtrVar = makeRule(
    traverse(TK_IgnoreUnlessSpelledInSource,
      functionDecl(
        hasDescendant(DeclReturn),
        hasDescendant(ReturnedVarInitWithMakeShared))),
    flatten(
      edit(changeTo(name("make_shared_invocation"), cat("make_unique"))),
      ifBound("varTypeSpec", changeTo(name("varTypeSpec"), cat("unique_ptr")))),
    msg
  );

  return applyFirst({
    UntemplatedRPMS,
    UntemplatedReturnSharedPtrVar,
  });
}

NoReturnPositionMakeSharedCheck::NoReturnPositionMakeSharedCheck(StringRef Name, ClangTidyContext *Context)
  : TransformerClangTidyCheck(Name, Context) {
    setRule(preferUniquePtrOverSharedPtr());
}

} // namespace tfg
} // namespace tidy
} // namespace clang
