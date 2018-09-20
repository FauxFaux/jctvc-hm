# (echo 'cmake_minimum_required(VERSION 3.2)'; echo 'project(hm)'; echo 'include_directories(Lib App)'; echo 'add_executable(dec '; find App/TAppDecoder Lib/TLibCommon Lib/TLibDecoder Lib/TAppCommon Lib/TLibVideoIO -name \*.cpp; echo Lib/libmd5/libmd5.c; echo ')') > CMakeLists.txt && rm -rf build && mkdir build && (cd build && cmake .. && make -j$(nproc))

cmake_minimum_required(VERSION 3.2)

project(hm)

include_directories(Lib App)

add_executable(dec
        App/TAppDecoder/TAppDecCfg.cpp
        App/TAppDecoder/TAppDecTop.cpp
        App/TAppDecoder/decmain.cpp
        Lib/TLibCommon/ContextModel.cpp
        Lib/TLibCommon/ContextModel3DBuffer.cpp
        Lib/TLibCommon/Debug.cpp
        Lib/TLibCommon/SEI.cpp
        Lib/TLibCommon/TComBitStream.cpp
        Lib/TLibCommon/TComCABACTables.cpp
        Lib/TLibCommon/TComChromaFormat.cpp
        Lib/TLibCommon/TComDataCU.cpp
        Lib/TLibCommon/TComInterpolationFilter.cpp
        Lib/TLibCommon/TComLoopFilter.cpp
        Lib/TLibCommon/TComMotionInfo.cpp
        Lib/TLibCommon/TComPattern.cpp
        Lib/TLibCommon/TComPic.cpp
        Lib/TLibCommon/TComPicSym.cpp
        Lib/TLibCommon/TComPicYuv.cpp
        Lib/TLibCommon/TComPicYuvMD5.cpp
        Lib/TLibCommon/TComPrediction.cpp
        Lib/TLibCommon/TComRdCost.cpp
        Lib/TLibCommon/TComRdCostWeightPrediction.cpp
        Lib/TLibCommon/TComRom.cpp
        Lib/TLibCommon/TComSampleAdaptiveOffset.cpp
        Lib/TLibCommon/TComSlice.cpp
        Lib/TLibCommon/TComTU.cpp
        Lib/TLibCommon/TComTrQuant.cpp
        Lib/TLibCommon/TComWeightPrediction.cpp
        Lib/TLibCommon/TComYuv.cpp
        Lib/TLibDecoder/AnnexBread.cpp
        Lib/TLibDecoder/NALread.cpp
        Lib/TLibDecoder/SEIread.cpp
        Lib/TLibDecoder/SyntaxElementParser.cpp
        Lib/TLibDecoder/TDecBinCoderCABAC.cpp
        Lib/TLibDecoder/TDecCAVLC.cpp
        Lib/TLibDecoder/TDecConformance.cpp
        Lib/TLibDecoder/TDecCu.cpp
        Lib/TLibDecoder/TDecEntropy.cpp
        Lib/TLibDecoder/TDecGop.cpp
        Lib/TLibDecoder/TDecSbac.cpp
        Lib/TLibDecoder/TDecSlice.cpp
        Lib/TLibDecoder/TDecTop.cpp
        Lib/TAppCommon/program_options_lite.cpp
        Lib/TLibVideoIO/TVideoIOYuv.cpp
        Lib/libmd5/libmd5.c
        )
