//===--- SharedPtrFactoryFunctionCheck.cpp - clang-tidy -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SharedPtrFactoryFunctionCheck.h"

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

RewriteRuleWith<std::string> makeSharedPtrFactoryFunctionRewriteRule() {
  const auto meta = cat("prefer ``unique_ptr`` over ``shared_ptr`` as a return type for factory functions");
  const std::string IdSharedPtr = "::std::shared_ptr",
                    IdMakeShared = "::std::make_shared",
                    IdOperatorEq = "operator=",
                    IdMakeSharedInvocation = "makeSharedInvocation",
                    IdVarTypeSpec = "varTypeSpec",
                    IdFuncTypeSpec = "funcTypeSpec",
                    IdReturnedVar = "returnedVar",
                    IdCandidateFunction = "candidateFunction",
                    IdMakeUnique = "make_unique",
                    uniquePtrName = "unique_ptr",
                    IdLocalVar = "localVar";

  auto SharedPtrDecl =
    cxxRecordDecl(hasName(IdSharedPtr));

  auto MakeSharedDecl =
    functionDecl(hasName(IdMakeShared));

  auto OperatorEqualsDecl =
    functionDecl(hasName(IdOperatorEq));

  auto MakeSharedCallExpr =
    callExpr(
      callee(MakeSharedDecl),
      has(
        declRefExpr().bind(IdMakeSharedInvocation)));

  auto ReturnsMakeShared =
    returnStmt(hasReturnValue(MakeSharedCallExpr));

  auto OfSharedPtrType =
    allOf(
      hasType(SharedPtrDecl),
      optionally(
        hasTypeLoc(
          elaboratedTypeLoc(
            hasNamedTypeLoc(
              templateSpecializationTypeLoc().bind(IdVarTypeSpec))))));

  auto InitWithMakeShared =
    hasInitializer(MakeSharedCallExpr);

  auto IsReturnedByOwningFunction =
    hasAncestor(
      decl(
        equalsBoundNode(IdCandidateFunction),
        hasDescendant(
          returnStmt(
            hasReturnValue(
              declRefExpr(
                to(
                  decl(equalsBoundNode(IdLocalVar)))))))));
  
  auto ForEachAssignmentOfMakeShared =
    hasAncestor(
      decl(
        equalsBoundNode(IdCandidateFunction),
          forEachDescendant(
            cxxOperatorCallExpr(
              has(
                declRefExpr(to(OperatorEqualsDecl))),
              has(
                declRefExpr(
                  to(
                    varDecl(
                      equalsBoundNode(IdLocalVar))))),
              has(MakeSharedCallExpr)))));

  auto ReturnedSharedPtrVarInit =
    varDecl(
      OfSharedPtrType,
      decl().bind(IdLocalVar),
      IsReturnedByOwningFunction,
      optionally(InitWithMakeShared));

  auto ReturnedSharedPtrVarAssignment =
    varDecl(
      OfSharedPtrType,
      decl().bind(IdLocalVar),
      IsReturnedByOwningFunction,
      ForEachAssignmentOfMakeShared);

  auto ReturnsSharedPtr =
    allOf(
      returns(
        hasDeclaration(SharedPtrDecl)),
      optionally(
        hasReturnTypeLoc(
          typeLoc(
            elaboratedTypeLoc(
              hasNamedTypeLoc(
                templateSpecializationTypeLoc().bind(IdFuncTypeSpec)))))));

  auto ReturnsFunctionParameter = 
    allOf(
      forEachDescendant(
        returnStmt(
          hasReturnValue(
            declRefExpr(
              to(
                decl().bind(IdReturnedVar)))))),
      hasAnyParameter(
        decl(equalsBoundNode(IdReturnedVar))));

  auto LocalVar =
    varDecl().bind(IdLocalVar);

  auto RefToLocalVar =
    declRefExpr(
      to(
        decl(
          equalsBoundNode(IdLocalVar))));

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
                  hasName("owner_before"),
                  hasName("swap"))))))));

  auto MemberCallEscapePoint =
    cxxMemberCallExpr(
      hasDescendant(RefToLocalVar),
      unless(MemberCalleeIsSafe));
  
  // auto CallsMethodOfInstantiatedClassTemplate =
  //   has(
  //     declRefExpr(
  //       to(
  //         cxxMethodDecl(
  //           ofClass(
  //             classTemplateSpecializationDecl(
  //               hasSpecializedTemplate(
  //                 classTemplateDecl().bind(IdOperatorClassTemplate))))))));

  // auto IsCalledOnVarOfTypeEqualToOrDerivedFromInstantiatedClassTemplate =
  //   has(
  //     declRefExpr(
  //       to(
  //         varDecl(
  //           equalsBoundNode(IdLocalVar),
  //           hasType(
  //             classTemplateSpecializationDecl(
  //               hasSpecializedTemplate(
  //                 classTemplateDecl(
  //                   anyOf(
  //                     equalsBoundNode(IdOperatorClassTemplate),
  //                     has(
  //                       cxxRecordDecl(
  //                         hasAnyBase(
  //                           cxxBaseSpecifier(
  //                             hasType(
  //                               classTemplateDecl(equalsBoundNode(IdOperatorClassTemplate))))))))))))))));

  // auto OperatorCallIsSafe =
  //   allOf(
  //     CallsMethodOfInstantiatedClassTemplate,
  //     IsCalledOnVarOfTypeEqualToOrDerivedFromInstantiatedClassTemplate);

  // We don't actually need to look at where it's implemented
  // like the above implementation did, because all the relevant
  // operators are declared as member functions and therefore
  // cannot be redefined.
  
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
      unless(HasEscapingReturnedSharedPtrVar)).bind(IdCandidateFunction);
  
  auto RuleRewriteReturnedMakeShared =
    makeRule(
      traverse(TK_IgnoreUnlessSpelledInSource, ReturnsMakeShared),
      changeTo(name(IdMakeSharedInvocation), cat(IdMakeUnique)));
  
  auto RuleRewriteVarTypeAndInitMakeShared =
    makeRule(
      traverse(TK_IgnoreUnlessSpelledInSource, ReturnedSharedPtrVarInit),
      flatten(
        edit(changeTo(name(IdMakeSharedInvocation), cat(IdMakeUnique))),
        ifBound(IdVarTypeSpec, changeTo(name(IdVarTypeSpec), cat(uniquePtrName)))));
  
  auto RuleRewriteVarAssignedMakeShared =
    makeRule(
      traverse(TK_IgnoreUnlessSpelledInSource, ReturnedSharedPtrVarAssignment),
      edit(changeTo(name(IdMakeSharedInvocation), cat(IdMakeUnique))));

  auto RuleReplaceSharedWithUniqueInFactoryFunction = makeRule(
    traverse(TK_IgnoreUnlessSpelledInSource, CandidateFunction),
    flattenVector({
      ifBound(IdFuncTypeSpec, changeTo(name(IdFuncTypeSpec), cat(uniquePtrName))),
      rewriteDescendants(IdCandidateFunction, RuleRewriteReturnedMakeShared),
      rewriteDescendants(IdCandidateFunction, RuleRewriteVarTypeAndInitMakeShared),
      rewriteDescendants(IdCandidateFunction, RuleRewriteVarAssignedMakeShared)}),
    meta);

  return applyFirst({
    RuleReplaceSharedWithUniqueInFactoryFunction
  });
}

SharedPtrFactoryFunctionCheck::SharedPtrFactoryFunctionCheck(StringRef Name, ClangTidyContext *Context)
  : TransformerClangTidyCheck(Name, Context) {
    setRule(makeSharedPtrFactoryFunctionRewriteRule());
}

} // namespace pointerusage
} // namespace tidy
} // namespace clang
