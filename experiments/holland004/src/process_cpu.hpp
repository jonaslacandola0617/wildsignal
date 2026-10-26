#pragma once
#include "pacing.hpp"
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/resource.h>
#endif
namespace pacing {
inline Ns processCpu(){
#ifdef _WIN32
 FILETIME created{},exited{},kernel{},user{};
 if(!GetProcessTimes(GetCurrentProcess(),&created,&exited,&kernel,&user))return -1;
 ULARGE_INTEGER k{},u{};k.LowPart=kernel.dwLowDateTime;k.HighPart=kernel.dwHighDateTime;
 u.LowPart=user.dwLowDateTime;u.HighPart=user.dwHighDateTime;
 return static_cast<Ns>((k.QuadPart+u.QuadPart)*100);
#else
 rusage r{};if(getrusage(RUSAGE_SELF,&r)!=0)return -1;
 return (pacing::Ns(r.ru_utime.tv_sec)+r.ru_stime.tv_sec)*second+
        (pacing::Ns(r.ru_utime.tv_usec)+r.ru_stime.tv_usec)*1000;
#endif
}
} // namespace pacing
