#pragma once

// Editable global compile-time settings. Values below match the defaults in "SharedExternal.h". This file is ignored in ".gitignore".

// OpenRHI stuff

//#define OPENRHI_TIMEOUT_PRESENT          1000u     // 1 sec
//#define OPENRHI_TIMEOUT_FENCE            5000u     // 5 sec
//#define OPENRHI_MAX_MESSAGE_LENGTH       2048u     // 2 Kb
//#define OPENRHI_ZERO_BUFFER_SIZE         4194304u  // 4 Mb
//#define OPENRHI_MAX_STACK_ALLOC_SIZE     32768u    // 32 Kb
//#define OPENRHI_FILE_SEPARATOR           '\\'      // path separator used in messages
//#define OPENRHI_INLINE                   inline    // we want to inline all functions, which are actually wrappers for the interface functions

/*
#ifdef NDEBUG
#    define OPENRHI_CHECK(condition, message)
#else
#    define OPENRHI_CHECK(condition, message) assert((condition) && message)
#endif
*/

// FFX stuff

/*
#define OPENRHI_FFX_DEBUG_LOG(messageType, message) \
    do { \
        MaybeUnused(messageType); \
        wprintf(L"FFX: %ls\n", message); \
    } while (false)
*/

// D3D12MA stuff

/*
#define D3D12MA_DEBUG_LOG(format, ...) \
    do { \
        wprintf(format, __VA_ARGS__); \
        wprintf(L"\n"); \
    } while (false)

#define D3D12MA_DEFAULT_BLOCK_SIZE 67108864u // 64 Mb
#define D3D12MA_ASSERT(cond)       OPENRHI_CHECK(cond, "D3D12MA assert failed!")
#define D3D12MA_HEAVY_ASSERT(expr)
*/

// VMA stuff

/*
#define VMA_DEBUG_LOG_FORMAT(format, ...) \
    do { \
        printf((format), __VA_ARGS__); \
        printf("\n"); \
    } while (false)

#define VMA_DEFAULT_LARGE_HEAP_BLOCK_SIZE 67108864u // 64 Mb
#define VMA_ASSERT(expr)                  OPENRHI_CHECK(expr, "VMA assert failed!")
#define VMA_ASSERT_LEAK(expr)             VMA_ASSERT(expr)
#define VMA_DEBUG_LOG(str)                VMA_DEBUG_LOG_FORMAT("%s", (str))
#define VMA_LEAK_LOG_FORMAT(format, ...)  VMA_DEBUG_LOG_FORMAT(format, __VA_ARGS__)
#define VMA_HEAVY_ASSERT(expr)
*/
