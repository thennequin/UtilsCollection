
#include "Benchmarker.h"

#include <stdint.h> // uint64_t
#include <stdio.h> // printf

#if defined(_WIN32)
#include <Windows.h>
#endif //_WIN32

#include <assert.h>
#define ASSERT(bTest) assert(bTest); if ((bTest) == false) exit(1);

static const int				c_iBenchmarker_MaxSuites = 32;
static const int				c_iBenchmarker_MaxChallenger = 64;
static const char*				c_pBenchmarker_Indentation = "                                 "; // 64 spaces => 32 indentation

typedef struct
{
	const char*			pName;

	uint32_t			iPassCount;
	uint32_t			iPassFailed;

	uint64_t			iCurrentPassStart;
	uint64_t			iCurrentPassEnd;
	bool				bCurrentPassOk;

	uint64_t			iTimeMin;
	uint64_t			iTimeMax;
	uint64_t			iTimeTotal;
} Benchmarker_VersusChalleneger;

typedef struct
{
	const char*			pName;
	const char*			pArg;
	bool				bActive;
	int					iCurrentPass;

	const char*			pBestAverage;
	uint64_t			iAverage;
	const char*			pBestMin;
	uint64_t			iMin;
	const char*			pBestMax;
	uint64_t			iMax;

	const char*			pCurrentChallengerName;
	bool				bCurrentChallengerOk;
	int					iCurrentChallenger;
	int					iChallengerCount;

	Benchmarker_VersusChalleneger	pChallengers[c_iBenchmarker_MaxChallenger];
} Benchmarker_Versus;

typedef struct
{
	const char*			pName;
	uint64_t			iTotalChecks;
	uint64_t			iFailedChecks;
} Benchmarker_Suite;

static int						s_iBenchmarker_CurrentSuite = -1;
static Benchmarker_Suite		s_pBenchmarker_Suites[c_iBenchmarker_MaxSuites];
static Benchmarker_Versus		s_oBenchmarker_Versus = { 0 };
static bool						s_oBenchmarker_Verbose = false;

uint64_t Benchmarker_Nanotime() {
#if defined(__linux__)
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * UINT64_C(1000000000) + ts.tv_nsec;
#elif defined(__MACH__)
	static mach_timebase_info_data_t info;
	if (info.denom == 0)
		mach_timebase_info(&info);
	return mach_absolute_time() * info.numer / info.denom;
#elif defined(_WIN32)
	static LARGE_INTEGER frequency;
	if (frequency.QuadPart == 0)
		QueryPerformanceFrequency(&frequency);
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return counter.QuadPart * UINT64_C(1000000000) / frequency.QuadPart;
#endif
}

void Benchmarker_GetReadableTime(uint64_t iNanoseconds, char* pBuffer, int iBufferSize)
{
	static const char* const pTimes[] = { "ns", "us", "ms", "s" };
	int iDiv = 1;
	int iRem = 0;

	while (iNanoseconds >= 1000 && iDiv < (sizeof pTimes / sizeof *pTimes)) {
		iRem = (iNanoseconds % 1000);
		iDiv++;
		iNanoseconds /= 1000;
	}

	snprintf(pBuffer, iBufferSize, "%.3f %s", (float)iNanoseconds + (float)iRem / 1000.0, pTimes[iDiv - 1]);
}

void Benchmarker_SetVerbose(bool bVerbose)
{
	s_oBenchmarker_Verbose = bVerbose;
}

void Benchmarker_LogDebug(const char* pFormat, ...)
{
	printf("\x1b[90m");
	va_list oArgs;
	va_start(oArgs, pFormat);
	vprintf(pFormat, oArgs);
	va_end(oArgs);
}

void Benchmarker_LogInfo(const char* pFormat, ...)
{
	printf("\x1b[37m");
	va_list oArgs;
	va_start(oArgs, pFormat);
	vprintf(pFormat, oArgs);
	va_end(oArgs);
}

void Benchmarker_LogOk(const char* pFormat, ...)
{
	printf("\x1b[32m");
	va_list oArgs;
	va_start(oArgs, pFormat);
	vprintf(pFormat, oArgs);
	va_end(oArgs);
}

void Benchmarker_LogError(const char* pFormat, ...)
{
	printf("\x1b[31m");
	va_list oArgs;
	va_start(oArgs, pFormat);
	vprintf(pFormat, oArgs);
	va_end(oArgs);
}

void Benchmarker_Suite_Begin(const char* const pSuiteName)
{
	s_iBenchmarker_CurrentSuite++;
	ASSERT(s_iBenchmarker_CurrentSuite < c_iBenchmarker_MaxSuites);
	s_pBenchmarker_Suites[s_iBenchmarker_CurrentSuite].pName = pSuiteName;
	s_pBenchmarker_Suites[s_iBenchmarker_CurrentSuite].iTotalChecks = 0;
	s_pBenchmarker_Suites[s_iBenchmarker_CurrentSuite].iFailedChecks = 0;

	Benchmarker_LogInfo("%.*s", s_iBenchmarker_CurrentSuite * 2, c_pBenchmarker_Indentation);
	Benchmarker_LogInfo("Begin suite \"%s\" :\n", pSuiteName);
}

void Benchmarker_Suite_End()
{
	ASSERT(s_iBenchmarker_CurrentSuite >= 0);

	Benchmarker_LogInfo("%.*s", s_iBenchmarker_CurrentSuite * 2, c_pBenchmarker_Indentation);
	Benchmarker_LogInfo("Ended suite \"%s\" : ", s_pBenchmarker_Suites[s_iBenchmarker_CurrentSuite].pName);

	if (s_iBenchmarker_CurrentSuite > 0)
	{
		s_pBenchmarker_Suites[s_iBenchmarker_CurrentSuite - 1].iTotalChecks += s_pBenchmarker_Suites[s_iBenchmarker_CurrentSuite].iTotalChecks;
		s_pBenchmarker_Suites[s_iBenchmarker_CurrentSuite - 1].iFailedChecks += s_pBenchmarker_Suites[s_iBenchmarker_CurrentSuite].iFailedChecks;
	}

	if (s_pBenchmarker_Suites[s_iBenchmarker_CurrentSuite].iFailedChecks > 0)
	{
		Benchmarker_LogError("%llu tests failed on %llu tests\n",
			s_pBenchmarker_Suites[s_iBenchmarker_CurrentSuite].iFailedChecks,
			s_pBenchmarker_Suites[s_iBenchmarker_CurrentSuite].iTotalChecks);
	}
	else
	{
		Benchmarker_LogOk("%llu tests failed on %llu tests\n",
			s_pBenchmarker_Suites[s_iBenchmarker_CurrentSuite].iFailedChecks,
			s_pBenchmarker_Suites[s_iBenchmarker_CurrentSuite].iTotalChecks);
	}
	--s_iBenchmarker_CurrentSuite;
}

bool Benchmarker_Test(const char* const pName, bool bTest)
{
	if (s_iBenchmarker_CurrentSuite >= 0)
	{
		s_pBenchmarker_Suites[s_iBenchmarker_CurrentSuite].iTotalChecks++;
	}

	if (bTest)
	{
		if (s_iBenchmarker_CurrentSuite >= 0)
		{
			Benchmarker_LogDebug("%.*s", (s_iBenchmarker_CurrentSuite + 1)* 2, c_pBenchmarker_Indentation);
			if (s_oBenchmarker_Verbose)
			{
				Benchmarker_LogDebug("%s : ", pName);
			}
			Benchmarker_LogOk("Ok\n");
		}

		if (s_oBenchmarker_Versus.bActive && s_oBenchmarker_Versus.iCurrentChallenger >= 0)
		{
			if (s_oBenchmarker_Verbose)
			{
				Benchmarker_LogDebug("%.*s", 2 * 2, c_pBenchmarker_Indentation);
				Benchmarker_LogDebug("%s :", pName);
				Benchmarker_LogOk("Ok\n");
			}
			else
			{
				Benchmarker_LogOk(".");
			}
		}
	}
	else
	{
		if (s_iBenchmarker_CurrentSuite >= 0)
		{
			s_pBenchmarker_Suites[s_iBenchmarker_CurrentSuite].iFailedChecks++;
			Benchmarker_LogDebug("%.*s", (s_iBenchmarker_CurrentSuite + 1) * 2, c_pBenchmarker_Indentation);
			if (s_oBenchmarker_Verbose)
			{
				Benchmarker_LogDebug("%s : ", pName);
			}
			Benchmarker_LogError("Fail\n");
		}

		if (s_oBenchmarker_Versus.bActive && s_oBenchmarker_Versus.iCurrentChallenger >= 0)
		{
			s_oBenchmarker_Versus.pChallengers[s_oBenchmarker_Versus.iCurrentChallenger].bCurrentPassOk = false;

			if (s_oBenchmarker_Verbose)
			{
				Benchmarker_LogDebug("   %s : ", pName);
				Benchmarker_LogError("Fail\n");
			}
			else
			{
				Benchmarker_LogError(".");
			}
		}
	}

	return bTest;
}

void Benchmarker_Versus_Begin(const char* const pName, const char* pArg)
{
	ASSERT(s_oBenchmarker_Versus.bActive == false);
	s_oBenchmarker_Versus.bActive = true;
	s_oBenchmarker_Versus.pName = pName;
	s_oBenchmarker_Versus.pArg = pArg;
	s_oBenchmarker_Versus.iCurrentPass = -1;
	s_oBenchmarker_Versus.pBestAverage = NULL;
	s_oBenchmarker_Versus.iAverage = 0;
	s_oBenchmarker_Versus.pBestMin = NULL;
	s_oBenchmarker_Versus.iMin = 0;
	s_oBenchmarker_Versus.pBestMax = NULL;
	s_oBenchmarker_Versus.iMax = 0;

	s_oBenchmarker_Versus.pCurrentChallengerName = NULL;
	s_oBenchmarker_Versus.iCurrentChallenger = -1;
	s_oBenchmarker_Versus.iChallengerCount = -1;
	Benchmarker_LogInfo("Begin Versus \"%s\"", pName);
	if (pArg != NULL)
	{
		Benchmarker_LogInfo(" with \"%s\"", pArg);
	}
	Benchmarker_LogInfo("\n");
}

void Benchmarker_Versus_End()
{
	ASSERT(s_oBenchmarker_Versus.bActive );
	s_oBenchmarker_Versus.bActive = false;

	if (s_oBenchmarker_Versus.iChallengerCount >= 0)
	{
		const char* pBestMin = NULL;
		uint64_t iBestMin = 0;
		const char* pBestMax = NULL;
		uint64_t iBestMax = 0;
		const char* pBestAverage = NULL;
		uint64_t iBestAverage = 0;

		int iChallengerCount = s_oBenchmarker_Versus.iCurrentChallenger + 1;
		for (int iChallenger = 0; iChallenger < iChallengerCount; ++iChallenger)
		{
			Benchmarker_VersusChalleneger& oChallenger = s_oBenchmarker_Versus.pChallengers[iChallenger];
			Benchmarker_LogInfo("\n  \"%s\" :", oChallenger.pName);
			if (oChallenger.iPassFailed > 0)
			{
				Benchmarker_LogError(" Failed %d times", oChallenger.iPassFailed);
			}
			Benchmarker_LogInfo("\n");

			uint64_t iTimeAverage = oChallenger.iTimeTotal / (s_oBenchmarker_Versus.iCurrentPass + 1);
			char pBuffer[256];
			Benchmarker_GetReadableTime(iTimeAverage, pBuffer, 256);
			Benchmarker_LogInfo("     Average : %s\n", pBuffer);
			Benchmarker_GetReadableTime(oChallenger.iTimeMin, pBuffer, 256);
			Benchmarker_LogInfo("     Min : %s\n", pBuffer);
			Benchmarker_GetReadableTime(oChallenger.iTimeMax, pBuffer, 256);
			Benchmarker_LogInfo("     Max : %s\n", pBuffer);

			if (oChallenger.iPassFailed == 0)
			{
				if (pBestMin == NULL || iBestMin > oChallenger.iTimeMin)
				{
					pBestMin = oChallenger.pName;
					iBestMin = oChallenger.iTimeMin;
				}

				if (pBestMax == NULL || iBestMax > oChallenger.iTimeMax)
				{
					pBestMax = oChallenger.pName;
					iBestMax = oChallenger.iTimeMax;
				}

				if (pBestAverage == NULL || iBestAverage > iTimeAverage)
				{
					pBestAverage = oChallenger.pName;
					iBestAverage = iTimeAverage;
				}
			}
		}

		Benchmarker_LogInfo("\n");
		Benchmarker_LogInfo("End Versus \"%s\"", s_oBenchmarker_Versus.pName);
		if (s_oBenchmarker_Versus.pArg != NULL)
		{
			Benchmarker_LogInfo(" with \"%s\"", s_oBenchmarker_Versus.pArg);
		}
		Benchmarker_LogInfo("\n");

		char pBuffer[256];
		if (pBestAverage != NULL)
		{
			Benchmarker_GetReadableTime(iBestAverage, pBuffer, 256);
			Benchmarker_LogInfo("  Best Average : \"%s\" : %s\n", pBestAverage, pBuffer);
		}
		if (pBestMin != NULL)
		{
			Benchmarker_GetReadableTime(iBestMin, pBuffer, 256);
			Benchmarker_LogInfo("  Best Min     : \"%s\" : %s\n", pBestMin, pBuffer);
		}
		if (pBestMax != NULL)
		{
			Benchmarker_GetReadableTime(iBestMax, pBuffer, 256);
			Benchmarker_LogInfo("  Best Max     : \"%s\" : %s\n", pBestMax, pBuffer);
		}
		Benchmarker_LogInfo("\n");
	}
}

void Benchmarker_Versus_Pass_Begin(int iPass)
{
	if (s_oBenchmarker_Verbose)
	{
		Benchmarker_LogDebug("Start pass %d\n", iPass);
	}
	else
	{
		Benchmarker_LogInfo(".");
	}
	ASSERT(s_oBenchmarker_Versus.bActive);
	s_oBenchmarker_Versus.iCurrentPass++;
	s_oBenchmarker_Versus.iCurrentChallenger = -1;
}

void Benchmarker_Versus_Pass_End()
{
	ASSERT(s_oBenchmarker_Versus.bActive);
	ASSERT(s_oBenchmarker_Versus.iCurrentPass >= 0);

	if (s_oBenchmarker_Versus.iCurrentPass == 0)
	{
		s_oBenchmarker_Versus.iChallengerCount = s_oBenchmarker_Versus.iCurrentChallenger + 1;
	}
	else
	{
		ASSERT( s_oBenchmarker_Versus.iChallengerCount == (s_oBenchmarker_Versus.iCurrentChallenger + 1));
	}
}

void Benchmarker_Versus_Pass_Challenger_Begin(const char* const pName)
{
	ASSERT(s_oBenchmarker_Versus.bActive);
	ASSERT(s_oBenchmarker_Versus.iCurrentPass >= 0);

	if (s_oBenchmarker_Versus.pCurrentChallengerName != pName)
	{
		// New challenger
		s_oBenchmarker_Versus.pCurrentChallengerName = pName;
		s_oBenchmarker_Versus.iCurrentChallenger += 1;
		Benchmarker_VersusChalleneger& oChallenger = s_oBenchmarker_Versus.pChallengers[s_oBenchmarker_Versus.iCurrentChallenger];
		oChallenger.pName = pName;
		oChallenger.bCurrentPassOk = true;
		oChallenger.iCurrentPassStart = Benchmarker_Nanotime();

		if (s_oBenchmarker_Versus.iCurrentPass == 0)
		{
			oChallenger.iTimeMin = (uint64_t)-1;
			oChallenger.iTimeMax = 0;
			oChallenger.iTimeTotal = 0;
			oChallenger.iPassCount = 0;
			oChallenger.iPassFailed = 0;
		}

		if (s_oBenchmarker_Verbose)
		{
			Benchmarker_LogDebug("  Running \"%s\"...\n", oChallenger.pName);
		}
		else
		{
			Benchmarker_LogDebug(".");
		}
	}
}

void Benchmarker_Versus_Pass_Challenger_End()
{
	ASSERT(s_oBenchmarker_Versus.bActive);
	ASSERT(s_oBenchmarker_Versus.iCurrentPass >= 0);
	ASSERT(s_oBenchmarker_Versus.pCurrentChallengerName != NULL);

	Benchmarker_VersusChalleneger& oChallenger = s_oBenchmarker_Versus.pChallengers[s_oBenchmarker_Versus.iCurrentChallenger];
	oChallenger.iCurrentPassEnd = Benchmarker_Nanotime();
	if (oChallenger.bCurrentPassOk)
	{
		oChallenger.iPassCount++;

		uint64_t iTime = oChallenger.iCurrentPassEnd - oChallenger.iCurrentPassStart;

		if (oChallenger.iTimeMin > iTime)
			oChallenger.iTimeMin = iTime;

		if (oChallenger.iTimeMax < iTime)
			oChallenger.iTimeMax = iTime;

		oChallenger.iTimeTotal += iTime;

		char pBuffer[256];
		Benchmarker_GetReadableTime(iTime, pBuffer, 256);

		if (s_oBenchmarker_Verbose)
		{
			Benchmarker_LogDebug("  Finished in %s\n", pBuffer);
		}
		else
		{
			Benchmarker_LogOk(".");
		}
	}
	else
	{
		oChallenger.iPassFailed++;
		if (s_oBenchmarker_Verbose)
		{
			Benchmarker_LogError("  Fail\n");
		}
		else
		{
			Benchmarker_LogError(".");
		}
	}
}