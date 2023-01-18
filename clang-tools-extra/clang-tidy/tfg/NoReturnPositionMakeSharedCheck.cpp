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

#include <string>

using namespace ::clang::ast_matchers;
using namespace ::clang::transformer;

namespace clang {
namespace tidy {
namespace tfg {

auto preferUniquePtrOverSharedPtr() -> RewriteRuleWith<std::string> {
  const auto meta = cat("Prefer ``unique_ptr`` over ``shared_ptr``");
  const std::string sharedPtr = "::std::shared_ptr",
                    makeShared = "::std::make_shared",
                    operatorEq = "operator=",
                    makeSharedInvocation = "makeSharedInvocation",
                    varTypeSpec = "varTypeSpec",
                    funcTypeSpec = "funcTypeSpec",
                    varInitWithMakeShared = "varInitWithMakeShared",
                    varAssignedToWithMakeShared = "varAssignedToWithMakeShared",
                    candidateFunction = "candidateFunction",
                    makeUniqueName = "make_unique",
                    uniquePtrName = "unique_ptr",
                    sharedPtrVar = "sharedPtrVar";

  auto SharedPtrDecl =
    cxxRecordDecl(hasName(sharedPtr));

  auto MakeSharedDecl =
    functionDecl(hasName(makeShared));

  auto OperatorEqualsDecl =
    functionDecl(hasName(operatorEq));

  auto MakeSharedCallExpr =
    callExpr(
      callee(MakeSharedDecl),
      has(
        declRefExpr().bind(makeSharedInvocation)));

  auto ReturnsMakeShared =
    returnStmt(hasReturnValue(MakeSharedCallExpr));

  auto OfSharedPtrType =
    allOf(
      hasType(SharedPtrDecl),
      optionally(
        hasTypeLoc(
          elaboratedTypeLoc(
            hasNamedTypeLoc(
              templateSpecializationTypeLoc().bind(varTypeSpec))))));

  auto InitWithMakeShared =
    hasInitializer(MakeSharedCallExpr);

  auto IsReturnedByOwningFunction =
    hasAncestor(
      decl(
        equalsBoundNode(candidateFunction),
        hasDescendant(
          returnStmt(
            hasReturnValue(
              declRefExpr(
                to(
                  decl(equalsBoundNode(sharedPtrVar)))))))));
  
  auto ForEachAssignmentOfMakeShared =
    hasAncestor(
      decl(
        equalsBoundNode(candidateFunction),
          forEachDescendant(
            cxxOperatorCallExpr(
              has(
                declRefExpr(to(OperatorEqualsDecl))),
              has(
                declRefExpr(
                  to(
                    varDecl(
                      equalsBoundNode(sharedPtrVar))))),
              has(MakeSharedCallExpr)))));

  auto ReturnedSharedPtrVar =
    varDecl(
      OfSharedPtrType,
      decl().bind(sharedPtrVar),
      IsReturnedByOwningFunction,
      eachOf(
        InitWithMakeShared,
        ForEachAssignmentOfMakeShared));

  auto ReturnsSharedPtr =
    allOf(
      returns(
        hasDeclaration(SharedPtrDecl)),
      optionally(hasReturnTypeLoc(typeLoc().bind(funcTypeSpec))));

  auto SharedPtrLocal =
    varDecl().bind(sharedPtrVar);

  auto RefToSharedPtrLocal =
    declRefExpr(
      to(
        decl(
          equalsBoundNode(sharedPtrVar))));

  auto ReturnedSharedPtrLocal =
    returnStmt(
      hasReturnValue(RefToSharedPtrLocal));

  auto IsInCompilerStd =
    allOf(
      isInStdNamespace(),
      isExpansionInSystemHeader());

  auto CalleeIsSafe =
    callee(
      declRefExpr(
        to(
          anyOf(
            functionDecl(IsInCompilerStd),
            cxxMethodDecl(
              ofClass(
                decl(IsInCompilerStd)))))));

  auto FunctionEscapePoint =
    callExpr(
      hasAnyArgument(
        RefToSharedPtrLocal),
      unless(CalleeIsSafe));

  auto MemberCalleeIsSafe =
    has(
      memberExpr(
        hasDeclaration(
          cxxMethodDecl(
            unless(
              anyOf(
                hasName("use_count"),
                hasName("unique"),
                hasName("owner_before")))))));

  auto MemberCallEscapePoint =
    cxxMemberCallExpr(
      onImplicitObjectArgument(
        hasType(SharedPtrDecl)),
      hasDescendant(RefToSharedPtrLocal.bind("refToLocal")),
      unless(MemberCalleeIsSafe));

  auto HasEscapePoint =
    anyOf(
      hasDescendant(FunctionEscapePoint),
      hasDescendant(MemberCallEscapePoint));

  auto HasEscapingReturnedSharedPtrVar =
    allOf(
      forEachDescendant(SharedPtrLocal),
      hasDescendant(ReturnedSharedPtrLocal),
      HasEscapePoint);

  auto CandidateFunction =
    functionDecl(
      ReturnsSharedPtr,
      unless(HasEscapingReturnedSharedPtrVar)).bind(candidateFunction);
  
  auto RuleRewriteReturnedMakeShared =
    makeRule(
      traverse(TK_IgnoreUnlessSpelledInSource, ReturnsMakeShared),
      changeTo(name(makeSharedInvocation), cat(makeUniqueName)));
  
  auto RuleRewriteVarTypeAndAssignedMakeShared =
    makeRule(
      traverse(TK_IgnoreUnlessSpelledInSource, ReturnedSharedPtrVar),
      flatten(
        edit(changeTo(name(makeSharedInvocation), cat(makeUniqueName))),
        ifBound(varTypeSpec, changeTo(name(varTypeSpec), cat(uniquePtrName)))));

  auto RuleUntemplatedReplaceSharedWithUnique = makeRule(
    traverse(TK_IgnoreUnlessSpelledInSource, CandidateFunction),
    flattenVector({
      ifBound(funcTypeSpec, changeTo(name(funcTypeSpec), cat(uniquePtrName))),
      rewriteDescendants(candidateFunction, RuleRewriteReturnedMakeShared),
      rewriteDescendants(candidateFunction, RuleRewriteVarTypeAndAssignedMakeShared)}),
    meta);

  return applyFirst({
    RuleUntemplatedReplaceSharedWithUnique
  });
}

NoReturnPositionMakeSharedCheck::NoReturnPositionMakeSharedCheck(StringRef Name, ClangTidyContext *Context)
  : TransformerClangTidyCheck(Name, Context) {
    setRule(preferUniquePtrOverSharedPtr());
}

} // namespace tfg
} // namespace tidy
} // namespace clang
