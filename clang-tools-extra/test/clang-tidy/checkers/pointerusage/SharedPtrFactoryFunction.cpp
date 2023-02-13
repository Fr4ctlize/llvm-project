// RUN: %check_clang_tidy %s tfg-SharedPtrFactoryFunction %t
#include <memory>
#include <vector>

std::shared_ptr<int> returnMakeShared() {
	return std::make_shared<int>(42);
}
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: Prefer ``unique_ptr`` over ``shared_ptr`` as a return type for factory functions [tfg-SharedPtrFactoryFunction]

std::shared_ptr<int> returnMakeSharedOfArgument(int x) {
	return std::make_shared<int>(x);
}
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: Prefer ``unique_ptr`` over ``shared_ptr`` as a return type for factory functions [tfg-SharedPtrFactoryFunction]

std::shared_ptr<int> returnSharedPtrVar() {
	std::shared_ptr<int> var = std::make_shared<int>(42);
	return var;
}
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: Prefer ``unique_ptr`` over ``shared_ptr`` as a return type for factory functions [tfg-SharedPtrFactoryFunction]

std::shared_ptr<int> returnSecondSharedPtrVar() {
	std::shared_ptr<int> var = std::make_shared<int>(42);
	std::shared_ptr<int> var2 = std::make_shared<int>(36);
	return var2;
}
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: Prefer ``unique_ptr`` over ``shared_ptr`` as a return type for factory functions [tfg-SharedPtrFactoryFunction]

std::shared_ptr<int> returnModifiedSharedPtrVar() {
	std::shared_ptr<int> var = std::make_shared<int>(42);
	*var = 99;
	return var;
}
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: Prefer ``unique_ptr`` over ``shared_ptr`` as a return type for factory functions [tfg-SharedPtrFactoryFunction]

std::shared_ptr<int> returnSharedPtrVarWithSharedPtrMemberCall() {
	std::shared_ptr<int> var = std::make_shared<int>(42);
	var.get();
	return var;
}
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: Prefer ``unique_ptr`` over ``shared_ptr`` as a return type for factory functions [tfg-SharedPtrFactoryFunction]

std::shared_ptr<int> returnSharedPtrWithDelayedInit() {
	std::shared_ptr<int> var;
	var = std::make_shared<int>(42);
	return var;
}
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: Prefer ``unique_ptr`` over ``shared_ptr`` as a return type for factory functions [tfg-SharedPtrFactoryFunction]

std::shared_ptr<int> returnReassignedSharedPtr() {
	auto var = std::make_shared<int>(36);
	var = std::make_shared<int>(17);
	return var;
}
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: Prefer ``unique_ptr`` over ``shared_ptr`` as a return type for factory functions [tfg-SharedPtrFactoryFunction]

std::shared_ptr<char *> returnMakeSharedWithBranch(const int a) {
	if (a % 64 == 0) {
		return std::make_shared<char *>(new char[] {"hello world"});
	} else {
		return std::make_shared<char *>(new char[] {"SALVE MUNDE"});
	}
}
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: Prefer ``unique_ptr`` over ``shared_ptr`` as a return type for factory functions [tfg-SharedPtrFactoryFunction]

std::shared_ptr<char *> returnSharedPtrWithBranch(const int a) {
	if (a % 64 == 0) {
		std::shared_ptr<char *> var = std::make_shared<char *>(new char[] {"hello world"});
		return var;
	} else {
		std::shared_ptr<char *> var = std::make_shared<char *>(new char[] {"SALVE MUNDE"});
		return var;
	}
}
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: Prefer ``unique_ptr`` over ``shared_ptr`` as a return type for factory functions [tfg-SharedPtrFactoryFunction]

// FIXME: Verify the applied fix.
//   * Make the CHECK patterns specific enough and try to make verified lines
//     unique to avoid incorrect matches.
//   * Use {{}} for regular expressions.
// CHECK-FIXES: {{^}}std::shared_ptr<{{.*}}> ();{{$}}

std::unique_ptr<int> returnPositionMakeUniqueTest() {
	return std::make_unique<int>(42);
}

std::shared_ptr<int> returnSharedPtrVarWithSharedPtrMemberCallExclusiveToSharedPointer() {
	std::shared_ptr<int> var = std::make_shared<int>(42);
	var.use_count();
	return var;
}

template<typename T>
void consumeSharedPtr(std::shared_ptr<T> p) { }

std::shared_ptr<int> returnSharedPtrThatEscapesToUserCode() {
	std::shared_ptr<int> var = std::make_shared<int>(42);
	consumeSharedPtr(var);
	return var;
}

std::shared_ptr<int> returnSharedPtrThatEscapesToSystemCode(std::vector<std::shared_ptr<int>>& v) {
	std::shared_ptr<int> var = std::make_shared<int>(42);
	v.push_back(var);
	return var;
}

std::shared_ptr<int> returnSharedPtrFromArgument(const std::shared_ptr<int> p) {
	return p;
}

class Animal {
public:
    virtual void Eat(/* ... */) = 0;
    virtual void Move(/* ... */) = 0;
    virtual void Attack(/* ... */) = 0;
};

class Pegasus : public Animal {
public:
    Pegasus() { /* ... */ }
    void Eat(/* ... */) override { /* ... */ }
    void Move(/* ... */) override { /* ... */ }
    void Attack(/* ... */) override { /* ... */ }
};

class Cockatrice : public Animal {
public:
    Cockatrice() { /* ... */ }
    void Eat(/* ... */) override { /* ... */ }
    void Move(/* ... */) override { /* ... */ }
    void Attack(/* ... */) override { /* ... */ }
};

using std::shared_ptr, std::make_shared;
class AnimalFactory {
	shared_ptr<Animal> customizedAnimal(const bool canFly, /* ... */ const int legCount) {
		if (canFly) {
			switch (legCount) {
			break; case 2:
				return make_shared<Cockatrice>();
			break; case 4:
				return make_shared<Pegasus>();
			/* ... */
			}
		}
		/* ... */
	}
};
