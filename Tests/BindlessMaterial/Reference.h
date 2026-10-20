// Independent scalar sampling oracle for authored BC4/5 blocks and BC7 mode 6
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace material_test {
inline void Check(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}

using Color = std::array<double, 4>;
enum class Encoding {
    BC4,
    BC5,
    BC7,
    BC7Srgb
};

struct Mip {
    uint32_t width, height, rowPitch;
    std::vector<uint8_t> bytes;
    std::vector<Color> texels;
    std::vector<Color> decodeBounds;
};

struct Image {
    Encoding encoding;
    uint32_t width, height;
    std::vector<Mip> mips;
};

struct View {
    uint32_t base, count;
};

struct SampleCase {
    uint32_t textureIndex, samplerIndex, operation, reserved;
    float uv[2], lod, padding, dx[2], dy[2];
};

struct SampleResult {
    float color[4];
    uint32_t width, height, levels, reserved;
};

static_assert(sizeof(SampleCase) == 48 && sizeof(SampleResult) == 32, "material shader ABI");
static_assert(offsetof(SampleCase, uv) == 16 && offsetof(SampleCase, lod) == 24 && offsetof(SampleCase, dx) == 32 && offsetof(SampleCase, dy) == 40, "sample-case field ABI");
static_assert(offsetof(SampleResult, width) == 16 && offsetof(SampleResult, height) == 20 && offsetof(SampleResult, levels) == 24 && offsetof(SampleResult, reserved) == 28, "sample-result field ABI");

inline uint32_t Levels(uint32_t width, uint32_t height) {
    uint32_t count = 1;

    for (uint32_t n = std::max(width, height); n > 1; n >>= 1)
        count++;

    return count;
}

// Range normalization is checked before subtraction or shifting
inline View Normalize(uint32_t width, uint32_t height, uint32_t levels, uint32_t base, uint32_t count) {
    Check(width && height && levels && levels <= Levels(width, height), "invalid original chain");
    Check(base < levels, "invalid base mip");

    if (!count)
        count = levels - base;
    Check(count && count <= (levels - base), "invalid mip count");

    return {base, count};
}

inline double Linear(double value) {
    return value <= .04045 ? value / 12.92 : std::pow((value + .055) / 1.055, 2.4);
}

// BC4 interpolants are represented in normalized arithmetic; native precision
// differences have a separately declared tolerance
inline void EncodeBc4(uint8_t* bytes, uint8_t a, uint8_t b, uint32_t seed) {
    bytes[0] = a;
    bytes[1] = b;
    uint64_t indices = 0;

    for (uint32_t i = 0; i < 16; i++)
        indices |= uint64_t((i * 3 + seed) % 8) << (3 * i);

    for (uint32_t i = 0; i < 6; i++)
        bytes[2 + i] = uint8_t(indices >> (8 * i));
}

inline double DecodeBc4(const uint8_t* bytes, uint32_t texel) {
    double palette[8] = {double(bytes[0]), double(bytes[1])};

    if (bytes[0] > bytes[1]) {
        for (uint32_t i = 2; i < 8; i++)
            palette[i] = ((8 - i) * palette[0] + (i - 1) * palette[1]) / 7;
    } else {
        for (uint32_t i = 2; i < 6; i++)
            palette[i] = ((6 - i) * palette[0] + (i - 1) * palette[1]) / 5;
        palette[6] = 0;
        palette[7] = 255;
    }
    uint64_t indices = 0;

    for (uint32_t i = 0; i < 6; i++)
        indices |= uint64_t(bytes[i + 2]) << (8 * i);

    return palette[(indices >> (3 * texel)) & 7] / 255;
}

// The functional texture specification permits endpoint-relative BC4/5 error
// while requiring endpoints and the special zero/one values to remain exact
inline double Bc4DecodeBound(const uint8_t* bytes, uint32_t texel) {
    uint64_t indices = 0;

    for (uint32_t i = 0; i < 6; i++)
        indices |= uint64_t(bytes[i + 2]) << (8 * i);
    const uint32_t index = uint32_t((indices >> (3 * texel)) & 7);

    if (index < 2 || (bytes[0] <= bytes[1] && index >= 6))
        return 0;

    return 1.0 / 65535 + .03 * std::abs(double(bytes[0]) - bytes[1]) / 255;
}

inline void PutBits(uint8_t* bytes, uint32_t& position, uint32_t value, uint32_t count) {
    for (uint32_t i = 0; i < count; i++, position++)
        bytes[position / 8] |= uint8_t(((value >> i) & 1) << (position % 8));
}

inline uint32_t GetBits(const uint8_t* bytes, uint32_t& position, uint32_t count) {
    uint32_t value = 0;

    for (uint32_t i = 0; i < count; i++, position++)
        value |= uint32_t((bytes[position / 8] >> (position % 8)) & 1) << i;

    return value;
}

inline void EncodeBc7(uint8_t* bytes, uint32_t seed) {
    uint32_t position = 0;
    PutBits(bytes, position, 64, 7);

    for (uint32_t c = 0; c < 4; c++) {
        PutBits(bytes, position, (16 + seed * 7 + c * 13) % 100, 7);
        PutBits(bytes, position, (110 - (seed * 3 + c * 7) % 60), 7);
    }
    PutBits(bytes, position, seed & 1, 1);
    PutBits(bytes, position, (seed + 1) & 1, 1);

    for (uint32_t i = 0; i < 16; i++)
        PutBits(bytes, position, i ? (i * 5 + seed) % 16 : seed % 8, i ? 4 : 3);
    Check(position == 128, "BC7 mode 6 encoder size");
}

inline Color DecodeBc7(const uint8_t* bytes, uint32_t texel) {
    uint32_t position = 0;
    Check(GetBits(bytes, position, 7) == 64, "reference only admits BC7 mode 6");
    uint32_t ends[4][2] = {};

    for (auto& channel : ends)
        for (auto& endpoint : channel)
            endpoint = GetBits(bytes, position, 7) * 2;
    const uint32_t p0 = GetBits(bytes, position, 1), p1 = GetBits(bytes, position, 1);

    for (auto& channel : ends) {
        channel[0] |= p0;
        channel[1] |= p1;
    }
    uint32_t index = 0;

    for (uint32_t i = 0; i <= texel; i++)
        index = GetBits(bytes, position, i ? 4 : 3);
    constexpr uint32_t weights[] = {0, 4, 9, 13, 17, 21, 26, 30, 34, 38, 43, 47, 51, 55, 60, 64};
    Color color = {};

    for (uint32_t c = 0; c < 4; c++)
        color[c] = double(((64 - weights[index]) * ends[c][0] + weights[index] * ends[c][1] + 32) >> 6) / 255;

    return color;
}

inline Image MakeImage(Encoding encoding, uint32_t width, uint32_t height, uint32_t seed) {
    Image image{encoding, width, height, {}};
    const uint32_t blockBytes = encoding == Encoding::BC4 ? 8 : 16;

    for (uint32_t mip = 0; mip < Levels(width, height); mip++) {
        Mip level = {std::max(width >> mip, 1u), std::max(height >> mip, 1u), 0, {}, {}, {}};
        const uint32_t blocksX = (level.width + 3) / 4, blocksY = (level.height + 3) / 4;
        level.rowPitch = blocksX * blockBytes;
        level.bytes.resize(size_t(level.rowPitch) * blocksY);
        level.texels.resize(size_t(level.width) * level.height);
        level.decodeBounds.resize(level.texels.size());

        for (uint32_t by = 0; by < blocksY; by++) {
            for (uint32_t bx = 0; bx < blocksX; bx++) {
                auto* block = level.bytes.data() + by * level.rowPitch + bx * blockBytes;
                const uint32_t pattern = seed + mip * 5 + bx * 3 + by * 7;

                if (encoding == Encoding::BC4 || encoding == Encoding::BC5) {
                    EncodeBc4(block, uint8_t(220 - pattern % 70), uint8_t(20 + pattern % 50), pattern);

                    if (encoding == Encoding::BC5)
                        EncodeBc4(block + 8, uint8_t(10 + pattern % 50), uint8_t(200 - pattern % 70), pattern + 2);
                } else
                    EncodeBc7(block, pattern);

                for (uint32_t y = 0; y < 4; y++) {
                    for (uint32_t x = 0; x < 4; x++) {
                        if ((bx * 4 + x) >= level.width || (by * 4 + y) >= level.height)
                            continue;
                        Color color = {0, 0, 0, 1};
                        Color bound = {};

                        if (encoding == Encoding::BC4 || encoding == Encoding::BC5) {
                            color[0] = DecodeBc4(block, y * 4 + x);
                            bound[0] = Bc4DecodeBound(block, y * 4 + x);

                            if (encoding == Encoding::BC5) {
                                color[1] = DecodeBc4(block + 8, y * 4 + x);
                                bound[1] = Bc4DecodeBound(block + 8, y * 4 + x);
                            }
                        } else {
                            color = DecodeBc7(block, y * 4 + x);

                            if (encoding == Encoding::BC7Srgb)
                                for (uint32_t c = 0; c < 3; c++)
                                    color[c] = Linear(color[c]);
                        }
                        level.texels[size_t(by * 4 + y) * level.width + bx * 4 + x] = color;
                        level.decodeBounds[size_t(by * 4 + y) * level.width + bx * 4 + x] = bound;
                    }
                }
            }
        }
        image.mips.push_back(std::move(level));
    }

    return image;
}

inline Color Add(Color a, Color b, double weight) {
    for (uint32_t c = 0; c < 4; c++)
        a[c] += b[c] * weight;

    return a;
}

inline Color Texel(const Mip& mip, int x, int y, bool repeat, bool decodeBound = false) {
    auto address = [repeat](int i, int n) { return repeat ? ((i % n) + n) % n : std::clamp(i, 0, n - 1); };

    return (decodeBound ? mip.decodeBounds : mip.texels)[size_t(address(y, int(mip.height))) * mip.width + address(x, int(mip.width))];
}

inline Color AtMip(const Mip& mip, double u, double v, bool linear, bool repeat, bool decodeBound = false) {
    if (!linear)
        return Texel(mip, int(std::floor(u * mip.width)), int(std::floor(v * mip.height)), repeat, decodeBound);
    const double x = u * mip.width - .5, y = v * mip.height - .5;
    const int ix = int(std::floor(x)), iy = int(std::floor(y));
    const double fx = x - ix, fy = y - iy;
    Color color = {};
    color = Add(color, Texel(mip, ix, iy, repeat, decodeBound), (1 - fx) * (1 - fy));
    color = Add(color, Texel(mip, ix + 1, iy, repeat, decodeBound), fx * (1 - fy));
    color = Add(color, Texel(mip, ix, iy + 1, repeat, decodeBound), (1 - fx) * fy);

    return Add(color, Texel(mip, ix + 1, iy + 1, repeat, decodeBound), fx * fy);
}

inline Color Reference(const Image& image, View view, uint32_t sampler, const SampleCase& c, bool decodeBound = false) {
    double lod = c.operation == 0 ? 0 : c.lod;

    if (c.operation == 2) {
        const double w = image.mips[view.base].width, h = image.mips[view.base].height;
        lod = std::log2(std::max(std::hypot(c.dx[0] * w, c.dx[1] * h), std::hypot(c.dy[0] * w, c.dy[1] * h)));
    }
    lod = std::clamp(lod, 0.0, double(view.count - 1));
    const bool linear = (sampler & 2) != 0, repeat = (sampler & 1) != 0;

    if (!linear)
        return AtMip(image.mips[view.base + uint32_t(std::floor(lod + .5))], c.uv[0], c.uv[1], false, repeat, decodeBound);
    const uint32_t first = uint32_t(std::floor(lod)), second = std::min(first + 1, view.count - 1);
    Color color = {};
    color = Add(color, AtMip(image.mips[view.base + first], c.uv[0], c.uv[1], true, repeat, decodeBound), 1 - (lod - first));

    return Add(color, AtMip(image.mips[view.base + second], c.uv[0], c.uv[1], true, repeat, decodeBound), lod - first);
}

inline uint32_t SelfTest() {
    uint32_t checks = 0;
    auto test = [&](bool value) {
        if (!value)
            throw std::runtime_error("independent reference self-test " + std::to_string(checks));
        checks++;
    };
    uint8_t bc4[8] = {255, 0, 0, 0, 0, 0, 0, 0};
    test(DecodeBc4(bc4, 0) == 1);
    bc4[2] = 1;
    test(DecodeBc4(bc4, 0) == 0);
    bc4[2] = 2;
    test(std::abs(DecodeBc4(bc4, 0) - 6.0 / 7) < 1e-12);
    test(std::abs(Bc4DecodeBound(bc4, 0) - (.03 + 1.0 / 65535)) < 1e-12);
    bc4[0] = 0;
    bc4[1] = 255;
    bc4[2] = 7;
    test(DecodeBc4(bc4, 0) == 1);
    test(Bc4DecodeBound(bc4, 0) == 0);
    // Literal BC7 mode 6 black endpoint, white endpoint; anchor 0 then index 15
    const uint8_t bc7[16] = {0x40, 0xc0, 0x1f, 0xf0, 0x07, 0xfc, 0x01, 0x7f, 0xf1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    const Color black = DecodeBc7(bc7, 0), white = DecodeBc7(bc7, 1);
    test(black == Color{0, 0, 0, 0});
    test(white == Color{1, 1, 1, 1});
    test(std::abs(Linear(.5) - .21404114048223255) < 1e-12);

    for (const auto dims : {std::array<uint32_t, 2>{17, 9}, {20, 12}, {1, 31}, {31, 1}}) {
        const auto levels = Levels(dims[0], dims[1]);

        for (uint32_t base = 0; base < levels; base++) {
            const auto view = Normalize(dims[0], dims[1], levels, base, 0);
            test(view.count == levels - base);
            test(std::max(dims[0] >> base, 1u) >= 1 && std::max(dims[1] >> base, 1u) >= 1);
        }
        bool rejected = false;

        try {
            Normalize(dims[0], dims[1], levels, levels, 0);
        } catch (const std::runtime_error&) {
            rejected = true;
        }
        test(rejected);
    }
    Mip grid{2, 1, 0, {}, {{0, 0, 0, 1}, {1, 1, 1, 1}}, {{.02, 0, 0, 0}, {.04, 0, 0, 0}}};
    test(AtMip(grid, .5, .5, true, false) == Color{.5, .5, .5, 1});
    test(AtMip(grid, 1.25, .5, false, true) == Color{0, 0, 0, 1});
    test(AtMip(grid, 1.25, .5, false, false) == Color{1, 1, 1, 1});
    test(std::abs(AtMip(grid, .5, .5, true, false, true)[0] - .03) < 1e-12);

    for (Encoding encoding : {Encoding::BC4, Encoding::BC5, Encoding::BC7, Encoding::BC7Srgb}) {
        const auto image = MakeImage(encoding, 17, 9, 3);

        for (uint32_t base = 0; base < image.mips.size(); base++) {
            const auto view = Normalize(17, 9, uint32_t(image.mips.size()), base, 0);

            for (uint32_t sampler = 0; sampler < 4; sampler++) {
                for (uint32_t operation = 0; operation < 3; operation++) {
                    SampleCase c = {};
                    c.operation = operation;
                    c.uv[0] = -.17f;
                    c.uv[1] = 1.31f;
                    c.lod = .27f;
                    c.dx[0] = float(std::exp2(.27) / image.mips[base].width);
                    c.dy[1] = float(std::exp2(.27) / image.mips[base].height);
                    const auto color = Reference(image, view, sampler, c);

                    for (double channel : color)
                        test(std::isfinite(channel) && channel >= 0 && channel <= 1);
                }
            }
        }
    }
    const auto unorm = MakeImage(Encoding::BC7, 17, 9, 3), srgb = MakeImage(Encoding::BC7Srgb, 17, 9, 3);

    for (size_t i = 0; i < unorm.mips[0].texels.size(); i++) {
        test(unorm.mips[0].texels[i][3] == srgb.mips[0].texels[i][3]);
        test(unorm.mips[0].texels[i][0] >= srgb.mips[0].texels[i][0]);
    }
    test(unorm.mips[0].texels[0][0] > srgb.mips[0].texels[0][0]);

    return checks;
}
} // namespace material_test
