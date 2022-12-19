// RUN: %check_clang_tidy %s tfg-NoReturnPositionMakeShared %t
#include <memory>

namespace tests {

	// FIXME: Add something that triggers the check here.
	std::shared_ptr<int> returnPositionMakeSharedTest() {
		return std::make_shared<int>(42);
	}

	std::shared_ptr<int> returnVarMakeShared() {
		auto var = std::make_shared<int>(42);
		*var = 99;
		return var;
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