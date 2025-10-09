#ifndef BA_EXPORTS_H
#define BA_EXPORTS_H

#ifdef _WIN32
#ifdef ba_lib_EXPORTS
#define BA_API __declspec(dllexport)
#else
#define BA_API __declspec(dllimport)
#endif
#else
#define BA_API
#endif

#endif
