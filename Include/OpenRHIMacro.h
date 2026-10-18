#pragma once

// Undef macros from "wingdi.h", interfering with OpenRHI
#undef RGB
#undef ERROR

#define OpenRhiBit(bit) (1u << (bit))
#define OpenRhiBgra(r, g, b) (0xFF000000u | (((r) & 0xFFu) << 16) | (((g) & 0xFFu) << 8) | ((b) & 0xFFu))

#define OPENRHI_NAME_C(name) OpenRhi##name
#define OPENRHI_FUNC_NAME_C(name) openrhi##name
#define OPENRHI_CONST_NAME_C(name) OPENRHI_##name

#define _OPENRHI_EXPAND(args) args

// IMPORTANT: 125 is the max supported value because MSVC doesn't support more than 127 arguments in macros
#ifdef _MSC_VER
    #define _OPENRHI_NARGS(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, _105, _106, _107, _108, _109, _110, _111, _112, _113, _114, _115, _116, _117, _118, _119, _120, _121, _122, _123, _124, _125, VAL, ...) VAL
    #define _OPENRHI_NARGS1(...) _OPENRHI_EXPAND(_OPENRHI_NARGS(__VA_ARGS__, 124, 123, 122, 121, 120, 119, 118, 117, 116, 115, 114, 113, 112, 111, 110, 109, 108, 107, 106, 105, 104, 103, 102, 101, 100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
    #define _OPENRHI_AUGMENTER(...) unused, __VA_ARGS__
    #define OPENRHI_NARGS(...) _OPENRHI_NARGS1(_OPENRHI_AUGMENTER(__VA_ARGS__))
#else
    #define _OPENRHI_NARGS(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, _105, _106, _107, _108, _109, _110, _111, _112, _113, _114, _115, _116, _117, _118, _119, _120, _121, _122, _123, _124, VAL, ...) VAL
    #define OPENRHI_NARGS(...) _OPENRHI_NARGS(0, ## __VA_ARGS__, 124, 123, 122, 121, 120, 119, 118, 117, 116, 115, 114, 113, 112, 111, 110, 109, 108, 107, 106, 105, 104, 103, 102, 101, 100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#endif

#define _OPENRHI_SEQ0(code, ...)
#define _OPENRHI_SEQ1(code, ...)  _OPENRHI_EXPAND(code(0, __VA_ARGS__))
#define _OPENRHI_SEQ2(code, ...)  _OPENRHI_SEQ1(code, __VA_ARGS__), _OPENRHI_EXPAND(code(1, __VA_ARGS__))
#define _OPENRHI_SEQ3(code, ...)  _OPENRHI_SEQ2(code, __VA_ARGS__), _OPENRHI_EXPAND(code(2, __VA_ARGS__))
#define _OPENRHI_SEQ4(code, ...)  _OPENRHI_SEQ3(code, __VA_ARGS__), _OPENRHI_EXPAND(code(3, __VA_ARGS__))
#define _OPENRHI_SEQ5(code, ...)  _OPENRHI_SEQ4(code, __VA_ARGS__), _OPENRHI_EXPAND(code(4, __VA_ARGS__))
#define _OPENRHI_SEQ6(code, ...)  _OPENRHI_SEQ5(code, __VA_ARGS__), _OPENRHI_EXPAND(code(5, __VA_ARGS__))
#define _OPENRHI_SEQ7(code, ...)  _OPENRHI_SEQ6(code, __VA_ARGS__), _OPENRHI_EXPAND(code(6, __VA_ARGS__))
#define _OPENRHI_SEQ8(code, ...)  _OPENRHI_SEQ7(code, __VA_ARGS__), _OPENRHI_EXPAND(code(7, __VA_ARGS__))
#define _OPENRHI_SEQ9(code, ...)  _OPENRHI_SEQ8(code, __VA_ARGS__), _OPENRHI_EXPAND(code(8, __VA_ARGS__))
#define _OPENRHI_SEQ10(code, ...) _OPENRHI_SEQ9(code, __VA_ARGS__), _OPENRHI_EXPAND(code(9, __VA_ARGS__))
#define _OPENRHI_SEQ11(code, ...) _OPENRHI_SEQ10(code, __VA_ARGS__), _OPENRHI_EXPAND(code(10, __VA_ARGS__))
#define _OPENRHI_SEQ12(code, ...) _OPENRHI_SEQ11(code, __VA_ARGS__), _OPENRHI_EXPAND(code(11, __VA_ARGS__))
#define _OPENRHI_SEQ13(code, ...) _OPENRHI_SEQ12(code, __VA_ARGS__), _OPENRHI_EXPAND(code(12, __VA_ARGS__))
#define _OPENRHI_SEQ14(code, ...) _OPENRHI_SEQ13(code, __VA_ARGS__), _OPENRHI_EXPAND(code(13, __VA_ARGS__))
#define _OPENRHI_SEQ15(code, ...) _OPENRHI_SEQ14(code, __VA_ARGS__), _OPENRHI_EXPAND(code(14, __VA_ARGS__))
#define _OPENRHI_SEQ16(code, ...) _OPENRHI_SEQ15(code, __VA_ARGS__), _OPENRHI_EXPAND(code(15, __VA_ARGS__))
#define _OPENRHI_SEQ17(code, ...) _OPENRHI_SEQ16(code, __VA_ARGS__), _OPENRHI_EXPAND(code(16, __VA_ARGS__))
#define _OPENRHI_SEQ18(code, ...) _OPENRHI_SEQ17(code, __VA_ARGS__), _OPENRHI_EXPAND(code(17, __VA_ARGS__))
#define _OPENRHI_SEQ19(code, ...) _OPENRHI_SEQ18(code, __VA_ARGS__), _OPENRHI_EXPAND(code(18, __VA_ARGS__))
#define _OPENRHI_SEQ20(code, ...) _OPENRHI_SEQ19(code, __VA_ARGS__), _OPENRHI_EXPAND(code(19, __VA_ARGS__))
#define _OPENRHI_SEQ21(code, ...) _OPENRHI_SEQ20(code, __VA_ARGS__), _OPENRHI_EXPAND(code(20, __VA_ARGS__))
#define _OPENRHI_SEQ22(code, ...) _OPENRHI_SEQ21(code, __VA_ARGS__), _OPENRHI_EXPAND(code(21, __VA_ARGS__))
#define _OPENRHI_SEQ23(code, ...) _OPENRHI_SEQ22(code, __VA_ARGS__), _OPENRHI_EXPAND(code(22, __VA_ARGS__))
#define _OPENRHI_SEQ24(code, ...) _OPENRHI_SEQ23(code, __VA_ARGS__), _OPENRHI_EXPAND(code(23, __VA_ARGS__))
#define _OPENRHI_SEQ25(code, ...) _OPENRHI_SEQ24(code, __VA_ARGS__), _OPENRHI_EXPAND(code(24, __VA_ARGS__))
#define _OPENRHI_SEQ26(code, ...) _OPENRHI_SEQ25(code, __VA_ARGS__), _OPENRHI_EXPAND(code(25, __VA_ARGS__))
#define _OPENRHI_SEQ27(code, ...) _OPENRHI_SEQ26(code, __VA_ARGS__), _OPENRHI_EXPAND(code(26, __VA_ARGS__))
#define _OPENRHI_SEQ28(code, ...) _OPENRHI_SEQ27(code, __VA_ARGS__), _OPENRHI_EXPAND(code(27, __VA_ARGS__))
#define _OPENRHI_SEQ29(code, ...) _OPENRHI_SEQ28(code, __VA_ARGS__), _OPENRHI_EXPAND(code(28, __VA_ARGS__))
#define _OPENRHI_SEQ30(code, ...) _OPENRHI_SEQ29(code, __VA_ARGS__), _OPENRHI_EXPAND(code(29, __VA_ARGS__))
#define _OPENRHI_SEQ31(code, ...) _OPENRHI_SEQ30(code, __VA_ARGS__), _OPENRHI_EXPAND(code(30, __VA_ARGS__))
#define _OPENRHI_SEQ32(code, ...) _OPENRHI_SEQ31(code, __VA_ARGS__), _OPENRHI_EXPAND(code(31, __VA_ARGS__))
#define _OPENRHI_SEQ33(code, ...) _OPENRHI_SEQ32(code, __VA_ARGS__), _OPENRHI_EXPAND(code(32, __VA_ARGS__))
#define _OPENRHI_SEQ34(code, ...) _OPENRHI_SEQ33(code, __VA_ARGS__), _OPENRHI_EXPAND(code(33, __VA_ARGS__))
#define _OPENRHI_SEQ35(code, ...) _OPENRHI_SEQ34(code, __VA_ARGS__), _OPENRHI_EXPAND(code(34, __VA_ARGS__))
#define _OPENRHI_SEQ36(code, ...) _OPENRHI_SEQ35(code, __VA_ARGS__), _OPENRHI_EXPAND(code(35, __VA_ARGS__))
#define _OPENRHI_SEQ37(code, ...) _OPENRHI_SEQ36(code, __VA_ARGS__), _OPENRHI_EXPAND(code(36, __VA_ARGS__))
#define _OPENRHI_SEQ38(code, ...) _OPENRHI_SEQ37(code, __VA_ARGS__), _OPENRHI_EXPAND(code(37, __VA_ARGS__))
#define _OPENRHI_SEQ39(code, ...) _OPENRHI_SEQ38(code, __VA_ARGS__), _OPENRHI_EXPAND(code(38, __VA_ARGS__))
#define _OPENRHI_SEQ40(code, ...) _OPENRHI_SEQ39(code, __VA_ARGS__), _OPENRHI_EXPAND(code(39, __VA_ARGS__))
#define _OPENRHI_SEQ41(code, ...) _OPENRHI_SEQ40(code, __VA_ARGS__), _OPENRHI_EXPAND(code(40, __VA_ARGS__))
#define _OPENRHI_SEQ42(code, ...) _OPENRHI_SEQ41(code, __VA_ARGS__), _OPENRHI_EXPAND(code(41, __VA_ARGS__))
#define _OPENRHI_SEQ43(code, ...) _OPENRHI_SEQ42(code, __VA_ARGS__), _OPENRHI_EXPAND(code(42, __VA_ARGS__))
#define _OPENRHI_SEQ44(code, ...) _OPENRHI_SEQ43(code, __VA_ARGS__), _OPENRHI_EXPAND(code(43, __VA_ARGS__))
#define _OPENRHI_SEQ45(code, ...) _OPENRHI_SEQ44(code, __VA_ARGS__), _OPENRHI_EXPAND(code(44, __VA_ARGS__))
#define _OPENRHI_SEQ46(code, ...) _OPENRHI_SEQ45(code, __VA_ARGS__), _OPENRHI_EXPAND(code(45, __VA_ARGS__))
#define _OPENRHI_SEQ47(code, ...) _OPENRHI_SEQ46(code, __VA_ARGS__), _OPENRHI_EXPAND(code(46, __VA_ARGS__))
#define _OPENRHI_SEQ48(code, ...) _OPENRHI_SEQ47(code, __VA_ARGS__), _OPENRHI_EXPAND(code(47, __VA_ARGS__))
#define _OPENRHI_SEQ49(code, ...) _OPENRHI_SEQ48(code, __VA_ARGS__), _OPENRHI_EXPAND(code(48, __VA_ARGS__))
#define _OPENRHI_SEQ50(code, ...) _OPENRHI_SEQ49(code, __VA_ARGS__), _OPENRHI_EXPAND(code(49, __VA_ARGS__))
#define _OPENRHI_SEQ51(code, ...) _OPENRHI_SEQ50(code, __VA_ARGS__), _OPENRHI_EXPAND(code(50, __VA_ARGS__))
#define _OPENRHI_SEQ52(code, ...) _OPENRHI_SEQ51(code, __VA_ARGS__), _OPENRHI_EXPAND(code(51, __VA_ARGS__))
#define _OPENRHI_SEQ53(code, ...) _OPENRHI_SEQ52(code, __VA_ARGS__), _OPENRHI_EXPAND(code(52, __VA_ARGS__))
#define _OPENRHI_SEQ54(code, ...) _OPENRHI_SEQ53(code, __VA_ARGS__), _OPENRHI_EXPAND(code(53, __VA_ARGS__))
#define _OPENRHI_SEQ55(code, ...) _OPENRHI_SEQ54(code, __VA_ARGS__), _OPENRHI_EXPAND(code(54, __VA_ARGS__))
#define _OPENRHI_SEQ56(code, ...) _OPENRHI_SEQ55(code, __VA_ARGS__), _OPENRHI_EXPAND(code(55, __VA_ARGS__))
#define _OPENRHI_SEQ57(code, ...) _OPENRHI_SEQ56(code, __VA_ARGS__), _OPENRHI_EXPAND(code(56, __VA_ARGS__))
#define _OPENRHI_SEQ58(code, ...) _OPENRHI_SEQ57(code, __VA_ARGS__), _OPENRHI_EXPAND(code(57, __VA_ARGS__))
#define _OPENRHI_SEQ59(code, ...) _OPENRHI_SEQ58(code, __VA_ARGS__), _OPENRHI_EXPAND(code(58, __VA_ARGS__))
#define _OPENRHI_SEQ60(code, ...) _OPENRHI_SEQ59(code, __VA_ARGS__), _OPENRHI_EXPAND(code(59, __VA_ARGS__))
#define _OPENRHI_SEQ61(code, ...) _OPENRHI_SEQ60(code, __VA_ARGS__), _OPENRHI_EXPAND(code(60, __VA_ARGS__))
#define _OPENRHI_SEQ62(code, ...) _OPENRHI_SEQ61(code, __VA_ARGS__), _OPENRHI_EXPAND(code(61, __VA_ARGS__))
#define _OPENRHI_SEQ63(code, ...) _OPENRHI_SEQ62(code, __VA_ARGS__), _OPENRHI_EXPAND(code(62, __VA_ARGS__))
#define _OPENRHI_SEQ64(code, ...) _OPENRHI_SEQ63(code, __VA_ARGS__), _OPENRHI_EXPAND(code(63, __VA_ARGS__))
#define _OPENRHI_SEQ65(code, ...) _OPENRHI_SEQ64(code, __VA_ARGS__), _OPENRHI_EXPAND(code(64, __VA_ARGS__))
#define _OPENRHI_SEQ66(code, ...) _OPENRHI_SEQ65(code, __VA_ARGS__), _OPENRHI_EXPAND(code(65, __VA_ARGS__))
#define _OPENRHI_SEQ67(code, ...) _OPENRHI_SEQ66(code, __VA_ARGS__), _OPENRHI_EXPAND(code(66, __VA_ARGS__))
#define _OPENRHI_SEQ68(code, ...) _OPENRHI_SEQ67(code, __VA_ARGS__), _OPENRHI_EXPAND(code(67, __VA_ARGS__))
#define _OPENRHI_SEQ69(code, ...) _OPENRHI_SEQ68(code, __VA_ARGS__), _OPENRHI_EXPAND(code(68, __VA_ARGS__))
#define _OPENRHI_SEQ70(code, ...) _OPENRHI_SEQ69(code, __VA_ARGS__), _OPENRHI_EXPAND(code(69, __VA_ARGS__))
#define _OPENRHI_SEQ71(code, ...) _OPENRHI_SEQ70(code, __VA_ARGS__), _OPENRHI_EXPAND(code(70, __VA_ARGS__))
#define _OPENRHI_SEQ72(code, ...) _OPENRHI_SEQ71(code, __VA_ARGS__), _OPENRHI_EXPAND(code(71, __VA_ARGS__))
#define _OPENRHI_SEQ73(code, ...) _OPENRHI_SEQ72(code, __VA_ARGS__), _OPENRHI_EXPAND(code(72, __VA_ARGS__))
#define _OPENRHI_SEQ74(code, ...) _OPENRHI_SEQ73(code, __VA_ARGS__), _OPENRHI_EXPAND(code(73, __VA_ARGS__))
#define _OPENRHI_SEQ75(code, ...) _OPENRHI_SEQ74(code, __VA_ARGS__), _OPENRHI_EXPAND(code(74, __VA_ARGS__))
#define _OPENRHI_SEQ76(code, ...) _OPENRHI_SEQ75(code, __VA_ARGS__), _OPENRHI_EXPAND(code(75, __VA_ARGS__))
#define _OPENRHI_SEQ77(code, ...) _OPENRHI_SEQ76(code, __VA_ARGS__), _OPENRHI_EXPAND(code(76, __VA_ARGS__))
#define _OPENRHI_SEQ78(code, ...) _OPENRHI_SEQ77(code, __VA_ARGS__), _OPENRHI_EXPAND(code(77, __VA_ARGS__))
#define _OPENRHI_SEQ79(code, ...) _OPENRHI_SEQ78(code, __VA_ARGS__), _OPENRHI_EXPAND(code(78, __VA_ARGS__))
#define _OPENRHI_SEQ80(code, ...) _OPENRHI_SEQ79(code, __VA_ARGS__), _OPENRHI_EXPAND(code(79, __VA_ARGS__))
#define _OPENRHI_SEQ81(code, ...) _OPENRHI_SEQ80(code, __VA_ARGS__), _OPENRHI_EXPAND(code(80, __VA_ARGS__))
#define _OPENRHI_SEQ82(code, ...) _OPENRHI_SEQ81(code, __VA_ARGS__), _OPENRHI_EXPAND(code(81, __VA_ARGS__))
#define _OPENRHI_SEQ83(code, ...) _OPENRHI_SEQ82(code, __VA_ARGS__), _OPENRHI_EXPAND(code(82, __VA_ARGS__))
#define _OPENRHI_SEQ84(code, ...) _OPENRHI_SEQ83(code, __VA_ARGS__), _OPENRHI_EXPAND(code(83, __VA_ARGS__))
#define _OPENRHI_SEQ85(code, ...) _OPENRHI_SEQ84(code, __VA_ARGS__), _OPENRHI_EXPAND(code(84, __VA_ARGS__))
#define _OPENRHI_SEQ86(code, ...) _OPENRHI_SEQ85(code, __VA_ARGS__), _OPENRHI_EXPAND(code(85, __VA_ARGS__))
#define _OPENRHI_SEQ87(code, ...) _OPENRHI_SEQ86(code, __VA_ARGS__), _OPENRHI_EXPAND(code(86, __VA_ARGS__))
#define _OPENRHI_SEQ88(code, ...) _OPENRHI_SEQ87(code, __VA_ARGS__), _OPENRHI_EXPAND(code(87, __VA_ARGS__))
#define _OPENRHI_SEQ89(code, ...) _OPENRHI_SEQ88(code, __VA_ARGS__), _OPENRHI_EXPAND(code(88, __VA_ARGS__))
#define _OPENRHI_SEQ90(code, ...) _OPENRHI_SEQ89(code, __VA_ARGS__), _OPENRHI_EXPAND(code(89, __VA_ARGS__))
#define _OPENRHI_SEQ91(code, ...) _OPENRHI_SEQ90(code, __VA_ARGS__), _OPENRHI_EXPAND(code(90, __VA_ARGS__))
#define _OPENRHI_SEQ92(code, ...) _OPENRHI_SEQ91(code, __VA_ARGS__), _OPENRHI_EXPAND(code(91, __VA_ARGS__))
#define _OPENRHI_SEQ93(code, ...) _OPENRHI_SEQ92(code, __VA_ARGS__), _OPENRHI_EXPAND(code(92, __VA_ARGS__))
#define _OPENRHI_SEQ94(code, ...) _OPENRHI_SEQ93(code, __VA_ARGS__), _OPENRHI_EXPAND(code(93, __VA_ARGS__))
#define _OPENRHI_SEQ95(code, ...) _OPENRHI_SEQ94(code, __VA_ARGS__), _OPENRHI_EXPAND(code(94, __VA_ARGS__))
#define _OPENRHI_SEQ96(code, ...) _OPENRHI_SEQ95(code, __VA_ARGS__), _OPENRHI_EXPAND(code(95, __VA_ARGS__))
#define _OPENRHI_SEQ97(code, ...) _OPENRHI_SEQ96(code, __VA_ARGS__), _OPENRHI_EXPAND(code(96, __VA_ARGS__))
#define _OPENRHI_SEQ98(code, ...) _OPENRHI_SEQ97(code, __VA_ARGS__), _OPENRHI_EXPAND(code(97, __VA_ARGS__))
#define _OPENRHI_SEQ99(code, ...) _OPENRHI_SEQ98(code, __VA_ARGS__), _OPENRHI_EXPAND(code(98, __VA_ARGS__))
#define _OPENRHI_SEQ100(code, ...) _OPENRHI_SEQ99(code, __VA_ARGS__), _OPENRHI_EXPAND(code(99, __VA_ARGS__))
#define _OPENRHI_SEQ101(code, ...) _OPENRHI_SEQ100(code, __VA_ARGS__), _OPENRHI_EXPAND(code(100, __VA_ARGS__))
#define _OPENRHI_SEQ102(code, ...) _OPENRHI_SEQ101(code, __VA_ARGS__), _OPENRHI_EXPAND(code(101, __VA_ARGS__))
#define _OPENRHI_SEQ103(code, ...) _OPENRHI_SEQ102(code, __VA_ARGS__), _OPENRHI_EXPAND(code(102, __VA_ARGS__))
#define _OPENRHI_SEQ104(code, ...) _OPENRHI_SEQ103(code, __VA_ARGS__), _OPENRHI_EXPAND(code(103, __VA_ARGS__))
#define _OPENRHI_SEQ105(code, ...) _OPENRHI_SEQ104(code, __VA_ARGS__), _OPENRHI_EXPAND(code(104, __VA_ARGS__))
#define _OPENRHI_SEQ106(code, ...) _OPENRHI_SEQ105(code, __VA_ARGS__), _OPENRHI_EXPAND(code(105, __VA_ARGS__))
#define _OPENRHI_SEQ107(code, ...) _OPENRHI_SEQ106(code, __VA_ARGS__), _OPENRHI_EXPAND(code(106, __VA_ARGS__))
#define _OPENRHI_SEQ108(code, ...) _OPENRHI_SEQ107(code, __VA_ARGS__), _OPENRHI_EXPAND(code(107, __VA_ARGS__))
#define _OPENRHI_SEQ109(code, ...) _OPENRHI_SEQ108(code, __VA_ARGS__), _OPENRHI_EXPAND(code(108, __VA_ARGS__))
#define _OPENRHI_SEQ110(code, ...) _OPENRHI_SEQ109(code, __VA_ARGS__), _OPENRHI_EXPAND(code(109, __VA_ARGS__))
#define _OPENRHI_SEQ111(code, ...) _OPENRHI_SEQ110(code, __VA_ARGS__), _OPENRHI_EXPAND(code(110, __VA_ARGS__))
#define _OPENRHI_SEQ112(code, ...) _OPENRHI_SEQ111(code, __VA_ARGS__), _OPENRHI_EXPAND(code(111, __VA_ARGS__))
#define _OPENRHI_SEQ113(code, ...) _OPENRHI_SEQ112(code, __VA_ARGS__), _OPENRHI_EXPAND(code(112, __VA_ARGS__))
#define _OPENRHI_SEQ114(code, ...) _OPENRHI_SEQ113(code, __VA_ARGS__), _OPENRHI_EXPAND(code(113, __VA_ARGS__))
#define _OPENRHI_SEQ115(code, ...) _OPENRHI_SEQ114(code, __VA_ARGS__), _OPENRHI_EXPAND(code(114, __VA_ARGS__))
#define _OPENRHI_SEQ116(code, ...) _OPENRHI_SEQ115(code, __VA_ARGS__), _OPENRHI_EXPAND(code(115, __VA_ARGS__))
#define _OPENRHI_SEQ117(code, ...) _OPENRHI_SEQ116(code, __VA_ARGS__), _OPENRHI_EXPAND(code(116, __VA_ARGS__))
#define _OPENRHI_SEQ118(code, ...) _OPENRHI_SEQ117(code, __VA_ARGS__), _OPENRHI_EXPAND(code(117, __VA_ARGS__))
#define _OPENRHI_SEQ119(code, ...) _OPENRHI_SEQ118(code, __VA_ARGS__), _OPENRHI_EXPAND(code(118, __VA_ARGS__))
#define _OPENRHI_SEQ120(code, ...) _OPENRHI_SEQ119(code, __VA_ARGS__), _OPENRHI_EXPAND(code(119, __VA_ARGS__))
#define _OPENRHI_SEQ121(code, ...) _OPENRHI_SEQ120(code, __VA_ARGS__), _OPENRHI_EXPAND(code(120, __VA_ARGS__))
#define _OPENRHI_SEQ122(code, ...) _OPENRHI_SEQ121(code, __VA_ARGS__), _OPENRHI_EXPAND(code(121, __VA_ARGS__))
#define _OPENRHI_SEQ123(code, ...) _OPENRHI_SEQ122(code, __VA_ARGS__), _OPENRHI_EXPAND(code(122, __VA_ARGS__))
#define _OPENRHI_SEQ124(code, ...) _OPENRHI_SEQ123(code, __VA_ARGS__), _OPENRHI_EXPAND(code(123, __VA_ARGS__))

#define _OPENRHI_MERGE_TOKENS(_0, _1) _0##_1
#define OPENRHI_MERGE_TOKENS(_0, _1) _OPENRHI_MERGE_TOKENS(_0, _1)
#define OPENRHI_SEQN(code, count, ...) OPENRHI_MERGE_TOKENS(_OPENRHI_SEQ, count)(code, __VA_ARGS__)

#define _OPENRHI_VA_ARGS_AT0(_0, ...) _0
#define _OPENRHI_VA_ARGS_AT1(_0, _1, ...) _1
#define _OPENRHI_VA_ARGS_AT2(_0, _1, _2, ...) _2
#define _OPENRHI_VA_ARGS_AT3(_0, _1, _2, _3, ...) _3
#define _OPENRHI_VA_ARGS_AT4(_0, _1, _2, _3, _4, ...) _4
#define _OPENRHI_VA_ARGS_AT5(_0, _1, _2, _3, _4, _5, ...) _5
#define _OPENRHI_VA_ARGS_AT6(_0, _1, _2, _3, _4, _5, _6, ...) _6
#define _OPENRHI_VA_ARGS_AT7(_0, _1, _2, _3, _4, _5, _6, _7, ...) _7
#define _OPENRHI_VA_ARGS_AT8(_0, _1, _2, _3, _4, _5, _6, _7, _8, ...) _8
#define _OPENRHI_VA_ARGS_AT9(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, ...) _9
#define _OPENRHI_VA_ARGS_AT10(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, ...) _10
#define _OPENRHI_VA_ARGS_AT11(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, ...) _11
#define _OPENRHI_VA_ARGS_AT12(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, ...) _12
#define _OPENRHI_VA_ARGS_AT13(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, ...) _13
#define _OPENRHI_VA_ARGS_AT14(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, ...) _14
#define _OPENRHI_VA_ARGS_AT15(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, ...) _15
#define _OPENRHI_VA_ARGS_AT16(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, ...) _16
#define _OPENRHI_VA_ARGS_AT17(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, ...) _17
#define _OPENRHI_VA_ARGS_AT18(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, ...) _18
#define _OPENRHI_VA_ARGS_AT19(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, ...) _19
#define _OPENRHI_VA_ARGS_AT20(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, ...) _20
#define _OPENRHI_VA_ARGS_AT21(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, ...) _21
#define _OPENRHI_VA_ARGS_AT22(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, ...) _22
#define _OPENRHI_VA_ARGS_AT23(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, ...) _23
#define _OPENRHI_VA_ARGS_AT24(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, ...) _24
#define _OPENRHI_VA_ARGS_AT25(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, ...) _25
#define _OPENRHI_VA_ARGS_AT26(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, ...) _26
#define _OPENRHI_VA_ARGS_AT27(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, ...) _27
#define _OPENRHI_VA_ARGS_AT28(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, ...) _28
#define _OPENRHI_VA_ARGS_AT29(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, ...) _29
#define _OPENRHI_VA_ARGS_AT30(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, ...) _30
#define _OPENRHI_VA_ARGS_AT31(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, ...) _31
#define _OPENRHI_VA_ARGS_AT32(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, ...) _32
#define _OPENRHI_VA_ARGS_AT33(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, ...) _33
#define _OPENRHI_VA_ARGS_AT34(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, ...) _34
#define _OPENRHI_VA_ARGS_AT35(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, ...) _35
#define _OPENRHI_VA_ARGS_AT36(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, ...) _36
#define _OPENRHI_VA_ARGS_AT37(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, ...) _37
#define _OPENRHI_VA_ARGS_AT38(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, ...) _38
#define _OPENRHI_VA_ARGS_AT39(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, ...) _39
#define _OPENRHI_VA_ARGS_AT40(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, ...) _40
#define _OPENRHI_VA_ARGS_AT41(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, ...) _41
#define _OPENRHI_VA_ARGS_AT42(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, ...) _42
#define _OPENRHI_VA_ARGS_AT43(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, ...) _43
#define _OPENRHI_VA_ARGS_AT44(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, ...) _44
#define _OPENRHI_VA_ARGS_AT45(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, ...) _45
#define _OPENRHI_VA_ARGS_AT46(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, ...) _46
#define _OPENRHI_VA_ARGS_AT47(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, ...) _47
#define _OPENRHI_VA_ARGS_AT48(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, ...) _48
#define _OPENRHI_VA_ARGS_AT49(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, ...) _49
#define _OPENRHI_VA_ARGS_AT50(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, ...) _50
#define _OPENRHI_VA_ARGS_AT51(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, ...) _51
#define _OPENRHI_VA_ARGS_AT52(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, ...) _52
#define _OPENRHI_VA_ARGS_AT53(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, ...) _53
#define _OPENRHI_VA_ARGS_AT54(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, ...) _54
#define _OPENRHI_VA_ARGS_AT55(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, ...) _55
#define _OPENRHI_VA_ARGS_AT56(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, ...) _56
#define _OPENRHI_VA_ARGS_AT57(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, ...) _57
#define _OPENRHI_VA_ARGS_AT58(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, ...) _58
#define _OPENRHI_VA_ARGS_AT59(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, ...) _59
#define _OPENRHI_VA_ARGS_AT60(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, ...) _60
#define _OPENRHI_VA_ARGS_AT61(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, ...) _61
#define _OPENRHI_VA_ARGS_AT62(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, ...) _62
#define _OPENRHI_VA_ARGS_AT63(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, ...) _63
#define _OPENRHI_VA_ARGS_AT64(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, ...) _64
#define _OPENRHI_VA_ARGS_AT65(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, ...) _65
#define _OPENRHI_VA_ARGS_AT66(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, ...) _66
#define _OPENRHI_VA_ARGS_AT67(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, ...) _67
#define _OPENRHI_VA_ARGS_AT68(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, ...) _68
#define _OPENRHI_VA_ARGS_AT69(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, ...) _69
#define _OPENRHI_VA_ARGS_AT70(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, ...) _70
#define _OPENRHI_VA_ARGS_AT71(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, ...) _71
#define _OPENRHI_VA_ARGS_AT72(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, ...) _72
#define _OPENRHI_VA_ARGS_AT73(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, ...) _73
#define _OPENRHI_VA_ARGS_AT74(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, ...) _74
#define _OPENRHI_VA_ARGS_AT75(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, ...) _75
#define _OPENRHI_VA_ARGS_AT76(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, ...) _76
#define _OPENRHI_VA_ARGS_AT77(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, ...) _77
#define _OPENRHI_VA_ARGS_AT78(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, ...) _78
#define _OPENRHI_VA_ARGS_AT79(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, ...) _79
#define _OPENRHI_VA_ARGS_AT80(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, ...) _80
#define _OPENRHI_VA_ARGS_AT81(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, ...) _81
#define _OPENRHI_VA_ARGS_AT82(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, ...) _82
#define _OPENRHI_VA_ARGS_AT83(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, ...) _83
#define _OPENRHI_VA_ARGS_AT84(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, ...) _84
#define _OPENRHI_VA_ARGS_AT85(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, ...) _85
#define _OPENRHI_VA_ARGS_AT86(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, ...) _86
#define _OPENRHI_VA_ARGS_AT87(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, ...) _87
#define _OPENRHI_VA_ARGS_AT88(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, ...) _88
#define _OPENRHI_VA_ARGS_AT89(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, ...) _89
#define _OPENRHI_VA_ARGS_AT90(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, ...) _90
#define _OPENRHI_VA_ARGS_AT91(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, ...) _91
#define _OPENRHI_VA_ARGS_AT92(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, ...) _92
#define _OPENRHI_VA_ARGS_AT93(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, ...) _93
#define _OPENRHI_VA_ARGS_AT94(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, ...) _94
#define _OPENRHI_VA_ARGS_AT95(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, ...) _95
#define _OPENRHI_VA_ARGS_AT96(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, ...) _96
#define _OPENRHI_VA_ARGS_AT97(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, ...) _97
#define _OPENRHI_VA_ARGS_AT98(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, ...) _98
#define _OPENRHI_VA_ARGS_AT99(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, ...) _99
#define _OPENRHI_VA_ARGS_AT100(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, ...) _100
#define _OPENRHI_VA_ARGS_AT101(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, ...) _101
#define _OPENRHI_VA_ARGS_AT102(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, ...) _102
#define _OPENRHI_VA_ARGS_AT103(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, ...) _103
#define _OPENRHI_VA_ARGS_AT104(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, ...) _104
#define _OPENRHI_VA_ARGS_AT105(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, _105, ...) _105
#define _OPENRHI_VA_ARGS_AT106(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, _105, _106, ...) _106
#define _OPENRHI_VA_ARGS_AT107(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, _105, _106, _107, ...) _107
#define _OPENRHI_VA_ARGS_AT108(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, _105, _106, _107, _108, ...) _108
#define _OPENRHI_VA_ARGS_AT109(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, _105, _106, _107, _108, _109, ...) _109
#define _OPENRHI_VA_ARGS_AT110(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, _105, _106, _107, _108, _109, _110, ...) _110
#define _OPENRHI_VA_ARGS_AT111(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, _105, _106, _107, _108, _109, _110, _111, ...) _111
#define _OPENRHI_VA_ARGS_AT112(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, _105, _106, _107, _108, _109, _110, _111, _112, ...) _112
#define _OPENRHI_VA_ARGS_AT113(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, _105, _106, _107, _108, _109, _110, _111, _112, _113, ...) _113
#define _OPENRHI_VA_ARGS_AT114(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, _105, _106, _107, _108, _109, _110, _111, _112, _113, _114, ...) _114
#define _OPENRHI_VA_ARGS_AT115(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, _105, _106, _107, _108, _109, _110, _111, _112, _113, _114, _115, ...) _115
#define _OPENRHI_VA_ARGS_AT116(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, _105, _106, _107, _108, _109, _110, _111, _112, _113, _114, _115, _116, ...) _116
#define _OPENRHI_VA_ARGS_AT117(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, _105, _106, _107, _108, _109, _110, _111, _112, _113, _114, _115, _116, _117, ...) _117
#define _OPENRHI_VA_ARGS_AT118(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, _105, _106, _107, _108, _109, _110, _111, _112, _113, _114, _115, _116, _117, _118, ...) _118
#define _OPENRHI_VA_ARGS_AT119(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, _105, _106, _107, _108, _109, _110, _111, _112, _113, _114, _115, _116, _117, _118, _119, ...) _119
#define _OPENRHI_VA_ARGS_AT120(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, _105, _106, _107, _108, _109, _110, _111, _112, _113, _114, _115, _116, _117, _118, _119, _120, ...) _120
#define _OPENRHI_VA_ARGS_AT121(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, _105, _106, _107, _108, _109, _110, _111, _112, _113, _114, _115, _116, _117, _118, _119, _120, _121, ...) _121
#define _OPENRHI_VA_ARGS_AT122(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, _105, _106, _107, _108, _109, _110, _111, _112, _113, _114, _115, _116, _117, _118, _119, _120, _121, _122, ...) _122
#define _OPENRHI_VA_ARGS_AT123(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, _105, _106, _107, _108, _109, _110, _111, _112, _113, _114, _115, _116, _117, _118, _119, _120, _121, _122, _123, ...) _123

#define OPENRHI_VA_ARGS_AT(index, ...) _OPENRHI_EXPAND(OPENRHI_MERGE_TOKENS(_OPENRHI_VA_ARGS_AT, index)(__VA_ARGS__))

#ifdef OPENRHI_CPP
    #define _OPENRHI_ENUM_ENTRY(index, ...) OPENRHI_VA_ARGS_AT(index, __VA_ARGS__)
    #define _OPENRHI_ENUM_EXPAND(...) OPENRHI_SEQN(_OPENRHI_ENUM_ENTRY, OPENRHI_NARGS(__VA_ARGS__), __VA_ARGS__)

    #define OpenRhi(name) name
    #define OpenRhiFunc(name) name
    #define OpenRhiConstant(name) name
    #define OpenRhiNamespaceBegin namespace openrhi {
    #define OpenRhiNamespaceEnd }
    #define OpenRhiForwardStruct(name) struct name
    #define OpenRhiStruct(name) struct name
    #define OpenRhiUnion(name) union name
    #define OpenRhiMember(name, member) member
    #define NonOpenRhiForwardStruct(name) struct name

    #define OpenRhiEnum(name, type, ...) \
        enum class name : type { \
            _OPENRHI_ENUM_EXPAND(__VA_ARGS__), \
            MAX_NUM, \
        }

    #define OpenRhiBits(name, type, ...) \
        enum class name : type; \
        constexpr name operator ~ (name val) { return (name)(~(type)val); } \
        constexpr type operator & (name val0, name val1) { return (type)val0 & (type)val1; } \
        constexpr name operator | (name val0, name val1) { return (name)((type)val0 | (type)val1); } \
        constexpr name& operator &= (name& val0, name val1) { val0 = (name)(val0 & val1); return val0; } \
        constexpr name& operator |= (name& val0, name val1) { val0 = (name)(val0 | val1); return val0; } \
        enum class name : type { \
            _OPENRHI_ENUM_EXPAND(__VA_ARGS__), \
        }

    #define OpenRhiDeref(arg) (&arg)
    #define OpenRhiDefault(arg) = arg
    #define OpenRhiZero {}
    #define OpenRhiScopedMember(name, member) name::member

    #define OPENRHI_REF &
#else
    #define OPENRHI_NAME_(name) OPENRHI_MERGE_TOKENS(OpenRhi(name), _)
    #define _OPENRHI_ENUM_ENTRY(index, prefix, ...) OPENRHI_MERGE_TOKENS(prefix, OPENRHI_VA_ARGS_AT(index, __VA_ARGS__))
    #define _OPENRHI_ENUM_EXPAND(prefix, ...) OPENRHI_SEQN(_OPENRHI_ENUM_ENTRY, OPENRHI_NARGS(__VA_ARGS__), prefix, __VA_ARGS__)

    #define OpenRhi(name) OPENRHI_NAME_C(name)
    #define OpenRhiFunc(name) OPENRHI_FUNC_NAME_C(name)
    #define OpenRhiConstant(name) OPENRHI_CONST_NAME_C(name)
    #define OpenRhiNamespaceBegin
    #define OpenRhiNamespaceEnd
    #define OpenRhiForwardStruct(name) typedef struct OpenRhi(name) OpenRhi(name)
    #define OpenRhiStruct(name) OpenRhiForwardStruct(name); struct OpenRhi(name)
    #define OpenRhiUnion(name) typedef union OpenRhi(name) OpenRhi(name); union OpenRhi(name)
    #define OpenRhiMember(name, member) OPENRHI_MERGE_TOKENS(OPENRHI_NAME_(name), member)
    #define NonOpenRhiForwardStruct(name) typedef struct name name

    #define OpenRhiEnum(name, type, ...) \
        typedef type OpenRhi(name); \
        typedef enum OPENRHI_NAME_(name) { \
            _OPENRHI_ENUM_EXPAND(OPENRHI_NAME_(name), __VA_ARGS__), \
            OPENRHI_MERGE_TOKENS(OPENRHI_NAME_(name), MAX_NUM), \
        } OPENRHI_NAME_(name)

    #define OpenRhiBits(name, type, ...) \
        typedef type OpenRhi(name); \
        typedef enum OPENRHI_NAME_(name) { \
            _OPENRHI_ENUM_EXPAND(OPENRHI_NAME_(name), __VA_ARGS__), \
        } OPENRHI_NAME_(name)

    #define OpenRhiDeref(arg) (arg)
    #define OpenRhiDefault(arg)
    #define OpenRhiZero {0}
    #define OpenRhiScopedMember(name, member) OpenRhiMember(name, member)

    #define OPENRHI_REF *
#endif

#define OpenRhiPtr(name) OpenRhi(name)*
#define OpenRhiRef(name) OpenRhi(name) OPENRHI_REF
#define NonOpenRhiRef(name) name OPENRHI_REF
