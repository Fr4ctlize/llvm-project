#include <memory>

auto returnMakeShared() {
	return std::make_shared<int>(42);
}

auto rmsWithBranch(const bool branchCondition) {
    if (branchCondition) {
    	return std::make_shared<int>(42);
    } else {
    	return std::make_shared<int>(21);
    }
}

std::shared_ptr<int> rmsWithReturnType() {
    return std::make_shared<int>(42);
}

auto rmsWithTrailingReturnType() -> std::shared_ptr<int> {
    return std::make_shared<int>(42);
}

std::shared_ptr<int> returnAutoVar() {
    auto v = std::make_shared<int>(42);
    return v;
}

std::shared_ptr<int> returnSecondAutoVar() {
    auto v = std::make_shared<int>(42);
    auto w = std::make_shared<int>(21);
    return w;
}

std::shared_ptr<int> returnReassignedAutoVar() {
    auto v = std::make_shared<int>(42);
    v = std::make_shared<int>(21);
    return v;
}

std::shared_ptr<int> returnAutoVarWithBranch(const bool branchCondition) {
    if (branchCondition) {
        auto v = std::make_shared<int>(42);
        return v;
    } else {
        auto v = std::make_shared<int>(21);
        return v;
    }
}

std::shared_ptr<int> returnTypedVar() {
    std::shared_ptr<int> v = std::make_shared<int>(42);
    return v;
}

std::shared_ptr<int> returnSecondTypedVar() {
    std::shared_ptr<int> v = std::make_shared<int>(42);
    std::shared_ptr<int> w = std::make_shared<int>(21);
    return w;
}

std::shared_ptr<int> returnTypedVarWithCond(const bool branchCondition) {
    if (branchCondition) {
        std::shared_ptr<int> v = std::make_shared<int>(42);
        return v;
    } else {
        std::shared_ptr<int> v = std::make_shared<int>(21);
        return v;
    }
}

struct A {
    template<typename T>
    A(const std::shared_ptr<T>& p) {}
};
A returnMakeSharedConvertedToNonSharedPtrType() {
    return std::make_shared<int>(42);
}

A returnSharedPtrVarConvertedToNonSharedPtrType() {
    std::shared_ptr<int> v = std::make_shared<int>(42);
    return v;
}

std::shared_ptr<int> returnFunctionArgument(std::shared_ptr<int> v) {
    return v;
}

std::shared_ptr<int> returnFunctionArgumentWithAlternativeMakeShared(std::shared_ptr<int> v) {
    if (v.unique()) {
        return v;
    } else {
        return std::make_shared<int>(42);
    }
}

void doSomethingUnsafeWithSharedPtr(std::shared_ptr<int>) {}
std::shared_ptr<int> returnVarEscapedToNonMemberFunction() {
    std::shared_ptr<int> v = std::make_shared<int>(42);
    doSomethingUnsafeWithSharedPtr(v);
    return v;
}

std::shared_ptr<int> returnVarEscapedToNonMemberFunctionWithAlternative() {
    std::shared_ptr<int> v = std::make_shared<int>(42);
    if (53 % 9 == 2) {
        doSomethingUnsafeWithSharedPtr(v);
        return v;
    } else {
        v;
    }
}

std::shared_ptr<int> returnVarEscapedToUseCount() {
    std::shared_ptr<int> v = std::make_shared<int>(42);
    int x = v.use_count();
    return v;
}
std::shared_ptr<int> returnVarEscapedToUnique() {
    std::shared_ptr<int> v = std::make_shared<int>(42);
    bool b = v.unique();
    return v;
}
std::shared_ptr<int> returnVarEscapedToOwnerBefore() {
    std::shared_ptr<int> v = std::make_shared<int>(42);
    std::shared_ptr<int> w = std::make_shared<int>(21);
    bool b = v.owner_before(w);
    return v;
}
std::shared_ptr<int> returnVarEscapedToSwap() {
    std::shared_ptr<int> v = std::make_shared<int>(42);
    std::shared_ptr<int> w = std::make_shared<int>(21);
    v.swap(w);
    return v;
}

std::shared_ptr<int> returnVarEscapedToUseGet() {
    std::shared_ptr<int> v = std::make_shared<int>(42);
    int* x = v.get();
    return v;
}
std::shared_ptr<int> returnVarEscapedToReset() {
    std::shared_ptr<int> v = std::make_shared<int>(42);
    v.reset();
    return v;
}

std::shared_ptr<int> returnVarEscapedToOperatorAssignment() {
    std::shared_ptr<int> v = std::make_shared<int>(42);
    v = std::make_shared<int>(21);
    return v;
}
std::shared_ptr<int> returnVarEscapedToOperatorDereference() {
    std::shared_ptr<int> v = std::make_shared<int>(42);
    *v = 21;
    return v;
}
struct B { int x; };
std::shared_ptr<B> returnVarEscapedToOperatorMemberAccessArrow() {
    std::shared_ptr<B> v = std::make_shared<B>(B { 42 });
    v->x = 21;
    return v;
}
std::shared_ptr<int[]> returnVarEscapedToOperatorSubscript() {
    std::shared_ptr<int[]> v = std::make_shared<int[]>(3);
    v[0] = 42;
    return v;
}

std::shared_ptr<int> returnVarEscapedToPoisoningOperatorAssignment(std::shared_ptr<int> w) {
    std::shared_ptr<int> v = std::make_shared<int>(42);
    v = w;
    return v;
}