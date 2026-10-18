// © 2026 NVIDIA Corporation

// Goal: backend-neutral hardware video encode/decode command submission
// Video formats: https://learn.microsoft.com/en-us/windows/win32/medfound/recommended-8-bit-yuv-formats-for-video-rendering

#pragma once

#define OPENRHI_VIDEO_H 1

OpenRhiNamespaceBegin

OpenRhiForwardStruct(VideoSession);
OpenRhiForwardStruct(VideoSessionParameters);
OpenRhiForwardStruct(VideoPicture);

OpenRhiEnum(VideoSessionType, uint8_t,
    DECODE,
    ENCODE
);

OpenRhiEnum(VideoDecodeArgumentType, uint8_t,
    PICTURE_PARAMETERS,
    INVERSE_QUANTIZATION_MATRIX,
    SLICE_CONTROL
);

OpenRhiEnum(VideoFrameType, uint8_t,
    IDR,
    I,
    P,
    B
);

OpenRhiEnum(VideoEncodeRateControlMode, uint8_t,
    CQP,
    CBR,
    VBR
);

OpenRhiEnum(VideoAV1ReferenceName, uint8_t,
    NONE,
    LAST,
    LAST2,
    LAST3,
    GOLDEN,
    BWDREF,
    ALTREF2,
    ALTREF
);

OpenRhiEnum(VideoPictureUsage, uint8_t,
    DECODE_OUTPUT,
    DECODE_REFERENCE,
    ENCODE_INPUT,
    ENCODE_REFERENCE
);

OpenRhiEnum(VideoPictureRole, uint8_t,
    DECODE_OUTPUT,             // distinct or output-only destination
    DECODE_REFERENCE,          // existing DPB picture read by the decode operation
    DECODE_SETUP,              // distinct reconstructed picture written into the DPB
    DECODE_OUTPUT_AND_SETUP,   // coincident destination and reconstructed DPB picture
    ENCODE_INPUT,              // source picture read by the encode operation
    ENCODE_REFERENCE,          // existing DPB picture read by the encode operation
    ENCODE_RECONSTRUCTED       // reconstructed picture written into the DPB
);

OpenRhiBits(VideoDecodeBitstreamSourceBits, uint8_t,
    NONE                                    = 0,
    BUFFER                                  = OpenRhiBit(0),
    HOST                                    = OpenRhiBit(1)
);

OpenRhiBits(VideoH264SequenceParameterSetBits, uint16_t,
    NONE                                    = 0,
    CONSTRAINT_SET0                         = OpenRhiBit(0),
    CONSTRAINT_SET1                         = OpenRhiBit(1),
    CONSTRAINT_SET2                         = OpenRhiBit(2),
    CONSTRAINT_SET3                         = OpenRhiBit(3),
    CONSTRAINT_SET4                         = OpenRhiBit(4),
    CONSTRAINT_SET5                         = OpenRhiBit(5),
    DIRECT_8X8_INFERENCE                    = OpenRhiBit(6),
    MB_ADAPTIVE_FRAME_FIELD                 = OpenRhiBit(7),
    FRAME_MBS_ONLY                          = OpenRhiBit(8),
    DELTA_PIC_ORDER_ALWAYS_ZERO             = OpenRhiBit(9),
    SEPARATE_COLOUR_PLANE                   = OpenRhiBit(10),
    GAPS_IN_FRAME_NUM_ALLOWED               = OpenRhiBit(11),
    QPPRIME_Y_ZERO_TRANSFORM_BYPASS         = OpenRhiBit(12)
);

OpenRhiBits(VideoH264PictureParameterSetBits, uint8_t,
    NONE                                    = 0,
    TRANSFORM_8X8_MODE                      = OpenRhiBit(0),
    REDUNDANT_PIC_CNT_PRESENT               = OpenRhiBit(1),
    CONSTRAINED_INTRA_PRED                  = OpenRhiBit(2),
    DEBLOCKING_FILTER_CONTROL_PRESENT       = OpenRhiBit(3),
    WEIGHTED_PRED                           = OpenRhiBit(4),
    BOTTOM_FIELD_PIC_ORDER_IN_FRAME         = OpenRhiBit(5),
    ENTROPY_CODING_MODE                     = OpenRhiBit(6)
);

OpenRhiBits(VideoH264DecodePictureBits, uint8_t,
    NONE                                    = 0,
    FIELD_PICTURE                           = OpenRhiBit(0),
    INTRA                                   = OpenRhiBit(1),
    IDR                                     = OpenRhiBit(2),
    BOTTOM_FIELD                            = OpenRhiBit(3),
    REFERENCE                               = OpenRhiBit(4),
    COMPLEMENTARY_FIELD_PAIR                = OpenRhiBit(5)
);

OpenRhiBits(VideoH264DecodeReferenceBits, uint8_t,
    NONE                                    = 0,
    TOP_FIELD                               = OpenRhiBit(0),
    BOTTOM_FIELD                            = OpenRhiBit(1),
    LONG_TERM                               = OpenRhiBit(2),
    NON_EXISTING                            = OpenRhiBit(3)
);

OpenRhiBits(VideoH265DecodePictureBits, uint8_t,
    NONE                                    = 0,
    IRAP                                    = OpenRhiBit(0),
    IDR                                     = OpenRhiBit(1),
    REFERENCE                               = OpenRhiBit(2),
    SHORT_TERM_REF_PIC_SET_SPS              = OpenRhiBit(3)
);

OpenRhiBits(VideoH265ProfileTierLevelBits, uint8_t,
    NONE                                    = 0,
    TIER                                    = OpenRhiBit(0),
    PROGRESSIVE_SOURCE                      = OpenRhiBit(1),
    INTERLACED_SOURCE                       = OpenRhiBit(2),
    NON_PACKED_CONSTRAINT                   = OpenRhiBit(3),
    FRAME_ONLY_CONSTRAINT                   = OpenRhiBit(4)
);

OpenRhiBits(VideoH265VideoParameterSetBits, uint8_t,
    NONE                                    = 0,
    TEMPORAL_ID_NESTING                     = OpenRhiBit(0),
    SUB_LAYER_ORDERING_INFO_PRESENT         = OpenRhiBit(1),
    TIMING_INFO_PRESENT                     = OpenRhiBit(2),
    POC_PROPORTIONAL_TO_TIMING              = OpenRhiBit(3)
);

OpenRhiBits(VideoH265SequenceParameterSetBits, uint32_t,
    NONE                                    = 0,
    TEMPORAL_ID_NESTING                     = OpenRhiBit(0),
    SEPARATE_COLOUR_PLANE                   = OpenRhiBit(1),
    CONFORMANCE_WINDOW                      = OpenRhiBit(2),
    SUB_LAYER_ORDERING_INFO_PRESENT         = OpenRhiBit(3),
    SCALING_LIST_ENABLED                    = OpenRhiBit(4),
    SCALING_LIST_DATA_PRESENT               = OpenRhiBit(5),
    AMP_ENABLED                             = OpenRhiBit(6),
    SAMPLE_ADAPTIVE_OFFSET_ENABLED          = OpenRhiBit(7),
    PCM_ENABLED                             = OpenRhiBit(8),
    PCM_LOOP_FILTER_DISABLED                = OpenRhiBit(9),
    LONG_TERM_REF_PICS_PRESENT              = OpenRhiBit(10),
    TEMPORAL_MVP_ENABLED                    = OpenRhiBit(11),
    STRONG_INTRA_SMOOTHING_ENABLED          = OpenRhiBit(12),
    VUI_PARAMETERS_PRESENT                  = OpenRhiBit(13)
);

OpenRhiBits(VideoH265PictureParameterSetBits, uint32_t,
    NONE                                    = 0,
    DEPENDENT_SLICE_SEGMENTS_ENABLED        = OpenRhiBit(0),
    OUTPUT_FLAG_PRESENT                     = OpenRhiBit(1),
    SIGN_DATA_HIDING_ENABLED                = OpenRhiBit(2),
    CABAC_INIT_PRESENT                      = OpenRhiBit(3),
    CONSTRAINED_INTRA_PRED                  = OpenRhiBit(4),
    TRANSFORM_SKIP_ENABLED                  = OpenRhiBit(5),
    CU_QP_DELTA_ENABLED                     = OpenRhiBit(6),
    SLICE_CHROMA_QP_OFFSETS_PRESENT         = OpenRhiBit(7),
    WEIGHTED_PRED                           = OpenRhiBit(8),
    WEIGHTED_BIPRED                         = OpenRhiBit(9),
    TRANSQUANT_BYPASS_ENABLED               = OpenRhiBit(10),
    TILES_ENABLED                           = OpenRhiBit(11),
    ENTROPY_CODING_SYNC_ENABLED             = OpenRhiBit(12),
    UNIFORM_SPACING                         = OpenRhiBit(13),
    LOOP_FILTER_ACROSS_TILES_ENABLED        = OpenRhiBit(14),
    LOOP_FILTER_ACROSS_SLICES_ENABLED       = OpenRhiBit(15),
    DEBLOCKING_FILTER_CONTROL_PRESENT       = OpenRhiBit(16),
    DEBLOCKING_FILTER_OVERRIDE_ENABLED      = OpenRhiBit(17),
    DEBLOCKING_FILTER_DISABLED              = OpenRhiBit(18),
    SCALING_LIST_DATA_PRESENT               = OpenRhiBit(19),
    LISTS_MODIFICATION_PRESENT              = OpenRhiBit(20),
    SLICE_SEGMENT_HEADER_EXTENSION_PRESENT  = OpenRhiBit(21)
);

OpenRhiBits(VideoH265ShortTermRefPicSetBits, uint8_t,
    NONE                                    = 0,
    INTER_REF_PIC_SET_PREDICTION            = OpenRhiBit(0),
    DELTA_RPS_SIGN                          = OpenRhiBit(1)
);

OpenRhiBits(VideoEncodeBits, uint8_t,
    NONE                                    = 0,
    FORCE_KEY_FRAME                         = OpenRhiBit(0)
);

OpenRhiBits(VideoAV1EncodeFeatureBits, uint32_t,
    NONE                                    = 0,
    ORDER_HINT_TOOLS                        = OpenRhiBit(0),
    LOOP_RESTORATION_FILTER                 = OpenRhiBit(1),
    FORCED_INTEGER_MOTION_VECTORS           = OpenRhiBit(2),
    AUTO_SEGMENTATION                       = OpenRhiBit(3),
    CDEF_FILTERING                          = OpenRhiBit(4),
    QUANTIZATION_DELTAS                     = OpenRhiBit(5),
    LOOP_FILTER_DELTAS                      = OpenRhiBit(6)
);

OpenRhiBits(VideoAV1SequenceBits, uint32_t,
    NONE                                    = 0,
    STILL_PICTURE                           = OpenRhiBit(0),
    REDUCED_STILL_PICTURE_HEADER            = OpenRhiBit(1),
    USE_128X128_SUPERBLOCK                  = OpenRhiBit(2),
    ENABLE_FILTER_INTRA                     = OpenRhiBit(3),
    ENABLE_INTRA_EDGE_FILTER                = OpenRhiBit(4),
    ENABLE_INTERINTRA_COMPOUND              = OpenRhiBit(5),
    ENABLE_MASKED_COMPOUND                  = OpenRhiBit(6),
    ENABLE_WARPED_MOTION                    = OpenRhiBit(7),
    ENABLE_DUAL_FILTER                      = OpenRhiBit(8),
    ENABLE_ORDER_HINT                       = OpenRhiBit(9),
    ENABLE_JNT_COMP                         = OpenRhiBit(10),
    ENABLE_REF_FRAME_MVS                    = OpenRhiBit(11),
    FRAME_ID_NUMBERS_PRESENT                = OpenRhiBit(12),
    ENABLE_SUPERRES                         = OpenRhiBit(13),
    ENABLE_CDEF                             = OpenRhiBit(14),
    ENABLE_RESTORATION                      = OpenRhiBit(15),
    FILM_GRAIN_PARAMS_PRESENT               = OpenRhiBit(16), // TODO: currently unsupported for decode
    TIMING_INFO_PRESENT                     = OpenRhiBit(17),
    INITIAL_DISPLAY_DELAY_PRESENT           = OpenRhiBit(18),
    MONO_CHROME                             = OpenRhiBit(19),
    COLOR_RANGE                             = OpenRhiBit(20),
    SEPARATE_UV_DELTA_Q                     = OpenRhiBit(21),
    COLOR_DESCRIPTION_PRESENT               = OpenRhiBit(22)
);

OpenRhiBits(VideoAV1PictureBits, uint32_t,
    NONE                                    = 0,
    ERROR_RESILIENT_MODE                    = OpenRhiBit(0),
    DISABLE_CDF_UPDATE                      = OpenRhiBit(1),
    USE_SUPERRES                            = OpenRhiBit(2),
    RENDER_AND_FRAME_SIZE_DIFFERENT         = OpenRhiBit(3),
    ALLOW_SCREEN_CONTENT_TOOLS              = OpenRhiBit(4),
    IS_FILTER_SWITCHABLE                    = OpenRhiBit(5),
    FORCE_INTEGER_MV                        = OpenRhiBit(6),
    FRAME_SIZE_OVERRIDE                     = OpenRhiBit(7),
    BUFFER_REMOVAL_TIME_PRESENT             = OpenRhiBit(8),
    ALLOW_INTRABC                           = OpenRhiBit(9),
    FRAME_REFS_SHORT_SIGNALING              = OpenRhiBit(10),
    ALLOW_HIGH_PRECISION_MV                 = OpenRhiBit(11),
    IS_MOTION_MODE_SWITCHABLE               = OpenRhiBit(12),
    USE_REF_FRAME_MVS                       = OpenRhiBit(13),
    DISABLE_FRAME_END_UPDATE_CDF            = OpenRhiBit(14),
    ALLOW_WARPED_MOTION                     = OpenRhiBit(15),
    REDUCED_TX_SET                          = OpenRhiBit(16),
    REFERENCE_SELECT                        = OpenRhiBit(17),
    SKIP_MODE_PRESENT                       = OpenRhiBit(18),
    DELTA_Q_PRESENT                         = OpenRhiBit(19),
    DELTA_LF_PRESENT                        = OpenRhiBit(20),
    DELTA_LF_MULTI                          = OpenRhiBit(21),
    SEGMENTATION_ENABLED                    = OpenRhiBit(22),
    SEGMENTATION_UPDATE_MAP                 = OpenRhiBit(23),
    SEGMENTATION_TEMPORAL_UPDATE            = OpenRhiBit(24),
    SEGMENTATION_UPDATE_DATA                = OpenRhiBit(25),
    USES_LR                                 = OpenRhiBit(26),
    USES_CHROMA_LR                          = OpenRhiBit(27),
    SHOW_FRAME                              = OpenRhiBit(28),
    SHOWABLE_FRAME                          = OpenRhiBit(29),
    APPLY_GRAIN                             = OpenRhiBit(30)  // TODO: currently unsupported for decode
);

OpenRhiStruct(VideoSessionDesc) {
    OpenRhi(VideoSessionType) type;
    OpenRhi(VideoCodec) codec; // fixed profiles: H.264 High, H.265 Main/Main10 (by format), AV1 Main; H.264 sessions are progressive-only
    OpenRhi(Format) format;
    uint32_t width;
    uint32_t height;
    uint32_t maxReferenceNum;
};

OpenRhiStruct(VideoCapabilities) {
    uint32_t widthMin;
    uint32_t heightMin;
    uint32_t widthMax;
    uint32_t heightMax;
    uint32_t pictureAccessGranularityWidth;
    uint32_t pictureAccessGranularityHeight;
    uint32_t maxReferenceNum;
    uint32_t bitstreamOffsetAlignment;
    uint32_t bitstreamSizeAlignment;
    uint64_t bitstreamSizeMax;
    uint32_t metadataOffsetAlignment;
    uint32_t resolvedMetadataOffsetAlignment;
    uint32_t encodeFeedbackMaxPendingNum;        // maximum per-session feedback ranges not yet consumed by "GetVideoEncodeFeedback"; reset by "ResetVideoSession"
    uint64_t metadataSize;                       // minimum backend encode metadata range
    uint64_t resolvedMetadataSize;               // minimum resolved metadata range
    OpenRhi(VideoDecodeBitstreamSourceBits) decodeBitstreamSourceMask;
    OpenRhi(AccessStage) resolvedMetadataState;      // required state while the resolved metadata is written
    OpenRhi(QueueType) resolvedMetadataQueueType;    // queue used by the operation that writes resolved metadata
    uint32_t dpbTextureArrayMinLayerNum;         // minimum layer count when "dpbTextureArrayRequired" is true
    bool decodeDpbAndOutputCoincide;             // decode output and reconstructed/DPB setup may use the same texture subresource
    bool decodeDpbAndOutputDistinct;             // decode output and reconstructed/DPB setup may use different texture subresources
    bool dpbTextureArrayRequired;                // all decode or encode DPB pictures must use layers of one texture
    bool decodeNativeArgumentsSupported;
    bool encodeBitstreamRangeSizeSupported;      // if false, encode output ranges must extend to the end of the buffer
    bool encodeFeedbackSupported;
    bool encodeFeedbackResolveRequired;          // if true, call "CmdResolveVideoEncodeFeedback" after the encode
};

OpenRhiStruct(VideoAV1Capabilities) {
    uint32_t av1MaxLevel;
    uint32_t av1MaxTileColumnNum;
    uint32_t av1MaxTileRowNum;
    uint32_t av1MinTileWidth;
    uint32_t av1MinTileHeight;
    uint32_t av1MaxTileWidth;
    uint32_t av1MaxTileHeight;
    uint32_t av1SuperblockSizeMask;
    uint32_t av1MaxSingleReferenceNum;
    uint32_t av1SingleReferenceNameMask;
    uint32_t av1MaxUnidirectionalCompoundReferenceNum;
    uint32_t av1UnidirectionalCompoundReferenceNameMask;
    uint32_t av1MaxBidirectionalCompoundReferenceNum;
    uint32_t av1BidirectionalCompoundReferenceNameMask;
    uint32_t av1MaxTemporalLayerNum;
    uint32_t av1MaxSpatialLayerNum;
    uint32_t av1MaxOperatingPointNum;
    uint32_t av1MinQIndex;
    uint32_t av1MaxQIndex;
    OpenRhi(VideoAV1EncodeFeatureBits) av1EncodeRequiredFeatureFlags;
    OpenRhi(VideoAV1EncodeFeatureBits) av1EncodeSupportedFeatureFlags;
};

OpenRhiStruct(VideoReference) {
    OpenRhiPtr(VideoPicture) picture;
    uint32_t slot;
};

OpenRhiStruct(VideoBitstreamRange) {
    OpenRhiPtr(Buffer) buffer;
    uint64_t offset;               // decode slice/tile offsets are relative to this offset
    uint64_t size;                 // decode bitstream byte range; H.264/H.265 neutral decode expects Annex-B byte stream data
};

OpenRhiStruct(VideoDecodeBitstreamRange) {
    OpenRhiOptional OpenRhiPtr(Buffer) buffer;
    OpenRhiOptional const void* data;  // host range starts at this address; "offset" applies only to "buffer"
    uint64_t offset;
    uint64_t size;                 // H.264/H.265 neutral decode expects Annex-B byte stream data
};

OpenRhiStruct(VideoPictureDesc) {
    OpenRhiPtr(Texture) texture;
    OpenRhi(VideoPictureUsage) usage;
    OpenRhiOptional OpenRhi(Dim_t) width;  // defaults to the texture width
    OpenRhiOptional OpenRhi(Dim_t) height; // defaults to the texture height
    OpenRhiOptional OpenRhi(Dim_t) layer;
};

OpenRhiStruct(VideoPictureState) {
    OpenRhi(AccessLayoutStage) required;             // state required while the picture participates in CmdDecodeVideo or CmdEncodeVideo
    OpenRhi(AccessLayoutStage) videoQueueAfter;      // destination state of the optional post-video transition
    OpenRhi(AccessLayoutStage) consumerQueueBefore;  // source state for the consumer queue transition
    bool transitionOnVideoQueue;                 // transition "required" -> "videoQueueAfter" before cross-queue consumption
};

OpenRhiStruct(VideoH264SequenceParameterSetDesc) {
    OpenRhi(VideoH264SequenceParameterSetBits) flags;
    uint8_t profileIdc;
    uint8_t levelIdc;
    uint8_t chromaFormatIdc;
    uint8_t sequenceParameterSetId;
    uint8_t bitDepthLumaMinus8;
    uint8_t bitDepthChromaMinus8;
    uint8_t log2MaxFrameNumMinus4;
    uint8_t pictureOrderCountType;
    int32_t offsetForNonReferencePicture;
    int32_t offsetForTopToBottomField;
    uint8_t log2MaxPictureOrderCountLsbMinus4;
    uint8_t referenceFrameNum;
    uint16_t pictureWidthInMbsMinus1;
    uint16_t pictureHeightInMapUnitsMinus1;
};

OpenRhiStruct(VideoH264PictureParameterSetDesc) {
    OpenRhi(VideoH264PictureParameterSetBits) flags;
    uint8_t sequenceParameterSetId;
    uint8_t pictureParameterSetId;
    uint8_t refIndexL0DefaultActiveMinus1;
    uint8_t refIndexL1DefaultActiveMinus1;
    uint8_t weightedBipredIdc;
    int8_t pictureInitQpMinus26;
    int8_t pictureInitQsMinus26;
    int8_t chromaQpIndexOffset;
    int8_t secondChromaQpIndexOffset;
};

OpenRhiStruct(VideoH264SessionParametersDesc) {
    const OpenRhiPtr(VideoH264SequenceParameterSetDesc) sequenceParameterSets;
    uint32_t sequenceParameterSetNum;
    const OpenRhiPtr(VideoH264PictureParameterSetDesc) pictureParameterSets;
    uint32_t pictureParameterSetNum;
    OpenRhiOptional uint32_t maxSequenceParameterSetNum; // defaults to "sequenceParameterSetNum"
    OpenRhiOptional uint32_t maxPictureParameterSetNum;  // defaults to "pictureParameterSetNum"
};

OpenRhiStruct(VideoH265ProfileTierLevelDesc) {
    OpenRhi(VideoH265ProfileTierLevelBits) flags;
    uint8_t generalProfileIdc;
    uint8_t generalLevelIdc;
};

OpenRhiStruct(VideoH265DecPicBufMgrDesc) {
    uint8_t maxDecPicBufferingMinus1[8];
    uint8_t maxNumReorderPics[8];
    uint32_t maxLatencyIncreasePlus1[8];
};

OpenRhiStruct(VideoH265ScalingListsDesc) {
    uint8_t scalingList4x4[6][16];
    uint8_t scalingList8x8[6][64];
    uint8_t scalingList16x16[6][64];
    uint8_t scalingList32x32[2][64];
    uint8_t scalingListDCCoef16x16[6];
    uint8_t scalingListDCCoef32x32[2];
};

OpenRhiStruct(VideoH265ShortTermRefPicSetDesc) {
    OpenRhi(VideoH265ShortTermRefPicSetBits) flags;
    uint8_t numNegativePics;
    uint8_t numPositivePics;
    uint32_t deltaIdxMinus1;
    uint16_t useDeltaFlag;
    uint16_t absDeltaRpsMinus1;
    uint16_t usedByCurrPicFlag;
    uint16_t usedByCurrPicS0Flag;
    uint16_t usedByCurrPicS1Flag;
    uint16_t deltaPocS0Minus1[16];
    uint16_t deltaPocS1Minus1[16];
};

OpenRhiStruct(VideoH265LongTermRefPicsSpsDesc) {
    uint32_t usedByCurrPicLtSpsFlag;
    uint32_t ltRefPicPocLsbSps[32];
};

OpenRhiStruct(VideoH265VideoParameterSetDesc) {
    OpenRhi(VideoH265VideoParameterSetBits) flags;
    uint8_t videoParameterSetId;
    uint8_t maxSubLayersMinus1;
    uint32_t numUnitsInTick;
    uint32_t timeScale;
    uint32_t numTicksPocDiffOneMinus1;
    OpenRhi(VideoH265ProfileTierLevelDesc) profileTierLevel;
    OpenRhi(VideoH265DecPicBufMgrDesc) decPicBufMgr;
};

OpenRhiStruct(VideoH265SequenceParameterSetDesc) {
    OpenRhi(VideoH265SequenceParameterSetBits) flags;
    uint8_t videoParameterSetId;
    uint8_t maxSubLayersMinus1;
    uint8_t sequenceParameterSetId;
    uint8_t chromaFormatIdc;
    uint32_t pictureWidthInLumaSamples;
    uint32_t pictureHeightInLumaSamples;
    uint8_t bitDepthLumaMinus8;
    uint8_t bitDepthChromaMinus8;
    uint8_t log2MaxPictureOrderCountLsbMinus4;
    uint8_t log2MinLumaCodingBlockSizeMinus3;
    uint8_t log2DiffMaxMinLumaCodingBlockSize;
    uint8_t log2MinLumaTransformBlockSizeMinus2;
    uint8_t log2DiffMaxMinLumaTransformBlockSize;
    uint8_t maxTransformHierarchyDepthInter;
    uint8_t maxTransformHierarchyDepthIntra;
    uint8_t pcmSampleBitDepthLumaMinus1;
    uint8_t pcmSampleBitDepthChromaMinus1;
    uint8_t log2MinPcmLumaCodingBlockSizeMinus3;
    uint8_t log2DiffMaxMinPcmLumaCodingBlockSize;
    uint32_t confWinLeftOffset;
    uint32_t confWinRightOffset;
    uint32_t confWinTopOffset;
    uint32_t confWinBottomOffset;
    const OpenRhiPtr(VideoH265ShortTermRefPicSetDesc) shortTermRefPicSets;
    uint8_t numShortTermRefPicSets;
    uint8_t numLongTermRefPicsSps;
    OpenRhi(VideoH265ProfileTierLevelDesc) profileTierLevel;
    OpenRhi(VideoH265DecPicBufMgrDesc) decPicBufMgr;
    OpenRhiOptional const OpenRhiPtr(VideoH265ScalingListsDesc) scalingLists;
    OpenRhiOptional const OpenRhiPtr(VideoH265LongTermRefPicsSpsDesc) longTermRefPicsSps;
};

OpenRhiStruct(VideoH265PictureParameterSetDesc) {
    OpenRhi(VideoH265PictureParameterSetBits) flags;
    uint8_t pictureParameterSetId;
    uint8_t sequenceParameterSetId;
    uint8_t videoParameterSetId;
    uint8_t numExtraSliceHeaderBits;
    uint8_t refIndexL0DefaultActiveMinus1;
    uint8_t refIndexL1DefaultActiveMinus1;
    int8_t initQpMinus26;
    uint8_t diffCuQpDeltaDepth;
    int8_t cbQpOffset;
    int8_t crQpOffset;
    int8_t betaOffsetDiv2;
    int8_t tcOffsetDiv2;
    uint8_t log2ParallelMergeLevelMinus2;
    uint8_t tileColumnNumMinus1;
    uint8_t tileRowNumMinus1;
    uint16_t columnWidthMinus1[19];
    uint16_t rowHeightMinus1[21];
    OpenRhiOptional const OpenRhiPtr(VideoH265ScalingListsDesc) scalingLists;
};

OpenRhiStruct(VideoH265SessionParametersDesc) {
    const OpenRhiPtr(VideoH265VideoParameterSetDesc) videoParameterSets;
    uint32_t videoParameterSetNum;
    const OpenRhiPtr(VideoH265SequenceParameterSetDesc) sequenceParameterSets;
    uint32_t sequenceParameterSetNum;
    const OpenRhiPtr(VideoH265PictureParameterSetDesc) pictureParameterSets;
    uint32_t pictureParameterSetNum;
    OpenRhiOptional uint32_t maxVideoParameterSetNum;    // defaults to "videoParameterSetNum"
    OpenRhiOptional uint32_t maxSequenceParameterSetNum; // defaults to "sequenceParameterSetNum"
    OpenRhiOptional uint32_t maxPictureParameterSetNum;  // defaults to "pictureParameterSetNum"
};

OpenRhiStruct(VideoAnnexBParameterSetsDesc) {
    OpenRhi(VideoCodec) codec;
    OpenRhiOptional const OpenRhiPtr(VideoH264SequenceParameterSetDesc) h264Sps;
    OpenRhiOptional const OpenRhiPtr(VideoH264PictureParameterSetDesc) h264Pps;
    OpenRhiOptional const OpenRhiPtr(VideoH265VideoParameterSetDesc) h265Vps;
    OpenRhiOptional const OpenRhiPtr(VideoH265SequenceParameterSetDesc) h265Sps;
    OpenRhiOptional const OpenRhiPtr(VideoH265PictureParameterSetDesc) h265Pps;
    OpenRhiOptional uint8_t* dst; // if null, only "writtenSize" is returned
    uint64_t dstSize;
    uint64_t writtenSize;
};

OpenRhiStruct(VideoAnnexBEndOfStreamDesc) {
    OpenRhi(VideoCodec) codec;
    OpenRhiOptional uint8_t* dst; // if null, only "writtenSize" is returned
    uint64_t dstSize;
    uint64_t writtenSize;
};

OpenRhiStruct(VideoAV1SequenceDesc) {
    OpenRhi(VideoAV1SequenceBits) flags;
    uint8_t seqProfile;
    uint8_t bitDepth;
    uint8_t subsamplingX;
    uint8_t subsamplingY;
    uint16_t maxFrameWidthMinus1;
    uint16_t maxFrameHeightMinus1;
    uint8_t frameWidthBitsMinus1;
    uint8_t frameHeightBitsMinus1;
    uint8_t deltaFrameIdLengthMinus2;
    uint8_t additionalFrameIdLengthMinus1;
    uint8_t orderHintBitsMinus1;
    uint8_t seqForceIntegerMv;
    uint8_t seqForceScreenContentTools;
    uint8_t level;
    uint8_t colorPrimaries;
    uint8_t transferCharacteristics;
    uint8_t matrixCoefficients;
    uint8_t chromaSamplePosition;
    uint32_t numUnitsInDisplayTick;
    uint32_t timeScale;
    uint32_t numTicksPerPictureMinus1;
};

OpenRhiStruct(VideoAV1ObuHeadersDesc) {
    OpenRhi(VideoAV1SequenceDesc) sequence;
    OpenRhiOptional uint8_t* dst; // if null, only "writtenSize" is returned
    uint64_t dstSize;
    uint64_t writtenSize;
};

OpenRhiStruct(VideoAV1SessionParametersDesc) {
    OpenRhi(VideoAV1SequenceDesc) sequence;
};

OpenRhiStruct(VideoSessionParametersDesc) {
    OpenRhiPtr(VideoSession) session;
    OpenRhiOptional const OpenRhiPtr(VideoH264SessionParametersDesc) h264Parameters; // required for H.264 except decode using native arguments on a supporting backend
    OpenRhiOptional const OpenRhiPtr(VideoH265SessionParametersDesc) h265Parameters; // required for neutral H.265 decode; optional for native decode arguments and encode
    OpenRhiOptional const OpenRhiPtr(VideoAV1SessionParametersDesc) av1Parameters;
};

OpenRhiStruct(VideoDecodeArgument) {
    OpenRhi(VideoDecodeArgumentType) type;
    uint32_t size;
    const void* data;
};

OpenRhiStruct(VideoH264EncodeReferenceDesc) {
    OpenRhi(VideoFrameType) frameType;
    uint8_t temporalLayer;
    uint8_t listIndex;
    uint8_t longTermReference;
    uint32_t frameNum;
    int32_t pictureOrderCount;
    uint32_t slot;
    uint16_t longTermPictureIndex;
    uint16_t longTermFrameIndex;
};

OpenRhiStruct(VideoH264DecodeReferenceDesc) {
    OpenRhi(VideoH264DecodeReferenceBits) flags;
    uint16_t frameNum;
    uint32_t slot;
    int32_t topFieldOrderCount;
    int32_t bottomFieldOrderCount;
};

OpenRhiStruct(VideoH264DecodePictureDesc) {
    OpenRhi(VideoH264DecodePictureBits) flags;
    uint8_t sequenceParameterSetId;
    uint8_t pictureParameterSetId;
    uint16_t frameNum;
    uint16_t idrPictureId;
    int32_t topFieldOrderCount;
    int32_t bottomFieldOrderCount;
    const uint32_t* sliceOffsets; // offsets to 4-byte Annex-B start codes, relative to the selected bitstream range start
    uint32_t sliceOffsetNum;
    bool hasReferenceSlot; // if false, VideoDecodeDesc::dstSlot is used
    uint32_t referenceSlot;
    OpenRhiOptional const OpenRhiPtr(VideoH264DecodeReferenceDesc) references;
    OpenRhiOptional uint32_t referenceNum;
};

OpenRhiStruct(VideoH265ReferenceDesc) {
    uint32_t slot;
    int32_t pictureOrderCount;
    uint8_t temporalLayer;
    OpenRhi(VideoFrameType) frameType;
    uint8_t longTerm;
    uint8_t listIndex;
};

OpenRhiStruct(VideoH265DecodePictureDesc) {
    OpenRhi(VideoH265DecodePictureBits) flags;
    uint8_t videoParameterSetId;
    uint8_t sequenceParameterSetId;
    uint8_t pictureParameterSetId;
    int32_t pictureOrderCount;
    uint8_t numDeltaPocsOfRefRpsIdx;
    uint16_t numBitsForShortTermRefPicSetInSlice;
    const uint32_t* sliceSegmentOffsets; // offsets to 4-byte Annex-B start codes, relative to the selected bitstream range start
    uint32_t sliceSegmentOffsetNum;
    OpenRhiOptional const OpenRhiPtr(VideoH265ReferenceDesc) references;
    OpenRhiOptional uint32_t referenceNum;
};

OpenRhiStruct(VideoEncodeRateControlDesc) {
    OpenRhi(VideoEncodeRateControlMode) mode;
    uint8_t qpI;
    uint8_t qpP;
    uint8_t qpB;
    OpenRhiOptional uint8_t qpMin;
    OpenRhiOptional uint8_t qpMax;
    uint32_t frameRateNumerator;
    uint32_t frameRateDenominator;
    uint64_t targetBitrate;             // CBR target bitrate, or VBR average bitrate
    OpenRhiOptional uint64_t maxBitrate;    // VBR peak bitrate; defaults to "targetBitrate"
    OpenRhiOptional uint64_t maxFrameBitSize;
    OpenRhiOptional uint32_t virtualBufferSizeMs;
    OpenRhiOptional uint32_t initialVirtualBufferSizeMs;
};

OpenRhiStruct(VideoEncodePictureDesc) {
    OpenRhi(VideoFrameType) frameType;
    uint8_t temporalLayer;
    uint16_t idrPictureId;
    uint32_t frameIndex;
    int32_t pictureOrderCount;
};

OpenRhiStruct(VideoH264EncodePictureDesc) {
    uint8_t sequenceParameterSetId;
    uint8_t pictureParameterSetId;
    OpenRhiOptional const OpenRhiPtr(VideoH264EncodeReferenceDesc) references;
    OpenRhiOptional uint32_t referenceNum;
};

OpenRhiStruct(VideoAV1ReferenceDesc) {
    OpenRhi(VideoAV1ReferenceName) name; // NONE describes an AV1 DPB slot that is not mapped to a current-frame reference name
    uint8_t refFrameIndex;
    OpenRhi(VideoFrameType) frameType;
    uint8_t orderHint;
    uint32_t frameId;
    uint32_t slot;
    OpenRhiOptional const uint8_t* savedOrderHints; // if provided, must include 8 entries
};

OpenRhiStruct(VideoAV1TileLayoutDesc) {
    uint8_t columnNum;
    uint8_t rowNum;
    uint8_t tileSizeBytesMinus1;
    uint8_t uniformSpacing;
    uint16_t contextUpdateTileId;
    OpenRhiOptional const uint16_t* miColumnStarts;             // if provided, must include "columnNum + 1" entries
    OpenRhiOptional const uint16_t* miRowStarts;                // if provided, must include "rowNum + 1" entries
    OpenRhiOptional const uint16_t* widthInSuperblocksMinus1;   // if provided, must include "columnNum" entries
    OpenRhiOptional const uint16_t* heightInSuperblocksMinus1;  // if provided, must include "rowNum" entries
};

OpenRhiStruct(VideoAV1QuantizationDesc) {
    int8_t deltaQYDc;
    int8_t deltaQUDc;
    int8_t deltaQUAc;
    int8_t deltaQVDc;
    int8_t deltaQVAc;
    uint8_t qmY;
    uint8_t qmU;
    uint8_t qmV;
    uint8_t usingQmatrix;
    uint8_t diffUvDelta;
};

OpenRhiStruct(VideoAV1LoopFilterDesc) {
    uint8_t level[4];
    uint8_t sharpness;
    uint8_t deltaEnabled;
    uint8_t deltaUpdate;
    uint8_t updateModeDelta;
    int8_t refDeltas[8];
    int8_t modeDeltas[2];
};

OpenRhiStruct(VideoAV1CdefDesc) {
    uint8_t yPrimaryStrength[8];
    uint8_t ySecondaryStrength[8];
    uint8_t uvPrimaryStrength[8];
    uint8_t uvSecondaryStrength[8];
};

OpenRhiStruct(VideoAV1SegmentationDesc) {
    uint8_t featureEnabled[8];
    int16_t featureData[8][8];
};

OpenRhiStruct(VideoAV1LoopRestorationDesc) {
    uint8_t frameRestorationType[3];
    uint8_t lrUnitShift;
    uint8_t lrUvShift;
};

OpenRhiStruct(VideoAV1GlobalMotionDesc) {
    uint8_t type[8];
    uint8_t invalid[8];
    int32_t params[8][6];
};

OpenRhiStruct(VideoAV1FilmGrainDesc) {
    uint8_t chromaScalingFromLuma;
    uint8_t overlapFlag;
    uint8_t clipToRestrictedRange;
    uint8_t updateGrain;
    uint8_t matrixCoeffIsIdentity;
    uint8_t grainScalingMinus8;
    uint8_t arCoeffLag;
    uint8_t arCoeffShiftMinus6;
    uint8_t grainScaleShift;
    uint8_t filmGrainParamsRefIdx;
    uint16_t grainSeed;
    uint8_t numYPoints;
    uint8_t pointYValue[14];
    uint8_t pointYScaling[14];
    uint8_t numCbPoints;
    uint8_t pointCbValue[10];
    uint8_t pointCbScaling[10];
    uint8_t numCrPoints;
    uint8_t pointCrValue[10];
    uint8_t pointCrScaling[10];
    uint8_t arCoeffsYPlus128[24];
    uint8_t arCoeffsCbPlus128[25];
    uint8_t arCoeffsCrPlus128[25];
    uint8_t cbMult;
    uint8_t cbLumaMult;
    int16_t cbOffset;
    uint8_t crMult;
    uint8_t crLumaMult;
    int16_t crOffset;
};

OpenRhiStruct(VideoAV1EncodePictureDesc) {
    uint32_t currentFrameId;
    uint8_t orderHint;
    uint8_t refreshFrameFlags;
    OpenRhi(VideoAV1ReferenceName) primaryReferenceName;
    OpenRhi(VideoAV1PictureBits) flags;
    uint16_t renderWidthMinus1;
    uint16_t renderHeightMinus1;
    uint8_t codedDenom;
    uint8_t interpolationFilter;
    uint8_t txMode;
    uint8_t baseQIndex;
    uint8_t cdefDampingMinus3;
    uint8_t cdefBits;
    uint8_t deltaQRes;
    uint8_t deltaLfRes;
    OpenRhiOptional const OpenRhiPtr(VideoAV1TileLayoutDesc) tileLayout;
    OpenRhiOptional const OpenRhiPtr(VideoAV1QuantizationDesc) quantization;
    OpenRhiOptional const OpenRhiPtr(VideoAV1LoopFilterDesc) loopFilter;
    OpenRhiOptional const OpenRhiPtr(VideoAV1CdefDesc) cdef;
    OpenRhiOptional const OpenRhiPtr(VideoAV1SegmentationDesc) segmentation;
    OpenRhiOptional const OpenRhiPtr(VideoAV1LoopRestorationDesc) loopRestoration;
    OpenRhiOptional const OpenRhiPtr(VideoAV1GlobalMotionDesc) globalMotion;
    OpenRhiOptional const uint8_t* orderHints; // if provided, must include 8 entries
    OpenRhiOptional const OpenRhiPtr(VideoAV1ReferenceDesc) references;
    OpenRhiOptional uint32_t referenceNum;
};

OpenRhiStruct(VideoAV1DecodeTileDesc) {
    uint32_t offset;
    uint32_t size;
    uint16_t row;
    uint16_t column;
    uint8_t anchorFrame;
};

OpenRhiStruct(VideoAV1DecodePictureDesc) {
    OpenRhi(VideoFrameType) frameType;
    uint8_t orderHint;
    uint8_t refreshFrameFlags;
    OpenRhi(VideoAV1ReferenceName) primaryReferenceName;
    uint32_t currentFrameId;
    uint32_t frameHeaderOffset;
    OpenRhi(VideoAV1PictureBits) flags;
    uint16_t renderWidthMinus1;
    uint16_t renderHeightMinus1;
    uint8_t baseQIndex;
    uint8_t superresDenom;
    uint8_t codedDenom;
    uint8_t interpolationFilter;
    uint8_t txMode;
    uint8_t cdefDampingMinus3;
    uint8_t cdefBits;
    uint8_t deltaQRes;
    uint8_t deltaLfRes;
    OpenRhiOptional const OpenRhiPtr(VideoAV1TileLayoutDesc) tileLayout;
    OpenRhiOptional const OpenRhiPtr(VideoAV1QuantizationDesc) quantization;
    OpenRhiOptional const OpenRhiPtr(VideoAV1LoopFilterDesc) loopFilter;
    OpenRhiOptional const OpenRhiPtr(VideoAV1CdefDesc) cdef;
    OpenRhiOptional const OpenRhiPtr(VideoAV1SegmentationDesc) segmentation;
    OpenRhiOptional const OpenRhiPtr(VideoAV1LoopRestorationDesc) loopRestoration;
    OpenRhiOptional const OpenRhiPtr(VideoAV1GlobalMotionDesc) globalMotion;
    OpenRhiOptional const OpenRhiPtr(VideoAV1FilmGrainDesc) filmGrain; // used with "APPLY_GRAIN"; TODO: currently unsupported for decode
    OpenRhiOptional const uint8_t* orderHints; // if provided, must include 8 entries
    const OpenRhiPtr(VideoAV1DecodeTileDesc) tiles;
    uint32_t tileNum;
    OpenRhiOptional const OpenRhiPtr(VideoAV1ReferenceDesc) references;
    OpenRhiOptional uint32_t referenceNum;
};

OpenRhiStruct(VideoDecodeDesc) {
    OpenRhiPtr(VideoSession) session;
    OpenRhiPtr(VideoSessionParameters) parameters;
    OpenRhi(VideoDecodeBitstreamRange) bitstream; // provide at least one source supported by "VideoCapabilities::decodeBitstreamSourceMask"
    OpenRhiPtr(VideoPicture) dstPicture;
    OpenRhiOptional OpenRhiPtr(VideoPicture) setupPicture; // reconstructed/DPB setup picture; required for distinct mode, may alias "dstPicture" in coincide mode
    OpenRhiOptional const OpenRhiPtr(VideoReference) references;
    OpenRhiOptional uint32_t referenceNum;
    uint32_t dstSlot; // default reconstructed picture slot; neutral H.264 may override it with "referenceSlot"; native DXVA "CurrPic" must use "dstSlot" and reference "PicEntry" indices must match "VideoReference::slot"
    OpenRhiOptional const OpenRhiPtr(VideoDecodeArgument) arguments; // native DXVA frame arguments; if omitted, the neutral codec picture description is used
    OpenRhiOptional uint32_t argumentNum;
    OpenRhiOptional const OpenRhiPtr(VideoH264DecodePictureDesc) h264PictureDesc; // neutral H.264 picture description; required if "argumentNum" is 0
    OpenRhiOptional const OpenRhiPtr(VideoH265DecodePictureDesc) h265PictureDesc; // neutral H.265 picture description; required if "argumentNum" is 0
    OpenRhiOptional const OpenRhiPtr(VideoAV1DecodePictureDesc) av1PictureDesc;   // neutral AV1 picture description; required if "argumentNum" is 0
};

OpenRhiStruct(VideoEncodeFeedback) {
    uint64_t errorFlags;
    uint64_t averageQp;
    uint64_t intraCodingUnitNum;
    uint64_t interCodingUnitNum;
    uint64_t skipCodingUnitNum;
    uint64_t averageMotionEstimationX;
    uint64_t averageMotionEstimationY;
    uint64_t encodedBitstreamWrittenBytes;
    uint64_t writtenSubregionNum;
    uint64_t encodedBitstreamOffset; // offset of valid encoded data relative to VideoEncodeDesc::dstBitstream.offset
};

OpenRhiStruct(VideoAV1EncodeDecodeInfoDesc) {
    const OpenRhiPtr(VideoEncodeFeedback) feedback;
    const OpenRhiPtr(VideoAV1SequenceDesc) sequence;
    OpenRhiOptional const uint8_t* encodedPayloadHeader; // first bytes of the encoded payload; returned bitstream ranges are bounded by "feedback"
    uint64_t encodedPayloadHeaderSize;
    OpenRhiOptional const OpenRhiPtr(VideoAV1ReferenceDesc) references; // previous AV1 DPB snapshot; required for inter frames
    OpenRhiOptional uint32_t referenceNum;
};

OpenRhiStruct(VideoAV1EncodeDecodeInfo) {
    uint64_t bitstreamOffset; // offset relative to VideoEncodeFeedback::encodedBitstreamOffset
    uint64_t bitstreamSize;
    OpenRhi(VideoAV1SequenceDesc) sequence;
    OpenRhi(VideoAV1DecodePictureDesc) picture;
    OpenRhi(VideoAV1DecodeTileDesc) tiles[64];
    OpenRhi(VideoAV1TileLayoutDesc) tileLayout;
    uint16_t miColumnStarts[65];
    uint16_t miRowStarts[65];
    uint16_t widthInSuperblocksMinus1[64];
    uint16_t heightInSuperblocksMinus1[64];
    OpenRhi(VideoAV1ReferenceDesc) references[8];
    OpenRhi(VideoAV1QuantizationDesc) quantization;
    OpenRhi(VideoAV1LoopFilterDesc) loopFilter;
    OpenRhi(VideoAV1CdefDesc) cdef;
    OpenRhi(VideoAV1SegmentationDesc) segmentation;
    OpenRhi(VideoAV1LoopRestorationDesc) loopRestoration;
    OpenRhi(VideoAV1GlobalMotionDesc) globalMotion;
};

OpenRhiStruct(VideoEncodeDesc) {
    OpenRhiPtr(VideoSession) session;
    OpenRhiPtr(VideoSessionParameters) parameters;
    OpenRhiPtr(VideoPicture) srcPicture;
    OpenRhi(VideoBitstreamRange) dstBitstream;
    uint64_t bitstreamMetadataSize; // bytes of codec metadata already written before the current frame payload
    OpenRhiOptional const OpenRhiPtr(VideoEncodePictureDesc) pictureDesc;
    OpenRhiOptional const OpenRhiPtr(VideoEncodeRateControlDesc) rateControlDesc;
    OpenRhi(VideoEncodeBits) flags;
    OpenRhiOptional OpenRhiPtr(VideoPicture) reconstructedPicture;
    OpenRhiOptional OpenRhiPtr(Buffer) metadata; // backend encode metadata; required if "VideoCapabilities::metadataSize" is non-zero
    uint64_t metadataOffset;
    OpenRhiOptional OpenRhiPtr(Buffer) resolvedMetadata; // backend metadata consumed by "GetVideoEncodeFeedback" after the required resolve operation; a pending range must not be reused
    uint64_t resolvedMetadataOffset;
    OpenRhiOptional const OpenRhiPtr(VideoReference) references;
    OpenRhiOptional uint32_t referenceNum;
    uint32_t reconstructedSlot;
    OpenRhiOptional const OpenRhiPtr(VideoH264EncodePictureDesc) h264PictureDesc;
    OpenRhiOptional const OpenRhiPtr(VideoAV1EncodePictureDesc) av1PictureDesc;
    OpenRhiOptional const OpenRhiPtr(VideoH265ReferenceDesc) h265ReferenceDescs;
};

// "ResetVideoSession" discards session state reserved while recording commands. Reset affected command buffers first
// Threadsafe: no
OpenRhiStruct(VideoInterface) {
    // Session
    // {
        OpenRhi(Result)     (OPENRHI_CALL *GetVideoCapabilities)            (const OpenRhiRef(Device) device, const OpenRhiRef(VideoSessionDesc) videoSessionDesc, OpenRhiOut OpenRhiRef(VideoCapabilities) videoCapabilities);
        OpenRhi(Result)     (OPENRHI_CALL *GetVideoAV1Capabilities)         (const OpenRhiRef(Device) device, const OpenRhiRef(VideoSessionDesc) videoSessionDesc, OpenRhiOut OpenRhiRef(VideoAV1Capabilities) videoAV1Capabilities);
        OpenRhi(Result)     (OPENRHI_CALL *CreateVideoSession)              (OpenRhiRef(Device) device, const OpenRhiRef(VideoSessionDesc) videoSessionDesc, OpenRhiOut OpenRhiRef(VideoSession*) videoSession);
        OpenRhi(Result)     (OPENRHI_CALL *CreateVideoSessionParameters)    (OpenRhiRef(Device) device, const OpenRhiRef(VideoSessionParametersDesc) videoSessionParametersDesc, OpenRhiOut OpenRhiRef(VideoSessionParameters*) videoSessionParameters);
        OpenRhi(Result)     (OPENRHI_CALL *CreateVideoPicture)              (OpenRhiRef(Device) device, const OpenRhiRef(VideoPictureDesc) videoPictureDesc, OpenRhiOut OpenRhiRef(VideoPicture*) videoPicture);
        void            (OPENRHI_CALL *DestroyVideoSession)             (OpenRhiPtr(VideoSession) videoSession);
        void            (OPENRHI_CALL *ResetVideoSession)               (OpenRhiRef(VideoSession) videoSession);
        void            (OPENRHI_CALL *DestroyVideoSessionParameters)   (OpenRhiPtr(VideoSessionParameters) videoSessionParameters);
        void            (OPENRHI_CALL *DestroyVideoPicture)             (OpenRhiPtr(VideoPicture) videoPicture);

        // Returns backend-specific states for explicit caller-recorded decode/encode picture barriers
        OpenRhi(Result)     (OPENRHI_CALL *GetVideoPictureState)            (const OpenRhiRef(VideoPicture) videoPicture, OpenRhi(VideoPictureRole) role, OpenRhiOut OpenRhiRef(VideoPictureState) state);

        // Serializes H.264 SPS/PPS or H.265 VPS/SPS/PPS parameter sets to Annex-B bytes. Pass "dst = nullptr" to query the required byte size in "writtenSize"
        OpenRhi(Result)     (OPENRHI_CALL *WriteVideoAnnexBParameterSets)   (OpenRhiRef(VideoAnnexBParameterSetsDesc) annexBParameterSetsDesc);

        // Serializes H.264 end-of-sequence/end-of-stream or H.265 EOS/EOB Annex-B NAL units. Pass "dst = nullptr" to query the required byte size in "writtenSize"
        OpenRhi(Result)     (OPENRHI_CALL *WriteVideoAnnexBEndOfStream)     (OpenRhiRef(VideoAnnexBEndOfStreamDesc) annexBEndOfStreamDesc);

        // Serializes AV1 temporal delimiter and sequence header OBUs. Pass "dst = nullptr" to query the required byte size in "writtenSize"
        OpenRhi(Result)     (OPENRHI_CALL *WriteVideoAV1ObuHeaders)         (OpenRhiRef(VideoAV1ObuHeadersDesc) av1ObuHeadersDesc);
    // }

    // Command buffer
    // {
        // Video decode/encode command buffers must be created from "QueueType::VIDEO_DECODE" or "QueueType::VIDEO_ENCODE" queues.
        // VK: video session initialization is implicit on first recorded use, so commands for the same session must be recorded in submit order
        void            (OPENRHI_CALL *CmdDecodeVideo)                  (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiRef(VideoDecodeDesc) videoDecodeDesc);
        void            (OPENRHI_CALL *CmdEncodeVideo)                  (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiRef(VideoEncodeDesc) videoEncodeDesc);

        // Required only if "VideoCapabilities::encodeFeedbackResolveRequired" is true. Uses the advertised queue and buffer state
        void            (OPENRHI_CALL *CmdResolveVideoEncodeFeedback)   (OpenRhiRef(CommandBuffer) commandBuffer, OpenRhiRef(VideoSession) videoSession, OpenRhiRef(Buffer) resolvedMetadata, uint64_t resolvedMetadataOffset);
        OpenRhi(Result)     (OPENRHI_CALL *GetVideoEncodeFeedback)          (OpenRhiRef(VideoSession) videoSession, OpenRhiRef(Buffer) resolvedMetadataReadback, uint64_t resolvedMetadataOffset, OpenRhiOut OpenRhiRef(VideoEncodeFeedback) feedback);
        OpenRhi(Result)     (OPENRHI_CALL *GetVideoAV1EncodeDecodeInfo)     (OpenRhiRef(VideoSession) videoSession, OpenRhiRef(Buffer) resolvedMetadataReadback, uint64_t resolvedMetadataOffset, const OpenRhiRef(VideoAV1EncodeDecodeInfoDesc) desc, OpenRhiOut OpenRhiRef(VideoAV1EncodeDecodeInfo) info);
    // }
};

OpenRhiNamespaceEnd
