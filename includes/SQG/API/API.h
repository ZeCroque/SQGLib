#pragma once

#ifdef SQGLib_EXPORTS
#define SQG_API __declspec(dllexport)
#else
#define SQG_API __declspec(dllimport)
#endif