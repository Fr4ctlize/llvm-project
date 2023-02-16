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
#include "SharedPtrFactoryFunctionCheck.h"
#include "SharedptrdatamemberCheck.h"
#include "SharedptrfunctionparameterCheck.h"

namespace clang {
namespace tidy {
namespace pointerusage {

/// A module containing checks of the C++ Core Guidelines
class PointerUsageModule : public ClangTidyModule {
public:
  void addCheckFactories(ClangTidyCheckFactories &CheckFactories) override {
    CheckFactories.registerCheck<SharedptrdatamemberCheck>(
        "pointerusage-SharedPtrDataMember");
    CheckFactories.registerCheck<SharedPtrFactoryFunctionCheck>(
        "pointerusage-SharedPtrFactoryFunction");
    CheckFactories.registerCheck<SharedptrfunctionparameterCheck>(
        "pointerusage-SharedPtrFunctionParameter");
  }
};

// Register the LLVMTidyModule using this statically initialized variable.
static ClangTidyModuleRegistry::Add<PointerUsageModule>
    X("pointerusage-module", "Adds checks for encouraging better use of smart pointers.");

} // namespace pointerusage

// This anchor is used to force the linker to link in the generated object file
// and thus register the PointerUsageModule.
volatile int PointerUsageModuleAnchorSource = 0;

} // namespace tidy
} // namespace clang
