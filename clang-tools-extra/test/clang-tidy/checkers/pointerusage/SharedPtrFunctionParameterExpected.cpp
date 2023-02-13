#include <memory>

auto takesSharedPtrArgument(int* p) { }

auto takesSharedPtrAsSecondArgument(double x, int* p) { }

auto takesMultipleSharedPtrArguments(int* p, int* q) { }

auto requiresRewritingCallToGet(int* p) {
    return p;
}

auto requiresRewritingMultipleCallToGet(int* p) {
    auto x = p;
    return p;
}

template<typename T>
void doSomethingEvilWithSharedPtr(T p) { }
auto receivesVarPassedToUserFunc(std::shared_ptr<int> p) {
    doSomethingEvilWithSharedPtr(p);
    return *p;
}

auto argumentPassedToReset(std::shared_ptr<int> p) {
    p.reset();
    return *p;
}
auto argumentPassedToSwap(std::shared_ptr<int> p) {
    std::shared_ptr<int> q = std::make_shared<int>(42);
    p.swap(q);
    return *p;
}
auto argumentPassedToUseCount(std::shared_ptr<int> p) {
    return p.use_count();
}
auto argumentPassedToUnique(std::shared_ptr<int> p) {
    return p.unique();
}
auto argumentPassedToOwnerBefore(std::shared_ptr<int> p) {
    std::shared_ptr<int> q = std::make_shared<int>(42);
    return p.owner_before(q);
}

struct S { int x; };
auto argumentPassedToOperatorAssignment(S* p) {
    *p = S { 42 };
}
auto argumentPassedToOperatorMemberAccessArrow(S* p) {
    p->x = 42;
}
auto argumentPassedToOperatorSubscript(int p[]) {
    p[0] = 42;
}