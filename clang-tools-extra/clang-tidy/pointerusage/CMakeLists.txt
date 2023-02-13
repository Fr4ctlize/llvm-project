set(LLVM_LINK_COMPONENTS
  FrontendOpenMP
  Support
  )

add_clang_library(clangTidyPointerUsageModule
  SharedPtrFactoryFunctionCheck.cpp
  PointerUsageTidyModule.cpp
  SharedptrfunctionparameterCheck.cpp

  LINK_LIBS
  clangTidy
  clangTidyMiscModule
  clangTidyModernizeModule
  clangTidyReadabilityModule
  clangTidyUtils

  DEPENDS
  omp_gen
  )

clang_target_link_libraries(clangTidyCppCoreGuidelinesModule
  PRIVATE
  clangAST
  clangASTMatchers
  clangBasic
  clangLex
  clangSerialization
  clangTooling
  )
