#include <memory>

class A {
    std::unique_ptr<int> p;
};

class B {
    std::unique_ptr<int> p1;
    std::unique_ptr<int> p2;
};

class C {
    C(int x) {
        p = std::make_unique<int>(x);
    }
    void update(int x) {
        p = std::make_unique<int>(42);
    }
    std::unique_ptr<int> p;
};

class D {
    D(int x, int y) {
        p1 = std::make_unique<int>(x);
        p2 = std::make_unique<int>(y);
    }
    std::unique_ptr<int> p1;
    std::unique_ptr<int> p2;
};

class E {
public:
    std::shared_ptr<int> p1;
protected:
    std::shared_ptr<int> p2;
private:
    std::unique_ptr<int> p3;
};

template<typename T>
void doSomethingEvilWithSharedPtr(T p) { }
class F {
    void triggerEscape() {
        doSomethingEvilWithSharedPtr(p);
    }
    std::shared_ptr<int> p;
};

class G {
    auto dataMemberPassedToSwap() {
        std::shared_ptr<int> q = std::make_shared<int>(42);
        p1.swap(q);
    }
    auto dataMemberPassedToUseCount() {
        return p2.use_count();
    }
    auto dataMemberPassedToUnique() {
        return p3.unique();
    }
    auto dataMemberPassedToOwnerBefore() {
        std::shared_ptr<int> q = std::make_shared<int>(42);
        return p4.owner_before(q);
    }
    std::shared_ptr<int> p1;
    std::shared_ptr<int> p2;
    std::shared_ptr<int> p3;
    std::shared_ptr<int> p4;
};

class H {
    struct I { int x; };
    auto dataMemberPassedToOperatorAssignment() {
        *p1 = I { 42 };
    }
    auto dataMemberPassedToOperatorMemberAccessArrow() {
        p2->x = 42;
    }
    auto dataMemberPassedToOperatorSubscript() {
        p3[0] = 42;
    }
    std::unique_ptr<I> p1;
    std::unique_ptr<I> p2;
    std::unique_ptr<int[]> p3;
};

class J {
public:
    std::shared_ptr<int> returnsPrivateDataMember1() {
        return p1;
    }
private:
    std::shared_ptr<int> returnsPrivateDataMember2() {
        return p2;
    }
    std::shared_ptr<int> p1;
    std::shared_ptr<int> p2;
};