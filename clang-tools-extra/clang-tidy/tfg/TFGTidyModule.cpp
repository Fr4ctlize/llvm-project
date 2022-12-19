//===-- CppCoreGuidelinesTidyModule.cpp - clang-tidy ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "../ClangTidy.h"
#include "../ClangTidyModule.h"
#include "../ClangTidyModuleRegistry.h"
#include "NoReturnPositionMakeSharedCheck.h"

namespace clang {
namespace tidy {
namespace tfg {

/// A module containing checks of the C++ Core Guidelines
class TFGModule : public ClangTidyModule {
public:
  void addCheckFactories(ClangTidyCheckFactories &CheckFactories) override {
    CheckFactories.registerCheck<NoReturnPositionMakeSharedCheck>(
        "tfg-NoReturnPositionMakeShared");
  }
};

// Register the LLVMTidyModule using this statically initialized variable.
static ClangTidyModuleRegistry::Add<TFGModule>
    X("tfg-module", "Adds checks developed in Francisco Ortiz's Undergraduate Degree Project.");

} // namespace tfg

// This anchor is used to force the linker to link in the generated object file
// and thus register the TFGModule.
volatile int TFGModuleAnchorSource = 0;

} // namespace tidy
} // namespace clang
