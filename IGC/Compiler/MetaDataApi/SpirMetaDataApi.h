/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

// This file is auto-generated, all changes must go through the MetaDataApiGenerator project.

#pragma once

#include "MetaDataApiUtils.h"

namespace IGC {
    namespace SPIRMD
    {
        //typedefs and forward declarations
        class VectorTypeHintMetaData;
        typedef MetaObjectHandle<VectorTypeHintMetaData> VectorTypeHintMetaDataHandle;

        class VersionMetaData;
        typedef MetaObjectHandle<VersionMetaData> VersionMetaDataHandle;

        class WorkGroupDimensionsMetaData;
        typedef MetaObjectHandle<WorkGroupDimensionsMetaData> WorkGroupDimensionsMetaDataHandle;

        class SubGroupDimensionsMetaData;
        typedef MetaObjectHandle<SubGroupDimensionsMetaData> SubGroupDimensionsMetaDataHandle;

        class WorkgroupWalkOrderMetaData;
        typedef MetaObjectHandle<WorkgroupWalkOrderMetaData> WorkgroupWalkOrderMetaDataHandle;

        class KernelMetaData;
        typedef MetaObjectHandle<KernelMetaData> KernelMetaDataHandle;

        typedef MetaDataList<VersionMetaDataHandle> OpenCLVersionsMetaDataList;
        typedef MetaObjectHandle<OpenCLVersionsMetaDataList> OpenCLVersionsMetaDataListHandle;

        typedef MetaDataList<VersionMetaDataHandle> SpirVersionsMetaDataList;
        typedef MetaObjectHandle<SpirVersionsMetaDataList> SpirVersionsMetaDataListHandle;

        typedef MetaDataList<std::string> InnerUsedKhrExtensionsMetaDataList;
        typedef MetaObjectHandle<InnerUsedKhrExtensionsMetaDataList> InnerUsedKhrExtensionsMetaDataListHandle;

        typedef MetaDataList<InnerUsedKhrExtensionsMetaDataListHandle> UsedKhrExtensionsMetaDataList;
        typedef MetaObjectHandle<UsedKhrExtensionsMetaDataList> UsedKhrExtensionsMetaDataListHandle;

        typedef MetaDataList<std::string> InnerUsedOptionalCoreFeaturesMetaDataList;
        typedef MetaObjectHandle<InnerUsedOptionalCoreFeaturesMetaDataList> InnerUsedOptionalCoreFeaturesMetaDataListHandle;

        typedef MetaDataList<InnerUsedOptionalCoreFeaturesMetaDataListHandle> UsedOptionalCoreFeaturesMetaDataList;
        typedef MetaObjectHandle<UsedOptionalCoreFeaturesMetaDataList> UsedOptionalCoreFeaturesMetaDataListHandle;

        typedef MetaDataList<int32_t> FloatingPointContractionsMetaDataList;
        typedef MetaObjectHandle<FloatingPointContractionsMetaDataList> FloatingPointContractionsMetaDataListHandle;

        typedef MetaDataList<KernelMetaDataHandle> KernelsMetaDataList;
        typedef MetaObjectHandle<KernelsMetaDataList> KernelsMetaDataListHandle;

        typedef MetaDataList<std::string> InnerCompilerOptionsMetaDataList;
        typedef MetaObjectHandle<InnerCompilerOptionsMetaDataList> InnerCompilerOptionsMetaDataListHandle;

        typedef MetaDataList<InnerCompilerOptionsMetaDataListHandle> CompilerOptionsMetaDataList;
        typedef MetaObjectHandle<CompilerOptionsMetaDataList> CompilerOptionsMetaDataListHandle;

        typedef MetaDataList<std::string> InnerCompilerExternalOptionsMetaDataList;
        typedef MetaObjectHandle<InnerCompilerExternalOptionsMetaDataList> InnerCompilerExternalOptionsMetaDataListHandle;

        typedef MetaDataList<InnerCompilerExternalOptionsMetaDataListHandle> CompilerExternalOptionsMetaDataList;
        typedef MetaObjectHandle<CompilerExternalOptionsMetaDataList> CompilerExternalOptionsMetaDataListHandle;

        typedef MetaDataList<int32_t> ArgAddressSpacesMetaDataList;
        typedef MetaObjectHandle<ArgAddressSpacesMetaDataList> ArgAddressSpacesMetaDataListHandle;

        typedef MetaDataList<std::string> ArgAccessQualifiersMetaDataList;
        typedef MetaObjectHandle<ArgAccessQualifiersMetaDataList> ArgAccessQualifiersMetaDataListHandle;

        typedef MetaDataList<std::string> ArgTypesMetaDataList;
        typedef MetaObjectHandle<ArgTypesMetaDataList> ArgTypesMetaDataListHandle;

        typedef MetaDataList<std::string> ArgBaseTypesMetaDataList;
        typedef MetaObjectHandle<ArgBaseTypesMetaDataList> ArgBaseTypesMetaDataListHandle;

        typedef MetaDataList<std::string> ArgTypeQualifiersMetaDataList;
        typedef MetaObjectHandle<ArgTypeQualifiersMetaDataList> ArgTypeQualifiersMetaDataListHandle;

        typedef MetaDataList<std::string> ArgNamesMetaDataList;
        typedef MetaObjectHandle<ArgNamesMetaDataList> ArgNamesMetaDataListHandle;

        ///
        // Read/Write the VectorTypeHint structure from/to LLVM metadata
        //
        class VectorTypeHintMetaData :public IMetaDataObject
        {
        public:
            typedef VectorTypeHintMetaData _Myt;
            typedef IMetaDataObject _Mybase;
            // typedefs for data member types
            typedef MetaDataValue<llvm::UndefValue>::value_type VecTypeType;
            typedef MetaDataValue<bool>::value_type SignType;

        public:
            ///
            // Factory method - creates the VectorTypeHintMetaData from the given metadata node
            //
            static _Myt* get(const llvm::MDNode* pNode, bool hasId = false)
            {
                return new _Myt(pNode, hasId);
            }

            ///
            // Factory method - create the default empty VectorTypeHintMetaData object
            static _Myt* get()
            {
                return new _Myt();
            }

            ///
            // Factory method - create the default empty named VectorTypeHintMetaData object
            static _Myt* get(const char* name)
            {
                return new _Myt(name);
            }

            ///
            // Ctor - loads the VectorTypeHintMetaData from the given metadata node
            //
            VectorTypeHintMetaData(const llvm::MDNode* pNode, bool hasId);

            ///
            // Default Ctor - creates the empty, not named VectorTypeHintMetaData object
            //
            VectorTypeHintMetaData();

            ///
            // Ctor - creates the empty, named VectorTypeHintMetaData object
            //
            VectorTypeHintMetaData(const char* name);

            /// VecType related methods
            VecTypeType getVecType() const
            {
                return m_VecType.get();
            }

            bool isVecTypeHasValue() const
            {
                return m_VecType.hasValue();
            }


            /// Sign related methods
            SignType getSign() const
            {
                return m_Sign.get();
            }

            bool isSignHasValue() const
            {
                return m_Sign.hasValue();
            }

            ///
            // Returns true if any of the VectorTypeHintMetaData`s members has changed
            bool dirty() const;

            ///
            // Returns true if the structure was loaded from the metadata or was changed
            bool hasValue() const;

            ///
            // Discards the changes done to the VectorTypeHintMetaData instance
            void discardChanges();

            ///
            // Generates the new MDNode hierarchy for the given structure
            llvm::Metadata* generateNode(llvm::LLVMContext& context) const;

            ///
            // Saves the structure changes to the given MDNode
            void save(llvm::LLVMContext& context, llvm::MDNode* pNode) const;

        private:
            ///
            // Returns true if the given MDNode could be saved to without replacement
            bool compatibleWith(const llvm::MDNode* pNode) const
            {
                return false;
            }

        private:
            typedef MetaDataIterator<llvm::MDNode> NodeIterator;

            llvm::Metadata* getVecTypeNode(const llvm::MDNode* pParentNode) const;
            llvm::Metadata* getSignNode(const llvm::MDNode* pParentNode) const;

        private:
            // data members
            MetaDataValue<llvm::UndefValue> m_VecType;
            MetaDataValue<bool> m_Sign;
            // parent node
            const llvm::MDNode* m_pNode;
        };

        ///
        // Read/Write the Version structure from/to LLVM metadata
        //
        class VersionMetaData :public IMetaDataObject
        {
        public:
            typedef VersionMetaData _Myt;
            typedef IMetaDataObject _Mybase;
            // typedefs for data member types
            typedef MetaDataValue<int32_t>::value_type MajorType;
            typedef MetaDataValue<int32_t>::value_type MinorType;

        public:
            ///
            // Factory method - creates the VersionMetaData from the given metadata node
            //
            static _Myt* get(const llvm::MDNode* pNode, bool hasId = false)
            {
                return new _Myt(pNode, hasId);
            }

            ///
            // Factory method - create the default empty VersionMetaData object
            static _Myt* get()
            {
                return new _Myt();
            }

            ///
            // Factory method - create the default empty named VersionMetaData object
            static _Myt* get(const char* name)
            {
                return new _Myt(name);
            }

            ///
            // Ctor - loads the VersionMetaData from the given metadata node
            //
            VersionMetaData(const llvm::MDNode* pNode, bool hasId);

            ///
            // Default Ctor - creates the empty, not named VersionMetaData object
            //
            VersionMetaData();

            ///
            // Ctor - creates the empty, named VersionMetaData object
            //
            VersionMetaData(const char* name);

            /// Major related methods
            MajorType getMajor() const
            {
                return m_Major.get();
            }

            bool isMajorHasValue() const
            {
                return m_Major.hasValue();
            }


            /// Minor related methods
            MinorType getMinor() const
            {
                return m_Minor.get();
            }

            bool isMinorHasValue() const
            {
                return m_Minor.hasValue();
            }

            ///
            // Returns true if any of the VersionMetaData`s members has changed
            bool dirty() const;

            ///
            // Returns true if the structure was loaded from the metadata or was changed
            bool hasValue() const;

            ///
            // Discards the changes done to the VersionMetaData instance
            void discardChanges();

            ///
            // Generates the new MDNode hierarchy for the given structure
            llvm::Metadata* generateNode(llvm::LLVMContext& context) const;

            ///
            // Saves the structure changes to the given MDNode
            void save(llvm::LLVMContext& context, llvm::MDNode* pNode) const;

        private:
            ///
            // Returns true if the given MDNode could be saved to without replacement
            bool compatibleWith(const llvm::MDNode* pNode) const
            {
                return false;
            }

        private:
            typedef MetaDataIterator<llvm::MDNode> NodeIterator;

            llvm::Metadata* getMajorNode(const llvm::MDNode* pParentNode) const;
            llvm::Metadata* getMinorNode(const llvm::MDNode* pParentNode) const;

        private:
            // data members
            MetaDataValue<int32_t> m_Major;
            MetaDataValue<int32_t> m_Minor;
            // parent node
            const llvm::MDNode* m_pNode;
        };

        ///
        // Read/Write the WorkGroupDimensions structure from/to LLVM metadata
        //
        class WorkGroupDimensionsMetaData :public IMetaDataObject
        {
        public:
            typedef WorkGroupDimensionsMetaData _Myt;
            typedef IMetaDataObject _Mybase;
            // typedefs for data member types
            typedef MetaDataValue<int32_t>::value_type XDimType;
            typedef MetaDataValue<int32_t>::value_type YDimType;
            typedef MetaDataValue<int32_t>::value_type ZDimType;

        public:
            ///
            // Factory method - creates the WorkGroupDimensionsMetaData from the given metadata node
            //
            static _Myt* get(const llvm::MDNode* pNode, bool hasId = false)
            {
                return new _Myt(pNode, hasId);
            }

            ///
            // Factory method - create the default empty WorkGroupDimensionsMetaData object
            static _Myt* get()
            {
                return new _Myt();
            }

            ///
            // Factory method - create the default empty named WorkGroupDimensionsMetaData object
            static _Myt* get(const char* name)
            {
                return new _Myt(name);
            }

            ///
            // Ctor - loads the WorkGroupDimensionsMetaData from the given metadata node
            //
            WorkGroupDimensionsMetaData(const llvm::MDNode* pNode, bool hasId);

            ///
            // Default Ctor - creates the empty, not named WorkGroupDimensionsMetaData object
            //
            WorkGroupDimensionsMetaData();

            ///
            // Ctor - creates the empty, named WorkGroupDimensionsMetaData object
            //
            WorkGroupDimensionsMetaData(const char* name);

            /// XDim related methods
            XDimType getXDim() const
            {
                return m_XDim.get();
            }

            bool isXDimHasValue() const
            {
                return m_XDim.hasValue();
            }


            /// YDim related methods
            YDimType getYDim() const
            {
                return m_YDim.get();
            }

            bool isYDimHasValue() const
            {
                return m_YDim.hasValue();
            }


            /// ZDim related methods
            ZDimType getZDim() const
            {
                return m_ZDim.get();
            }

            bool isZDimHasValue() const
            {
                return m_ZDim.hasValue();
            }

            ///
            // Returns true if any of the WorkGroupDimensionsMetaData`s members has changed
            bool dirty() const;

            ///
            // Returns true if the structure was loaded from the metadata or was changed
            bool hasValue() const;

            ///
            // Discards the changes done to the WorkGroupDimensionsMetaData instance
            void discardChanges();

            ///
            // Generates the new MDNode hierarchy for the given structure
            llvm::Metadata* generateNode(llvm::LLVMContext& context) const;

            ///
            // Saves the structure changes to the given MDNode
            void save(llvm::LLVMContext& context, llvm::MDNode* pNode) const;

        private:
            ///
            // Returns true if the given MDNode could be saved to without replacement
            bool compatibleWith(const llvm::MDNode* pNode) const
            {
                return false;
            }

        private:
            typedef MetaDataIterator<llvm::MDNode> NodeIterator;

            llvm::Metadata* getXDimNode(const llvm::MDNode* pParentNode) const;
            llvm::Metadata* getYDimNode(const llvm::MDNode* pParentNode) const;
            llvm::Metadata* getZDimNode(const llvm::MDNode* pParentNode) const;

        private:
            // data members
            MetaDataValue<int32_t> m_XDim;
            MetaDataValue<int32_t> m_YDim;
            MetaDataValue<int32_t> m_ZDim;
            // parent node
            const llvm::MDNode* m_pNode;
        };

        ///
        // Read/Write the SubGroupDimensions structure from/to LLVM metadata
        //
        class SubGroupDimensionsMetaData :public IMetaDataObject
        {
        public:
            typedef SubGroupDimensionsMetaData _Myt;
            typedef IMetaDataObject _Mybase;
            // typedefs for data member types
            typedef MetaDataValue<int32_t>::value_type SIMD_SizeType;

        public:
            ///
            // Factory method - creates the SubGroupDimensionsMetaData from the given metadata node
            //
            static _Myt* get(const llvm::MDNode* pNode, bool hasId = false)
            {
                return new _Myt(pNode, hasId);
            }

            ///
            // Factory method - create the default empty SubGroupDimensionsMetaData object
            static _Myt* get()
            {
                return new _Myt();
            }

            ///
            // Factory method - create the default empty named SubGroupDimensionsMetaData object
            static _Myt* get(const char* name)
            {
                return new _Myt(name);
            }

            ///
            // Ctor - loads the SubGroupDimensionsMetaData from the given metadata node
            //
            SubGroupDimensionsMetaData(const llvm::MDNode* pNode, bool hasId);

            ///
            // Default Ctor - creates the empty, not named SubGroupDimensionsMetaData object
            //
            SubGroupDimensionsMetaData();

            ///
            // Ctor - creates the empty, named SubGroupDimensionsMetaData object
            //
            SubGroupDimensionsMetaData(const char* name);

            /// SIMD_Size related methods
            SIMD_SizeType getSIMD_Size() const
            {
                return m_SIMD_Size.get();
            }

            bool isSIMD_SizeHasValue() const
            {
                return m_SIMD_Size.hasValue();
            }

            ///
            // Returns true if any of the SubGroupDimensionsMetaData`s members has changed
            bool dirty() const;

            ///
            // Returns true if the structure was loaded from the metadata or was changed
            bool hasValue() const;

            ///
            // Discards the changes done to the SubGroupDimensionsMetaData instance
            void discardChanges();

            ///
            // Generates the new MDNode hierarchy for the given structure
            llvm::Metadata* generateNode(llvm::LLVMContext& context) const;

            ///
            // Saves the structure changes to the given MDNode
            void save(llvm::LLVMContext& context, llvm::MDNode* pNode) const;

        private:
            ///
            // Returns true if the given MDNode could be saved to without replacement
            bool compatibleWith(const llvm::MDNode* pNode) const
            {
                return false;
            }

        private:
            typedef MetaDataIterator<llvm::MDNode> NodeIterator;

            llvm::Metadata* getSIMD_SizeNode(const llvm::MDNode* pParentNode) const;

        private:
            // data members
            MetaDataValue<int32_t> m_SIMD_Size;
            // parent node
            const llvm::MDNode* m_pNode;
        };

        ///
        // Read/Write the WorkgroupWalkOrder structure from/to LLVM metadata
        //
        class WorkgroupWalkOrderMetaData :public IMetaDataObject
        {
        public:
            typedef WorkgroupWalkOrderMetaData _Myt;
            typedef IMetaDataObject _Mybase;
            // typedefs for data member types
            typedef MetaDataValue<int32_t>::value_type WalkOrderDimType;

        public:
            ///
            // Factory method - creates the SubGroupSizeMetaData from the given metadata node
            //
            static _Myt* get(const llvm::MDNode* pNode, bool hasId = false)
            {
                return new _Myt(pNode, hasId);
            }

            ///
            // Factory method - create the default empty SubGroupSizeMetaData object
            static _Myt* get()
            {
                return new _Myt();
            }

            ///
            // Factory method - create the default empty named SubGroupSizeMetaData object
            static _Myt* get(const char* name)
            {
                return new _Myt(name);
            }

            ///
            // Ctor - loads the SubGroupSizeMetaData from the given metadata node
            //
            WorkgroupWalkOrderMetaData(const llvm::MDNode* pNode, bool hasId);

            ///
            // Default Ctor - creates the empty, not named SubGroupSizeMetaData object
            //
            WorkgroupWalkOrderMetaData();

            ///
            // Ctor - creates the empty, named SubGroupSizeMetaData object
            //
            WorkgroupWalkOrderMetaData(const char* name);

            /// workgroup walk order related methods
            WalkOrderDimType getDim0() const
            {
                return m_Dim0.get();
            }
            WalkOrderDimType getDim1() const
            {
                return m_Dim1.get();
            }
            WalkOrderDimType getDim2() const
            {
                return m_Dim2.get();
            }

            ///
            // Returns true if any of the SubGroupSizeMetaData`s members has changed
            bool dirty() const;

            ///
            // Returns true if the structure was loaded from the metadata or was changed
            bool hasValue() const;

            ///
            // Discards the changes done to the SubGroupSizeMetaData instance
            void discardChanges();

            ///
            // Generates the new MDNode hierarchy for the given structure
            llvm::Metadata* generateNode(llvm::LLVMContext& context) const;

            ///
            // Saves the structure changes to the given MDNode
            void save(llvm::LLVMContext& context, llvm::MDNode* pNode) const;

        private:
            ///
            // Returns true if the given MDNode could be saved to without replacement
            bool compatibleWith(const llvm::MDNode* pNode) const
            {
                return false;
            }

        private:
            typedef MetaDataIterator<llvm::MDNode> NodeIterator;

            llvm::Metadata* getDim0Node(const llvm::MDNode* pParentNode) const;
            llvm::Metadata* getDim1Node(const llvm::MDNode* pParentNode) const;
            llvm::Metadata* getDim2Node(const llvm::MDNode* pParentNode) const;

        private:
            // data members
            MetaDataValue<int32_t> m_Dim0;
            MetaDataValue<int32_t> m_Dim1;
            MetaDataValue<int32_t> m_Dim2;
            // parent node
            const llvm::MDNode* m_pNode;
        };

        ///
        // Read/Write the Kernel structure from/to LLVM metadata
        //
        class KernelMetaData :public IMetaDataObject
        {
        public:
            typedef KernelMetaData _Myt;
            typedef IMetaDataObject _Mybase;
            // typedefs for data member types
            typedef MetaDataValue<llvm::Function>::value_type FunctionType;




            typedef MetaDataList<int32_t> ArgAddressSpacesList;
            typedef MetaDataList<std::string> ArgAccessQualifiersList;
            typedef MetaDataList<std::string> ArgTypesList;
            typedef MetaDataList<std::string> ArgBaseTypesList;
            typedef MetaDataList<std::string> ArgTypeQualifiersList;
            typedef MetaDataList<std::string> ArgNamesList;

        public:
            ///
            // Factory method - creates the KernelMetaData from the given metadata node
            //
            static _Myt* get(const llvm::MDNode* pNode, bool hasId = false)
            {
                return new _Myt(pNode, hasId);
            }

            ///
            // Factory method - create the default empty KernelMetaData object
            static _Myt* get()
            {
                return new _Myt();
            }

            ///
            // Factory method - create the default empty named KernelMetaData object
            static _Myt* get(const char* name)
            {
                return new _Myt(name);
            }

            ///
            // Ctor - loads the KernelMetaData from the given metadata node
            //
            KernelMetaData(const llvm::MDNode* pNode, bool hasId);

            ///
            // Default Ctor - creates the empty, not named KernelMetaData object
            //
            KernelMetaData();

            ///
            // Ctor - creates the empty, named KernelMetaData object
            //
            KernelMetaData(const char* name);

            /// Function related methods
            FunctionType getFunction() const
            {
                return m_Function.get();
            }

            bool isFunctionHasValue() const
            {
                return m_Function.hasValue();
            }


            /// WorkGroupSizeHint related methods

            const WorkGroupDimensionsMetaDataHandle getWorkGroupSizeHint()
            {
                return m_WorkGroupSizeHint;
            }


            /// RequiredWorkGroupSize related methods

            const WorkGroupDimensionsMetaDataHandle getRequiredWorkGroupSize()
            {
                return m_RequiredWorkGroupSize;
            }


            /// RequiredSubGroupSize related methods

            const SubGroupDimensionsMetaDataHandle getRequiredSubGroupSize()
            {
                return m_RequiredSubGroupSize;
            }

            /// WorkgroupWalkOrder related methods

            const WorkgroupWalkOrderMetaDataHandle getWorkgroupWalkOrder()
            {
                return m_WorkgroupWalkOrder;
            }


            /// VectorTypeHint related methods

            const VectorTypeHintMetaDataHandle getVectorTypeHint()
            {
                return m_VectorTypeHint;
            }


            /// ArgAddressSpaces related methods

            ArgAddressSpacesList::const_iterator begin_ArgAddressSpaces() const
            {
                return m_ArgAddressSpaces.begin();
            }

            ArgAddressSpacesList::const_iterator end_ArgAddressSpaces() const
            {
                return m_ArgAddressSpaces.end();
            }

            size_t size_ArgAddressSpaces()  const
            {
                return m_ArgAddressSpaces.size();
            }

            bool empty_ArgAddressSpaces()  const
            {
                return m_ArgAddressSpaces.empty();
            }

            bool isArgAddressSpacesHasValue() const
            {
                return m_ArgAddressSpaces.hasValue();
            }

            const ArgAddressSpacesList::item_type getArgAddressSpacesItem(size_t index) const
            {
                return m_ArgAddressSpaces.getItem(index);
            }



            /// ArgAccessQualifiers related methods

            ArgAccessQualifiersList::const_iterator begin_ArgAccessQualifiers() const
            {
                return m_ArgAccessQualifiers.begin();
            }

            ArgAccessQualifiersList::const_iterator end_ArgAccessQualifiers() const
            {
                return m_ArgAccessQualifiers.end();
            }

            size_t size_ArgAccessQualifiers()  const
            {
                return m_ArgAccessQualifiers.size();
            }

            bool empty_ArgAccessQualifiers()  const
            {
                return m_ArgAccessQualifiers.empty();
            }

            bool isArgAccessQualifiersHasValue() const
            {
                return m_ArgAccessQualifiers.hasValue();
            }

            const ArgAccessQualifiersList::item_type getArgAccessQualifiersItem(size_t index) const
            {
                return m_ArgAccessQualifiers.getItem(index);
            }



            /// ArgTypes related methods

            ArgTypesList::const_iterator begin_ArgTypes() const
            {
                return m_ArgTypes.begin();
            }

            ArgTypesList::const_iterator end_ArgTypes() const
            {
                return m_ArgTypes.end();
            }

            size_t size_ArgTypes()  const
            {
                return m_ArgTypes.size();
            }

            bool empty_ArgTypes()  const
            {
                return m_ArgTypes.empty();
            }

            bool isArgTypesHasValue() const
            {
                return m_ArgTypes.hasValue();
            }

            const ArgTypesList::item_type getArgTypesItem(size_t index) const
            {
                return m_ArgTypes.getItem(index);
            }



            /// ArgBaseTypes related methods

            ArgBaseTypesList::const_iterator begin_ArgBaseTypes() const
            {
                return m_ArgBaseTypes.begin();
            }

            ArgBaseTypesList::const_iterator end_ArgBaseTypes() const
            {
                return m_ArgBaseTypes.end();
            }

            size_t size_ArgBaseTypes()  const
            {
                return m_ArgBaseTypes.size();
            }

            bool empty_ArgBaseTypes()  const
            {
                return m_ArgBaseTypes.empty();
            }

            bool isArgBaseTypesHasValue() const
            {
                return m_ArgBaseTypes.hasValue();
            }

            const ArgBaseTypesList::item_type getArgBaseTypesItem(size_t index) const
            {
                return m_ArgBaseTypes.getItem(index);
            }



            /// ArgTypeQualifiers related methods

            ArgTypeQualifiersList::const_iterator begin_ArgTypeQualifiers() const
            {
                return m_ArgTypeQualifiers.begin();
            }

            ArgTypeQualifiersList::const_iterator end_ArgTypeQualifiers() const
            {
                return m_ArgTypeQualifiers.end();
            }

            size_t size_ArgTypeQualifiers()  const
            {
                return m_ArgTypeQualifiers.size();
            }

            bool empty_ArgTypeQualifiers()  const
            {
                return m_ArgTypeQualifiers.empty();
            }

            bool isArgTypeQualifiersHasValue() const
            {
                return m_ArgTypeQualifiers.hasValue();
            }

            const ArgTypeQualifiersList::item_type getArgTypeQualifiersItem(size_t index) const
            {
                return m_ArgTypeQualifiers.getItem(index);
            }



            /// ArgNames related methods

            ArgNamesList::const_iterator begin_ArgNames() const
            {
                return m_ArgNames.begin();
            }

            ArgNamesList::const_iterator end_ArgNames() const
            {
                return m_ArgNames.end();
            }

            size_t size_ArgNames()  const
            {
                return m_ArgNames.size();
            }

            bool empty_ArgNames()  const
            {
                return m_ArgNames.empty();
            }

            bool isArgNamesHasValue() const
            {
                return m_ArgNames.hasValue();
            }

            const ArgNamesList::item_type getArgNamesItem(size_t index) const
            {
                return m_ArgNames.getItem(index);
            }

            ///
            // Returns true if any of the KernelMetaData`s members has changed
            bool dirty() const;

            ///
            // Returns true if the structure was loaded from the metadata or was changed
            bool hasValue() const;

            ///
            // Discards the changes done to the KernelMetaData instance
            void discardChanges();

            ///
            // Generates the new MDNode hierarchy for the given structure
            llvm::Metadata* generateNode(llvm::LLVMContext& context) const;

            ///
            // Saves the structure changes to the given MDNode
            void save(llvm::LLVMContext& context, llvm::MDNode* pNode) const;

        private:
            ///
            // Returns true if the given MDNode could be saved to without replacement
            bool compatibleWith(const llvm::MDNode* pNode) const
            {
                return false;
            }

        private:
            typedef MetaDataIterator<llvm::MDNode> NodeIterator;

            llvm::Metadata* getFunctionNode(const llvm::MDNode* pParentNode) const;
            llvm::MDNode* getWorkGroupSizeHintNode(const llvm::MDNode* pParentNode) const;
            llvm::MDNode* getRequiredWorkGroupSizeNode(const llvm::MDNode* pParentNode) const;
            llvm::MDNode* getRequiredSubGroupSizeNode(const llvm::MDNode* pParentNode) const;
            llvm::MDNode* getWorkgroupWalkOrderNode(const llvm::MDNode* pParentNode) const;
            llvm::MDNode* getVectorTypeHintNode(const llvm::MDNode* pParentNode) const;
            llvm::MDNode* getArgAddressSpacesNode(const llvm::MDNode* pParentNode) const;
            llvm::MDNode* getArgAccessQualifiersNode(const llvm::MDNode* pParentNode) const;
            llvm::MDNode* getArgTypesNode(const llvm::MDNode* pParentNode) const;
            llvm::MDNode* getArgBaseTypesNode(const llvm::MDNode* pParentNode) const;
            llvm::MDNode* getArgTypeQualifiersNode(const llvm::MDNode* pParentNode) const;
            llvm::MDNode* getArgNamesNode(const llvm::MDNode* pParentNode) const;

        private:
            // data members
            MetaDataValue<llvm::Function> m_Function;
            WorkGroupDimensionsMetaDataHandle m_WorkGroupSizeHint;
            WorkGroupDimensionsMetaDataHandle m_RequiredWorkGroupSize;
            SubGroupDimensionsMetaDataHandle m_RequiredSubGroupSize;
            WorkgroupWalkOrderMetaDataHandle m_WorkgroupWalkOrder;
            VectorTypeHintMetaDataHandle m_VectorTypeHint;
            MetaDataList<int32_t> m_ArgAddressSpaces;
            MetaDataList<std::string> m_ArgAccessQualifiers;
            MetaDataList<std::string> m_ArgTypes;
            MetaDataList<std::string> m_ArgBaseTypes;
            MetaDataList<std::string> m_ArgTypeQualifiers;
            MetaDataList<std::string> m_ArgNames;
            // parent node
            const llvm::MDNode* m_pNode;
        };


        class SpirMetaDataUtils
        {
        public:
            // typedefs for the data members types
            typedef NamedMDNodeList<KernelMetaDataHandle> KernelsList;
            typedef NamedMDNodeList<InnerCompilerOptionsMetaDataListHandle> CompilerOptionsList;
            typedef NamedMDNodeList<InnerCompilerExternalOptionsMetaDataListHandle> CompilerExternalOptionsList;
            typedef NamedMDNodeList<int32_t> FloatingPointContractionsList;
            typedef NamedMDNodeList<InnerUsedOptionalCoreFeaturesMetaDataListHandle> UsedOptionalCoreFeaturesList;
            typedef NamedMDNodeList<InnerUsedKhrExtensionsMetaDataListHandle> UsedKhrExtensionsList;
            typedef NamedMDNodeList<VersionMetaDataHandle> SpirVersionsList;
            typedef NamedMDNodeList<VersionMetaDataHandle> OpenCLVersionsList;

        public:
            // If using this constructor, setting the llvm module by the setModule
            // function is needed for correct operation.
            SpirMetaDataUtils() {}



            SpirMetaDataUtils(llvm::Module* pModule) :
                m_Kernels(pModule->getNamedMetadata("opencl.kernels")),
                m_CompilerOptions(pModule->getNamedMetadata("opencl.compiler.options")),
                m_CompilerExternalOptions(pModule->getNamedMetadata("opencl.compiler.ext.options")),
                m_FloatingPointContractions(pModule->getNamedMetadata("opencl.enable.FP_CONTRACT")),
                m_UsedOptionalCoreFeatures(pModule->getNamedMetadata("opencl.used.optional.core.features")),
                m_UsedKhrExtensions(pModule->getNamedMetadata("opencl.used.extensions")),
                m_SpirVersions(pModule->getNamedMetadata("opencl.spir.version")),
                m_OpenCLVersions(pModule->getNamedMetadata("opencl.ocl.version")),
                m_pModule(pModule)
            {
            }

            void setModule(llvm::Module* pModule) {
                m_Kernels = pModule->getNamedMetadata("opencl.kernels");
                m_CompilerOptions = pModule->getNamedMetadata("opencl.compiler.options");
                m_CompilerExternalOptions = pModule->getNamedMetadata("opencl.compiler.ext.options");
                m_FloatingPointContractions = pModule->getNamedMetadata("opencl.enable.FP_CONTRACT");
                m_UsedOptionalCoreFeatures = pModule->getNamedMetadata("opencl.used.optional.core.features");
                m_UsedKhrExtensions = pModule->getNamedMetadata("opencl.used.extensions");
                m_SpirVersions = pModule->getNamedMetadata("opencl.spir.version");
                m_OpenCLVersions = pModule->getNamedMetadata("opencl.ocl.version");
                m_pModule = pModule;
            }

            ~SpirMetaDataUtils() {}

            /// Kernels related methods


            KernelsList::const_iterator begin_Kernels() const
            {
                return m_Kernels.begin();
            }

            KernelsList::const_iterator end_Kernels() const
            {
                return m_Kernels.end();
            }

            size_t size_Kernels()  const
            {
                return m_Kernels.size();
            }

            bool empty_Kernels()  const
            {
                return m_Kernels.empty();
            }

            bool isKernelsHasValue() const
            {
                return m_Kernels.hasValue();
            }

            const KernelsList::item_type getKernelsItem(size_t index) const
            {
                return m_Kernels.getItem(index);
            }



            /// CompilerOptions related methods


            CompilerOptionsList::const_iterator begin_CompilerOptions() const
            {
                return m_CompilerOptions.begin();
            }

            CompilerOptionsList::const_iterator end_CompilerOptions() const
            {
                return m_CompilerOptions.end();
            }

            size_t size_CompilerOptions()  const
            {
                return m_CompilerOptions.size();
            }

            bool empty_CompilerOptions()  const
            {
                return m_CompilerOptions.empty();
            }

            bool isCompilerOptionsHasValue() const
            {
                return m_CompilerOptions.hasValue();
            }

            const CompilerOptionsList::item_type getCompilerOptionsItem(size_t index) const
            {
                return m_CompilerOptions.getItem(index);
            }



            /// CompilerExternalOptions related methods


            CompilerExternalOptionsList::const_iterator begin_CompilerExternalOptions() const
            {
                return m_CompilerExternalOptions.begin();
            }

            CompilerExternalOptionsList::const_iterator end_CompilerExternalOptions() const
            {
                return m_CompilerExternalOptions.end();
            }

            size_t size_CompilerExternalOptions()  const
            {
                return m_CompilerExternalOptions.size();
            }

            bool empty_CompilerExternalOptions()  const
            {
                return m_CompilerExternalOptions.empty();
            }

            bool isCompilerExternalOptionsHasValue() const
            {
                return m_CompilerExternalOptions.hasValue();
            }

            const CompilerExternalOptionsList::item_type getCompilerExternalOptionsItem(size_t index) const
            {
                return m_CompilerExternalOptions.getItem(index);
            }



            /// FloatingPointContractions related methods


            FloatingPointContractionsList::const_iterator begin_FloatingPointContractions() const
            {
                return m_FloatingPointContractions.begin();
            }

            FloatingPointContractionsList::const_iterator end_FloatingPointContractions() const
            {
                return m_FloatingPointContractions.end();
            }

            size_t size_FloatingPointContractions()  const
            {
                return m_FloatingPointContractions.size();
            }

            bool empty_FloatingPointContractions()  const
            {
                return m_FloatingPointContractions.empty();
            }

            bool isFloatingPointContractionsHasValue() const
            {
                return m_FloatingPointContractions.hasValue();
            }

            const FloatingPointContractionsList::item_type getFloatingPointContractionsItem(size_t index) const
            {
                return m_FloatingPointContractions.getItem(index);
            }



            /// UsedOptionalCoreFeatures related methods


            UsedOptionalCoreFeaturesList::const_iterator begin_UsedOptionalCoreFeatures() const
            {
                return m_UsedOptionalCoreFeatures.begin();
            }

            UsedOptionalCoreFeaturesList::const_iterator end_UsedOptionalCoreFeatures() const
            {
                return m_UsedOptionalCoreFeatures.end();
            }

            size_t size_UsedOptionalCoreFeatures()  const
            {
                return m_UsedOptionalCoreFeatures.size();
            }

            bool empty_UsedOptionalCoreFeatures()  const
            {
                return m_UsedOptionalCoreFeatures.empty();
            }

            bool isUsedOptionalCoreFeaturesHasValue() const
            {
                return m_UsedOptionalCoreFeatures.hasValue();
            }

            const UsedOptionalCoreFeaturesList::item_type getUsedOptionalCoreFeaturesItem(size_t index) const
            {
                return m_UsedOptionalCoreFeatures.getItem(index);
            }



            /// UsedKhrExtensions related methods


            UsedKhrExtensionsList::const_iterator begin_UsedKhrExtensions() const
            {
                return m_UsedKhrExtensions.begin();
            }

            UsedKhrExtensionsList::const_iterator end_UsedKhrExtensions() const
            {
                return m_UsedKhrExtensions.end();
            }

            size_t size_UsedKhrExtensions()  const
            {
                return m_UsedKhrExtensions.size();
            }

            bool empty_UsedKhrExtensions()  const
            {
                return m_UsedKhrExtensions.empty();
            }

            bool isUsedKhrExtensionsHasValue() const
            {
                return m_UsedKhrExtensions.hasValue();
            }

            const UsedKhrExtensionsList::item_type getUsedKhrExtensionsItem(size_t index) const
            {
                return m_UsedKhrExtensions.getItem(index);
            }



            /// SpirVersions related methods


            SpirVersionsList::const_iterator begin_SpirVersions() const
            {
                return m_SpirVersions.begin();
            }

            SpirVersionsList::const_iterator end_SpirVersions() const
            {
                return m_SpirVersions.end();
            }

            size_t size_SpirVersions()  const
            {
                return m_SpirVersions.size();
            }

            bool empty_SpirVersions()  const
            {
                return m_SpirVersions.empty();
            }

            bool isSpirVersionsHasValue() const
            {
                return m_SpirVersions.hasValue();
            }

            const SpirVersionsList::item_type getSpirVersionsItem(size_t index) const
            {
                return m_SpirVersions.getItem(index);
            }



            /// OpenCLVersions related methods


            OpenCLVersionsList::const_iterator begin_OpenCLVersions() const
            {
                return m_OpenCLVersions.begin();
            }

            OpenCLVersionsList::const_iterator end_OpenCLVersions() const
            {
                return m_OpenCLVersions.end();
            }

            size_t size_OpenCLVersions()  const
            {
                return m_OpenCLVersions.size();
            }

            bool empty_OpenCLVersions()  const
            {
                return m_OpenCLVersions.empty();
            }

            bool isOpenCLVersionsHasValue() const
            {
                return m_OpenCLVersions.hasValue();
            }

            const OpenCLVersionsList::item_type getOpenCLVersionsItem(size_t index) const
            {
                return m_OpenCLVersions.getItem(index);
            }



            void deleteMetadata()
            {
                llvm::NamedMDNode* KernelsNode = m_pModule->getNamedMetadata("opencl.kernels");
                if (KernelsNode)
                {
                    m_pModule->eraseNamedMetadata(KernelsNode);
                }

                llvm::NamedMDNode* CompilerOptionsNode = m_pModule->getNamedMetadata("opencl.compiler.options");
                if (CompilerOptionsNode)
                {
                    m_pModule->eraseNamedMetadata(CompilerOptionsNode);
                }

                llvm::NamedMDNode* CompilerExternalOptionsNode = m_pModule->getNamedMetadata("opencl.compiler.ext.options");
                if (CompilerExternalOptionsNode)
                {
                    m_pModule->eraseNamedMetadata(CompilerExternalOptionsNode);
                }

                llvm::NamedMDNode* FloatingPointContractionsNode = m_pModule->getNamedMetadata("opencl.enable.FP_CONTRACT");
                if (FloatingPointContractionsNode)
                {
                    m_pModule->eraseNamedMetadata(FloatingPointContractionsNode);
                }

                llvm::NamedMDNode* UsedOptionalCoreFeaturesNode = m_pModule->getNamedMetadata("opencl.used.optional.core.features");
                if (UsedOptionalCoreFeaturesNode)
                {
                    m_pModule->eraseNamedMetadata(UsedOptionalCoreFeaturesNode);
                }

                llvm::NamedMDNode* UsedKhrExtensionsNode = m_pModule->getNamedMetadata("opencl.used.extensions");
                if (UsedKhrExtensionsNode)
                {
                    m_pModule->eraseNamedMetadata(UsedKhrExtensionsNode);
                }

                llvm::NamedMDNode* SpirVersionsNode = m_pModule->getNamedMetadata("opencl.spir.version");
                if (SpirVersionsNode)
                {
                    m_pModule->eraseNamedMetadata(SpirVersionsNode);
                }

                llvm::NamedMDNode* OpenCLVersionsNode = m_pModule->getNamedMetadata("opencl.ocl.version");
                if (OpenCLVersionsNode)
                {
                    m_pModule->eraseNamedMetadata(OpenCLVersionsNode);
                }
            }

        private:
            // data members
            NamedMDNodeList<KernelMetaDataHandle> m_Kernels;
            NamedMDNodeList<InnerCompilerOptionsMetaDataListHandle> m_CompilerOptions;
            NamedMDNodeList<InnerCompilerExternalOptionsMetaDataListHandle> m_CompilerExternalOptions;
            NamedMDNodeList<int32_t> m_FloatingPointContractions;
            NamedMDNodeList<InnerUsedOptionalCoreFeaturesMetaDataListHandle> m_UsedOptionalCoreFeatures;
            NamedMDNodeList<InnerUsedKhrExtensionsMetaDataListHandle> m_UsedKhrExtensions;
            NamedMDNodeList<VersionMetaDataHandle> m_SpirVersions;
            NamedMDNodeList<VersionMetaDataHandle> m_OpenCLVersions;
            llvm::Module* m_pModule;
        };


    }
} //namespace
