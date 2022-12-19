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

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace tfg {

void NoReturnPositionMakeSharedCheck::registerMatchers(MatchFinder *Finder) {
  auto UntemplatedRPMS = returnStmt(
        hasReturnValue(
          callExpr(
            callee(
              functionDecl(
                hasName("::std::make_shared"))
              .bind("make_shared")))
          .bind("make_shared_invocation")))
      .bind("returning_make_shared");
  
  Finder->addMatcher(traverse(TK_IgnoreUnlessSpelledInSource, UntemplatedRPMS), this);
}

void NoReturnPositionMakeSharedCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MatchedExpr = Result.Nodes.getNodeAs<CallExpr>("make_shared_invocation");
  if (!MatchedExpr) {
    return;
  }
  diag(MatchedExpr->getExprLoc(), "make_shared at %0");
}

} // namespace tfg
} // namespace tidy
} // namespace clang
