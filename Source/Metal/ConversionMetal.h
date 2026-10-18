// © 2026 NVIDIA Corporation

#pragma once

namespace openrhi {

MTL::PixelFormat GetPixelFormatMetal(Format format);
MTL::VertexFormat GetVertexFormatMetal(Format format);
FormatSupportBits GetFormatSupportMetal(MTL::Device& device, Format format);
MTL::CompareFunction GetCompareMetal(CompareOp compareOp);

} // namespace openrhi
