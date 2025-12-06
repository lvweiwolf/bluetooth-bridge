#ifndef CORE_DEFINES_H_
#define CORE_DEFINES_H_


#if defined(CORE_DYNMIC_LIBS)
#if defined(_MSC_VER)
#define CORE_IMPORT __declspec(dllimport)
#define CORE_EXPORT __declspec(dllexport)
#elif defined(__GUNC__)
#define CORE_IMPORT
#define CORE_EXPORT __attribute__((visibility("default")))
#else
#define CORE_IMPORT
#define CORE_EXPORT
#endif
#else
#define CORE_IMPORT
#define CORE_EXPORT
#endif

#ifdef gimserver_EXPORTS
#define CORE_API CORE_EXPORT
#else
#define CORE_API CORE_IMPORT
#endif

#define NO_CORE_API

#if defined(NDEBUG)
#undef CORE_DEBUG
#else
#define CORE_DEBUG
#endif

#if defined(__ANDROID__)
#define CORE_OS_ANDROID
#endif

// =============================== LINUX/UNIX defines ===========================
#if defined(__linux__)
#define CORE_OS_LINUX
#define CORE_OS_UNIX

#ifndef CORE_OS_ANDROID
#define CORE_OS_X11
#endif

#if defined(__INTEL_COMPILER)
#define CORE_COMPILER_INTEL
#elif defined(__clang__)
#define CORE_COMPILER_CLANG
#elif defined(__GNUC__)
#define CORE_COMPILER_GCC
#else
#error "Unsupported compiler"
#endif

#if defined(__x86_64)
#define CORE_ARCH_64
#define CORE_PROCESSOR_X86
#else
#define CORE_ARCH_32
#endif

// =============================== WINDOWS defines =============================
#elif defined(_WIN32) || defined(_WIN64)

#define CORE_OS_WINDOWS
#define CORE_PROCESSOR_X86

#if defined(_MSC_VER)
#define CORE_COMPILER_MSVC
#elif defined(__MINGGW32__) || defined(__MINGGW64__)
#define CORE_COMPILER_MINGW
#endif

#if defined(_WIN64)
#define CORE_ARCH_64
#else
#define CORE_ARCH_32
#endif


// =============================== APPLE defines =============================
#elif defined(__APPLE__)

#define CORE_OS_APPLE
#define CORE_OS_UNIX

#if defined(__clang__)
#define CORE_COMPILER_CLANG
#elif defined(__GNUC__)
#define CORE_COMPILER_GCC
#else
#error "Unsupported compiler"
#endif

#if defined(__x86_64) || defined(__ppc64__) || defined(__arm64__) || defined(__aarch64__) ||       \
	(defined(__riscv) && __riscv_xlen == 64) || defined(__loongarch_lp64)
#define CORE_ARCH_64
#else
#define CORE_ARCH_32
#endif

// =============================== Emscripten defines  ======================

#elif defined(__EMSCRIPTEN__)

#define CORE_OS_UNIX
#define CORE_OS_LINUX
#define CORE_OS_EMSCRIPTEN
#define CORE_ARCH_64
#define CORE_COMPILER_EMSCRIPTEN
#define CORE_COMPILER_CLANG

// =============================== Unsupported =============================
#else
#error "Unsupported operating system"
#endif



#if defined(CORE_COMPILER_GCC) || defined(CORE_COMPILER_CLANG) || defined(CORE_COMPILER_MINGW) ||  \
	defined(CORE_COMPILER_EMSCRIPTEN)
#define CORE_COMPILER_GCC_FAMILY
#endif


#if defined(CORE_COMPILER_CLANG) || defined(CORE_COMPILER_EMSCRIPTEN)
#if __has_feature(cxx_noexcept)
#define CORE_NOEXCEPT noexcept
#endif
#endif

#ifndef CORE_NOEXCEPT
#define CORE_NOEXCEPT noexcept
#endif

#if defined(CORE_COMPILER_MSVC)
#define CORE_NORETURN_DECL __declspec(noreturn)
#else
#define CORE_NORETURN_DECL
#endif

#if defined(CORE_COMPILER_GCC_FAMILY) || defined(CORE_COMPILER_INTEL)
#define CORE_NORETURN __attribute__((noreturn))
#else
#define CORE_NORETURN
#endif

#ifdef CORE_COMPILER_CLANG
#pragma GCC diagnostic ignored "-Walloca"
#endif

#endif // CORE_DEFINES_H_