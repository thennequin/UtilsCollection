#ifndef __BENCHMARKER_H__
#define __BENCHMARKER_H__

#include <stdbool.h>

#ifdef BENCHMARKER_USE_MACROS

#define BEGIN_TEST_SUITE(SuiteName) Benchmarker_Suite_Begin(SuiteName); do{
#define END_TEST_SUITE() } while(0); Benchmarker_Suite_End();
#define CHECK(Test) Benchmarker_Test(#Test, (Test));
#define CHECK_TRY(Test) { bool __bTest = true; try { (Test); }catch(...){__bTest = false;}; Benchmarker_Test(#Test, __bTest); }
#define CHECK_FATAL(Test) if (Benchmarker_Test(#Test, (Test)) == false) break;
#define CHECK_EQUALITY(ValueLeft, ValueRight) Benchmarker_Test(#ValueLeft " == " #ValueRight, (ValueLeft) == (ValueRight));

#define BEGIN_BENCHMARK_VERSUS_EX(VersusName, PassCount, Arg) Benchmarker_Versus_Begin(VersusName, Arg); \
	for (int __Pass = 0; __Pass < PassCount; ++__Pass) { \
	Benchmarker_Versus_Pass_Begin(__Pass); {
#define BEGIN_BENCHMARK_VERSUS(VersusName) BEGIN_BENCHMARK_VERSUS_EX(VersusName, 10, NULL)
#define END_BENCHMARK_VERSUS() } Benchmarker_Versus_Pass_End(); } Benchmarker_Versus_End();

#define BEGIN_BENCHMARK_VERSUS_WITH_ARG_EX(VersusName, PassCount, Type, ...) { \
	Type __VersusArgs[] = { __VA_ARGS__ }; \
	for (size_t iIndex = 0; iIndex < (sizeof(__VersusArgs) / sizeof(Type)); ++iIndex) { \
		Type VERSUS_ARG = __VersusArgs[iIndex]; \
		BEGIN_BENCHMARK_VERSUS_EX(VersusName, PassCount, VERSUS_ARG)
#define BEGIN_BENCHMARK_VERSUS_WITH_ARG(VersusName, Type, ...) BEGIN_BENCHMARK_VERSUS_WITH_ARG_EX(VersusName, 10, Type, __VA_ARGS__)
#define END_BENCHMARK_VERSUS_ARGS() END_BENCHMARK_VERSUS() } }
#define BEGIN_BENCHMARK_VERSUS_CHALLENGER(ChallengerName) Benchmarker_Versus_Pass_Challenger_Begin(ChallengerName); do {
#define END_BENCHMARK_VERSUS_CHALLENGER() } while(0); Benchmarker_Versus_Pass_Challenger_End();

#endif //BENCHMARKER_USE_MACROS

void Benchmarker_SetVerbose(bool bVerbose); // Neeed to be call first if needed

void Benchmarker_Suite_Begin(const char* const pSuiteName);
void Benchmarker_Suite_End();
bool Benchmarker_Test(const char* const pName, bool bTest);

void Benchmarker_Versus_Begin(const char* const pName, const char* pArg);
void Benchmarker_Versus_End();

void Benchmarker_Versus_Pass_Begin(int iPass);
void Benchmarker_Versus_Pass_End();
void Benchmarker_Versus_Pass_Challenger_Begin(const char* const pName);
void Benchmarker_Versus_Pass_Challenger_End();

/*
Usage:

#define BENCHMARKER_USE_MACROS
#include "Benchmarker.h"

#include <string>

void main()
{
	Benchmarker_SetVerbose(true);

	BEGIN_TEST_SUITE("Numerics")
		CHECK(123.f == 123)
		CHECK(10.f < 123)
		CHECK(1230.f > 123)
	END_TEST_SUITE()

	Benchmarker_SetVerbose(false);

	BEGIN_BENCHMARK_VERSUS("String")
		BEGIN_BENCHMARK_VERSUS_CHALLENGER("100")
			for (int i = 0; i < 100; ++i)
			{
				std::string sTest = "test";
			}
		END_BENCHMARK_VERSUS_CHALLENGER()

		BEGIN_BENCHMARK_VERSUS_CHALLENGER("1000")
			for (int i = 0; i < 1000; ++i)
			{
				std::string sTest = "test";
			}
		END_BENCHMARK_VERSUS_CHALLENGER()

		BEGIN_BENCHMARK_VERSUS_CHALLENGER("10000")
			for (int i = 0; i < 10000; ++i)
			{
				std::string sTest = "test";
			}
		END_BENCHMARK_VERSUS_CHALLENGER()
	END_BENCHMARK_VERSUS()
}
*/

#endif //__BENCHMARKER_H__
