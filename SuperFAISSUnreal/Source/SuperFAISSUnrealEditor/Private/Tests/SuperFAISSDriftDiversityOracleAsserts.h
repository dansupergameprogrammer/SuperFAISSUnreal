// SuperFAISS For Unreal 3.4 (drift + diversity) -- the two named assertion helpers §11 of
// the 3.4 drift-and-diversity plan requires every drift/diversity
// oracle or mutant-discrimination comparison to route through, and nowhere else
// (D-INSP-64/D-INSP-67). This file is deliberately OUTSIDE the governed
// `Tests/DriftDiversityOracle/` directory -- it is the shared surface the governed files call
// into, not one of them -- because `AssertWithinQuantizationTolerance`'s own body legitimately
// contains the four-argument `TestEqual(..., Tolerance)` call
// `Tools/PublishGates/check_assertion_exactness.py --enforce-allowlist` exists to ban
// everywhere else. Read once by hand at Gate 2 and Gate 8, confirming each helper's body
// calls the overload its own declaration states and no other (§11's own "the script does not
// read a function's internals" note).
//
// `AssertExactValue` is the sole entry point for every governed comparison: no
// tolerance/epsilon parameter exists on any overload. The int32/int64/bool overloads forward
// to the engine's three-argument `TestEqual(Description, Actual, Expected)`, which is exact for
// those types. The float/double overloads forward to
// `TestEqual(Description, Actual, Expected, 0)`: the engine's float/double `TestEqual`
// overloads DEFAULT `Tolerance` to `UE_KINDA_SMALL_NUMBER` (1e-4, `AutomationTest.h`), so the
// three-argument call on a float is NOT exact, and the explicit zero is what makes it exact.
// The FString overload forwards to `TestEqualSensitive(Description, FStringView, FStringView)`:
// the engine's string `TestEqual` overloads compare case-INSENSITIVELY (`FCString::Stricmp`
// on the `TCHAR*` form an FString call resolves to, `ESearchCase::IgnoreCase` on the
// `FStringView` form -- `AutomationTest.cpp`), so `TestEqual` on two strings is NOT exact.
// `check_assertion_exactness.py --enforce-allowlist` additionally bans every bare
// `Test<Identifier>(`/`Add<Identifier>(` call other than `AssertExactValue`/
// `AssertWithinQuantizationTolerance` inside the governed directory -- so a governed test file
// cannot call `TestTrue`/`TestFalse`/`TestEqual`/`AddError` etc. directly; a boolean or status
// condition is asserted through `AssertExactValue`'s `bool`/`int32` overloads instead.
//
// Not production code: compiled only inside `WITH_DEV_AUTOMATION_TESTS` translation units.

#pragma once

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Containers/UnrealString.h"

// Exact-comparison entry point (D-INSP-64). One overload per governed-comparison value type;
// each body is the engine's exact comparison for its type (the float/double bodies pass a zero
// tolerance explicitly, and the FString body uses the case-sensitive comparison, above). Adding a type used by a future governed cell means adding an
// overload here, never reaching for `FMath::IsNearlyEqual` or a four-argument `TestEqual` at
// the call site.
void AssertExactValue(FAutomationTestBase& Test, const FString& Description, float Actual, float Expected);
void AssertExactValue(FAutomationTestBase& Test, const FString& Description, double Actual, double Expected);
void AssertExactValue(FAutomationTestBase& Test, const FString& Description, int32 Actual, int32 Expected);
void AssertExactValue(FAutomationTestBase& Test, const FString& Description, int64 Actual, int64 Expected);
void AssertExactValue(FAutomationTestBase& Test, const FString& Description, bool Actual, bool Expected);
void AssertExactValue(FAutomationTestBase& Test, const FString& Description, const FString& Actual, const FString& Expected);

// Fixture A's sole exception (§11's own "the one exception is unaffected and remains correctly
// scoped" paragraph): a closed-form value compared against the panel's int8-quantized
// computation under the pre-existing, separately-governed quantization-rounding tolerance
// (the test-design record §1.2's stated
// bound -- answers "does the computation match its own exact prediction", never "are two
// builds' outputs distinguishable", the question every other governed comparison in this suite
// answers). `Bound` is always the fixture's own pre-derived, named constant -- never an
// ad hoc epsilon chosen at the call site.
void AssertWithinQuantizationTolerance(FAutomationTestBase& Test, const FString& Description,
	float Actual, float Expected, float Bound);

#endif // WITH_DEV_AUTOMATION_TESTS
