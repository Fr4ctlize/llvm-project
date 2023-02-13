//===--- SharedPtrFactoryFunctionCheck.h - clang-tidy ---------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_POINTERUSAGE_SHAREDPTRFACTORYFUNCTIONCHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_POINTERUSAGE_SHAREDPTRFACTORYFUNCTIONCHECK_H

#include "../utils/TransformerClangTidyCheck.h"

namespace clang {
namespace tidy {
namespace pointerusage {

/// FIXME: Write a short description.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/pointerusage/NoReturnPositionMakeShared.html
class SharedPtrFactoryFunctionCheck : public utils::TransformerClangTidyCheck {
public:
  SharedPtrFactoryFunctionCheck(StringRef Name, ClangTidyContext *Context);
};

} // namespace pointerusage
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_POINTERUSAGE_SHAREDPTRFACTORYFUNCTIONCHECK_H
