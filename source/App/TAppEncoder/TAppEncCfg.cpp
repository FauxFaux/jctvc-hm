/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2014, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/** \file     TAppEncCfg.cpp
    \brief    Handle encoder configuration parameters
*/

#include <stdlib.h>
#include <cassert>
#include <cstring>
#include <string>
#include "TLibCommon/TComRom.h"
#include "TAppEncCfg.h"
#include "TAppCommon/program_options_lite.h"
#include "TLibEncoder/TEncRateCtrl.h"
#ifdef WIN32
#define strdup _strdup
#endif

#define MACRO_TO_STRING_HELPER(val) #val
#define MACRO_TO_STRING(val) MACRO_TO_STRING_HELPER(val)

using namespace std;
namespace po = df::program_options_lite;



enum ExtendedProfileName // this is used for determining profile strings, where multiple profiles map to a single profile idc with various constraint flag combinations
{
  NONE = 0,
  MAIN = 1,
  MAIN10 = 2,
  MAINSTILLPICTURE = 3,
  MAINREXT = 4,
  HIGHREXT = 30, // Placeholder profile for development
  // The following are RExt profiles, which would map to the MAINREXT profile idc.
  // The enumeration indicates the bit-depth constraint in the bottom 2 digits
  //                           the chroma format in the next digit
  //                           the intra constraint in the top digit
  MONOCHROME_12     = 1012,
  MONOCHROME_16     = 1016,
  MAIN_12           = 1112,
  MAIN_422_10       = 1210,
  MAIN_422_12       = 1212,
  MAIN_444          = 1308,
  MAIN_444_10       = 1310,
  MAIN_444_12       = 1312,
  MAIN_444_16       = 1316, // non-standard profile definition, used for development purposes
  MAIN_INTRA        = 2108,
  MAIN_10_INTRA     = 2110,
  MAIN_12_INTRA     = 2112,
  MAIN_422_10_INTRA = 2210,
  MAIN_422_12_INTRA = 2212,
  MAIN_444_INTRA    = 2308,
  MAIN_444_10_INTRA = 2310,
  MAIN_444_12_INTRA = 2312,
  MAIN_444_16_INTRA = 2316
};


//! \ingroup TAppEncoder
//! \{

// ====================================================================================================================
// Constructor / destructor / initialization / destroy
// ====================================================================================================================

TAppEncCfg::TAppEncCfg()
: m_pchInputFile()
, m_pchBitstreamFile()
, m_pchReconFile()
, m_inputColourSpaceConvert(IPCOLOURSPACE_UNCHANGED)
, m_snrInternalColourSpace(false)
, m_outputInternalColourSpace(false)
, m_pchdQPFile()
, m_pColumnWidth()
, m_pRowHeight()
, m_scalingListFile()
{
  m_aidQP = NULL;
  m_startOfCodedInterval = NULL;
  m_codedPivotValue = NULL;
  m_targetPivotValue = NULL;
  m_masteringDisplayPrimaries = NULL;
  m_masteringDisplayWhitePoint = NULL;
}

TAppEncCfg::~TAppEncCfg()
{
  if ( m_aidQP )
  {
    delete[] m_aidQP;
  }
  if ( m_startOfCodedInterval )
  {
    delete[] m_startOfCodedInterval;
    m_startOfCodedInterval = NULL;
  }
   if ( m_codedPivotValue )
  {
    delete[] m_codedPivotValue;
    m_codedPivotValue = NULL;
  }
  if ( m_targetPivotValue )
  {
    delete[] m_targetPivotValue;
    m_targetPivotValue = NULL;
  }

  if( m_masteringDisplayPrimaries )
  {
    delete[] m_masteringDisplayPrimaries;
    m_masteringDisplayPrimaries = NULL;
  }
    
  if( m_masteringDisplayPrimaries )
  {
    delete[] m_masteringDisplayWhitePoint;
    m_masteringDisplayWhitePoint = NULL;
  }
    
  free(m_pchInputFile);
  free(m_pchBitstreamFile);
  free(m_pchReconFile);
  free(m_pchdQPFile);
  free(m_pColumnWidth);
  free(m_pRowHeight);
  free(m_scalingListFile);
}

Void TAppEncCfg::create()
{
}

Void TAppEncCfg::destroy()
{
}

std::istringstream &operator>>(std::istringstream &in, GOPEntry &entry)     //input
{
  in>>entry.m_sliceType;
  in>>entry.m_POC;
  in>>entry.m_QPOffset;
  in>>entry.m_QPFactor;
  in>>entry.m_tcOffsetDiv2;
  in>>entry.m_betaOffsetDiv2;
  in>>entry.m_temporalId;
  in>>entry.m_numRefPicsActive;
  in>>entry.m_numRefPics;
  for ( Int i = 0; i < entry.m_numRefPics; i++ )
  {
    in>>entry.m_referencePics[i];
  }
  in>>entry.m_interRPSPrediction;
#if AUTO_INTER_RPS
  if (entry.m_interRPSPrediction==1)
  {
    in>>entry.m_deltaRPS;
    in>>entry.m_numRefIdc;
    for ( Int i = 0; i < entry.m_numRefIdc; i++ )
    {
      in>>entry.m_refIdc[i];
    }
  }
  else if (entry.m_interRPSPrediction==2)
  {
    in>>entry.m_deltaRPS;
  }
#else
  if (entry.m_interRPSPrediction)
  {
    in>>entry.m_deltaRPS;
    in>>entry.m_numRefIdc;
    for ( Int i = 0; i < entry.m_numRefIdc; i++ )
    {
      in>>entry.m_refIdc[i];
    }
  }
#endif
  return in;
}

Bool confirmPara(Bool bflag, const Char* message);

static inline ChromaFormat numberToChromaFormat(const Int val)
{
  switch (val)
  {
    case 400: return CHROMA_400; break;
    case 420: return CHROMA_420; break;
    case 422: return CHROMA_422; break;
    case 444: return CHROMA_444; break;
    default:  return NUM_CHROMA_FORMAT;
  }
}

static const struct MapStrToProfile
{
  const Char* str;
  Profile::Name value;
}
strToProfile[] =
{
  {"none",               Profile::NONE            },
  {"main",               Profile::MAIN            },
  {"main10",             Profile::MAIN10          },
  {"main-still-picture", Profile::MAINSTILLPICTURE},
  {"main-RExt",          Profile::MAINREXT        },
  {"high-RExt",          Profile::HIGHREXT        }
};

static const struct MapStrToExtendedProfile
{
  const Char* str;
  ExtendedProfileName value;
}
strToExtendedProfile[] =
{
    {"none",               NONE             },
    {"main",               MAIN             },
    {"main10",             MAIN10           },
    {"main-still-picture", MAINSTILLPICTURE },
    {"main-RExt",          MAINREXT         },
    {"high-RExt",          HIGHREXT         },
    {"monochrome12",       MONOCHROME_12    },
    {"monochrome16",       MONOCHROME_16    },
    {"main12",             MAIN_12          },
    {"main_422_10",        MAIN_422_10      },
    {"main_422_12",        MAIN_422_12      },
    {"main_444",           MAIN_444         },
    {"main_444_10",        MAIN_444_10      },
    {"main_444_12",        MAIN_444_12      },
    {"main_444_16",        MAIN_444_16      },
    {"main_intra",         MAIN_INTRA       },
    {"main_10_intra",      MAIN_10_INTRA    },
    {"main_12_intra",      MAIN_12_INTRA    },
    {"main_422_10_intra",  MAIN_422_10_INTRA},
    {"main_422_12_intra",  MAIN_422_12_INTRA},
    {"main_444_intra",     MAIN_444_INTRA   },
    {"main_444_10_intra",  MAIN_444_10_INTRA},
    {"main_444_12_intra",  MAIN_444_12_INTRA},
    {"main_444_16_intra",  MAIN_444_16_INTRA}
};

static const ExtendedProfileName validRExtProfileNames[2/* intraConstraintFlag*/][4/* bit depth constraint 8=0, 10=1, 12=2, 16=3*/][4/*chroma format*/]=
{
    {
        { NONE,          NONE,          NONE,              MAIN_444          }, // 8-bit  inter for 400, 420, 422 and 444
        { NONE,          NONE,          MAIN_422_10,       MAIN_444_10       }, // 10-bit inter for 400, 420, 422 and 444
        { MONOCHROME_12, MAIN_12,       MAIN_422_12,       MAIN_444_12       }, // 12-bit inter for 400, 420, 422 and 444
        { MONOCHROME_16, NONE,          NONE,              MAIN_444_16       }  // 16-bit inter for 400, 420, 422 and 444 (the latter is non standard used for development)
    },
    {
        { NONE,          MAIN_INTRA,    NONE,              MAIN_444_INTRA    }, // 8-bit  intra for 400, 420, 422 and 444
        { NONE,          MAIN_10_INTRA, MAIN_422_10_INTRA, MAIN_444_10_INTRA }, // 10-bit intra for 400, 420, 422 and 444
        { NONE,          MAIN_12_INTRA, MAIN_422_12_INTRA, MAIN_444_12_INTRA }, // 12-bit intra for 400, 420, 422 and 444
        { NONE,          NONE,          NONE,              MAIN_444_16_INTRA }  // 16-bit intra for 400, 420, 422 and 444
    }
};

static const struct MapStrToTier
{
  const Char* str;
  Level::Tier value;
}
strToTier[] =
{
  {"main", Level::MAIN},
  {"high", Level::HIGH},
};

static const struct MapStrToLevel
{
  const Char* str;
  Level::Name value;
}
strToLevel[] =
{
  {"none",Level::NONE},
  {"1",   Level::LEVEL1},
  {"2",   Level::LEVEL2},
  {"2.1", Level::LEVEL2_1},
  {"3",   Level::LEVEL3},
  {"3.1", Level::LEVEL3_1},
  {"4",   Level::LEVEL4},
  {"4.1", Level::LEVEL4_1},
  {"5",   Level::LEVEL5},
  {"5.1", Level::LEVEL5_1},
  {"5.2", Level::LEVEL5_2},
  {"6",   Level::LEVEL6},
  {"6.1", Level::LEVEL6_1},
  {"6.2", Level::LEVEL6_2},
  {"8.5", Level::LEVEL8_5},
};

static const struct MapStrToCostMode
{
  const Char* str;
  CostMode    value;
}
strToCostMode[] =
{
  {"lossy",                     COST_STANDARD_LOSSY},
  {"sequence_level_lossless",   COST_SEQUENCE_LEVEL_LOSSLESS},
  {"lossless",                  COST_LOSSLESS_CODING},
  {"mixed_lossless_lossy",      COST_MIXED_LOSSLESS_LOSSY_CODING}
};

template<typename T, typename P>
static std::string enumToString(P map[], unsigned long mapLen, const T val)
{
  for (Int i = 0; i < mapLen; i++)
  {
    if (val == map[i].value)
    {
      return map[i].str;
    }
  }
  return std::string();
}

template<typename T, typename P>
static istream& readStrToEnum(P map[], unsigned long mapLen, istream &in, T &val)
{
  string str;
  in >> str;

  for (Int i = 0; i < mapLen; i++)
  {
    if (str == map[i].str)
    {
      val = map[i].value;
      goto found;
    }
  }
  /* not found */
  in.setstate(ios::failbit);
found:
  return in;
}

//inline to prevent compiler warnings for "unused static function"
namespace Profile
{
  static inline istream& operator >> (istream &in, Name &profile)
  {
    return readStrToEnum(strToProfile, sizeof(strToProfile)/sizeof(*strToProfile), in, profile);
  }
}

static inline istream& operator >> (istream &in, ExtendedProfileName &profile)
{
  return readStrToEnum(strToExtendedProfile, sizeof(strToExtendedProfile)/sizeof(*strToExtendedProfile), in, profile);
}

namespace Level
{
  static inline istream& operator >> (istream &in, Tier &tier)
  {
    return readStrToEnum(strToTier, sizeof(strToTier)/sizeof(*strToTier), in, tier);
  }

  static inline istream& operator >> (istream &in, Name &level)
  {
    return readStrToEnum(strToLevel, sizeof(strToLevel)/sizeof(*strToLevel), in, level);
  }
}

static inline istream& operator >> (istream &in, CostMode &mode)
{
  return readStrToEnum(strToCostMode, sizeof(strToCostMode)/sizeof(*strToCostMode), in, mode);
}

static Void
automaticallySelectRExtProfile(const Bool bUsingGeneralRExtTools,
                               const Bool bUsingChromaQPAdjustment,
                               const Bool bUsingExtendedPrecision,
                               const Bool bIntraConstraintFlag,
                               UInt &bitDepthConstraint,
                               ChromaFormat &chromaFormatConstraint,
                               const Int  maxBitDepth,
                               const ChromaFormat chromaFormat)
{
  // Try to choose profile, according to table in Q1013.
  UInt trialBitDepthConstraint=maxBitDepth;
  if (trialBitDepthConstraint<8) trialBitDepthConstraint=8;
  else if (trialBitDepthConstraint==9 || trialBitDepthConstraint==11) trialBitDepthConstraint++;
  else if (trialBitDepthConstraint>12) trialBitDepthConstraint=16;

  // both format and bit depth constraints are unspecified
  if (bUsingExtendedPrecision || trialBitDepthConstraint==16)
  {
    bitDepthConstraint = 16;
    chromaFormatConstraint = (!bIntraConstraintFlag && chromaFormat==CHROMA_400) ? CHROMA_400 : CHROMA_444;
  }
  else if (bUsingGeneralRExtTools)
  {
    if (chromaFormat == CHROMA_400 && !bIntraConstraintFlag)
    {
      bitDepthConstraint = 16;
      chromaFormatConstraint = CHROMA_400;
    }
    else
    {
      bitDepthConstraint = trialBitDepthConstraint;
      chromaFormatConstraint = CHROMA_444;
    }
  }
  else if (chromaFormat == CHROMA_400)
  {
    if (bIntraConstraintFlag)
    {
      chromaFormatConstraint = CHROMA_420; // there is no intra 4:0:0 profile.
      bitDepthConstraint     = trialBitDepthConstraint;
    }
    else
    {
      chromaFormatConstraint = CHROMA_400;
      bitDepthConstraint     = 12;
    }
  }
  else
  {
    bitDepthConstraint = trialBitDepthConstraint;
    chromaFormatConstraint = chromaFormat;
    if (bUsingChromaQPAdjustment && chromaFormat == CHROMA_420) chromaFormatConstraint = CHROMA_422; // 4:2:0 cannot use the chroma qp tool.
    if (chromaFormatConstraint == CHROMA_422 && bitDepthConstraint == 8) bitDepthConstraint = 10; // there is no 8-bit 4:2:2 profile.
    if (chromaFormatConstraint == CHROMA_420 && !bIntraConstraintFlag) bitDepthConstraint = 12; // there is no 8 or 10-bit 4:2:0 inter RExt profile.
  }
}
// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/** \param  argc        number of arguments
    \param  argv        array of arguments
    \retval             true when success
 */
Bool TAppEncCfg::parseCfg( Int argc, Char* argv[] )
{
  Bool do_help = false;

  string cfg_InputFile;
  string cfg_BitstreamFile;
  string cfg_ReconFile;
  string cfg_dQPFile;
  string cfg_ColumnWidth;
  string cfg_RowHeight;
  string cfg_ScalingListFile;
  string cfg_startOfCodedInterval;
  string cfg_codedPivotValue;
  string cfg_targetPivotValue;
  string cfg_kneeSEIInputKneePointValue;
  string cfg_kneeSEIOutputKneePointValue;
  string cfg_DisplayPrimariesCode;
  string cfg_DisplayWhitePointCode;

  Int tmpChromaFormat;
  Int tmpInputChromaFormat;
  Int tmpConstraintChromaFormat;
  string inputColourSpaceConvert;
  ExtendedProfileName extendedProfile;
#if RExt__Q0044_SAO_OFFSET_BIT_SHIFT_ADAPTATION
  Int saoOffsetBitShift[MAX_NUM_CHANNEL_TYPE];
#endif

  po::Options opts;
  opts.addOptions()
  ("help",                                            do_help,                                          false, "this help text")
  ("c",    po::parseConfigFile, "configuration file name")

  // File, I/O and source parameters
  ("InputFile,i",                                     cfg_InputFile,                               string(""), "Original YUV input file name")
  ("BitstreamFile,b",                                 cfg_BitstreamFile,                           string(""), "Bitstream output file name")
  ("ReconFile,o",                                     cfg_ReconFile,                               string(""), "Reconstructed YUV output file name")
  ("SourceWidth,-wdt",                                m_iSourceWidth,                                       0, "Source picture width")
  ("SourceHeight,-hgt",                               m_iSourceHeight,                                      0, "Source picture height")
  ("InputBitDepth",                                   m_inputBitDepth[CHANNEL_TYPE_LUMA],                   8, "Bit-depth of input file")
  ("OutputBitDepth",                                  m_outputBitDepth[CHANNEL_TYPE_LUMA],                  0, "Bit-depth of output file (default:InternalBitDepth)")
  ("MSBExtendedBitDepth",                             m_MSBExtendedBitDepth[CHANNEL_TYPE_LUMA],             0, "bit depth of luma component after addition of MSBs of value 0 (used for synthesising High Dynamic Range source material). (default:InputBitDepth)")
  ("InternalBitDepth",                                m_internalBitDepth[CHANNEL_TYPE_LUMA],                0, "Bit-depth the codec operates at. (default:MSBExtendedBitDepth). If different to MSBExtendedBitDepth, source data will be converted")
  ("InputBitDepthC",                                  m_inputBitDepth[CHANNEL_TYPE_CHROMA],                 0, "As per InputBitDepth but for chroma component. (default:InputBitDepth)")
  ("OutputBitDepthC",                                 m_outputBitDepth[CHANNEL_TYPE_CHROMA],                0, "As per OutputBitDepth but for chroma component. (default:InternalBitDepthC)")
  ("MSBExtendedBitDepthC",                            m_MSBExtendedBitDepth[CHANNEL_TYPE_CHROMA],           0, "As per MSBExtendedBitDepth but for chroma component. (default:MSBExtendedBitDepth)")
  ("InternalBitDepthC",                               m_internalBitDepth[CHANNEL_TYPE_CHROMA],              0, "As per InternalBitDepth but for chroma component. (default:InternalBitDepth)")
  ("ExtendedPrecision",                               m_useExtendedPrecision,                           false, "Increased internal accuracies to support high bit depths (not valid in V1 profiles)")
  ("HighPrecisionPredictionWeighting",                m_useHighPrecisionPredictionWeighting,            false, "Use high precision option for weighted prediction (not valid in V1 profiles)")
  ("InputColourSpaceConvert",                         inputColourSpaceConvert,                     string(""), "Colour space conversion to apply to input video. Permitted values are (empty string=UNCHANGED) " + getListOfColourSpaceConverts(true))
  ("SNRInternalColourSpace",                          m_snrInternalColourSpace,                         false, "If true, then no colour space conversion is applied prior to SNR, otherwise inverse of input is applied.")
  ("OutputInternalColourSpace",                       m_outputInternalColourSpace,                      false, "If true, then no colour space conversion is applied for reconstructed video, otherwise inverse of input is applied.")
  ("InputChromaFormat",                               tmpInputChromaFormat,                               420, "InputChromaFormatIDC")
  ("MSEBasedSequencePSNR",                            m_printMSEBasedSequencePSNR,                      false, "0 (default) emit sequence PSNR only as a linear average of the frame PSNRs, 1 = also emit a sequence PSNR based on an average of the frame MSEs")
  ("PrintFrameMSE",                                   m_printFrameMSE,                                  false, "0 (default) emit only bit count and PSNRs for each frame, 1 = also emit MSE values")
  ("PrintSequenceMSE",                                m_printSequenceMSE,                               false, "0 (default) emit only bit rate and PSNRs for the whole sequence, 1 = also emit MSE values")
  ("ChromaFormatIDC,-cf",                             tmpChromaFormat,                                      0, "ChromaFormatIDC (400|420|422|444 or set 0 (default) for same as InputChromaFormat)")
  ("ConformanceMode",                                 m_conformanceMode,                                    0, "Window conformance mode (0: no window, 1:automatic padding, 2:padding, 3:conformance")
  ("HorizontalPadding,-pdx",                          m_aiPad[0],                                           0, "Horizontal source padding for conformance window mode 2")
  ("VerticalPadding,-pdy",                            m_aiPad[1],                                           0, "Vertical source padding for conformance window mode 2")
  ("ConfLeft",                                        m_confLeft,                                           0, "Left offset for window conformance mode 3")
  ("ConfRight",                                       m_confRight,                                          0, "Right offset for window conformance mode 3")
  ("ConfTop",                                         m_confTop,                                            0, "Top offset for window conformance mode 3")
  ("ConfBottom",                                      m_confBottom,                                         0, "Bottom offset for window conformance mode 3")
  ("FrameRate,-fr",                                   m_iFrameRate,                                         0, "Frame rate")
  ("FrameSkip,-fs",                                   m_FrameSkip,                                         0u, "Number of frames to skip at start of input YUV")
  ("FramesToBeEncoded,f",                             m_framesToBeEncoded,                                  0, "Number of frames to be encoded (default=all)")

  //Field coding parameters
  ("FieldCoding",                                     m_isField,                                        false, "Signals if it's a field based coding")
  ("TopFieldFirst, Tff",                              m_isTopFieldFirst,                                false, "In case of field based coding, signals whether if it's a top field first or not")

  // Profile and level
  ("Profile",                                         extendedProfile,                                   NONE, "Profile name to use for encoding. Use main (for main), main10 (for main10), main-still-picture, main-RExt (for Range Extensions profile), any of the RExt specific profile names, or none")
  ("Level",                                           m_level,                                    Level::NONE, "Level limit to be used, eg 5.1, or none")
  ("Tier",                                            m_levelTier,                                Level::MAIN, "Tier to use for interpretation of --Level (main or high only)")
  ("MaxBitDepthConstraint",                           m_bitDepthConstraint,                                0u, "Bit depth to use for profile-constraint for RExt profiles. 0=automatically choose based upon other parameters")
  ("MaxChromaFormatConstraint",                       tmpConstraintChromaFormat,                            0, "Chroma-format to use for the profile-constraint for RExt profiles. 0=automatically choose based upon other parameters")
  ("IntraConstraintFlag",                             m_intraConstraintFlag,                            false, "Value of general_intra_constraint_flag to use for RExt profiles (not used if an explicit RExt sub-profile is specified)")
  ("LowerBitRateConstraintFlag",                      m_lowerBitRateConstraintFlag,                      true, "Value of general_lower_bit_rate_constraint_flag to use for RExt profiles")

  ("ProgressiveSource",                               m_progressiveSourceFlag,                          false, "Indicate that source is progressive")
  ("InterlacedSource",                                m_interlacedSourceFlag,                           false, "Indicate that source is interlaced")
  ("NonPackedSource",                                 m_nonPackedConstraintFlag,                        false, "Indicate that source does not contain frame packing")
  ("FrameOnly",                                       m_frameOnlyConstraintFlag,                        false, "Indicate that the bitstream contains only frames")

  // Unit definition parameters
  ("MaxCUWidth",                                      m_uiMaxCUWidth,                                     64u)
  ("MaxCUHeight",                                     m_uiMaxCUHeight,                                    64u)
  // todo: remove defaults from MaxCUSize
  ("MaxCUSize,s",                                     m_uiMaxCUWidth,                                     64u, "Maximum CU size")
  ("MaxCUSize,s",                                     m_uiMaxCUHeight,                                    64u, "Maximum CU size")
  ("MaxPartitionDepth,h",                             m_uiMaxCUDepth,                                      4u, "CU depth")

  ("QuadtreeTULog2MaxSize",                           m_uiQuadtreeTULog2MaxSize,                           6u, "Maximum TU size in logarithm base 2")
  ("QuadtreeTULog2MinSize",                           m_uiQuadtreeTULog2MinSize,                           2u, "Minimum TU size in logarithm base 2")

  ("QuadtreeTUMaxDepthIntra",                         m_uiQuadtreeTUMaxDepthIntra,                         1u, "Depth of TU tree for intra CUs")
  ("QuadtreeTUMaxDepthInter",                         m_uiQuadtreeTUMaxDepthInter,                         2u, "Depth of TU tree for inter CUs")

  // Coding structure paramters
  ("IntraPeriod,-ip",                                 m_iIntraPeriod,                                      -1, "Intra period in frames, (-1: only first frame)")
#if ALLOW_RECOVERY_POINT_AS_RAP
  ("DecodingRefreshType,-dr",                         m_iDecodingRefreshType,                               0, "Intra refresh type (0:none 1:CRA 2:IDR 3:RecPointSEI)")
#else
  ("DecodingRefreshType,-dr",                         m_iDecodingRefreshType,                               0, "Intra refresh type (0:none 1:CRA 2:IDR)")
#endif
  ("GOPSize,g",                                       m_iGOPSize,                                           1, "GOP size of temporal structure")

  // motion search options
  ("FastSearch",                                      m_iFastSearch,                                        1, "0:Full search  1:Diamond  2:PMVFAST")
  ("SearchRange,-sr",                                 m_iSearchRange,                                      96, "Motion search range")
  ("BipredSearchRange",                               m_bipredSearchRange,                                  4, "Motion search range for bipred refinement")
  ("SingleComponentLoopInterSearch",                  m_singleComponentLoopInterSearch,                 false, "For inter residual estimation, loop over components once, testing all mode options for each")
  ("HadamardME",                                      m_bUseHADME,                                       true, "Hadamard ME for fractional-pel")
  ("ASR",                                             m_bUseASR,                                        false, "Adaptive motion search range")

  // Mode decision parameters
  ("LambdaModifier0,-LM0",                            m_adLambdaModifier[ 0 ],                  ( Double )1.0, "Lambda modifier for temporal layer 0")
  ("LambdaModifier1,-LM1",                            m_adLambdaModifier[ 1 ],                  ( Double )1.0, "Lambda modifier for temporal layer 1")
  ("LambdaModifier2,-LM2",                            m_adLambdaModifier[ 2 ],                  ( Double )1.0, "Lambda modifier for temporal layer 2")
  ("LambdaModifier3,-LM3",                            m_adLambdaModifier[ 3 ],                  ( Double )1.0, "Lambda modifier for temporal layer 3")
  ("LambdaModifier4,-LM4",                            m_adLambdaModifier[ 4 ],                  ( Double )1.0, "Lambda modifier for temporal layer 4")
  ("LambdaModifier5,-LM5",                            m_adLambdaModifier[ 5 ],                  ( Double )1.0, "Lambda modifier for temporal layer 5")
  ("LambdaModifier6,-LM6",                            m_adLambdaModifier[ 6 ],                  ( Double )1.0, "Lambda modifier for temporal layer 6")

  /* Quantization parameters */
  ("QP,q",                                            m_fQP,                                             30.0, "Qp value, if value is float, QP is switched once during encoding")
  ("DeltaQpRD,-dqr",                                  m_uiDeltaQpRD,                                       0u, "max dQp offset for slice")
  ("MaxDeltaQP,d",                                    m_iMaxDeltaQP,                                        0, "max dQp offset for block")
  ("MaxCuDQPDepth,-dqd",                              m_iMaxCuDQPDepth,                                     0, "max depth for a minimum CuDQP")
  ("MaxCUChromaQpAdjustmentDepth",                    m_maxCUChromaQpAdjustmentDepth,                      -1, "Maximum depth for CU chroma Qp adjustment - set less than 0 to disable")

  ("CbQpOffset,-cbqpofs",                             m_cbQpOffset,                                         0, "Chroma Cb QP Offset")
  ("CrQpOffset,-crqpofs",                             m_crQpOffset,                                         0, "Chroma Cr QP Offset")

#if ADAPTIVE_QP_SELECTION
  ("AdaptiveQpSelection,-aqps",                       m_bUseAdaptQpSelect,                              false, "AdaptiveQpSelection")
#endif

  ("AdaptiveQP,-aq",                                  m_bUseAdaptiveQP,                                 false, "QP adaptation based on a psycho-visual model")
  ("MaxQPAdaptationRange,-aqr",                       m_iQPAdaptationRange,                                 6, "QP adaptation range")
  ("dQPFile,m",                                       cfg_dQPFile,                                 string(""), "dQP file name")
  ("RDOQ",                                            m_useRDOQ,                                         true)
  ("RDOQTS",                                          m_useRDOQTS,                                       true)
  ("RDpenalty",                                       m_rdPenalty,                                          0,  "RD-penalty for 32x32 TU for intra in non-intra slices. 0:disabled  1:RD-penalty  2:maximum RD-penalty")

  // Deblocking filter parameters
  ("LoopFilterDisable",                               m_bLoopFilterDisable,                             false)
  ("LoopFilterOffsetInPPS",                           m_loopFilterOffsetInPPS,                          false)
  ("LoopFilterBetaOffset_div2",                       m_loopFilterBetaOffsetDiv2,                           0)
  ("LoopFilterTcOffset_div2",                         m_loopFilterTcOffsetDiv2,                             0)
  ("DeblockingFilterControlPresent",                  m_DeblockingFilterControlPresent,                 false)
  ("DeblockingFilterMetric",                          m_DeblockingFilterMetric,                         false)

  // Coding tools
  ("AMP",                                             m_enableAMP,                                       true, "Enable asymmetric motion partitions")
  ("CrossComponentPrediction",                        m_useCrossComponentPrediction,                    false, "Enable the use of cross-component prediction (not valid in V1 profiles)")
  ("ReconBasedCrossCPredictionEstimate",              m_reconBasedCrossCPredictionEstimate,             false, "When determining the alpha value for cross-component prediction, use the decoded residual rather than the pre-transform encoder-side residual")
#if RExt__Q0044_SAO_OFFSET_BIT_SHIFT_ADAPTATION
  ("SaoLumaOffsetBitShift",                           saoOffsetBitShift[CHANNEL_TYPE_LUMA],                 0, "Specify the luma SAO bit-shift. If negative, automatically calculate a suitable value based upon bit depth and initial QP")
  ("SaoChromaOffsetBitShift",                         saoOffsetBitShift[CHANNEL_TYPE_CHROMA],               0, "Specify the chroma SAO bit-shift. If negative, automatically calculate a suitable value based upon bit depth and initial QP")
#else
  ("SaoLumaOffsetBitShift",                           m_saoOffsetBitShift[CHANNEL_TYPE_LUMA],              0u)
  ("SaoChromaOffsetBitShift",                         m_saoOffsetBitShift[CHANNEL_TYPE_CHROMA],            0u)
#endif
  ("TransformSkip",                                   m_useTransformSkip,                               false, "Intra transform skipping")
  ("TransformSkipFast",                               m_useTransformSkipFast,                           false, "Fast intra transform skipping")
  ("TransformSkipLog2MaxSize",                        m_transformSkipLog2MaxSize,                          2U, "Specify transform-skip maximum size. Minimum 2. (not valid in V1 profiles)")
  ("ImplicitResidualDPCM",                            m_useResidualDPCM[RDPCM_SIGNAL_IMPLICIT],         false, "Enable implicitly signalled residual DPCM for intra (also known as sample-adaptive intra predict) (not valid in V1 profiles)")
  ("ExplicitResidualDPCM",                            m_useResidualDPCM[RDPCM_SIGNAL_EXPLICIT],         false, "Enable explicitly signalled residual DPCM for inter (not valid in V1 profiles)")
  ("ResidualRotation",                                m_useResidualRotation,                            false, "Enable rotation of transform-skipped and transquant-bypassed TUs through 180 degrees prior to entropy coding (not valid in V1 profiles)")
  ("SingleSignificanceMapContext",                    m_useSingleSignificanceMapContext,                false, "Enable, for transform-skipped and transquant-bypassed TUs, the selection of a single significance map context variable for all coefficients (not valid in V1 profiles)")
  ("GolombRiceParameterAdaptation",                   m_useGolombRiceParameterAdaptation,               false, "Enable the adaptation of the Golomb-Rice parameter over the course of each slice")
  ("AlignCABACBeforeBypass",                          m_alignCABACBeforeBypass,                         false, "Align the CABAC engine to a defined fraction of a bit prior to coding bypass data. Must be 1 in high bit rate profile, 0 otherwise" )
  ("SAO",                                             m_bUseSAO,                                         true, "Enable Sample Adaptive Offset")
  ("MaxNumOffsetsPerPic",                             m_maxNumOffsetsPerPic,                             2048, "Max number of SAO offset per picture (Default: 2048)")
  ("SAOLcuBoundary",                                  m_saoLcuBoundary,                                 false, "0: right/bottom LCU boundary areas skipped from SAO parameter estimation, 1: non-deblocked pixels are used for those areas")
  ("SliceMode",                                       m_sliceMode,                                          0, "0: Disable all Recon slice limits, 1: Enforce max # of LCUs, 2: Enforce max # of bytes, 3:specify tiles per dependent slice")
  ("SliceArgument",                                   m_sliceArgument,                                      0, "Depending on SliceMode being:"
                                                                                                               "\t1: max number of CTUs per slice"
                                                                                                               "\t2: max number of bytes per slice"
                                                                                                               "\t3: max number of tiles per slice")
  ("SliceSegmentMode",                                m_sliceSegmentMode,                                   0, "0: Disable all slice segment limits, 1: Enforce max # of LCUs, 2: Enforce max # of bytes, 3:specify tiles per dependent slice")
  ("SliceSegmentArgument",                            m_sliceSegmentArgument,                               0, "Depending on SliceSegmentMode being:"
                                                                                                               "\t1: max number of CTUs per slice segment"
                                                                                                               "\t2: max number of bytes per slice segment"
                                                                                                               "\t3: max number of tiles per slice segment")
  ("LFCrossSliceBoundaryFlag",                        m_bLFCrossSliceBoundaryFlag,                       true)

  ("ConstrainedIntraPred",                            m_bUseConstrainedIntraPred,                       false, "Constrained Intra Prediction")
  ("PCMEnabledFlag",                                  m_usePCM,                                         false)
  ("PCMLog2MaxSize",                                  m_pcmLog2MaxSize,                                    5u)
  ("PCMLog2MinSize",                                  m_uiPCMLog2MinSize,                                  3u)

  ("PCMInputBitDepthFlag",                            m_bPCMInputBitDepthFlag,                           true)
  ("PCMFilterDisableFlag",                            m_bPCMFilterDisableFlag,                          false)
  ("IntraReferenceSmoothing",                         m_enableIntraReferenceSmoothing,                   true, "0: Disable use of intra reference smoothing. 1: Enable use of intra reference smoothing (not valid in V1 profiles)")
  ("WeightedPredP,-wpP",                              m_useWeightedPred,                                false, "Use weighted prediction in P slices")
  ("WeightedPredB,-wpB",                              m_useWeightedBiPred,                              false, "Use weighted (bidirectional) prediction in B slices")
  ("Log2ParallelMergeLevel",                          m_log2ParallelMergeLevel,                            2u, "Parallel merge estimation region")
  ("UniformSpacingIdc",                               m_iUniformSpacingIdr,                                 0, "Indicates if the column and row boundaries are distributed uniformly")
  ("NumTileColumnsMinus1",                            m_iNumColumnsMinus1,                                  0, "Number of columns in a picture minus 1")
  ("ColumnWidthArray",                                cfg_ColumnWidth,                             string(""), "Array containing ColumnWidth values in units of LCU")
  ("NumTileRowsMinus1",                               m_iNumRowsMinus1,                                     0, "Number of rows in a picture minus 1")
  ("RowHeightArray",                                  cfg_RowHeight,                               string(""), "Array containing RowHeight values in units of LCU")
  ("LFCrossTileBoundaryFlag",                         m_bLFCrossTileBoundaryFlag,                        true, "1: cross-tile-boundary loop filtering. 0:non-cross-tile-boundary loop filtering")
  ("WaveFrontSynchro",                                m_iWaveFrontSynchro,                                  0, "0: no synchro; 1 synchro with TR; 2 TRR etc")
  ("ScalingList",                                     m_useScalingListId,                                   0, "0: no scaling list, 1: default scaling lists, 2: scaling lists specified in ScalingListFile")
  ("ScalingListFile",                                 cfg_ScalingListFile,                         string(""), "Scaling list file name")
  ("SignHideFlag,-SBH",                               m_signHideFlag,                                       1)
  ("MaxNumMergeCand",                                 m_maxNumMergeCand,                                   5u, "Maximum number of merge candidates")
  /* Misc. */
  ("SEIDecodedPictureHash",                           m_decodedPictureHashSEIEnabled,                       0, "Control generation of decode picture hash SEI messages\n"
                                                                                                               "\t3: checksum\n"
                                                                                                               "\t2: CRC\n"
                                                                                                               "\t1: use MD5\n"
                                                                                                               "\t0: disable")
  ("SEIpictureDigest",                                m_decodedPictureHashSEIEnabled,                       0, "deprecated alias for SEIDecodedPictureHash")
  ("TMVPMode",                                        m_TMVPModeId,                                         1, "TMVP mode 0: TMVP disable for all slices. 1: TMVP enable for all slices (default) 2: TMVP enable for certain slices only")
  ("FEN",                                             m_bUseFastEnc,                                    false, "fast encoder setting")
  ("ECU",                                             m_bUseEarlyCU,                                    false, "Early CU setting")
  ("FDM",                                             m_useFastDecisionForMerge,                         true, "Fast decision for Merge RD Cost")
  ("CFM",                                             m_bUseCbfFastMode,                                false, "Cbf fast mode setting")
  ("ESD",                                             m_useEarlySkipDetection,                          false, "Early SKIP detection setting")
  ( "RateControl",                                    m_RCEnableRateControl,                            false, "Rate control: enable rate control" )
  ( "TargetBitrate",                                  m_RCTargetBitrate,                                    0, "Rate control: target bit-rate" )
  ( "KeepHierarchicalBit",                            m_RCKeepHierarchicalBit,                              0, "Rate control: 0: equal bit allocation; 1: fixed ratio bit allocation; 2: adaptive ratio bit allocation" )
  ( "LCULevelRateControl",                            m_RCLCULevelRC,                                    true, "Rate control: true: LCU level RC; false: picture level RC" )
  ( "RCLCUSeparateModel",                             m_RCUseLCUSeparateModel,                           true, "Rate control: use LCU level separate R-lambda model" )
  ( "InitialQP",                                      m_RCInitialQP,                                        0, "Rate control: initial QP" )
  ( "RCForceIntraQP",                                 m_RCForceIntraQP,                                 false, "Rate control: force intra QP to be equal to initial QP" )

  ("TransquantBypassEnableFlag",                      m_TransquantBypassEnableFlag,                     false, "transquant_bypass_enable_flag indicator in PPS")
  ("CUTransquantBypassFlagForce",                     m_CUTransquantBypassFlagForce,                    false, "Force transquant bypass mode, when transquant_bypass_enable_flag is enabled")
  ("CostMode",                                        m_costMode,                         COST_STANDARD_LOSSY, "Use alternative cost functions: choose between 'lossy', 'sequence_level_lossless', 'lossless' (which forces QP to " MACRO_TO_STRING(RExt__LOSSLESS_AND_MIXED_LOSSLESS_RD_COST_TEST_QP) ") and 'mixed_lossless_lossy' (which used QP'=" MACRO_TO_STRING(RExt__LOSSLESS_AND_MIXED_LOSSLESS_RD_COST_TEST_QP_PRIME) " for pre-estimates of transquant-bypass blocks).")
  ("RecalculateQPAccordingToLambda",                  m_recalculateQPAccordingToLambda,                 false, "Recalculate QP values according to lambda values. Do not suggest to be enabled in all intra case")
  ("StrongIntraSmoothing,-sis",                       m_useStrongIntraSmoothing,                         true, "Enable strong intra smoothing for 32x32 blocks")
  ("SEIActiveParameterSets",                          m_activeParameterSetsSEIEnabled,                      0, "Enable generation of active parameter sets SEI messages")
  ("VuiParametersPresent,-vui",                       m_vuiParametersPresentFlag,                       false, "Enable generation of vui_parameters()")
  ("AspectRatioInfoPresent",                          m_aspectRatioInfoPresentFlag,                     false, "Signals whether aspect_ratio_idc is present")
  ("AspectRatioIdc",                                  m_aspectRatioIdc,                                     0, "aspect_ratio_idc")
  ("SarWidth",                                        m_sarWidth,                                           0, "horizontal size of the sample aspect ratio")
  ("SarHeight",                                       m_sarHeight,                                          0, "vertical size of the sample aspect ratio")
  ("OverscanInfoPresent",                             m_overscanInfoPresentFlag,                        false, "Indicates whether conformant decoded pictures are suitable for display using overscan\n")
  ("OverscanAppropriate",                             m_overscanAppropriateFlag,                        false, "Indicates whether conformant decoded pictures are suitable for display using overscan\n")
  ("VideoSignalTypePresent",                          m_videoSignalTypePresentFlag,                     false, "Signals whether video_format, video_full_range_flag, and colour_description_present_flag are present")
  ("VideoFormat",                                     m_videoFormat,                                        5, "Indicates representation of pictures")
  ("VideoFullRange",                                  m_videoFullRangeFlag,                             false, "Indicates the black level and range of luma and chroma signals")
  ("ColourDescriptionPresent",                        m_colourDescriptionPresentFlag,                   false, "Signals whether colour_primaries, transfer_characteristics and matrix_coefficients are present")
  ("ColourPrimaries",                                 m_colourPrimaries,                                    2, "Indicates chromaticity coordinates of the source primaries")
  ("TransferCharateristics",                          m_transferCharacteristics,                            2, "Indicates the opto-electronic transfer characteristics of the source")
  ("MatrixCoefficients",                              m_matrixCoefficients,                                 2, "Describes the matrix coefficients used in deriving luma and chroma from RGB primaries")
  ("ChromaLocInfoPresent",                            m_chromaLocInfoPresentFlag,                       false, "Signals whether chroma_sample_loc_type_top_field and chroma_sample_loc_type_bottom_field are present")
  ("ChromaSampleLocTypeTopField",                     m_chromaSampleLocTypeTopField,                        0, "Specifies the location of chroma samples for top field")
  ("ChromaSampleLocTypeBottomField",                  m_chromaSampleLocTypeBottomField,                     0, "Specifies the location of chroma samples for bottom field")
  ("NeutralChromaIndication",                         m_neutralChromaIndicationFlag,                    false, "Indicates that the value of all decoded chroma samples is equal to 1<<(BitDepthCr-1)")
  ("DefaultDisplayWindowFlag",                        m_defaultDisplayWindowFlag,                       false, "Indicates the presence of the Default Window parameters")
  ("DefDispWinLeftOffset",                            m_defDispWinLeftOffset,                               0, "Specifies the left offset of the default display window from the conformance window")
  ("DefDispWinRightOffset",                           m_defDispWinRightOffset,                              0, "Specifies the right offset of the default display window from the conformance window")
  ("DefDispWinTopOffset",                             m_defDispWinTopOffset,                                0, "Specifies the top offset of the default display window from the conformance window")
  ("DefDispWinBottomOffset",                          m_defDispWinBottomOffset,                             0, "Specifies the bottom offset of the default display window from the conformance window")
  ("FrameFieldInfoPresentFlag",                       m_frameFieldInfoPresentFlag,                      false, "Indicates that pic_struct and field coding related values are present in picture timing SEI messages")
  ("PocProportionalToTimingFlag",                     m_pocProportionalToTimingFlag,                    false, "Indicates that the POC value is proportional to the output time w.r.t. first picture in CVS")
  ("NumTicksPocDiffOneMinus1",                        m_numTicksPocDiffOneMinus1,                           0, "Number of ticks minus 1 that for a POC difference of one")
  ("BitstreamRestriction",                            m_bitstreamRestrictionFlag,                       false, "Signals whether bitstream restriction parameters are present")
  ("TilesFixedStructure",                             m_tilesFixedStructureFlag,                        false, "Indicates that each active picture parameter set has the same values of the syntax elements related to tiles")
  ("MotionVectorsOverPicBoundaries",                  m_motionVectorsOverPicBoundariesFlag,             false, "Indicates that no samples outside the picture boundaries are used for inter prediction")
  ("MaxBytesPerPicDenom",                             m_maxBytesPerPicDenom,                                2, "Indicates a number of bytes not exceeded by the sum of the sizes of the VCL NAL units associated with any coded picture")
  ("MaxBitsPerMinCuDenom",                            m_maxBitsPerMinCuDenom,                               1, "Indicates an upper bound for the number of bits of coding_unit() data")
  ("Log2MaxMvLengthHorizontal",                       m_log2MaxMvLengthHorizontal,                         15, "Indicate the maximum absolute value of a decoded horizontal MV component in quarter-pel luma units")
  ("Log2MaxMvLengthVertical",                         m_log2MaxMvLengthVertical,                           15, "Indicate the maximum absolute value of a decoded vertical MV component in quarter-pel luma units")
  ("SEIRecoveryPoint",                                m_recoveryPointSEIEnabled,                            0, "Control generation of recovery point SEI messages")
  ("SEIBufferingPeriod",                              m_bufferingPeriodSEIEnabled,                          0, "Control generation of buffering period SEI messages")
  ("SEIPictureTiming",                                m_pictureTimingSEIEnabled,                            0, "Control generation of picture timing SEI messages")
  ("SEIToneMappingInfo",                              m_toneMappingInfoSEIEnabled,                      false, "Control generation of Tone Mapping SEI messages")
  ("SEIToneMapId",                                    m_toneMapId,                                          0, "Specifies Id of Tone Mapping SEI message for a given session")
  ("SEIToneMapCancelFlag",                            m_toneMapCancelFlag,                              false, "Indicates that Tone Mapping SEI message cancels the persistence or follows")
  ("SEIToneMapPersistenceFlag",                       m_toneMapPersistenceFlag,                          true, "Specifies the persistence of the Tone Mapping SEI message")
  ("SEIToneMapCodedDataBitDepth",                     m_toneMapCodedDataBitDepth,                           8, "Specifies Coded Data BitDepth of Tone Mapping SEI messages")
  ("SEIToneMapTargetBitDepth",                        m_toneMapTargetBitDepth,                              8, "Specifies Output BitDepth of Tone mapping function")
  ("SEIToneMapModelId",                               m_toneMapModelId,                                     0, "Specifies Model utilized for mapping coded data into target_bit_depth range\n"
                                                                                                               "\t0:  linear mapping with clipping\n"
                                                                                                               "\t1:  sigmoidal mapping\n"
                                                                                                               "\t2:  user-defined table mapping\n"
                                                                                                               "\t3:  piece-wise linear mapping\n"
                                                                                                               "\t4:  luminance dynamic range information ")
  ("SEIToneMapMinValue",                              m_toneMapMinValue,                                    0, "Specifies the minimum value in mode 0")
  ("SEIToneMapMaxValue",                              m_toneMapMaxValue,                                 1023, "Specifies the maximum value in mode 0")
  ("SEIToneMapSigmoidMidpoint",                       m_sigmoidMidpoint,                                  512, "Specifies the centre point in mode 1")
  ("SEIToneMapSigmoidWidth",                          m_sigmoidWidth,                                     960, "Specifies the distance between 5% and 95% values of the target_bit_depth in mode 1")
  ("SEIToneMapStartOfCodedInterval",                  cfg_startOfCodedInterval,                    string(""), "Array of user-defined mapping table")
  ("SEIToneMapNumPivots",                             m_numPivots,                                          0, "Specifies the number of pivot points in mode 3")
  ("SEIToneMapCodedPivotValue",                       cfg_codedPivotValue,                         string(""), "Array of pivot point")
  ("SEIToneMapTargetPivotValue",                      cfg_targetPivotValue,                        string(""), "Array of pivot point")
  ("SEIToneMapCameraIsoSpeedIdc",                     m_cameraIsoSpeedIdc,                                  0, "Indicates the camera ISO speed for daylight illumination")
  ("SEIToneMapCameraIsoSpeedValue",                   m_cameraIsoSpeedValue,                              400, "Specifies the camera ISO speed for daylight illumination of Extended_ISO")
  ("SEIToneMapExposureIndexIdc",                      m_exposureIndexIdc,                                   0, "Indicates the exposure index setting of the camera")
  ("SEIToneMapExposureIndexValue",                    m_exposureIndexValue,                               400, "Specifies the exposure index setting of the camera of Extended_ISO")
  ("SEIToneMapExposureCompensationValueSignFlag",     m_exposureCompensationValueSignFlag,                  0, "Specifies the sign of ExposureCompensationValue")
  ("SEIToneMapExposureCompensationValueNumerator",    m_exposureCompensationValueNumerator,                 0, "Specifies the numerator of ExposureCompensationValue")
  ("SEIToneMapExposureCompensationValueDenomIdc",     m_exposureCompensationValueDenomIdc,                  2, "Specifies the denominator of ExposureCompensationValue")
  ("SEIToneMapRefScreenLuminanceWhite",               m_refScreenLuminanceWhite,                          350, "Specifies reference screen brightness setting in units of candela per square metre")
  ("SEIToneMapExtendedRangeWhiteLevel",               m_extendedRangeWhiteLevel,                          800, "Indicates the luminance dynamic range")
  ("SEIToneMapNominalBlackLevelLumaCodeValue",        m_nominalBlackLevelLumaCodeValue,                    16, "Specifies luma sample value of the nominal black level assigned decoded pictures")
  ("SEIToneMapNominalWhiteLevelLumaCodeValue",        m_nominalWhiteLevelLumaCodeValue,                   235, "Specifies luma sample value of the nominal white level assigned decoded pictures")
  ("SEIToneMapExtendedWhiteLevelLumaCodeValue",       m_extendedWhiteLevelLumaCodeValue,                  300, "Specifies luma sample value of the extended dynamic range assigned decoded pictures")
  ("SEIChromaSamplingFilterHint",                     m_chromaSamplingFilterSEIenabled,                 false, "Control generation of the chroma sampling filter hint SEI message")
  ("SEIChromaSamplingHorizontalFilterType",           m_chromaSamplingHorFilterIdc,                         2, "Defines the Index of the chroma sampling horizontal filter\n"
                                                                                                               "\t0: unspecified  - Chroma filter is unknown or is determined by the application"
                                                                                                               "\t1: User-defined - Filter coefficients are specified in the chroma sampling filter hint SEI message"
                                                                                                               "\t2: Standards-defined - ITU-T Rec. T.800 | ISO/IEC15444-1, 5/3 filter")
  ("SEIChromaSamplingVerticalFilterType",             m_chromaSamplingVerFilterIdc,                         2, "Defines the Index of the chroma sampling vertical filter\n"
                                                                                                               "\t0: unspecified  - Chroma filter is unknown or is determined by the application"
                                                                                                               "\t1: User-defined - Filter coefficients are specified in the chroma sampling filter hint SEI message"
                                                                                                               "\t2: Standards-defined - ITU-T Rec. T.800 | ISO/IEC15444-1, 5/3 filter")
  ("SEIFramePacking",                                 m_framePackingSEIEnabled,                             0, "Control generation of frame packing SEI messages")
  ("SEIFramePackingType",                             m_framePackingSEIType,                                0, "Define frame packing arrangement\n"
                                                                                                               "\t0: checkerboard - pixels alternatively represent either frames\n"
                                                                                                               "\t1: column alternation - frames are interlaced by column\n"
                                                                                                               "\t2: row alternation - frames are interlaced by row\n"
                                                                                                               "\t3: side by side - frames are displayed horizontally\n"
                                                                                                               "\t4: top bottom - frames are displayed vertically\n"
                                                                                                               "\t5: frame alternation - one frame is alternated with the other")
  ("SEIFramePackingId",                               m_framePackingSEIId,                                  0, "Id of frame packing SEI message for a given session")
  ("SEIFramePackingQuincunx",                         m_framePackingSEIQuincunx,                            0, "Indicate the presence of a Quincunx type video frame")
  ("SEIFramePackingInterpretation",                   m_framePackingSEIInterpretation,                      0, "Indicate the interpretation of the frame pair\n"
                                                                                                               "\t0: unspecified\n"
                                                                                                               "\t1: stereo pair, frame0 represents left view\n"
                                                                                                               "\t2: stereo pair, frame0 represents right view")
  ("SEIDisplayOrientation",                           m_displayOrientationSEIAngle,                         0, "Control generation of display orientation SEI messages\n"
                                                                                                               "\tN: 0 < N < (2^16 - 1) enable display orientation SEI message with anticlockwise_rotation = N and display_orientation_repetition_period = 1\n"
                                                                                                               "\t0: disable")
  ("SEITemporalLevel0Index",                          m_temporalLevel0IndexSEIEnabled,                      0, "Control generation of temporal level 0 index SEI messages")
  ("SEIGradualDecodingRefreshInfo",                   m_gradualDecodingRefreshInfoEnabled,                  0, "Control generation of gradual decoding refresh information SEI message")
  ("SEINoDisplay",                                    m_noDisplaySEITLayer,                                 0, "Control generation of no display SEI message\n"
                                                                                                               "\tN: 0 < N enable no display SEI message for temporal layer N or higher\n"
                                                                                                               "\t0: disable")
  ("SEIDecodingUnitInfo",                             m_decodingUnitInfoSEIEnabled,                         0, "Control generation of decoding unit information SEI message.")
  ("SEISOPDescription",                               m_SOPDescriptionSEIEnabled,                           0, "Control generation of SOP description SEI messages")
  ("SEIScalableNesting",                              m_scalableNestingSEIEnabled,                          0, "Control generation of scalable nesting SEI messages")
  ("SEITempMotionConstrainedTileSets",                m_tmctsSEIEnabled,                                false, "Control generation of temporal motion constrained tile sets SEI message")
  ("SEITimeCode",                                     m_timeCodeSEIEnabled,                             false, "Control generation of time code information SEI message")
  ("SEIKneeFunctionInfo",                             m_kneeSEIEnabled,                                 false, "Control generation of Knee function SEI messages")
  ("SEIKneeFunctionId",                               m_kneeSEIId,                                          0, "Specifies Id of Knee function SEI message for a given session")
  ("SEIKneeFunctionCancelFlag",                       m_kneeSEICancelFlag,                              false, "Indicates that Knee function SEI message cancels the persistence or follows")
  ("SEIKneeFunctionPersistenceFlag",                  m_kneeSEIPersistenceFlag,                          true, "Specifies the persistence of the Knee function SEI message")
  ("SEIKneeFunctionMappingFlag",                      m_kneeSEIMappingFlag,                             false, "Specifies the mapping mode of the Knee function SEI message")
  ("SEIKneeFunctionInputDrange",                      m_kneeSEIInputDrange,                              1000, "Specifies the peak luminance level for the input picture of Knee function SEI messages")
  ("SEIKneeFunctionInputDispLuminance",               m_kneeSEIInputDispLuminance,                        100, "Specifies the expected display brightness for the input picture of Knee function SEI messages")
  ("SEIKneeFunctionOutputDrange",                     m_kneeSEIOutputDrange,                             4000, "Specifies the peak luminance level for the output picture of Knee function SEI messages")
  ("SEIKneeFunctionOutputDispLuminance",              m_kneeSEIOutputDispLuminance,                       800, "Specifies the expected display brightness for the output picture of Knee function SEI messages")
  ("SEIKneeFunctionNumKneePointsMinus1",              m_kneeSEINumKneePointsMinus1,                         2, "Specifies the number of knee points - 1")
  ("SEIKneeFunctionInputKneePointValue",              cfg_kneeSEIInputKneePointValue,   string("600 800 900"), "Array of input knee point")
  ("SEIKneeFunctionOutputKneePointValue",             cfg_kneeSEIOutputKneePointValue,  string("100 250 450"), "Array of output knee point")
  ("SEIMasteringDisplayColourVolume",                 m_masteringDisplayColourVolumeSEIEnabled,         false, "Control generation of mastering display colour volume SEI messages")
  ("SEIMasteringDisplayMaxLuminance",                 m_masteringDisplayMaxLuminance,                  10000u, "Specifies the mastering display maximum luminance value in units of 1/10000 candela per square metre (32-bit code value)")
  ("SEIMasteringDisplayMinLuminance",                 m_masteringDisplayMinLuminance,                      0u, "Specifies the mastering display minimum luminance value in units of 1/10000 candela per square metre (32-bit code value)")
  ("SEIMasteringDisplayPrimaries",                    cfg_DisplayPrimariesCode, string("0 50000 0 0 50000 0"), "Mastering display primaries for all three colour planes in CIE xy coordinates in increments of 1/50000 (results in the ranges 0 to 50000 inclusive)")
  ("SEIMasteringDisplayWhitePoint",                   cfg_DisplayWhitePointCode,        string("16667 16667"), "Mastering display white point CIE xy coordinates in normalised increments of 1/50000 (e.g. 0.333 = 16667)")
    
  ;

  for(Int i=1; i<MAX_GOP+1; i++) {
    std::ostringstream cOSS;
    cOSS<<"Frame"<<i;
    opts.addOptions()(cOSS.str(), m_GOPList[i-1], GOPEntry());
  }
  po::setDefaults(opts);
  const list<const Char*>& argv_unhandled = po::scanArgv(opts, argc, (const Char**) argv);

  for (list<const Char*>::const_iterator it = argv_unhandled.begin(); it != argv_unhandled.end(); it++)
  {
    fprintf(stderr, "Unhandled argument ignored: `%s'\n", *it);
  }

  if (argc == 1 || do_help)
  {
    /* argc == 1: no options have been specified */
    po::doHelp(cout, opts);
    return false;
  }

  /*
   * Set any derived parameters
   */
  /* convert std::string to c string for compatability */
  m_pchInputFile = cfg_InputFile.empty() ? NULL : strdup(cfg_InputFile.c_str());
  m_pchBitstreamFile = cfg_BitstreamFile.empty() ? NULL : strdup(cfg_BitstreamFile.c_str());
  m_pchReconFile = cfg_ReconFile.empty() ? NULL : strdup(cfg_ReconFile.c_str());
  m_pchdQPFile = cfg_dQPFile.empty() ? NULL : strdup(cfg_dQPFile.c_str());

  if(m_isField)
  {
    //Frame height
    m_iSourceHeightOrg = m_iSourceHeight;
    //Field height
    m_iSourceHeight = m_iSourceHeight >> 1;
    //number of fields to encode
    m_framesToBeEncoded *= 2;
  }

  Char* pColumnWidth = cfg_ColumnWidth.empty() ? NULL: strdup(cfg_ColumnWidth.c_str());
  Char* pRowHeight = cfg_RowHeight.empty() ? NULL : strdup(cfg_RowHeight.c_str());
  if( m_iUniformSpacingIdr == 0 && m_iNumColumnsMinus1 > 0 )
  {
    char *columnWidth;
    int  i=0;
    m_pColumnWidth = new UInt[m_iNumColumnsMinus1];
    columnWidth = strtok(pColumnWidth, " ,-");
    while(columnWidth!=NULL)
    {
      if( i>=m_iNumColumnsMinus1 )
      {
        printf( "The number of columns whose width are defined is larger than the allowed number of columns.\n" );
        exit( EXIT_FAILURE );
      }
      *( m_pColumnWidth + i ) = atoi( columnWidth );
      columnWidth = strtok(NULL, " ,-");
      i++;
    }
    if( i<m_iNumColumnsMinus1 )
    {
      printf( "The width of some columns is not defined.\n" );
      exit( EXIT_FAILURE );
    }
  }
  else
  {
    m_pColumnWidth = NULL;
  }

  if( m_iUniformSpacingIdr == 0 && m_iNumRowsMinus1 > 0 )
  {
    char *rowHeight;
    int  i=0;
    m_pRowHeight = new UInt[m_iNumRowsMinus1];
    rowHeight = strtok(pRowHeight, " ,-");
    while(rowHeight!=NULL)
    {
      if( i>=m_iNumRowsMinus1 )
      {
        printf( "The number of rows whose height are defined is larger than the allowed number of rows.\n" );
        exit( EXIT_FAILURE );
      }
      *( m_pRowHeight + i ) = atoi( rowHeight );
      rowHeight = strtok(NULL, " ,-");
      i++;
    }
    if( i<m_iNumRowsMinus1 )
    {
      printf( "The height of some rows is not defined.\n" );
      exit( EXIT_FAILURE );
   }
  }
  else
  {
    m_pRowHeight = NULL;
  }

  m_scalingListFile = cfg_ScalingListFile.empty() ? NULL : strdup(cfg_ScalingListFile.c_str());

  /* rules for input, output and internal bitdepths as per help text */
  if (m_MSBExtendedBitDepth[CHANNEL_TYPE_LUMA  ] == 0) { m_MSBExtendedBitDepth[CHANNEL_TYPE_LUMA  ] = m_inputBitDepth      [CHANNEL_TYPE_LUMA  ]; }
  if (m_MSBExtendedBitDepth[CHANNEL_TYPE_CHROMA] == 0) { m_MSBExtendedBitDepth[CHANNEL_TYPE_CHROMA] = m_MSBExtendedBitDepth[CHANNEL_TYPE_LUMA  ]; }
  if (m_internalBitDepth   [CHANNEL_TYPE_LUMA  ] == 0) { m_internalBitDepth   [CHANNEL_TYPE_LUMA  ] = m_MSBExtendedBitDepth[CHANNEL_TYPE_LUMA  ]; }
  if (m_internalBitDepth   [CHANNEL_TYPE_CHROMA] == 0) { m_internalBitDepth   [CHANNEL_TYPE_CHROMA] = m_internalBitDepth   [CHANNEL_TYPE_LUMA  ]; }
  if (m_inputBitDepth      [CHANNEL_TYPE_CHROMA] == 0) { m_inputBitDepth      [CHANNEL_TYPE_CHROMA] = m_inputBitDepth      [CHANNEL_TYPE_LUMA  ]; }
  if (m_outputBitDepth     [CHANNEL_TYPE_LUMA  ] == 0) { m_outputBitDepth     [CHANNEL_TYPE_LUMA  ] = m_internalBitDepth   [CHANNEL_TYPE_LUMA  ]; }
  if (m_outputBitDepth     [CHANNEL_TYPE_CHROMA] == 0) { m_outputBitDepth     [CHANNEL_TYPE_CHROMA] = m_internalBitDepth   [CHANNEL_TYPE_CHROMA]; }

  m_InputChromaFormatIDC = numberToChromaFormat(tmpInputChromaFormat);
  m_chromaFormatIDC      = ((tmpChromaFormat == 0) ? (m_InputChromaFormatIDC) : (numberToChromaFormat(tmpChromaFormat)));

  if (extendedProfile >= 1000 && extendedProfile <= 2316)
  {
    m_profile = Profile::MAINREXT;
    if (m_bitDepthConstraint != 0 || tmpConstraintChromaFormat != 0)
    {
      fprintf(stderr, "Error: The bit depth and chroma format constraints are not used when an explicit RExt profile is specified\n");
      exit(EXIT_FAILURE);
    }
    m_bitDepthConstraint     = (extendedProfile%100);
    m_intraConstraintFlag    = (extendedProfile>=2000);
    switch ((extendedProfile/100)%10)
    {
      case 0:  tmpConstraintChromaFormat=400; break;
      case 1:  tmpConstraintChromaFormat=420; break;
      case 2:  tmpConstraintChromaFormat=422; break;
      default: tmpConstraintChromaFormat=444; break;
    }
  }
  else
  {
    m_profile = Profile::Name(extendedProfile);
  }

  if (m_profile == Profile::MAINREXT || m_profile == Profile::HIGHREXT )
  {
    if (m_bitDepthConstraint == 0 && tmpConstraintChromaFormat == 0)
    {
      // produce a valid combination, if possible.
      const Bool bUsingGeneralRExtTools  = m_useResidualRotation                    ||
                                           m_useSingleSignificanceMapContext        ||
                                           m_useResidualDPCM[RDPCM_SIGNAL_IMPLICIT] ||
                                           m_useResidualDPCM[RDPCM_SIGNAL_EXPLICIT] ||
                                           !m_enableIntraReferenceSmoothing         ||
                                           m_useGolombRiceParameterAdaptation       ||
                                           m_transformSkipLog2MaxSize!=2;
      const Bool bUsingChromaQPAdjustment= m_maxCUChromaQpAdjustmentDepth >= 0;
      const Bool bUsingExtendedPrecision = m_useExtendedPrecision;
      m_chromaFormatConstraint = NUM_CHROMA_FORMAT;
      automaticallySelectRExtProfile(bUsingGeneralRExtTools,
                                     bUsingChromaQPAdjustment,
                                     bUsingExtendedPrecision,
                                     m_intraConstraintFlag,
                                     m_bitDepthConstraint,
                                     m_chromaFormatConstraint,
                                     m_chromaFormatIDC==CHROMA_400 ? m_internalBitDepth[CHANNEL_TYPE_LUMA] : std::max(m_internalBitDepth[CHANNEL_TYPE_LUMA], m_internalBitDepth[CHANNEL_TYPE_CHROMA]),
                                     m_chromaFormatIDC);
    }
    else if (m_bitDepthConstraint == 0 || tmpConstraintChromaFormat == 0)
    {
      fprintf(stderr, "Error: The bit depth and chroma format constraints must either both be specified or both be configured automatically\n");
      exit(EXIT_FAILURE);
    }
    else
    {
      m_chromaFormatConstraint = numberToChromaFormat(tmpConstraintChromaFormat);
    }
  }
  else
  {
    m_chromaFormatConstraint = (tmpConstraintChromaFormat == 0) ? m_chromaFormatIDC : numberToChromaFormat(tmpConstraintChromaFormat);
    m_bitDepthConstraint = (m_profile == Profile::MAIN10?10:8);
  }


  m_inputColourSpaceConvert = stringToInputColourSpaceConvert(inputColourSpaceConvert, true);

  switch (m_conformanceMode)
  {
  case 0:
    {
      // no conformance or padding
      m_confLeft = m_confRight = m_confTop = m_confBottom = 0;
      m_aiPad[1] = m_aiPad[0] = 0;
      break;
    }
  case 1:
    {
      // automatic padding to minimum CU size
      Int minCuSize = m_uiMaxCUHeight >> (m_uiMaxCUDepth - 1);
      if (m_iSourceWidth % minCuSize)
      {
        m_aiPad[0] = m_confRight  = ((m_iSourceWidth / minCuSize) + 1) * minCuSize - m_iSourceWidth;
        m_iSourceWidth  += m_confRight;
      }
      if (m_iSourceHeight % minCuSize)
      {
        m_aiPad[1] = m_confBottom = ((m_iSourceHeight / minCuSize) + 1) * minCuSize - m_iSourceHeight;
        m_iSourceHeight += m_confBottom;
        if ( m_isField )
        {
          m_iSourceHeightOrg += m_confBottom << 1;
          m_aiPad[1] = m_confBottom << 1;
        }
      }
      if (m_aiPad[0] % TComSPS::getWinUnitX(m_chromaFormatIDC) != 0)
      {
        fprintf(stderr, "Error: picture width is not an integer multiple of the specified chroma subsampling\n");
        exit(EXIT_FAILURE);
      }
      if (m_aiPad[1] % TComSPS::getWinUnitY(m_chromaFormatIDC) != 0)
      {
        fprintf(stderr, "Error: picture height is not an integer multiple of the specified chroma subsampling\n");
        exit(EXIT_FAILURE);
      }
      break;
    }
  case 2:
    {
      //padding
      m_iSourceWidth  += m_aiPad[0];
      m_iSourceHeight += m_aiPad[1];
      m_confRight  = m_aiPad[0];
      m_confBottom = m_aiPad[1];
      break;
    }
  case 3:
    {
      // conformance
      if ((m_confLeft == 0) && (m_confRight == 0) && (m_confTop == 0) && (m_confBottom == 0))
      {
        fprintf(stderr, "Warning: Conformance window enabled, but all conformance window parameters set to zero\n");
      }
      if ((m_aiPad[1] != 0) || (m_aiPad[0]!=0))
      {
        fprintf(stderr, "Warning: Conformance window enabled, padding parameters will be ignored\n");
      }
      m_aiPad[1] = m_aiPad[0] = 0;
      break;
    }
  }

  // allocate slice-based dQP values
  m_aidQP = new Int[ m_framesToBeEncoded + m_iGOPSize + 1 ];
  ::memset( m_aidQP, 0, sizeof(Int)*( m_framesToBeEncoded + m_iGOPSize + 1 ) );

  // handling of floating-point QP values
  // if QP is not integer, sequence is split into two sections having QP and QP+1
  m_iQP = (Int)( m_fQP );
  if ( m_iQP < m_fQP )
  {
    Int iSwitchPOC = (Int)( m_framesToBeEncoded - (m_fQP - m_iQP)*m_framesToBeEncoded + 0.5 );

    iSwitchPOC = (Int)( (Double)iSwitchPOC / m_iGOPSize + 0.5 )*m_iGOPSize;
    for ( Int i=iSwitchPOC; i<m_framesToBeEncoded + m_iGOPSize + 1; i++ )
    {
      m_aidQP[i] = 1;
    }
  }

#if RExt__Q0044_SAO_OFFSET_BIT_SHIFT_ADAPTATION
  for(UInt ch=0; ch<MAX_NUM_CHANNEL_TYPE; ch++)
  {
    if (saoOffsetBitShift[ch]<0)
    {
      if (m_internalBitDepth[ch]>10)
      {
        m_saoOffsetBitShift[ch]=UInt(Clip3<Int>(0, m_internalBitDepth[ch]-10, Int(m_internalBitDepth[ch]-10 + 0.165*m_iQP - 3.22 + 0.5) ) );
      }
      else
      {
        m_saoOffsetBitShift[ch]=0;
      }
    }
    else
    {
      m_saoOffsetBitShift[ch]=UInt(saoOffsetBitShift[ch]);
    }
  }
#endif

  // reading external dQP description from file
  if ( m_pchdQPFile )
  {
    FILE* fpt=fopen( m_pchdQPFile, "r" );
    if ( fpt )
    {
      Int iValue;
      Int iPOC = 0;
      while ( iPOC < m_framesToBeEncoded )
      {
        if ( fscanf(fpt, "%d", &iValue ) == EOF ) break;
        m_aidQP[ iPOC ] = iValue;
        iPOC++;
      }
      fclose(fpt);
    }
  }
  m_iWaveFrontSubstreams = m_iWaveFrontSynchro ? (m_iSourceHeight + m_uiMaxCUHeight - 1) / m_uiMaxCUHeight : 1;

  if( m_masteringDisplayColourVolumeSEIEnabled )
  {
    Char* pcDisplayPrimariesValue = cfg_DisplayPrimariesCode.empty() ? NULL: strdup(cfg_DisplayPrimariesCode.c_str());
    Char* pcDisplayWhitePointValue = cfg_DisplayWhitePointCode.empty() ? NULL: strdup(cfg_DisplayWhitePointCode.c_str());

    m_masteringDisplayPrimaries = new UShort[6];
    m_masteringDisplayWhitePoint = new UShort[2];
    
    char *displayPrimaryValue = strtok( pcDisplayPrimariesValue, " .");
    int idx = 0;
      
    while( displayPrimaryValue != NULL && idx < 6)
    {
      m_masteringDisplayPrimaries[idx] = atoi( displayPrimaryValue );
      displayPrimaryValue = strtok(NULL, " .");
      idx++;
    }

      
    char *displayWhitePointValue = strtok( pcDisplayWhitePointValue, " .");
    idx = 0;
      
    while( displayWhitePointValue != NULL && idx < 2)
    {
      m_masteringDisplayWhitePoint[idx] = atoi( displayWhitePointValue );
      displayWhitePointValue = strtok(NULL, " .");
      idx++;
    }
  }
    
  if( m_toneMappingInfoSEIEnabled && !m_toneMapCancelFlag )
  {
    Char* pcStartOfCodedInterval = cfg_startOfCodedInterval.empty() ? NULL: strdup(cfg_startOfCodedInterval.c_str());
    Char* pcCodedPivotValue = cfg_codedPivotValue.empty() ? NULL: strdup(cfg_codedPivotValue.c_str());
    Char* pcTargetPivotValue = cfg_targetPivotValue.empty() ? NULL: strdup(cfg_targetPivotValue.c_str());
    if( m_toneMapModelId == 2 && pcStartOfCodedInterval )
    {
      char *startOfCodedInterval;
      UInt num = 1u<< m_toneMapTargetBitDepth;
      m_startOfCodedInterval = new Int[num];
      ::memset( m_startOfCodedInterval, 0, sizeof(Int)*num );
      startOfCodedInterval = strtok(pcStartOfCodedInterval, " .");
      int i = 0;
      while( startOfCodedInterval && ( i < num ) )
      {
        m_startOfCodedInterval[i] = atoi( startOfCodedInterval );
        startOfCodedInterval = strtok(NULL, " .");
        i++;
      }
    }
    else
    {
      m_startOfCodedInterval = NULL;
    }
    if( ( m_toneMapModelId == 3 ) && ( m_numPivots > 0 ) )
    {
      if( pcCodedPivotValue && pcTargetPivotValue )
      {
        char *codedPivotValue;
        char *targetPivotValue;
        m_codedPivotValue = new Int[m_numPivots];
        m_targetPivotValue = new Int[m_numPivots];
        ::memset( m_codedPivotValue, 0, sizeof(Int)*( m_numPivots ) );
        ::memset( m_targetPivotValue, 0, sizeof(Int)*( m_numPivots ) );
        codedPivotValue = strtok(pcCodedPivotValue, " .");
        int i=0;
        while(codedPivotValue&&i<m_numPivots)
        {
          m_codedPivotValue[i] = atoi( codedPivotValue );
          codedPivotValue = strtok(NULL, " .");
          i++;
        }
        i=0;
        targetPivotValue = strtok(pcTargetPivotValue, " .");
        while(targetPivotValue&&i<m_numPivots)
        {
          m_targetPivotValue[i]= atoi( targetPivotValue );
          targetPivotValue = strtok(NULL, " .");
          i++;
        }
      }
    }
    else
    {
      m_codedPivotValue = NULL;
      m_targetPivotValue = NULL;
    }
  }

  if( m_kneeSEIEnabled && !m_kneeSEICancelFlag )
  {
    Char* pcInputKneePointValue  = cfg_kneeSEIInputKneePointValue.empty()  ? NULL : strdup(cfg_kneeSEIInputKneePointValue.c_str());
    Char* pcOutputKneePointValue = cfg_kneeSEIOutputKneePointValue.empty() ? NULL : strdup(cfg_kneeSEIOutputKneePointValue.c_str());
    assert ( m_kneeSEINumKneePointsMinus1 >= 0 && m_kneeSEINumKneePointsMinus1 < 999 );
    m_kneeSEIInputKneePoint = new Int[m_kneeSEINumKneePointsMinus1+1];
    m_kneeSEIOutputKneePoint = new Int[m_kneeSEINumKneePointsMinus1+1];
    char *InputVal = strtok(pcInputKneePointValue, " .,");
    Int i=0;
    while( InputVal && i<(m_kneeSEINumKneePointsMinus1+1) )
    {
      m_kneeSEIInputKneePoint[i] = (UInt) atoi( InputVal );
      InputVal = strtok(NULL, " .,");
      i++;
    }
    char *OutputVal = strtok(pcOutputKneePointValue, " .,");
    i=0;
    while( OutputVal && i<(m_kneeSEINumKneePointsMinus1+1) )
    {
      m_kneeSEIOutputKneePoint[i] = (UInt) atoi( OutputVal );
      OutputVal = strtok(NULL, " .,");
      i++;
    }
  }

  // check validity of input parameters
  xCheckParameter();

  // set global varibles
  xSetGlobal();

  // print-out parameters
  xPrintParameter();

  return true;
}


// ====================================================================================================================
// Private member functions
// ====================================================================================================================

Void TAppEncCfg::xCheckParameter()
{
  if (!m_decodedPictureHashSEIEnabled)
  {
    fprintf(stderr, "******************************************************************\n");
    fprintf(stderr, "** WARNING: --SEIDecodedPictureHash is now disabled by default. **\n");
    fprintf(stderr, "**          Automatic verification of decoded pictures by a     **\n");
    fprintf(stderr, "**          decoder requires this option to be enabled.         **\n");
    fprintf(stderr, "******************************************************************\n");
  }
  if( m_profile==Profile::NONE )
  {
    fprintf(stderr, "***************************************************************************\n");
    fprintf(stderr, "** WARNING: For conforming bitstreams a valid Profile value must be set! **\n");
    fprintf(stderr, "***************************************************************************\n");
  }
  if( m_level==Level::NONE )
  {
    fprintf(stderr, "***************************************************************************\n");
    fprintf(stderr, "** WARNING: For conforming bitstreams a valid Level value must be set!   **\n");
    fprintf(stderr, "***************************************************************************\n");
  }

  Bool check_failed = false; /* abort if there is a fatal configuration problem */
#define xConfirmPara(a,b) check_failed |= confirmPara(a,b)

  const UInt maxBitDepth=(m_chromaFormatIDC==CHROMA_400) ? m_internalBitDepth[CHANNEL_TYPE_LUMA] : std::max(m_internalBitDepth[CHANNEL_TYPE_LUMA], m_internalBitDepth[CHANNEL_TYPE_CHROMA]);
  xConfirmPara(m_bitDepthConstraint<maxBitDepth, "The internalBitDepth must not be greater than the bitDepthConstraint value");
  xConfirmPara(m_chromaFormatConstraint<m_chromaFormatIDC, "The chroma format used must not be greater than the chromaFormatConstraint value");

  if (m_profile==Profile::MAINREXT || m_profile==Profile::HIGHREXT)
  {
    // NOTE: RExt - consider adjusting so that only the restricted legal combinations are possible
    // m_intraConstraintFlag is checked below.
    xConfirmPara(m_lowerBitRateConstraintFlag==false && m_intraConstraintFlag==false, "The lowerBitRateConstraint flag cannot be false when intraConstraintFlag is false");
    xConfirmPara(m_alignCABACBeforeBypass && m_profile!=Profile::HIGHREXT, "AlignCABACBeforeBypass must not be enabled unless the high bit rate profile is being used.");
    if (m_profile == Profile::MAINREXT)
    {
      const UInt intraIdx = m_intraConstraintFlag ? 1:0;
      const UInt bitDepthIdx = (m_bitDepthConstraint == 8 ? 0 : (m_bitDepthConstraint ==10 ? 1 : (m_bitDepthConstraint == 12 ? 2 : (m_bitDepthConstraint == 16 ? 3 : 4 ))));
      const UInt chromaFormatIdx = UInt(m_chromaFormatConstraint);
      const Bool bValidProfile = (bitDepthIdx > 3 || chromaFormatIdx>3) ? false : (validRExtProfileNames[intraIdx][bitDepthIdx][chromaFormatIdx] != NONE);
      xConfirmPara(!bValidProfile, "Invalid intra constraint flag, bit depth constraint flag and chroma format constraint flag combination for a RExt profile");
      const Bool bUsingGeneralRExtTools  = m_useResidualRotation                    ||
                                           m_useSingleSignificanceMapContext        ||
                                           m_useResidualDPCM[RDPCM_SIGNAL_IMPLICIT] ||
                                           m_useResidualDPCM[RDPCM_SIGNAL_EXPLICIT] ||
                                           !m_enableIntraReferenceSmoothing         ||
                                           m_useGolombRiceParameterAdaptation       ||
                                           m_transformSkipLog2MaxSize!=2;
      const Bool bUsingChromaQPTool      = m_maxCUChromaQpAdjustmentDepth >= 0;
      const Bool bUsingExtendedPrecision = m_useExtendedPrecision;

      xConfirmPara((m_chromaFormatConstraint==CHROMA_420 || m_chromaFormatConstraint==CHROMA_400) && bUsingChromaQPTool, "CU Chroma QP adjustment cannot be used for 4:0:0 or 4:2:0 RExt profiles");
      xConfirmPara(m_bitDepthConstraint != 16 && bUsingExtendedPrecision, "Extended precision can only be used in 16-bit RExt profiles");
      if (!(m_chromaFormatConstraint == CHROMA_400 && m_bitDepthConstraint == 16) && m_chromaFormatConstraint!=CHROMA_444)
      {
        xConfirmPara(bUsingGeneralRExtTools, "Combination of tools and profiles are not possible in the specified RExt profile.");
      }
      if (!m_intraConstraintFlag && m_bitDepthConstraint==16 && m_chromaFormatConstraint==CHROMA_444)
      {
        fprintf(stderr, "********************************************************************************************************\n");
        fprintf(stderr, "** WARNING: The RExt constraint flags describe a non standard combination (used for development only) **\n");
        fprintf(stderr, "********************************************************************************************************\n");
      }
    }
  }
  else
  {
    xConfirmPara(m_bitDepthConstraint!=((m_profile==Profile::MAIN10)?10:8), "BitDepthConstraint must be 8 for MAIN profile and 10 for MAIN10 profile.");
    xConfirmPara(m_chromaFormatConstraint!=CHROMA_420, "ChromaFormatConstraint must be 420 for non main-RExt profiles.");
    xConfirmPara(m_intraConstraintFlag==true, "IntraConstraintFlag must be false for non main_RExt profiles.");
    xConfirmPara(m_lowerBitRateConstraintFlag==false, "LowerBitrateConstraintFlag must be true for non main-RExt profiles.");

    xConfirmPara(m_useCrossComponentPrediction==true, "CrossComponentPrediction must not be used for non main-RExt profiles.");
    xConfirmPara(m_transformSkipLog2MaxSize!=2, "Transform Skip Log2 Max Size must be 2 for V1 profiles.");
    xConfirmPara(m_useResidualRotation==true, "UseResidualRotation must not be enabled for non main-RExt profiles.");
    xConfirmPara(m_useSingleSignificanceMapContext==true, "UseSingleSignificanceMapContext must not be enabled for non main-RExt profiles.");
    xConfirmPara(m_useResidualDPCM[RDPCM_SIGNAL_IMPLICIT]==true, "ImplicitResidualDPCM must not be enabled for non main-RExt profiles.");
    xConfirmPara(m_useResidualDPCM[RDPCM_SIGNAL_EXPLICIT]==true, "ExplicitResidualDPCM must not be enabled for non main-RExt profiles.");
    xConfirmPara(m_useGolombRiceParameterAdaptation==true, "GolombRiceParameterAdaption must not be enabled for non main-RExt profiles.");
    xConfirmPara(m_useExtendedPrecision==true, "UseExtendedPrecision must not be enabled for non main-RExt profiles.");
    xConfirmPara(m_useHighPrecisionPredictionWeighting==true, "UseHighPrecisionPredictionWeighting must not be enabled for non main-RExt profiles.");
    xConfirmPara(m_enableIntraReferenceSmoothing==false, "EnableIntraReferenceSmoothing must be enabled for non main-RExt profiles.");
    xConfirmPara(m_alignCABACBeforeBypass, "AlignCABACBeforeBypass cannot be enabled for non main-RExt profiles.");
  }

  // check range of parameters
  xConfirmPara( m_inputBitDepth[CHANNEL_TYPE_LUMA  ] < 8,                                   "InputBitDepth must be at least 8" );
  xConfirmPara( m_inputBitDepth[CHANNEL_TYPE_CHROMA] < 8,                                   "InputBitDepthC must be at least 8" );

#if !RExt__HIGH_BIT_DEPTH_SUPPORT
  if (m_useExtendedPrecision)
  {
    for (UInt channelType = 0; channelType < MAX_NUM_CHANNEL_TYPE; channelType++)
    {
      xConfirmPara((m_internalBitDepth[channelType] > 8) , "Model is not configured to support high enough internal accuracies - enable RExt__HIGH_BIT_DEPTH_SUPPORT to use increased precision internal data types etc...");
    }
  }
  else
  {
    for (UInt channelType = 0; channelType < MAX_NUM_CHANNEL_TYPE; channelType++)
    {
      xConfirmPara((m_internalBitDepth[channelType] > 12) , "Model is not configured to support high enough internal accuracies - enable RExt__HIGH_BIT_DEPTH_SUPPORT to use increased precision internal data types etc...");
    }
  }
#endif

  xConfirmPara( (m_MSBExtendedBitDepth[CHANNEL_TYPE_LUMA  ] < m_inputBitDepth[CHANNEL_TYPE_LUMA  ]), "MSB-extended bit depth for luma channel (--MSBExtendedBitDepth) must be greater than or equal to input bit depth for luma channel (--InputBitDepth)" );
  xConfirmPara( (m_MSBExtendedBitDepth[CHANNEL_TYPE_CHROMA] < m_inputBitDepth[CHANNEL_TYPE_CHROMA]), "MSB-extended bit depth for chroma channel (--MSBExtendedBitDepthC) must be greater than or equal to input bit depth for chroma channel (--InputBitDepthC)" );

  xConfirmPara( m_saoOffsetBitShift[CHANNEL_TYPE_LUMA]   > (m_internalBitDepth[CHANNEL_TYPE_LUMA  ]<10?0:(m_internalBitDepth[CHANNEL_TYPE_LUMA  ]-10)), "SaoLumaOffsetBitShift must be in the range of 0 to InternalBitDepth-10, inclusive");
  xConfirmPara( m_saoOffsetBitShift[CHANNEL_TYPE_CHROMA] > (m_internalBitDepth[CHANNEL_TYPE_CHROMA]<10?0:(m_internalBitDepth[CHANNEL_TYPE_CHROMA]-10)), "SaoChromaOffsetBitShift must be in the range of 0 to InternalBitDepthC-10, inclusive");

  xConfirmPara( m_chromaFormatIDC >= NUM_CHROMA_FORMAT,                                     "ChromaFormatIDC must be either 400, 420, 422 or 444" );
  std::string sTempIPCSC="InputColourSpaceConvert must be empty, "+getListOfColourSpaceConverts(true);
  xConfirmPara( m_inputColourSpaceConvert >= NUMBER_INPUT_COLOUR_SPACE_CONVERSIONS,         sTempIPCSC.c_str() );
  xConfirmPara( m_InputChromaFormatIDC >= NUM_CHROMA_FORMAT,                                "InputChromaFormatIDC must be either 400, 420, 422 or 444" );
  xConfirmPara( m_iFrameRate <= 0,                                                          "Frame rate must be more than 1" );
  xConfirmPara( m_framesToBeEncoded <= 0,                                                   "Total Number Of Frames encoded must be more than 0" );
  xConfirmPara( m_iGOPSize < 1 ,                                                            "GOP Size must be greater or equal to 1" );
  xConfirmPara( m_iGOPSize > 1 &&  m_iGOPSize % 2,                                          "GOP Size must be a multiple of 2, if GOP Size is greater than 1" );
  xConfirmPara( (m_iIntraPeriod > 0 && m_iIntraPeriod < m_iGOPSize) || m_iIntraPeriod == 0, "Intra period must be more than GOP size, or -1 , not 0" );
#if ALLOW_RECOVERY_POINT_AS_RAP
  xConfirmPara( m_iDecodingRefreshType < 0 || m_iDecodingRefreshType > 3,                   "Decoding Refresh Type must be comprised between 0 and 3 included" );
  if(m_iDecodingRefreshType == 3)
  {
    xConfirmPara( !m_recoveryPointSEIEnabled,                                               "When using RecoveryPointSEI messages as RA points, recoveryPointSEI must be enabled" );
  }
#else
  xConfirmPara( m_iDecodingRefreshType < 0 || m_iDecodingRefreshType > 2,                   "Decoding Refresh Type must be equal to 0, 1 or 2" );
#endif

  if(m_useCrossComponentPrediction && (m_chromaFormatIDC != CHROMA_444))
  {
      fprintf(stderr, "****************************************************************************\n");
      fprintf(stderr, "** WARNING: Cross-component prediction is specified for 4:4:4 format only **\n");
      fprintf(stderr, "****************************************************************************\n");

      m_useCrossComponentPrediction = false;
  }

  xConfirmPara (m_transformSkipLog2MaxSize < 2, "Transform Skip Log2 Max Size must be at least 2 (4x4)");

  if (m_transformSkipLog2MaxSize!=2 && m_useTransformSkipFast)
  {
    fprintf(stderr, "***************************************************************************\n");
    fprintf(stderr, "** WARNING: Transform skip fast is enabled (which only tests NxN splits),**\n");
    fprintf(stderr, "**          but transform skip log2 max size is not 2 (4x4)              **\n");
    fprintf(stderr, "**          It may be better to disable transform skip fast mode         **\n");
    fprintf(stderr, "***************************************************************************\n");
  }

  xConfirmPara( m_iQP <  -6 * (m_internalBitDepth[CHANNEL_TYPE_LUMA] - 8) || m_iQP > 51,    "QP exceeds supported range (-QpBDOffsety to 51)" );
  xConfirmPara( m_loopFilterBetaOffsetDiv2 < -6 || m_loopFilterBetaOffsetDiv2 > 6,        "Loop Filter Beta Offset div. 2 exceeds supported range (-6 to 6)");
  xConfirmPara( m_loopFilterTcOffsetDiv2 < -6 || m_loopFilterTcOffsetDiv2 > 6,            "Loop Filter Tc Offset div. 2 exceeds supported range (-6 to 6)");
  xConfirmPara( m_iFastSearch < 0 || m_iFastSearch > 2,                                     "Fast Search Mode is not supported value (0:Full search  1:Diamond  2:PMVFAST)" );
  xConfirmPara( m_iSearchRange < 0 ,                                                        "Search Range must be more than 0" );
  xConfirmPara( m_bipredSearchRange < 0 ,                                                   "Search Range must be more than 0" );
  xConfirmPara( m_iMaxDeltaQP > 7,                                                          "Absolute Delta QP exceeds supported range (0 to 7)" );
  xConfirmPara( m_iMaxCuDQPDepth > m_uiMaxCUDepth - 1,                                          "Absolute depth for a minimum CuDQP exceeds maximum coding unit depth" );

  xConfirmPara( m_cbQpOffset < -12,   "Min. Chroma Cb QP Offset is -12" );
  xConfirmPara( m_cbQpOffset >  12,   "Max. Chroma Cb QP Offset is  12" );
  xConfirmPara( m_crQpOffset < -12,   "Min. Chroma Cr QP Offset is -12" );
  xConfirmPara( m_crQpOffset >  12,   "Max. Chroma Cr QP Offset is  12" );

  xConfirmPara( m_iQPAdaptationRange <= 0,                                                  "QP Adaptation Range must be more than 0" );
  if (m_iDecodingRefreshType == 2)
  {
    xConfirmPara( m_iIntraPeriod > 0 && m_iIntraPeriod <= m_iGOPSize ,                      "Intra period must be larger than GOP size for periodic IDR pictures");
  }
  xConfirmPara( (m_uiMaxCUWidth  >> m_uiMaxCUDepth) < 4,                                    "Minimum partition width size should be larger than or equal to 8");
  xConfirmPara( (m_uiMaxCUHeight >> m_uiMaxCUDepth) < 4,                                    "Minimum partition height size should be larger than or equal to 8");
  xConfirmPara( m_uiMaxCUWidth < 16,                                                        "Maximum partition width size should be larger than or equal to 16");
  xConfirmPara( m_uiMaxCUHeight < 16,                                                       "Maximum partition height size should be larger than or equal to 16");
  xConfirmPara( (m_iSourceWidth  % (m_uiMaxCUWidth  >> (m_uiMaxCUDepth-1)))!=0,             "Resulting coded frame width must be a multiple of the minimum CU size");
  xConfirmPara( (m_iSourceHeight % (m_uiMaxCUHeight >> (m_uiMaxCUDepth-1)))!=0,             "Resulting coded frame height must be a multiple of the minimum CU size");

  xConfirmPara( m_uiQuadtreeTULog2MinSize < 2,                                        "QuadtreeTULog2MinSize must be 2 or greater.");
  xConfirmPara( m_uiQuadtreeTULog2MaxSize > 5,                                        "QuadtreeTULog2MaxSize must be 5 or smaller.");
  xConfirmPara( m_uiQuadtreeTULog2MaxSize < m_uiQuadtreeTULog2MinSize,                "QuadtreeTULog2MaxSize must be greater than or equal to m_uiQuadtreeTULog2MinSize.");
  xConfirmPara( (1<<m_uiQuadtreeTULog2MaxSize) > m_uiMaxCUWidth,                      "QuadtreeTULog2MaxSize must be log2(maxCUSize) or smaller.");
  xConfirmPara( (1<<m_uiQuadtreeTULog2MinSize)>(m_uiMaxCUWidth >>(m_uiMaxCUDepth-1)), "QuadtreeTULog2MinSize must not be greater than minimum CU size" ); // HS
  xConfirmPara( (1<<m_uiQuadtreeTULog2MinSize)>(m_uiMaxCUHeight>>(m_uiMaxCUDepth-1)), "QuadtreeTULog2MinSize must not be greater than minimum CU size" ); // HS
  xConfirmPara( ( 1 << m_uiQuadtreeTULog2MinSize ) > ( m_uiMaxCUWidth  >> m_uiMaxCUDepth ), "Minimum CU width must be greater than minimum transform size." );
  xConfirmPara( ( 1 << m_uiQuadtreeTULog2MinSize ) > ( m_uiMaxCUHeight >> m_uiMaxCUDepth ), "Minimum CU height must be greater than minimum transform size." );
  xConfirmPara( m_uiQuadtreeTUMaxDepthInter < 1,                                                         "QuadtreeTUMaxDepthInter must be greater than or equal to 1" );
  xConfirmPara( m_uiMaxCUWidth < ( 1 << (m_uiQuadtreeTULog2MinSize + m_uiQuadtreeTUMaxDepthInter - 1) ), "QuadtreeTUMaxDepthInter must be less than or equal to the difference between log2(maxCUSize) and QuadtreeTULog2MinSize plus 1" );
  xConfirmPara( m_uiQuadtreeTUMaxDepthIntra < 1,                                                         "QuadtreeTUMaxDepthIntra must be greater than or equal to 1" );
  xConfirmPara( m_uiMaxCUWidth < ( 1 << (m_uiQuadtreeTULog2MinSize + m_uiQuadtreeTUMaxDepthIntra - 1) ), "QuadtreeTUMaxDepthInter must be less than or equal to the difference between log2(maxCUSize) and QuadtreeTULog2MinSize plus 1" );

  xConfirmPara(  m_maxNumMergeCand < 1,  "MaxNumMergeCand must be 1 or greater.");
  xConfirmPara(  m_maxNumMergeCand > 5,  "MaxNumMergeCand must be 5 or smaller.");

#if ADAPTIVE_QP_SELECTION
  xConfirmPara( m_bUseAdaptQpSelect == true && m_iQP < 0,                                              "AdaptiveQpSelection must be disabled when QP < 0.");
  xConfirmPara( m_bUseAdaptQpSelect == true && (m_cbQpOffset !=0 || m_crQpOffset != 0 ),               "AdaptiveQpSelection must be disabled when ChromaQpOffset is not equal to 0.");
#endif

  if( m_usePCM)
  {
    for (UInt channelType = 0; channelType < MAX_NUM_CHANNEL_TYPE; channelType++)
    {
      xConfirmPara(((m_MSBExtendedBitDepth[channelType] > m_internalBitDepth[channelType]) && m_bPCMInputBitDepthFlag), "PCM bit depth cannot be greater than internal bit depth (PCMInputBitDepthFlag cannot be used when InputBitDepth or MSBExtendedBitDepth > InternalBitDepth)");
    }
    xConfirmPara(  m_uiPCMLog2MinSize < 3,                                      "PCMLog2MinSize must be 3 or greater.");
    xConfirmPara(  m_uiPCMLog2MinSize > 5,                                      "PCMLog2MinSize must be 5 or smaller.");
    xConfirmPara(  m_pcmLog2MaxSize > 5,                                        "PCMLog2MaxSize must be 5 or smaller.");
    xConfirmPara(  m_pcmLog2MaxSize < m_uiPCMLog2MinSize,                       "PCMLog2MaxSize must be equal to or greater than m_uiPCMLog2MinSize.");
  }

  xConfirmPara( m_sliceMode < 0 || m_sliceMode > 3, "SliceMode exceeds supported range (0 to 3)" );
  if (m_sliceMode!=0)
  {
    xConfirmPara( m_sliceArgument < 1 ,         "SliceArgument should be larger than or equal to 1" );
  }
  xConfirmPara( m_sliceSegmentMode < 0 || m_sliceSegmentMode > 3, "SliceSegmentMode exceeds supported range (0 to 3)" );
  if (m_sliceSegmentMode!=0)
  {
    xConfirmPara( m_sliceSegmentArgument < 1 ,         "SliceSegmentArgument should be larger than or equal to 1" );
  }

  Bool tileFlag = (m_iNumColumnsMinus1 > 0 || m_iNumRowsMinus1 > 0 );
  xConfirmPara( tileFlag && m_iWaveFrontSynchro,            "Tile and Wavefront can not be applied together");

  xConfirmPara( m_iSourceWidth  % TComSPS::getWinUnitX(m_chromaFormatIDC) != 0, "Picture width must be an integer multiple of the specified chroma subsampling");
  xConfirmPara( m_iSourceHeight % TComSPS::getWinUnitY(m_chromaFormatIDC) != 0, "Picture height must be an integer multiple of the specified chroma subsampling");

  xConfirmPara( m_aiPad[0] % TComSPS::getWinUnitX(m_chromaFormatIDC) != 0, "Horizontal padding must be an integer multiple of the specified chroma subsampling");
  xConfirmPara( m_aiPad[1] % TComSPS::getWinUnitY(m_chromaFormatIDC) != 0, "Vertical padding must be an integer multiple of the specified chroma subsampling");

  xConfirmPara( m_confLeft   % TComSPS::getWinUnitX(m_chromaFormatIDC) != 0, "Left conformance window offset must be an integer multiple of the specified chroma subsampling");
  xConfirmPara( m_confRight  % TComSPS::getWinUnitX(m_chromaFormatIDC) != 0, "Right conformance window offset must be an integer multiple of the specified chroma subsampling");
  xConfirmPara( m_confTop    % TComSPS::getWinUnitY(m_chromaFormatIDC) != 0, "Top conformance window offset must be an integer multiple of the specified chroma subsampling");
  xConfirmPara( m_confBottom % TComSPS::getWinUnitY(m_chromaFormatIDC) != 0, "Bottom conformance window offset must be an integer multiple of the specified chroma subsampling");

  // max CU width and height should be power of 2
  UInt ui = m_uiMaxCUWidth;
  while(ui)
  {
    ui >>= 1;
    if( (ui & 1) == 1)
      xConfirmPara( ui != 1 , "Width should be 2^n");
  }
  ui = m_uiMaxCUHeight;
  while(ui)
  {
    ui >>= 1;
    if( (ui & 1) == 1)
      xConfirmPara( ui != 1 , "Height should be 2^n");
  }

  /* if this is an intra-only sequence, ie IntraPeriod=1, don't verify the GOP structure
   * This permits the ability to omit a GOP structure specification */
  if (m_iIntraPeriod == 1 && m_GOPList[0].m_POC == -1)
  {
    m_GOPList[0] = GOPEntry();
    m_GOPList[0].m_QPFactor = 1;
    m_GOPList[0].m_betaOffsetDiv2 = 0;
    m_GOPList[0].m_tcOffsetDiv2 = 0;
    m_GOPList[0].m_POC = 1;
    m_GOPList[0].m_numRefPicsActive = 4;
  }
  else
  {
    xConfirmPara( m_intraConstraintFlag, "IntraConstraintFlag cannot be 1 for inter sequences");
  }

  Bool verifiedGOP=false;
  Bool errorGOP=false;
  Int checkGOP=1;
  Int numRefs = m_isField ? 2 : 1;
  Int refList[MAX_NUM_REF_PICS+1];
  refList[0]=0;
  if(m_isField)
  {
    refList[1] = 1;
  }
  Bool isOK[MAX_GOP];
  for(Int i=0; i<MAX_GOP; i++)
  {
    isOK[i]=false;
  }
  Int numOK=0;
  xConfirmPara( m_iIntraPeriod >=0&&(m_iIntraPeriod%m_iGOPSize!=0), "Intra period must be a multiple of GOPSize, or -1" );

  for(Int i=0; i<m_iGOPSize; i++)
  {
    if(m_GOPList[i].m_POC==m_iGOPSize)
    {
      xConfirmPara( m_GOPList[i].m_temporalId!=0 , "The last frame in each GOP must have temporal ID = 0 " );
    }
  }

  if ( (m_iIntraPeriod != 1) && !m_loopFilterOffsetInPPS && m_DeblockingFilterControlPresent && (!m_bLoopFilterDisable) )
  {
    for(Int i=0; i<m_iGOPSize; i++)
    {
      xConfirmPara( (m_GOPList[i].m_betaOffsetDiv2 + m_loopFilterBetaOffsetDiv2) < -6 || (m_GOPList[i].m_betaOffsetDiv2 + m_loopFilterBetaOffsetDiv2) > 6, "Loop Filter Beta Offset div. 2 for one of the GOP entries exceeds supported range (-6 to 6)" );
      xConfirmPara( (m_GOPList[i].m_tcOffsetDiv2 + m_loopFilterTcOffsetDiv2) < -6 || (m_GOPList[i].m_tcOffsetDiv2 + m_loopFilterTcOffsetDiv2) > 6, "Loop Filter Tc Offset div. 2 for one of the GOP entries exceeds supported range (-6 to 6)" );
    }
  }

  m_extraRPSs=0;
  //start looping through frames in coding order until we can verify that the GOP structure is correct.
  while(!verifiedGOP&&!errorGOP)
  {
    Int curGOP = (checkGOP-1)%m_iGOPSize;
    Int curPOC = ((checkGOP-1)/m_iGOPSize)*m_iGOPSize + m_GOPList[curGOP].m_POC;
    if(m_GOPList[curGOP].m_POC<0)
    {
      printf("\nError: found fewer Reference Picture Sets than GOPSize\n");
      errorGOP=true;
    }
    else
    {
      //check that all reference pictures are available, or have a POC < 0 meaning they might be available in the next GOP.
      Bool beforeI = false;
      for(Int i = 0; i< m_GOPList[curGOP].m_numRefPics; i++)
      {
        Int absPOC = curPOC+m_GOPList[curGOP].m_referencePics[i];
        if(absPOC < 0)
        {
          beforeI=true;
        }
        else
        {
          Bool found=false;
          for(Int j=0; j<numRefs; j++)
          {
            if(refList[j]==absPOC)
            {
              found=true;
              for(Int k=0; k<m_iGOPSize; k++)
              {
                if(absPOC%m_iGOPSize == m_GOPList[k].m_POC%m_iGOPSize)
                {
                  if(m_GOPList[k].m_temporalId==m_GOPList[curGOP].m_temporalId)
                  {
                    m_GOPList[k].m_refPic = true;
                  }
                  m_GOPList[curGOP].m_usedByCurrPic[i]=m_GOPList[k].m_temporalId<=m_GOPList[curGOP].m_temporalId;
                }
              }
            }
          }
          if(!found)
          {
            printf("\nError: ref pic %d is not available for GOP frame %d\n",m_GOPList[curGOP].m_referencePics[i],curGOP+1);
            errorGOP=true;
          }
        }
      }
      if(!beforeI&&!errorGOP)
      {
        //all ref frames were present
        if(!isOK[curGOP])
        {
          numOK++;
          isOK[curGOP]=true;
          if(numOK==m_iGOPSize)
          {
            verifiedGOP=true;
          }
        }
      }
      else
      {
        //create a new GOPEntry for this frame containing all the reference pictures that were available (POC > 0)
        m_GOPList[m_iGOPSize+m_extraRPSs]=m_GOPList[curGOP];
        Int newRefs=0;
        for(Int i = 0; i< m_GOPList[curGOP].m_numRefPics; i++)
        {
          Int absPOC = curPOC+m_GOPList[curGOP].m_referencePics[i];
          if(absPOC>=0)
          {
            m_GOPList[m_iGOPSize+m_extraRPSs].m_referencePics[newRefs]=m_GOPList[curGOP].m_referencePics[i];
            m_GOPList[m_iGOPSize+m_extraRPSs].m_usedByCurrPic[newRefs]=m_GOPList[curGOP].m_usedByCurrPic[i];
            newRefs++;
          }
        }
        Int numPrefRefs = m_GOPList[curGOP].m_numRefPicsActive;

        for(Int offset = -1; offset>-checkGOP; offset--)
        {
          //step backwards in coding order and include any extra available pictures we might find useful to replace the ones with POC < 0.
          Int offGOP = (checkGOP-1+offset)%m_iGOPSize;
          Int offPOC = ((checkGOP-1+offset)/m_iGOPSize)*m_iGOPSize + m_GOPList[offGOP].m_POC;
          if(offPOC>=0&&m_GOPList[offGOP].m_temporalId<=m_GOPList[curGOP].m_temporalId)
          {
            Bool newRef=false;
            for(Int i=0; i<numRefs; i++)
            {
              if(refList[i]==offPOC)
              {
                newRef=true;
              }
            }
            for(Int i=0; i<newRefs; i++)
            {
              if(m_GOPList[m_iGOPSize+m_extraRPSs].m_referencePics[i]==offPOC-curPOC)
              {
                newRef=false;
              }
            }
            if(newRef)
            {
              Int insertPoint=newRefs;
              //this picture can be added, find appropriate place in list and insert it.
              if(m_GOPList[offGOP].m_temporalId==m_GOPList[curGOP].m_temporalId)
              {
                m_GOPList[offGOP].m_refPic = true;
              }
              for(Int j=0; j<newRefs; j++)
              {
                if(m_GOPList[m_iGOPSize+m_extraRPSs].m_referencePics[j]<offPOC-curPOC||m_GOPList[m_iGOPSize+m_extraRPSs].m_referencePics[j]>0)
                {
                  insertPoint = j;
                  break;
                }
              }
              Int prev = offPOC-curPOC;
              Int prevUsed = m_GOPList[offGOP].m_temporalId<=m_GOPList[curGOP].m_temporalId;
              for(Int j=insertPoint; j<newRefs+1; j++)
              {
                Int newPrev = m_GOPList[m_iGOPSize+m_extraRPSs].m_referencePics[j];
                Int newUsed = m_GOPList[m_iGOPSize+m_extraRPSs].m_usedByCurrPic[j];
                m_GOPList[m_iGOPSize+m_extraRPSs].m_referencePics[j]=prev;
                m_GOPList[m_iGOPSize+m_extraRPSs].m_usedByCurrPic[j]=prevUsed;
                prevUsed=newUsed;
                prev=newPrev;
              }
              newRefs++;
            }
          }
          if(newRefs>=numPrefRefs)
          {
            break;
          }
        }
        m_GOPList[m_iGOPSize+m_extraRPSs].m_numRefPics=newRefs;
        m_GOPList[m_iGOPSize+m_extraRPSs].m_POC = curPOC;
        if (m_extraRPSs == 0)
        {
          m_GOPList[m_iGOPSize+m_extraRPSs].m_interRPSPrediction = 0;
          m_GOPList[m_iGOPSize+m_extraRPSs].m_numRefIdc = 0;
        }
        else
        {
          Int rIdx =  m_iGOPSize + m_extraRPSs - 1;
          Int refPOC = m_GOPList[rIdx].m_POC;
          Int refPics = m_GOPList[rIdx].m_numRefPics;
          Int newIdc=0;
          for(Int i = 0; i<= refPics; i++)
          {
            Int deltaPOC = ((i != refPics)? m_GOPList[rIdx].m_referencePics[i] : 0);  // check if the reference abs POC is >= 0
            Int absPOCref = refPOC+deltaPOC;
            Int refIdc = 0;
            for (Int j = 0; j < m_GOPList[m_iGOPSize+m_extraRPSs].m_numRefPics; j++)
            {
              if ( (absPOCref - curPOC) == m_GOPList[m_iGOPSize+m_extraRPSs].m_referencePics[j])
              {
                if (m_GOPList[m_iGOPSize+m_extraRPSs].m_usedByCurrPic[j])
                {
                  refIdc = 1;
                }
                else
                {
                  refIdc = 2;
                }
              }
            }
            m_GOPList[m_iGOPSize+m_extraRPSs].m_refIdc[newIdc]=refIdc;
            newIdc++;
          }
          m_GOPList[m_iGOPSize+m_extraRPSs].m_interRPSPrediction = 1;
          m_GOPList[m_iGOPSize+m_extraRPSs].m_numRefIdc = newIdc;
          m_GOPList[m_iGOPSize+m_extraRPSs].m_deltaRPS = refPOC - m_GOPList[m_iGOPSize+m_extraRPSs].m_POC;
        }
        curGOP=m_iGOPSize+m_extraRPSs;
        m_extraRPSs++;
      }
      numRefs=0;
      for(Int i = 0; i< m_GOPList[curGOP].m_numRefPics; i++)
      {
        Int absPOC = curPOC+m_GOPList[curGOP].m_referencePics[i];
        if(absPOC >= 0)
        {
          refList[numRefs]=absPOC;
          numRefs++;
        }
      }
      refList[numRefs]=curPOC;
      numRefs++;
    }
    checkGOP++;
  }
  xConfirmPara(errorGOP,"Invalid GOP structure given");
  m_maxTempLayer = 1;
  for(Int i=0; i<m_iGOPSize; i++)
  {
    if(m_GOPList[i].m_temporalId >= m_maxTempLayer)
    {
      m_maxTempLayer = m_GOPList[i].m_temporalId+1;
    }
    xConfirmPara(m_GOPList[i].m_sliceType!='B' && m_GOPList[i].m_sliceType!='P' && m_GOPList[i].m_sliceType!='I', "Slice type must be equal to B or P or I");
  }
  for(Int i=0; i<MAX_TLAYER; i++)
  {
    m_numReorderPics[i] = 0;
    m_maxDecPicBuffering[i] = 1;
  }
  for(Int i=0; i<m_iGOPSize; i++)
  {
    if(m_GOPList[i].m_numRefPics+1 > m_maxDecPicBuffering[m_GOPList[i].m_temporalId])
    {
      m_maxDecPicBuffering[m_GOPList[i].m_temporalId] = m_GOPList[i].m_numRefPics + 1;
    }
    Int highestDecodingNumberWithLowerPOC = 0;
    for(Int j=0; j<m_iGOPSize; j++)
    {
      if(m_GOPList[j].m_POC <= m_GOPList[i].m_POC)
      {
        highestDecodingNumberWithLowerPOC = j;
      }
    }
    Int numReorder = 0;
    for(Int j=0; j<highestDecodingNumberWithLowerPOC; j++)
    {
      if(m_GOPList[j].m_temporalId <= m_GOPList[i].m_temporalId &&
        m_GOPList[j].m_POC > m_GOPList[i].m_POC)
      {
        numReorder++;
      }
    }
    if(numReorder > m_numReorderPics[m_GOPList[i].m_temporalId])
    {
      m_numReorderPics[m_GOPList[i].m_temporalId] = numReorder;
    }
  }
  for(Int i=0; i<MAX_TLAYER-1; i++)
  {
    // a lower layer can not have higher value of m_numReorderPics than a higher layer
    if(m_numReorderPics[i+1] < m_numReorderPics[i])
    {
      m_numReorderPics[i+1] = m_numReorderPics[i];
    }
    // the value of num_reorder_pics[ i ] shall be in the range of 0 to max_dec_pic_buffering[ i ] - 1, inclusive
    if(m_numReorderPics[i] > m_maxDecPicBuffering[i] - 1)
    {
      m_maxDecPicBuffering[i] = m_numReorderPics[i] + 1;
    }
    // a lower layer can not have higher value of m_uiMaxDecPicBuffering than a higher layer
    if(m_maxDecPicBuffering[i+1] < m_maxDecPicBuffering[i])
    {
      m_maxDecPicBuffering[i+1] = m_maxDecPicBuffering[i];
    }
  }

  // the value of num_reorder_pics[ i ] shall be in the range of 0 to max_dec_pic_buffering[ i ] -  1, inclusive
  if(m_numReorderPics[MAX_TLAYER-1] > m_maxDecPicBuffering[MAX_TLAYER-1] - 1)
  {
    m_maxDecPicBuffering[MAX_TLAYER-1] = m_numReorderPics[MAX_TLAYER-1] + 1;
  }

  if(m_vuiParametersPresentFlag && m_bitstreamRestrictionFlag)
  {
    Int PicSizeInSamplesY =  m_iSourceWidth * m_iSourceHeight;
    if(tileFlag)
    {
      Int maxTileWidth = 0;
      Int maxTileHeight = 0;
      Int widthInCU = (m_iSourceWidth % m_uiMaxCUWidth) ? m_iSourceWidth/m_uiMaxCUWidth + 1: m_iSourceWidth/m_uiMaxCUWidth;
      Int heightInCU = (m_iSourceHeight % m_uiMaxCUHeight) ? m_iSourceHeight/m_uiMaxCUHeight + 1: m_iSourceHeight/m_uiMaxCUHeight;
      if(m_iUniformSpacingIdr)
      {
        maxTileWidth = m_uiMaxCUWidth*((widthInCU+m_iNumColumnsMinus1)/(m_iNumColumnsMinus1+1));
        maxTileHeight = m_uiMaxCUHeight*((heightInCU+m_iNumRowsMinus1)/(m_iNumRowsMinus1+1));
        // if only the last tile-row is one treeblock higher than the others
        // the maxTileHeight becomes smaller if the last row of treeblocks has lower height than the others
        if(!((heightInCU-1)%(m_iNumRowsMinus1+1)))
        {
          maxTileHeight = maxTileHeight - m_uiMaxCUHeight + (m_iSourceHeight % m_uiMaxCUHeight);
        }
        // if only the last tile-column is one treeblock wider than the others
        // the maxTileWidth becomes smaller if the last column of treeblocks has lower width than the others
        if(!((widthInCU-1)%(m_iNumColumnsMinus1+1)))
        {
          maxTileWidth = maxTileWidth - m_uiMaxCUWidth + (m_iSourceWidth % m_uiMaxCUWidth);
        }
      }
      else // not uniform spacing
      {
        if(m_iNumColumnsMinus1<1)
        {
          maxTileWidth = m_iSourceWidth;
        }
        else
        {
          Int accColumnWidth = 0;
          for(Int col=0; col<(m_iNumColumnsMinus1); col++)
          {
            maxTileWidth = m_pColumnWidth[col]>maxTileWidth ? m_pColumnWidth[col]:maxTileWidth;
            accColumnWidth += m_pColumnWidth[col];
          }
          maxTileWidth = (widthInCU-accColumnWidth)>maxTileWidth ? m_uiMaxCUWidth*(widthInCU-accColumnWidth):m_uiMaxCUWidth*maxTileWidth;
        }
        if(m_iNumRowsMinus1<1)
        {
          maxTileHeight = m_iSourceHeight;
        }
        else
        {
          Int accRowHeight = 0;
          for(Int row=0; row<(m_iNumRowsMinus1); row++)
          {
            maxTileHeight = m_pRowHeight[row]>maxTileHeight ? m_pRowHeight[row]:maxTileHeight;
            accRowHeight += m_pRowHeight[row];
          }
          maxTileHeight = (heightInCU-accRowHeight)>maxTileHeight ? m_uiMaxCUHeight*(heightInCU-accRowHeight):m_uiMaxCUHeight*maxTileHeight;
        }
      }
      Int maxSizeInSamplesY = maxTileWidth*maxTileHeight;
      m_minSpatialSegmentationIdc = 4*PicSizeInSamplesY/maxSizeInSamplesY-4;
    }
    else if(m_iWaveFrontSynchro)
    {
      m_minSpatialSegmentationIdc = 4*PicSizeInSamplesY/((2*m_iSourceHeight+m_iSourceWidth)*m_uiMaxCUHeight)-4;
    }
    else if(m_sliceMode == 1)
    {
      m_minSpatialSegmentationIdc = 4*PicSizeInSamplesY/(m_sliceArgument*m_uiMaxCUWidth*m_uiMaxCUHeight)-4;
    }
    else
    {
      m_minSpatialSegmentationIdc = 0;
    }
  }

  xConfirmPara( m_iWaveFrontSynchro < 0, "WaveFrontSynchro cannot be negative" );
  xConfirmPara( m_iWaveFrontSubstreams <= 0, "WaveFrontSubstreams must be positive" );
  xConfirmPara( m_iWaveFrontSubstreams > 1 && !m_iWaveFrontSynchro, "Must have WaveFrontSynchro > 0 in order to have WaveFrontSubstreams > 1" );

  xConfirmPara( m_decodedPictureHashSEIEnabled<0 || m_decodedPictureHashSEIEnabled>3, "this hash type is not correct!\n");

  if (m_toneMappingInfoSEIEnabled)
  {
    xConfirmPara( m_toneMapCodedDataBitDepth < 8 || m_toneMapCodedDataBitDepth > 14 , "SEIToneMapCodedDataBitDepth must be in rage 8 to 14");
    xConfirmPara( m_toneMapTargetBitDepth < 1 || (m_toneMapTargetBitDepth > 16 && m_toneMapTargetBitDepth < 255) , "SEIToneMapTargetBitDepth must be in rage 1 to 16 or equal to 255");
    xConfirmPara( m_toneMapModelId < 0 || m_toneMapModelId > 4 , "SEIToneMapModelId must be in rage 0 to 4");
    xConfirmPara( m_cameraIsoSpeedValue == 0, "SEIToneMapCameraIsoSpeedValue shall not be equal to 0");
    xConfirmPara( m_exposureIndexValue  == 0, "SEIToneMapExposureIndexValue shall not be equal to 0");
    xConfirmPara( m_extendedRangeWhiteLevel < 100, "SEIToneMapExtendedRangeWhiteLevel should be greater than or equal to 100");
    xConfirmPara( m_nominalBlackLevelLumaCodeValue >= m_nominalWhiteLevelLumaCodeValue, "SEIToneMapNominalWhiteLevelLumaCodeValue shall be greater than SEIToneMapNominalBlackLevelLumaCodeValue");
    xConfirmPara( m_extendedWhiteLevelLumaCodeValue < m_nominalWhiteLevelLumaCodeValue, "SEIToneMapExtendedWhiteLevelLumaCodeValue shall be greater than or equal to SEIToneMapNominalWhiteLevelLumaCodeValue");
  }

  if (m_kneeSEIEnabled && !m_kneeSEICancelFlag)
  {
    xConfirmPara( m_kneeSEINumKneePointsMinus1 < 0 || m_kneeSEINumKneePointsMinus1 > 998, "SEIKneeFunctionNumKneePointsMinus1 must be in the range of 0 to 998");
    for ( UInt i=0; i<=m_kneeSEINumKneePointsMinus1; i++ ){
      xConfirmPara( m_kneeSEIInputKneePoint[i] < 1 || m_kneeSEIInputKneePoint[i] > 999, "SEIKneeFunctionInputKneePointValue must be in the range of 1 to 999");
      xConfirmPara( m_kneeSEIOutputKneePoint[i] < 0 || m_kneeSEIOutputKneePoint[i] > 1000, "SEIKneeFunctionInputKneePointValue must be in the range of 0 to 1000");
      if ( i > 0 )
      {
        xConfirmPara( m_kneeSEIInputKneePoint[i-1] >= m_kneeSEIInputKneePoint[i],  "The i-th SEIKneeFunctionInputKneePointValue must be greater than the (i-1)-th value");
      }
    }
  }

  if ( m_RCEnableRateControl )
  {
    if ( m_RCForceIntraQP )
    {
      if ( m_RCInitialQP == 0 )
      {
        printf( "\nInitial QP for rate control is not specified. Reset not to use force intra QP!" );
        m_RCForceIntraQP = false;
      }
    }
    xConfirmPara( m_uiDeltaQpRD > 0, "Rate control cannot be used together with slice level multiple-QP optimization!\n" );
  }

  xConfirmPara(!m_TransquantBypassEnableFlag && m_CUTransquantBypassFlagForce, "CUTransquantBypassFlagForce cannot be 1 when TransquantBypassEnableFlag is 0");

  xConfirmPara(m_log2ParallelMergeLevel < 2, "Log2ParallelMergeLevel should be larger than or equal to 2");

  if (m_framePackingSEIEnabled)
  {
    xConfirmPara(m_framePackingSEIType < 3 || m_framePackingSEIType > 5 , "SEIFramePackingType must be in rage 3 to 5");
  }

  if((m_iNumColumnsMinus1<=0 && m_iNumRowsMinus1<=0) && m_tmctsSEIEnabled)
  {
    printf("SEITempMotionConstrainedTileSets is set to false to disable 'temporal_motion_constrained_tile_sets' SEI because there are no tiles enabled\n");
    m_tmctsSEIEnabled = false;
  }

#undef xConfirmPara
  if (check_failed)
  {
    exit(EXIT_FAILURE);
  }
}

/** \todo use of global variables should be removed later
 */
Void TAppEncCfg::xSetGlobal()
{
  // set max CU width & height
  g_uiMaxCUWidth  = m_uiMaxCUWidth;
  g_uiMaxCUHeight = m_uiMaxCUHeight;

  // compute actual CU depth with respect to config depth and max transform size
  g_uiAddCUDepth  = 0;
  while( (m_uiMaxCUWidth>>m_uiMaxCUDepth) > ( 1 << ( m_uiQuadtreeTULog2MinSize + g_uiAddCUDepth )  ) ) g_uiAddCUDepth++;

  g_uiAddCUDepth+=getMaxCUDepthOffset(m_chromaFormatIDC, m_uiQuadtreeTULog2MinSize); // NOTE: RExt - if minimum TU larger than 4x4, allow for additional part indices for 4:2:2 SubTUs.

  m_uiMaxCUDepth += g_uiAddCUDepth;
  g_uiAddCUDepth++;
  g_uiMaxCUDepth = m_uiMaxCUDepth;

  // set internal bit-depth and constants
  for (UInt channelType = 0; channelType < MAX_NUM_CHANNEL_TYPE; channelType++)
  {
#if RExt__O0043_BEST_EFFORT_DECODING
    g_bitDepthInStream[channelType] = g_bitDepth[channelType] = m_internalBitDepth[channelType];
#else
    g_bitDepth   [channelType] = m_internalBitDepth[channelType];
#endif
    g_PCMBitDepth[channelType] = m_bPCMInputBitDepthFlag ? m_MSBExtendedBitDepth[channelType] : m_internalBitDepth[channelType];

    if (m_useExtendedPrecision) g_maxTrDynamicRange[channelType] = std::max<Int>(15, (g_bitDepth[channelType] + 6));
    else                        g_maxTrDynamicRange[channelType] = 15;
  }
}

const Char *profileToString(const Profile::Name profile)
{
  static const UInt numberOfProfiles = sizeof(strToProfile)/sizeof(*strToProfile);

  for (UInt profileIndex = 0; profileIndex < numberOfProfiles; profileIndex++)
  {
    if (strToProfile[profileIndex].value == profile) return strToProfile[profileIndex].str;
  }

  //if we get here, we didn't find this profile in the list - so there is an error
  std::cerr << "ERROR: Unknown profile \"" << profile << "\" in profileToString" << std::endl;
  assert(false);
  exit(1);
  return "";
}

Void TAppEncCfg::xPrintParameter()
{
  printf("\n");
  printf("Input          File               : %s\n", m_pchInputFile          );
  printf("Bitstream      File               : %s\n", m_pchBitstreamFile      );
  printf("Reconstruction File               : %s\n", m_pchReconFile          );
  printf("Real     Format                   : %dx%d %dHz\n", m_iSourceWidth - m_confLeft - m_confRight, m_iSourceHeight - m_confTop - m_confBottom, m_iFrameRate );
  printf("Internal Format                   : %dx%d %dHz\n", m_iSourceWidth, m_iSourceHeight, m_iFrameRate );
  printf("Sequence PSNR output              : %s\n", (m_printMSEBasedSequencePSNR ? "Linear average, MSE-based" : "Linear average only") );
  printf("Sequence MSE output               : %s\n", (m_printSequenceMSE ? "Enabled" : "Disabled") );
  printf("Frame MSE output                  : %s\n", (m_printFrameMSE    ? "Enabled" : "Disabled") );
  if (m_isField)
  {
    printf("Frame/Field                       : Field based coding\n");
    printf("Field index                       : %u - %d (%d fields)\n", m_FrameSkip, m_FrameSkip+m_framesToBeEncoded-1, m_framesToBeEncoded );
    printf("Field Order                       : %s field first\n", m_isTopFieldFirst?"Top":"Bottom");

  }
  else
  {
    printf("Frame/Field                       : Frame based coding\n");
    printf("Frame index                       : %u - %d (%d frames)\n", m_FrameSkip, m_FrameSkip+m_framesToBeEncoded-1, m_framesToBeEncoded );
  }
  if (m_profile == Profile::MAINREXT)
  {
    const UInt intraIdx = m_intraConstraintFlag ? 1:0;
    const UInt bitDepthIdx = (m_bitDepthConstraint == 8 ? 0 : (m_bitDepthConstraint ==10 ? 1 : (m_bitDepthConstraint == 12 ? 2 : (m_bitDepthConstraint == 16 ? 3 : 4 ))));
    const UInt chromaFormatIdx = UInt(m_chromaFormatConstraint);
    const ExtendedProfileName validProfileName = (bitDepthIdx > 3 || chromaFormatIdx>3) ? NONE : validRExtProfileNames[intraIdx][bitDepthIdx][chromaFormatIdx];
    std::string rextSubProfile;
    if (validProfileName!=NONE) rextSubProfile=enumToString(strToExtendedProfile, sizeof(strToExtendedProfile)/sizeof(*strToExtendedProfile), validProfileName);
    if (rextSubProfile == "main_444_16") rextSubProfile="main_444_16 [NON STANDARD]";
    printf("Profile                           : %s (%s)\n", profileToString(m_profile), (rextSubProfile.empty())?"INVALID REXT PROFILE":rextSubProfile.c_str() );
  }
  else
  {
    printf("Profile                           : %s\n", profileToString(m_profile) );
  }
  printf("CU size / depth                   : %d / %d\n", m_uiMaxCUWidth, m_uiMaxCUDepth );
  printf("RQT trans. size (min / max)       : %d / %d\n", 1 << m_uiQuadtreeTULog2MinSize, 1 << m_uiQuadtreeTULog2MaxSize );
  printf("Max RQT depth inter               : %d\n", m_uiQuadtreeTUMaxDepthInter);
  printf("Max RQT depth intra               : %d\n", m_uiQuadtreeTUMaxDepthIntra);
  printf("Min PCM size                      : %d\n", 1 << m_uiPCMLog2MinSize);
  printf("Motion search range               : %d\n", m_iSearchRange );
  printf("Inter search component loop       : %s\n", (m_singleComponentLoopInterSearch ? "Single (Non-HM-compatible)" : "Triple (HM-compatible)") );
  printf("Intra period                      : %d\n", m_iIntraPeriod );
  printf("Decoding refresh type             : %d\n", m_iDecodingRefreshType );
  printf("QP                                : %5.2f\n", m_fQP );
  printf("Max dQP signaling depth           : %d\n", m_iMaxCuDQPDepth);

  printf("Cb QP Offset                      : %d\n", m_cbQpOffset   );
  printf("Cr QP Offset                      : %d\n", m_crQpOffset);
  printf("Max CU chroma QP adjustment depth : %d\n", m_maxCUChromaQpAdjustmentDepth);
  printf("QP adaptation                     : %d (range=%d)\n", m_bUseAdaptiveQP, (m_bUseAdaptiveQP ? m_iQPAdaptationRange : 0) );
  printf("GOP size                          : %d\n", m_iGOPSize );
  printf("Input bit depth                   : (Y:%d, C:%d)\n", m_inputBitDepth[CHANNEL_TYPE_LUMA], m_inputBitDepth[CHANNEL_TYPE_CHROMA] );
  printf("MSB-extended bit depth            : (Y:%d, C:%d)\n", m_MSBExtendedBitDepth[CHANNEL_TYPE_LUMA], m_MSBExtendedBitDepth[CHANNEL_TYPE_CHROMA] );
  printf("Internal bit depth                : (Y:%d, C:%d)\n", m_internalBitDepth[CHANNEL_TYPE_LUMA], m_internalBitDepth[CHANNEL_TYPE_CHROMA] );
  printf("PCM sample bit depth              : (Y:%d, C:%d)\n", g_PCMBitDepth[CHANNEL_TYPE_LUMA],      g_PCMBitDepth[CHANNEL_TYPE_CHROMA] );
  printf("Extended precision processing     : %s\n", (m_useExtendedPrecision                   ? "Enabled" : "Disabled") );
  printf("Intra reference smoothing         : %s\n", (m_enableIntraReferenceSmoothing          ? "Enabled" : "Disabled") );
  printf("Implicit residual DPCM            : %s\n", (m_useResidualDPCM[RDPCM_SIGNAL_IMPLICIT] ? "Enabled" : "Disabled") );
  printf("Explicit residual DPCM            : %s\n", (m_useResidualDPCM[RDPCM_SIGNAL_EXPLICIT] ? "Enabled" : "Disabled") );
  printf("Residual rotation                 : %s\n", (m_useResidualRotation                    ? "Enabled" : "Disabled") );
  printf("Single significance map context   : %s\n", (m_useSingleSignificanceMapContext        ? "Enabled" : "Disabled") );
  printf("Cross-component prediction        : %s\n", (m_useCrossComponentPrediction            ? (m_reconBasedCrossCPredictionEstimate ? "Enabled (reconstructed-residual-based estimate)" : "Enabled (encoder-side-residual-based estimate)") : "Disabled") );
  printf("High-precision prediction weight  : %s\n", (m_useHighPrecisionPredictionWeighting    ? "Enabled" : "Disabled") );
  printf("Golomb-Rice parameter adaptation  : %s\n", (m_useGolombRiceParameterAdaptation       ? "Enabled" : "Disabled") );
  printf("CABAC bypass bit alignment        : %s\n", (m_alignCABACBeforeBypass                 ? "Enabled" : "Disabled") );
  if (m_bUseSAO)
  {
    printf("Sao Luma Offset bit shifts        : %d\n", m_saoOffsetBitShift[CHANNEL_TYPE_LUMA]);
    printf("Sao Chroma Offset bit shifts      : %d\n", m_saoOffsetBitShift[CHANNEL_TYPE_CHROMA]);
  }

  switch (m_costMode)
  {
    case COST_STANDARD_LOSSY:               printf("Cost function:                    : Lossy coding (default)\n"); break;
    case COST_SEQUENCE_LEVEL_LOSSLESS:      printf("Cost function:                    : Sequence_level_lossless coding\n"); break;
    case COST_LOSSLESS_CODING:              printf("Cost function:                    : Lossless coding with fixed QP of %d\n", RExt__LOSSLESS_AND_MIXED_LOSSLESS_RD_COST_TEST_QP); break;
    case COST_MIXED_LOSSLESS_LOSSY_CODING:  printf("Cost function:                    : Mixed_lossless_lossy coding with QP'=%d for lossless evaluation\n", RExt__LOSSLESS_AND_MIXED_LOSSLESS_RD_COST_TEST_QP_PRIME); break;
    default:                                printf("Cost function:                    : Unknown\n"); break;
  }

  printf("RateControl                       : %d\n", m_RCEnableRateControl );

  if(m_RCEnableRateControl)
  {
    printf("TargetBitrate                     : %d\n", m_RCTargetBitrate );
    printf("KeepHierarchicalBit               : %d\n", m_RCKeepHierarchicalBit );
    printf("LCULevelRC                        : %d\n", m_RCLCULevelRC );
    printf("UseLCUSeparateModel               : %d\n", m_RCUseLCUSeparateModel );
    printf("InitialQP                         : %d\n", m_RCInitialQP );
    printf("ForceIntraQP                      : %d\n", m_RCForceIntraQP );
  }

  printf("Max Num Merge Candidates          : %d\n", m_maxNumMergeCand);
  printf("\n");

  printf("TOOL CFG: ");
  printf("IBD:%d ", ((g_bitDepth[CHANNEL_TYPE_LUMA] > m_MSBExtendedBitDepth[CHANNEL_TYPE_LUMA]) || (g_bitDepth[CHANNEL_TYPE_CHROMA] > m_MSBExtendedBitDepth[CHANNEL_TYPE_CHROMA])));
  printf("HAD:%d ", m_bUseHADME           );
  printf("RDQ:%d ", m_useRDOQ            );
  printf("RDQTS:%d ", m_useRDOQTS        );
  printf("RDpenalty:%d ", m_rdPenalty  );
  printf("SQP:%d ", m_uiDeltaQpRD         );
  printf("ASR:%d ", m_bUseASR             );
  printf("FEN:%d ", m_bUseFastEnc         );
  printf("ECU:%d ", m_bUseEarlyCU         );
  printf("FDM:%d ", m_useFastDecisionForMerge );
  printf("CFM:%d ", m_bUseCbfFastMode         );
  printf("ESD:%d ", m_useEarlySkipDetection  );
  printf("RQT:%d ", 1     );
  printf("TransformSkip:%d ",     m_useTransformSkip              );
  printf("TransformSkipFast:%d ", m_useTransformSkipFast       );
  printf("TransformSkipLog2MaxSize:%d ", m_transformSkipLog2MaxSize);
  printf("Slice: M=%d ", m_sliceMode);
  if (m_sliceMode!=0)
  {
    printf("A=%d ", m_sliceArgument);
  }
  printf("SliceSegment: M=%d ",m_sliceSegmentMode);
  if (m_sliceSegmentMode!=0)
  {
    printf("A=%d ", m_sliceSegmentArgument);
  }
  printf("CIP:%d ", m_bUseConstrainedIntraPred);
  printf("SAO:%d ", (m_bUseSAO)?(1):(0));
  printf("PCM:%d ", (m_usePCM && (1<<m_uiPCMLog2MinSize) <= m_uiMaxCUWidth)? 1 : 0);

  if (m_TransquantBypassEnableFlag && m_CUTransquantBypassFlagForce)
  {
    printf("TransQuantBypassEnabled: =1");
  }
  else
  {
    printf("TransQuantBypassEnabled:%d ", (m_TransquantBypassEnableFlag)? 1:0 );
  }

  printf("WPP:%d ", (Int)m_useWeightedPred);
  printf("WPB:%d ", (Int)m_useWeightedBiPred);
  printf("PME:%d ", m_log2ParallelMergeLevel);
  printf(" WaveFrontSynchro:%d WaveFrontSubstreams:%d",
          m_iWaveFrontSynchro, m_iWaveFrontSubstreams);
  printf(" ScalingList:%d ", m_useScalingListId );
  printf("TMVPMode:%d ", m_TMVPModeId     );
#if ADAPTIVE_QP_SELECTION
  printf("AQpS:%d", m_bUseAdaptQpSelect   );
#endif

  printf(" SignBitHidingFlag:%d ", m_signHideFlag);
  printf("RecalQP:%d", m_recalculateQPAccordingToLambda ? 1 : 0 );

  printf("\n\n");

  fflush(stdout);
}

Bool confirmPara(Bool bflag, const Char* message)
{
  if (!bflag)
    return false;

  printf("Error: %s\n",message);
  return true;
}

//! \}
