/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef GEN_BUILDER_META_HPP
#define GEN_BUILDER_META_HPP

Value *VGATHERPD(Value *Src, Value *Base, Value *Indices, Value *Mask,
                 Value *Scale, const llvm::Twine &Name = "") {
  SmallVector<Type *, 5> ArgTypes;
  ArgTypes.push_back(Src->getType());
  ArgTypes.push_back(Base->getType());
  ArgTypes.push_back(Indices->getType());
  ArgTypes.push_back(Mask->getType());
  ArgTypes.push_back(Scale->getType());
  auto *FTy = FunctionType::get(Src->getType(), ArgTypes, false);
  auto *F =
      cast<Function>(M->getOrInsertFunction("meta.intrinsic.VGATHERPD", FTy));
  return CALL(
      F, std::initializer_list<Value *>{Src, Base, Indices, Mask, Scale}, Name);
}

Value *VGATHERPS(Value *Src, Value *Base, Value *Indices, Value *Mask,
                 Value *Scale, const llvm::Twine &Name = "") {
  SmallVector<Type *, 5> ArgTypes;
  ArgTypes.push_back(Src->getType());
  ArgTypes.push_back(Base->getType());
  ArgTypes.push_back(Indices->getType());
  ArgTypes.push_back(Mask->getType());
  ArgTypes.push_back(Scale->getType());
  auto *FTy = FunctionType::get(Src->getType(), ArgTypes, false);
  auto *F =
      cast<Function>(M->getOrInsertFunction("meta.intrinsic.VGATHERPS", FTy));
  return CALL(
      F, std::initializer_list<Value *>{Src, Base, Indices, Mask, Scale}, Name);
}

Value *VGATHERDD(Value *Src, Value *Base, Value *Indices, Value *Mask,
                 Value *Scale, const llvm::Twine &Name = "") {
  SmallVector<Type *, 5> ArgTypes;
  ArgTypes.push_back(Src->getType());
  ArgTypes.push_back(Base->getType());
  ArgTypes.push_back(Indices->getType());
  ArgTypes.push_back(Mask->getType());
  ArgTypes.push_back(Scale->getType());
  auto *FTy = FunctionType::get(Src->getType(), ArgTypes, false);
  auto *F =
      cast<Function>(M->getOrInsertFunction("meta.intrinsic.VGATHERDD", FTy));
  return CALL(
      F, std::initializer_list<Value *>{Src, Base, Indices, Mask, Scale}, Name);
}

Value *VRCPPS(Value *A, const llvm::Twine &Name = "") {
  SmallVector<Type *, 1> ArgTypes;
  ArgTypes.push_back(A->getType());
  auto *FTy = FunctionType::get(A->getType(), ArgTypes, false);
  auto *F =
      cast<Function>(M->getOrInsertFunction("meta.intrinsic.VRCPPS", FTy));
  return CALL(F, std::initializer_list<Value *>{A}, Name);
}

Value *VROUND(Value *A, Value *Rounding, const llvm::Twine &Name = "") {
  SmallVector<Type *, 2> ArgTypes;
  ArgTypes.push_back(A->getType());
  ArgTypes.push_back(Rounding->getType());
  auto *FTy = FunctionType::get(A->getType(), ArgTypes, false);
  auto *F =
      cast<Function>(M->getOrInsertFunction("meta.intrinsic.VROUND", FTy));
  return CALL(F, std::initializer_list<Value *>{A, Rounding}, Name);
}

Value *BEXTR_32(Value *Src, Value *Control, const llvm::Twine &Name = "") {
  SmallVector<Type *, 2> ArgTypes;
  ArgTypes.push_back(Src->getType());
  ArgTypes.push_back(Control->getType());
  auto *FTy = FunctionType::get(Src->getType(), ArgTypes, false);
  auto *F =
      cast<Function>(M->getOrInsertFunction("meta.intrinsic.BEXTR_32", FTy));
  return CALL(F, std::initializer_list<Value *>{Src, Control}, Name);
}

Value *VPSHUFB(Value *A, Value *B, const llvm::Twine &Name = "") {
  SmallVector<Type *, 2> ArgTypes;
  ArgTypes.push_back(A->getType());
  ArgTypes.push_back(B->getType());
  auto *FTy = FunctionType::get(A->getType(), ArgTypes, false);
  auto *F =
      cast<Function>(M->getOrInsertFunction("meta.intrinsic.VPSHUFB", FTy));
  return CALL(F, std::initializer_list<Value *>{A, B}, Name);
}

Value *VPERMD(Value *A, Value *Idx, const llvm::Twine &Name = "") {
  SmallVector<Type *, 2> ArgTypes;
  ArgTypes.push_back(A->getType());
  ArgTypes.push_back(Idx->getType());
  auto *FTy = FunctionType::get(A->getType(), ArgTypes, false);
  auto *F =
      cast<Function>(M->getOrInsertFunction("meta.intrinsic.VPERMD", FTy));
  return CALL(F, std::initializer_list<Value *>{A, Idx}, Name);
}

Value *VPERMPS(Value *Idx, Value *A, const llvm::Twine &Name = "") {
  SmallVector<Type *, 2> ArgTypes;
  ArgTypes.push_back(Idx->getType());
  ArgTypes.push_back(A->getType());
  auto *FTy = FunctionType::get(A->getType(), ArgTypes, false);
  auto *F =
      cast<Function>(M->getOrInsertFunction("meta.intrinsic.VPERMPS", FTy));
  return CALL(F, std::initializer_list<Value *>{Idx, A}, Name);
}

Value *VCVTPD2PS(Value *A, const llvm::Twine &Name = "") {
  SmallVector<Type *, 1> ArgTypes;
  ArgTypes.push_back(A->getType());
  auto *FTy = FunctionType::get(
      IGCLLVM::FixedVectorType::get(
          FP32Ty,
          cast<IGCLLVM::FixedVectorType>(A->getType())->getNumElements()),
      ArgTypes, false);
  auto *F =
      cast<Function>(M->getOrInsertFunction("meta.intrinsic.VCVTPD2PS", FTy));
  return CALL(F, std::initializer_list<Value *>{A}, Name);
}

Value *VCVTPH2PS(Value *A, const llvm::Twine &Name = "") {
  SmallVector<Type *, 1> ArgTypes;
  ArgTypes.push_back(A->getType());
  auto *FTy = FunctionType::get(
      IGCLLVM::FixedVectorType::get(
          FP32Ty,
          cast<IGCLLVM::FixedVectorType>(A->getType())->getNumElements()),
      ArgTypes, false);
  auto *F =
      cast<Function>(M->getOrInsertFunction("meta.intrinsic.VCVTPH2PS", FTy));
  return CALL(F, std::initializer_list<Value *>{A}, Name);
}

Value *VCVTPS2PH(Value *A, Value *Round, const llvm::Twine &Name = "") {
  SmallVector<Type *, 2> ArgTypes;
  ArgTypes.push_back(A->getType());
  ArgTypes.push_back(Round->getType());
  auto *FTy = FunctionType::get(SimdInt16Ty, ArgTypes, false);
  auto *F =
      cast<Function>(M->getOrInsertFunction("meta.intrinsic.VCVTPS2PH", FTy));
  return CALL(F, std::initializer_list<Value *>{A, Round}, Name);
}

Value *VHSUBPS(Value *A, Value *B, const llvm::Twine &Name = "") {
  SmallVector<Type *, 2> ArgTypes;
  ArgTypes.push_back(A->getType());
  ArgTypes.push_back(B->getType());
  auto *FTy = FunctionType::get(A->getType(), ArgTypes, false);
  auto *F =
      cast<Function>(M->getOrInsertFunction("meta.intrinsic.VHSUBPS", FTy));
  return CALL(F, std::initializer_list<Value *>{A, B}, Name);
}

Value *VPTESTC(Value *A, Value *B, const llvm::Twine &Name = "") {
  SmallVector<Type *, 2> ArgTypes;
  ArgTypes.push_back(A->getType());
  ArgTypes.push_back(B->getType());
  auto *FTy = FunctionType::get(Int32Ty, ArgTypes, false);
  auto *F =
      cast<Function>(M->getOrInsertFunction("meta.intrinsic.VPTESTC", FTy));
  return CALL(F, std::initializer_list<Value *>{A, B}, Name);
}

Value *VPTESTZ(Value *A, Value *B, const llvm::Twine &Name = "") {
  SmallVector<Type *, 2> ArgTypes;
  ArgTypes.push_back(A->getType());
  ArgTypes.push_back(B->getType());
  auto *FTy = FunctionType::get(Int32Ty, ArgTypes, false);
  auto *F =
      cast<Function>(M->getOrInsertFunction("meta.intrinsic.VPTESTZ", FTy));
  return CALL(F, std::initializer_list<Value *>{A, B}, Name);
}

Value *VFMADDPS(Value *A, Value *B, Value *C, const llvm::Twine &Name = "") {
  SmallVector<Type *, 3> ArgTypes;
  ArgTypes.push_back(A->getType());
  ArgTypes.push_back(B->getType());
  ArgTypes.push_back(C->getType());
  auto *FTy = FunctionType::get(A->getType(), ArgTypes, false);
  auto *F =
      cast<Function>(M->getOrInsertFunction("meta.intrinsic.VFMADDPS", FTy));
  return CALL(F, std::initializer_list<Value *>{A, B, C}, Name);
}

Value *VPHADDD(Value *A, Value *B, const llvm::Twine &Name = "") {
  SmallVector<Type *, 2> ArgTypes;
  ArgTypes.push_back(A->getType());
  ArgTypes.push_back(B->getType());
  auto *FTy = FunctionType::get(A->getType(), ArgTypes, false);
  auto *F =
      cast<Function>(M->getOrInsertFunction("meta.intrinsic.VPHADDD", FTy));
  return CALL(F, std::initializer_list<Value *>{A, B}, Name);
}

Value *PDEP32(Value *A, Value *B, const llvm::Twine &Name = "") {
  SmallVector<Type *, 2> ArgTypes;
  ArgTypes.push_back(A->getType());
  ArgTypes.push_back(B->getType());
  auto *FTy = FunctionType::get(A->getType(), ArgTypes, false);
  auto *F =
      cast<Function>(M->getOrInsertFunction("meta.intrinsic.PDEP32", FTy));
  return CALL(F, std::initializer_list<Value *>{A, B}, Name);
}

Value *RDTSC(const llvm::Twine &Name = "") {
  auto *FTy = FunctionType::get(Int64Ty, {}, false);
  auto *F = cast<Function>(M->getOrInsertFunction("meta.intrinsic.RDTSC", FTy));
  return CALL(F, std::initializer_list<Value *>{}, Name);
}

#endif // GEN_BUILDER_META_HPP
