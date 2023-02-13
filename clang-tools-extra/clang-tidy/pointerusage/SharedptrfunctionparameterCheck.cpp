//===--- SharedptrfunctionparameterCheck.cpp - clang-tidy -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SharedptrfunctionparameterCheck.h"

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

RewriteRuleWith<std::string> makeSharedPtrFucnctionParameterRewriteRule() {
  const auto meta = cat("use T* or T& to indicate non-owning function arguments instead of ``shared_ptr<T>``");
  const std::string IdSharedPtr = "::std::shared_ptr",
                    IdOperatorDeref = "operator*",
                    IdOperatorArrow = "operator->",
                    IdOperatorIndex = "operator[]",
                    IdMemberGet = "get",
                    IdCallToMemberGet = "callToMemberGet",
                    IdCandidateParam = "candidateParam",
                    IdParamRef = "paramRef",
                    IdArrayElementType = "arrayElementType",
                    IdParamTypeSpec = "paramTypeSpec",
                    IdInnerParamTypeSpec = "innerParamTypeSpec",
                    IdParentFunction = "parentFunction";

  auto SharedPtrDecl = cxxRecordDecl(hasName(IdSharedPtr));

  auto RefToParam = declRefExpr(
    to(
      decl(
        equalsBoundNode(IdCandidateParam))));

  auto CalleeIsSafe = unless(anything());

  auto FunctionEscapePoint = callExpr(
    callee(
      declRefExpr(
        to(
          functionDecl(
            unless(cxxMethodDecl()))))),
    hasAnyArgument(
      RefToParam),
    unless(CalleeIsSafe));
      
  auto MemberCalleeIsSafe = allOf(
    onImplicitObjectArgument(RefToParam),
    has(
      memberExpr(
        hasDeclaration(
          cxxMethodDecl(
            hasName(IdMemberGet))))));

  auto MemberCallEscapePoint = cxxMemberCallExpr(
    hasDescendant(RefToParam),
    unless(MemberCalleeIsSafe));

  auto OperatorCallIsSafe = callee(
    declRefExpr(
      to(
        cxxMethodDecl(
          anyOf(
            hasName(IdOperatorDeref),
            hasName(IdOperatorArrow),
            hasName(IdOperatorIndex))))));

  auto OperatorCallEscapePoint = cxxOperatorCallExpr(
    anyOf(
      hasEitherOperand(RefToParam),
      hasUnaryOperand(RefToParam)),
    unless(OperatorCallIsSafe));

  auto HasEscapePoint = anyOf(
    hasDescendant(FunctionEscapePoint),
    hasDescendant(MemberCallEscapePoint),
    hasDescendant(OperatorCallEscapePoint));

  auto SharedPtrFunctionParam = parmVarDecl(
    hasType(SharedPtrDecl),
    decl().bind(IdCandidateParam));

  auto CallToMemberFunctionGet = cxxMemberCallExpr(
    onImplicitObjectArgument(RefToParam.bind(IdParamRef)),
    has(
      memberExpr(
        hasDeclaration(
          cxxMethodDecl(
            hasName(IdMemberGet))))));
    
  auto BindArrayType = hasType(
    hasUnderlyingUnqualifiedType(
      elaboratedType(
        namesType(
          hasUnderlyingUnqualifiedType(
            templateSpecializationType(
                hasTemplateArgument(
                  0,
                  refersToType(
                    hasUnderlyingUnqualifiedType(
                      arrayType(
                        hasElementType(type().bind(IdArrayElementType))))))))))));

  auto BindTypeSpecifier = hasTypeLoc(
    elaboratedTypeLoc(
      hasNamedTypeLoc(
        templateSpecializationTypeLoc(
          hasTemplateArgumentLoc(
            0,
            hasTypeLoc(
              typeLoc().bind(IdInnerParamTypeSpec)))))).bind(IdParamTypeSpec));
  
  auto BindCallsToGet = hasAncestor(
    functionDecl(
      equalsBoundNode(IdParentFunction),
      forEachDescendant(CallToMemberFunctionGet.bind(IdCallToMemberGet))));

  auto CandidateFunctionParameter =  parmVarDecl(
    hasType(SharedPtrDecl),
    decl().bind(IdCandidateParam),
    hasAncestor(
      functionDecl(
        hasAnyParameter(
        decl(
          equalsBoundNode(IdCandidateParam))),
        unless(HasEscapePoint)).bind(IdParentFunction)),
    optionally(BindArrayType),
    eachOf(
      BindTypeSpecifier,
      BindCallsToGet));

  auto RuleReplaceSharedPtrParamWithPointer =
    makeRule(
      traverse(TK_IgnoreUnlessSpelledInSource, CandidateFunctionParameter),
      flatten(
        ifBound(IdParamTypeSpec, changeTo(
          node(IdCandidateParam),
          ifBound(IdArrayElementType,
            cat(describe(IdArrayElementType), " ", name(IdCandidateParam), "[]"),
            cat(node(IdInnerParamTypeSpec), "* ", name(IdCandidateParam))))),
        ifBound(IdCallToMemberGet, changeTo(node(IdCallToMemberGet), cat(node(IdParamRef))))),
      meta);

  return applyFirst({
    RuleReplaceSharedPtrParamWithPointer
  });
}

SharedptrfunctionparameterCheck::SharedptrfunctionparameterCheck(StringRef Name, ClangTidyContext *Context) : TransformerClangTidyCheck(Name, Context) {
  setRule(makeSharedPtrFucnctionParameterRewriteRule());
}

} // namespace pointerusage
} // namespace tidy
} // namespace clang
