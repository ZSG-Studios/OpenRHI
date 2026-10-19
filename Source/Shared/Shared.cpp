// © 2021 NVIDIA Corporation

#include "SharedExternal.h"

#include "HelperInterface.h"
#include "ImguiInterface.h"
#include "StreamerInterface.h"
#include "UpscalerInterface.h"

#if OPENRHI_ENABLE_METAL_SUPPORT
#    define METALCPP_SYMBOL_VISIBILITY_HIDDEN
#    if OPENRHI_METAL_CPP_PRIVATE_IMPLEMENTATION
#        define MTLFX_PRIVATE_IMPLEMENTATION
#    endif
#    include <MetalFX/MetalFX.hpp> // must precede "using namespace openrhi"
#endif

using namespace openrhi;

#include "HelperInterface.hpp"
#include "ImguiInterface.hpp"
#include "StreamerInterface.hpp"
#include "UpscalerInterface.hpp"
#include "VendorFeatures.hpp"

#include "SharedExternal.hpp"
#include "SharedLibrary.hpp"
