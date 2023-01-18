// RUN: %check_clang_tidy %s tfg-NoReturnPositionMakeShared %t
#include <memory>

namespace tests {

	// FIXME: Add something that triggers the check here.
	std::shared_ptr<int> returnMakeShared() {
		return std::make_shared<int>(42);
	}

	std::shared_ptr<int> returnSharedPtrVar() {
		std::shared_ptr<int> var = std::make_shared<int>(42);
		return var;
	}

	std::shared_ptr<int> returnSecondSharedPtrVar() {
		std::shared_ptr<int> var = std::make_shared<int>(42);
		std::shared_ptr<int> var2 = std::make_shared<int>(36);
		return var2;
	}

	std::shared_ptr<int> returnModifiedSharedPtrVar() {
		std::shared_ptr<int> var = std::make_shared<int>(42);
		*var = 99;
		return var;
	}

	std::shared_ptr<int> returnSharedPtrVarWithSharedPtrMemberCall() {
		std::shared_ptr<int> var = std::make_shared<int>(42);
		var.use_count();
		return var;
	}

	std::shared_ptr<int> returnSharedPtrWithDelayedInit() {
		std::shared_ptr<int> var;
		var = std::make_shared<int>(42);
		return var;
	}

	std::shared_ptr<int> returnReassignedSharedPtr() {
		auto var = std::make_shared<int>(36);
		var = std::make_shared<int>(17);
		return var;
	}

	template<typename T>
	void consumeSharedPtr(std::shared_ptr<T> p) { }

	std::shared_ptr<int> returnEscapingSharedPtr() {
		std::shared_ptr<int> var = std::make_shared<int>(42);
		consumeSharedPtr(var);
		return var;
	}

	std::shared_ptr<char *> returnMakeSharedWithBranch(const int a) {
		if (a % 64 == 0) {
			return std::make_shared<char *>(new char[] {"hello world"});
		} else {
			return std::make_shared<char *>(new char[] {"SALVE MUNDE"});
		}
	}

	std::shared_ptr<char *> returnSharedPtrBranching(const int a) {
		if (a % 64 == 0) {
			std::shared_ptr<char *> var = std::make_shared<char *>(new char[] {"hello world"});
			return var;
		} else {
			std::shared_ptr<char *> var = std::make_shared<char *>(new char[] {"SALVE MUNDE"});
			return var;
		}
	}

	template<typename T>
	std::shared_ptr<T> returnGenericMakeShared(const T t) {
		return std::make_shared<T>(t);
	}

	// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: function 'f' is insufficiently awesome [tfg-NoFactoryReturningSharedPtrCheck]

	// FIXME: Verify the applied fix.
	//   * Make the CHECK patterns specific enough and try to make verified lines
	//     unique to avoid incorrect matches.
	//   * Use {{}} for regular expressions.
	// CHECK-FIXES: {{^}}void awesome_f();{{$}}

	// FIXME: Add something that doesn't trigger the check here.
	std::unique_ptr<int> returnPositionMakeUniqueTest() {
		return std::make_unique<int>(42);
	}
}