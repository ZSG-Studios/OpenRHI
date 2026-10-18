// © 2021 NVIDIA Corporation

#if defined(_WIN32)
#    include <windows.h>
#    undef LoadLibrary

Library* openrhi::LoadSharedLibrary(const char* path) {
    return (Library*)LoadLibraryA(path);
}

void* openrhi::GetSharedLibraryFunction(Library& library, const char* name) {
    return (void*)GetProcAddress((HMODULE)&library, name);
}

void openrhi::UnloadSharedLibrary(Library& library) {
    FreeLibrary((HMODULE)&library);
}
#else
#    include <dlfcn.h>

Library* openrhi::LoadSharedLibrary(const char* path) {
    return (Library*)dlopen(path, RTLD_NOW);
}

void* openrhi::GetSharedLibraryFunction(Library& library, const char* name) {
    return dlsym((void*)&library, name);
}

void openrhi::UnloadSharedLibrary(Library& library) {
    dlclose((void*)&library);
}
#endif
