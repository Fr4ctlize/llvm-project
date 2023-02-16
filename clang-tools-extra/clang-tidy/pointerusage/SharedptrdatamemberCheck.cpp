//===--- SharedptrdatamemberCheck.cpp - clang-tidy ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SharedptrdatamemberCheck.h"

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
namespace pointerusage {

RewriteRuleWith<std::string> makeSharedPtrDataMemberRewriteRule() {
  const auto meta = cat("prefer ``unique_ptr`` over ``shared_ptr`` as a type for class-internal data members.");
  const std::string IdMakeSharedInvocation = "makeSharedInvocation",
                    IdDataMemberTypeSpec = "dataMemberTypeSpec",
                    IdCandidateDataMember = "candidateDataMember",
                    IdLocalVar = "localVar";
  
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
        declRefExpr().bind(IdMakeSharedInvocation)));

  auto RefToDataMember =
    memberExpr(
      hasDeclaration(
        decl(
          equalsBoundNode(IdCandidateDataMember))));

  auto CalleeIsSafe =
    unless(anything());
    
  auto FunctionEscapePoint =
    callExpr(
      callee(
        declRefExpr(
          to(
            functionDecl(
              unless(cxxMethodDecl()))))),
      hasAnyArgument(
        RefToDataMember),
      unless(CalleeIsSafe));

  auto MemberCalleeIsSafe =
    allOf(
      onImplicitObjectArgument(RefToDataMember),
      has(
        memberExpr(
          hasDeclaration(
            cxxMethodDecl(
              unless(
                anyOf(
                  hasName("use_count"),
                  hasName("unique"),
                  hasName("owner_before"),
                  hasName("swap"))))))));

  auto MemberCallEscapePoint =
    cxxMemberCallExpr(
      hasDescendant(RefToDataMember),
      unless(MemberCalleeIsSafe));

  auto AssignmentNotOfMakeShared =
    allOf(
      callee(
        declRefExpr(
          to(OperatorEqualsDecl))),
      hasRHS(
        expr(unless(MakeSharedCallExpr))));

  auto OperatorCallIsSafe =
    allOf(
      callee(
        declRefExpr(
          to(
            cxxMethodDecl()))),
      unless(AssignmentNotOfMakeShared));

  auto OperatorCallEscapePoint =
    cxxOperatorCallExpr(
      anyOf(
        hasEitherOperand(RefToDataMember),
        hasUnaryOperand(RefToDataMember)),
      unless(OperatorCallIsSafe));

  auto ReturnStmtEscapePoint =
    returnStmt(
      hasReturnValue(RefToDataMember));

  auto HasEscapePoint =
    anyOf(
      hasDescendant(FunctionEscapePoint),
      hasDescendant(MemberCallEscapePoint),
      hasDescendant(OperatorCallEscapePoint),
      hasDescendant(ReturnStmtEscapePoint));

  auto EscapesFromClass =
    hasParent(
      cxxRecordDecl(
        hasDescendant(
          functionDecl(
            HasEscapePoint))));

  auto BindAssignedCallsToMakeShared =
    hasParent(
      cxxRecordDecl(
        forEachDescendant(
          cxxOperatorCallExpr(
            has(
              declRefExpr(to(OperatorEqualsDecl))),
            has(RefToDataMember),
            has(MakeSharedCallExpr)))));
  
  auto BindDataMemberTypeSpecifier =
    hasTypeLoc(
      elaboratedTypeLoc(
      hasNamedTypeLoc(
        templateSpecializationTypeLoc().bind(IdDataMemberTypeSpec))));

  auto CandidateDataMember =
    fieldDecl(
      hasType(SharedPtrDecl),
      isPrivate(),
      decl().bind(IdCandidateDataMember),
      unless(EscapesFromClass),
      eachOf(
        BindDataMemberTypeSpecifier,
        BindAssignedCallsToMakeShared));
  
  auto RuleReplaceSharedPtrWithUniquePtrForPrivateNonEscapingDataMembers =
    makeRule(
      traverse(TK_IgnoreUnlessSpelledInSource, CandidateDataMember),
      flatten(
        ifBound(IdDataMemberTypeSpec, changeTo(name(IdDataMemberTypeSpec), cat("unique_ptr"))),
        ifBound(IdMakeSharedInvocation, changeTo(name(IdMakeSharedInvocation), cat("make_unique")))),
      meta);

  return applyFirst({
    RuleReplaceSharedPtrWithUniquePtrForPrivateNonEscapingDataMembers
  });
}

SharedptrdatamemberCheck::SharedptrdatamemberCheck(StringRef Name, ClangTidyContext *Context) 
  : TransformerClangTidyCheck(Name, Context) {
  setRule(makeSharedPtrDataMemberRewriteRule());
}

} // namespace pointerusage
} // namespace tidy
} // namespace clang
