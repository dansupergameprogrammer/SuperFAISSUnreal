// See SuperFAISSDriftDiversityOracleAsserts.h. Not production code.

#include "SuperFAISSDriftDiversityOracleAsserts.h"

#if WITH_DEV_AUTOMATION_TESTS

// The engine's float and double TestEqual overloads DEFAULT their Tolerance parameter to
// UE_KINDA_SMALL_NUMBER (1e-4, Engine/Source/Runtime/Core/Public/Misc/AutomationTest.h), so
// the three-argument call is a tolerant comparison. A tolerance of exactly zero makes it the
// exact one (|Actual - Expected| <= 0). On a mismatch the full-precision values are logged,
// since TestEqual's own message prints six decimals.
void AssertExactValue(FAutomationTestBase& Test, const FString& Description, float Actual, float Expected)
{
	if (!Test.TestEqual(Description, Actual, Expected, 0.0f))
	{
		Test.AddInfo(FString::Printf(TEXT("  %s: actual %.9g, expected %.9g (exact comparison)"),
			*Description, static_cast<double>(Actual), static_cast<double>(Expected)));
	}
}

void AssertExactValue(FAutomationTestBase& Test, const FString& Description, double Actual, double Expected)
{
	if (!Test.TestEqual(Description, Actual, Expected, 0.0))
	{
		Test.AddInfo(FString::Printf(TEXT("  %s: actual %.17g, expected %.17g (exact comparison)"),
			*Description, Actual, Expected));
	}
}

void AssertExactValue(FAutomationTestBase& Test, const FString& Description, int32 Actual, int32 Expected)
{
	Test.TestEqual(Description, Actual, Expected);
}

void AssertExactValue(FAutomationTestBase& Test, const FString& Description, int64 Actual, int64 Expected)
{
	Test.TestEqual(Description, Actual, Expected);
}

void AssertExactValue(FAutomationTestBase& Test, const FString& Description, bool Actual, bool Expected)
{
	Test.TestEqual(Description, Actual, Expected);
}

// The engine's string TestEqual overloads are case-INSENSITIVE: the TCHAR* form the FString
// call resolves to compares with FCString::Stricmp, and the FStringView form with
// ESearchCase::IgnoreCase (Engine/Source/Runtime/Core/Private/Misc/AutomationTest.cpp). The
// exact comparison is TestEqualSensitive over FStringView, which compares length and every
// character with ESearchCase::CaseSensitive.
void AssertExactValue(FAutomationTestBase& Test, const FString& Description, const FString& Actual, const FString& Expected)
{
	Test.TestEqualSensitive(*Description, FStringView(Actual), FStringView(Expected));
}

void AssertWithinQuantizationTolerance(FAutomationTestBase& Test, const FString& Description,
	float Actual, float Expected, float Bound)
{
	Test.TestEqual(Description, Actual, Expected, Bound);
}

#endif // WITH_DEV_AUTOMATION_TESTS
