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
#include "clang/Tooling/Transformer/RangeSelector.h"
#include "clang/Tooling/Transformer/Stencil.h"

#include "clang/Basic/LangOptions.h"
#include "clang/AST/PrettyPrinter.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Error.h"

using namespace ::clang::ast_matchers;
using namespace ::clang::transformer;

namespace clang {
namespace tidy {
namespace tfg {

using MatchResult = MatchFinder::MatchResult;

static Expected<DynTypedNode> getNode(const ast_matchers::BoundNodes &Nodes,
                                      StringRef ID) {
  auto &NodesMap = Nodes.getMap();
  auto It = NodesMap.find(ID);
  if (It == NodesMap.end())
    return llvm::make_error<llvm::StringError>(llvm::errc::invalid_argument, "ID not bound: " + ID);
  return It->second;
}

auto nameWithoutTemplateArgs(std::string ID) -> RangeSelector {
  return [ID](const MatchResult &Result) -> Expected<CharSourceRange> {
    Expected<DynTypedNode> N = getNode(Result.Nodes, ID);
    if (!N)
      return N.takeError();
    auto &Node = *N;
    if (const auto *TST = Node.get<TemplateSpecializationTypeLoc>()) {
      // if (const auto TST = T->getNamedTypeLoc().getAs<TemplateSpecializationTypeLoc>()) {
        auto range = TST->getSourceRange();
        range.setEnd(TST->getTemplateNameLoc());
        return CharSourceRange::getTokenRange(range);
      // }
    }
    return name(ID)(Result);
  };
}

auto preferUniquePtrOverSharedPtr() -> RewriteRuleWith<std::string> {
  auto msg = cat("Prefer ``make_unique`` over ``make_shared``");

  auto SharedPtrDecl =
    cxxRecordDecl(hasName("::std::shared_ptr"));

  auto MakeSharedDecl =
    functionDecl(hasName("::std::make_shared"));

  auto OperatorEqualsDecl =
    functionDecl(hasName("operator="));

  auto MakeSharedCallExpr =
    callExpr(
      callee(MakeSharedDecl),
      has(
        declRefExpr().bind("make_shared_invocation")));

  auto ReturnsMakeShared =
    returnStmt(hasReturnValue(MakeSharedCallExpr));

  auto SharedPtrTypeLoc =
    allOf(
      hasType(SharedPtrDecl),
      hasTypeLoc(
        elaboratedTypeLoc(
          hasNamedTypeLoc(
            templateSpecializationTypeLoc().bind("varTypeSpec")))));

  auto InitWithMakeShared =
    hasInitializer(MakeSharedCallExpr);

  auto VarInitWithMakeShared =
    varDecl(
      InitWithMakeShared,
      optionally(SharedPtrTypeLoc)).bind("varInitWithMakeShared");

  auto VarAssignedMakeShared =
    cxxOperatorCallExpr(
      has(
        declRefExpr(to(OperatorEqualsDecl))),
      has(
        declRefExpr(
          to(varDecl(optionally(SharedPtrTypeLoc)).bind("varAssignedToWithMakeShared")))),
      has(MakeSharedCallExpr));

  auto ReturnsReferenceToDecl =
    returnStmt(
      hasReturnValue(
        declRefExpr(
          to(decl(
            anyOf(
              equalsBoundNode("varInitWithMakeShared"),
              equalsBoundNode("varAssignedToWithMakeShared")))))));

  auto ReturnSharedPtrVar =
    functionDecl(
      anyOf(
        forEachDescendant(
          VarInitWithMakeShared),
        forEachDescendant(
          VarAssignedMakeShared)),
      hasDescendant(ReturnsReferenceToDecl));

  auto RuleUntemplatedRPMS = makeRule(
    traverse(TK_IgnoreUnlessSpelledInSource,
      returnStmt(
        hasReturnValue(MakeSharedCallExpr))),
    changeTo(name("make_shared_invocation"), cat("make_unique")),
    msg
  );

  auto RuleUntemplatedReturnSharedPtrVar = makeRule(
    traverse(TK_IgnoreUnlessSpelledInSource, ReturnSharedPtrVar),
    flatten(
      edit(changeTo(name("make_shared_invocation"), cat("make_unique"))),
      ifBound("varTypeSpec", changeTo(nameWithoutTemplateArgs("varTypeSpec"), cat("unique_ptr")))),
    msg
  );

  return applyFirst({
    RuleUntemplatedRPMS,
    RuleUntemplatedReturnSharedPtrVar,
  });
}

NoReturnPositionMakeSharedCheck::NoReturnPositionMakeSharedCheck(StringRef Name, ClangTidyContext *Context)
  : TransformerClangTidyCheck(Name, Context) {
    setRule(preferUniquePtrOverSharedPtr());
}

} // namespace tfg
} // namespace tidy
} // namespace clang
