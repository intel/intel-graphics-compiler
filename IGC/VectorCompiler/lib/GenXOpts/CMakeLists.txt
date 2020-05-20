set(OPT_SOURCES
  CMAnalysis/InstructionSimplifyGenX.cpp
  CMAnalysis/ConstantFoldingGenX.cpp
  CMTrans/CMABI.cpp
  CMTrans/CMImpParam.cpp
  CMTrans/CMKernelArgOffset.cpp
  CMTrans/CMRegion.cpp
  CMPacketize/GenXPacketize.cpp
  CMPacketize/PacketBuilder.cpp
  CMPacketize/PacketBuilder_math.cpp
  CMPacketize/PacketBuilder_mem.cpp
  CMPacketize/PacketBuilder_misc.cpp
  CMPacketize/WIAnalysis.cpp
)

add_library(VCTransforms ${OPT_SOURCES})
target_link_libraries(VCTransforms
  VCHeaders
  LLVMGenXIntrinsics
  )
