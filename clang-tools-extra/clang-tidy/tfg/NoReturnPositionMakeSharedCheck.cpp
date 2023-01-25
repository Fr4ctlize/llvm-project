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
#include "clang/Tooling/Transformer/RangeSelector.h"
#include "clang/Tooling/Transformer/RewriteRule.h"
#include "clang/Tooling/Transformer/Stencil.h"

#include "clang/AST/PrettyPrinter.h"
#include "clang/Basic/LangOptions.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/Debug.h"

#include <string>

using namespace ::clang::ast_matchers;
using namespace ::clang::transformer;

namespace clang {
namespace tidy {
namespace tfg {

auto preferUniquePtrOverSharedPtr() -> RewriteRuleWith<std::string> {
  const auto meta = cat("prefer ``unique_ptr`` over ``shared_ptr`` as a return type for factory functions");
  const std::string sharedPtr = "::std::shared_ptr",
                    makeShared = "::std::make_shared",
                    operatorEq = "operator=",
                    makeSharedInvocation = "makeSharedInvocation",
                    varTypeSpec = "varTypeSpec",
                    funcTypeSpec = "funcTypeSpec",
                    varInitWithMakeShared = "varInitWithMakeShared",
                    varAssignedToWithMakeShared = "varAssignedToWithMakeShared",
                    operatorClassTemplate = "operatorClassTemplate",
                    returnedVar = "returnedVar",
                    candidateFunction = "candidateFunction",
                    makeUniqueName = "make_unique",
                    uniquePtrName = "unique_ptr",
                    localVar = "localVar";

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
                  decl(equalsBoundNode(localVar)))))))));
  
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
                      equalsBoundNode(localVar))))),
              has(MakeSharedCallExpr)))));

  auto ReturnedSharedPtrVar =
    varDecl(
      OfSharedPtrType,
      decl().bind(localVar),
      IsReturnedByOwningFunction,
      eachOf(
        InitWithMakeShared,
        ForEachAssignmentOfMakeShared));

  auto ReturnsSharedPtr =
    allOf(
      returns(
        hasDeclaration(SharedPtrDecl)),
      optionally(hasReturnTypeLoc(typeLoc().bind(funcTypeSpec))));

  auto ReturnsFunctionParameter = 
    allOf(
      forEachDescendant(
        returnStmt(
          hasReturnValue(
            declRefExpr(
              to(
                decl().bind(returnedVar)))))),
      hasAnyParameter(
        decl(equalsBoundNode(returnedVar))));

  auto LocalVar =
    varDecl().bind(localVar);

  auto RefToLocalVar =
    declRefExpr(
      to(
        decl(
          equalsBoundNode(localVar))));

  auto ReturnOfLocalVar =
    returnStmt(
      hasReturnValue(RefToLocalVar));

  // Function calls are never safe for our conservative analysis
  auto CalleeIsSafe = unless(anything());
  
  auto FunctionEscapePoint =
    callExpr(
      callee(
        declRefExpr(
          to(
            functionDecl(
              unless(cxxMethodDecl()))))),
      hasAnyArgument(
        RefToLocalVar),
      unless(CalleeIsSafe));

  auto MemberCalleeIsSafe =
    allOf(
      onImplicitObjectArgument(RefToLocalVar),
      has(
        memberExpr(
          hasDeclaration(
            cxxMethodDecl(
              unless(
                anyOf(
                  hasName("use_count"),
                  hasName("unique"),
                  hasName("owner_before"))))))));

  auto MemberCallEscapePoint =
    cxxMemberCallExpr(
      hasDescendant(RefToLocalVar),
      unless(MemberCalleeIsSafe));
  
  auto CallsMethodOfInstantiatedClassTemplate =
    has(
      declRefExpr(
        to(
          cxxMethodDecl(
            ofClass(
              classTemplateSpecializationDecl(
                hasSpecializedTemplate(
                  classTemplateDecl().bind(operatorClassTemplate))))))));

  auto IsCalledOnVarOfTypeEqualToOrDerivedFromInstantiatedClassTemplate =
    has(
      declRefExpr(
        to(
          varDecl(
            equalsBoundNode(localVar),
            hasType(
              classTemplateSpecializationDecl(
                hasSpecializedTemplate(
                  classTemplateDecl(
                    anyOf(
                      equalsBoundNode(operatorClassTemplate),
                      has(
                        cxxRecordDecl(
                          hasAnyBase(
                            cxxBaseSpecifier(
                              hasType(
                                classTemplateDecl(equalsBoundNode(operatorClassTemplate))))))))))))))));

  auto OperatorCallIsSafe =
    allOf(
      CallsMethodOfInstantiatedClassTemplate,
      IsCalledOnVarOfTypeEqualToOrDerivedFromInstantiatedClassTemplate);

  auto OperatorCallEscapePoint =
    cxxOperatorCallExpr(
      anyOf(
        hasEitherOperand(RefToLocalVar),
        hasUnaryOperand(RefToLocalVar)),
      unless(OperatorCallIsSafe));

  auto HasEscapePoint =
    anyOf(
      hasDescendant(FunctionEscapePoint),
      hasDescendant(MemberCallEscapePoint),
      hasDescendant(OperatorCallEscapePoint));

  auto HasEscapingReturnedSharedPtrVar =
    allOf(
      forEachDescendant(LocalVar),
      hasDescendant(ReturnOfLocalVar),
      HasEscapePoint);

  auto CandidateFunction =
    functionDecl(
      ReturnsSharedPtr,
      unless(ReturnsFunctionParameter),
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
