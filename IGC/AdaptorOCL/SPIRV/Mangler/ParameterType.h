//===------------------------- ParameterType.h ---------------------------===//
//
//                              SPIR Tools
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===//
/*
 * Contributed by: Intel Corporation.
 */

#ifndef __PARAMETER_TYPE_H__
#define __PARAMETER_TYPE_H__

#include "Refcount.h"
#include <string>
#include <vector>

// The Type class hierarchy models the different types in OCL.

namespace SPIR {

  // Supported SPIR versions
  enum SPIRversion {
    SPIR12 = 1,
    SPIR20 = 2
  };

  // Error Status values
  enum MangleError {
    MANGLE_SUCCESS,
    MANGLE_TYPE_NOT_SUPPORTED,
    MANGLE_NULL_FUNC_DESCRIPTOR
  };

  enum TypePrimitiveEnum {
    PRIMITIVE_FIRST,
    PRIMITIVE_BOOL = PRIMITIVE_FIRST,
    PRIMITIVE_UCHAR,
    PRIMITIVE_CHAR,
    PRIMITIVE_USHORT,
    PRIMITIVE_SHORT,
    PRIMITIVE_UINT,
    PRIMITIVE_INT,
    PRIMITIVE_ULONG,
    PRIMITIVE_LONG,
    PRIMITIVE_HALF,
    PRIMITIVE_FLOAT,
    PRIMITIVE_DOUBLE,
    PRIMITIVE_VOID,
    PRIMITIVE_VAR_ARG,
    PRIMITIVE_STRUCT_FIRST,
    PRIMITIVE_IMAGE_1D_T = PRIMITIVE_STRUCT_FIRST,
    PRIMITIVE_IMAGE_1D_ARRAY_T,
    PRIMITIVE_IMAGE_1D_BUFFER_T,
    PRIMITIVE_IMAGE_2D_T,
    PRIMITIVE_IMAGE_2D_ARRAY_T,
    PRIMITIVE_IMAGE_3D_T,
    PRIMITIVE_IMAGE_2D_MSAA_T,
    PRIMITIVE_IMAGE_2D_ARRAY_MSAA_T,
    PRIMITIVE_IMAGE_2D_MSAA_DEPTH_T,
    PRIMITIVE_IMAGE_2D_ARRAY_MSAA_DEPTH_T,
    PRIMITIVE_IMAGE_2D_DEPTH_T,
    PRIMITIVE_IMAGE_2D_ARRAY_DEPTH_T,
    PRIMITIVE_EVENT_T,
    PRIMITIVE_PIPE_T,
    PRIMITIVE_RESERVE_ID_T,
    PRIMITIVE_QUEUE_T,
    PRIMITIVE_NDRANGE_T,
    PRIMITIVE_CLK_EVENT_T,
    PRIMITIVE_STRUCT_LAST = PRIMITIVE_CLK_EVENT_T,
    PRIMITIVE_SAMPLER_T,
    PRIMITIVE_KERNEL_ENQUEUE_FLAGS_T,
    PRIMITIVE_CLK_PROFILING_INFO,
    PRIMITIVE_LAST = PRIMITIVE_CLK_PROFILING_INFO,
    PRIMITIVE_NONE,
    // Keep this at the end.
    PRIMITIVE_NUM = PRIMITIVE_NONE
  };

  enum TypeEnum {
    TYPE_ID_PRIMITIVE,
    TYPE_ID_POINTER,
    TYPE_ID_VECTOR,
    TYPE_ID_ATOMIC,
    TYPE_ID_BLOCK,
    TYPE_ID_STRUCTURE
  };

  enum TypeAttributeEnum {
    ATTR_QUALIFIER_FIRST = 0,
    ATTR_RESTRICT = ATTR_QUALIFIER_FIRST,
    ATTR_VOLATILE,
    ATTR_CONST,
    ATTR_QUALIFIER_LAST = ATTR_CONST,
    ATTR_ADDR_SPACE_FIRST,
    ATTR_PRIVATE = ATTR_ADDR_SPACE_FIRST,
    ATTR_GLOBAL,
    ATTR_CONSTANT,
    ATTR_LOCAL,
    ATTR_GENERIC,
    ATTR_ADDR_SPACE_LAST = ATTR_GENERIC,
    ATTR_NONE,
    ATTR_NUM = ATTR_NONE
  };

  // Forward declaration for abstract structure.
  struct ParamType;
  typedef RefCount<ParamType> RefParamType;

  // Forward declaration for abstract structure.
  struct TypeVisitor;

  struct ParamType {
    /// @brief Constructor.
    /// @param TypeEnum type id.
    ParamType(TypeEnum typeId) : m_typeId(typeId) {};

    /// @brief Destructor.
    virtual ~ParamType() {};

    /// Abstract Methods ///

    /// @brief Visitor service method. (see TypeVisitor for more details).
    ///        When overridden in subclasses, preform a 'double dispatch' to the
    ///        appropriate visit method in the given visitor.
    /// @param TypeVisitor type visitor.
    virtual MangleError accept(TypeVisitor*) const = 0;

    /// @brief Returns a string representation of the underlying type.
    /// @return type as string.
    virtual std::string toString() const = 0;

    /// @brief Returns true if given param type is equal to this type.
    /// @param ParamType given param type.
    /// @return true if given param type is equal to this type and false otherwise.
    virtual bool equals(const ParamType*) const = 0;

    /// Common Base-Class Methods ///

    /// @brief Returns type id of underlying type.
    /// @return type id.
    TypeEnum getTypeId() const {
      return m_typeId;
    }

  private:
    // @brief Default Constructor.
    ParamType();

  protected:
    /// An enumeration to identify the type id of this instance.
    TypeEnum m_typeId;
  };


  struct PrimitiveType : public ParamType {
    /// An enumeration to identify the type id of this class.
    const static TypeEnum enumTy;

    /// @brief Constructor.
    /// @param TypePrimitiveEnum primitive id.
    PrimitiveType(TypePrimitiveEnum);

    /// Implementation of Abstract Methods ///

    /// @brief Visitor service method. (see TypeVisitor for more details).
    ///        When overridden in subclasses, preform a 'double dispatch' to the
    ///        appropriate visit method in the given visitor.
    /// @param TypeVisitor type visitor.
    MangleError accept(TypeVisitor*) const;

    /// @brief Returns a string representation of the underlying type.
    /// @return type as string.
    std::string toString() const;

    /// @brief Returns true if given param type is equal to this type.
    /// @param ParamType given param type.
    /// @return true if given param type is equal to this type and false otherwise.
    bool equals(const ParamType*) const;

    /// Non-Common Methods ///

    /// @brief Returns the primitive enumeration of the type.
    /// @return primitive type.
    TypePrimitiveEnum getPrimitive() const {
      return m_primitive;
    }

  protected:
    /// An enumeration to identify the primitive type.
    TypePrimitiveEnum m_primitive;
  };

  struct PointerType: public ParamType {
    /// An enumeration to identify the type id of this class.
    const static TypeEnum enumTy;

    /// @brief Constructor.
    /// @param RefParamType the type of pointee (that the pointer points at).
    PointerType(const RefParamType type);

    /// Implementation of Abstract Methods ///

    /// @brief Visitor service method. (see TypeVisitor for more details).
    ///        When overridden in subclasses, preform a 'double dispatch' to the
    ///        appropriate visit method in the given visitor.
    /// @param TypeVisitor type visitor
    MangleError accept(TypeVisitor*) const;

    /// @brief Returns a string representation of the underlying type.
    /// @return type as string.
    std::string toString() const;

    /// @brief Returns true if given param type is equal to this type.
    /// @param ParamType given param type.
    /// @return true if given param type is equal to this type and false otherwise.
    bool equals(const ParamType*) const;

    /// Non-Common Methods ///

    /// @brief Returns the type the pointer is pointing at.
    /// @return pointee type.
    const RefParamType& getPointee() const {
      return m_pType;
    }

    /// @brief Sets the address space attribute - default is __private
    /// @param TypeAttributeEnum address space attribute id.
    void setAddressSpace(TypeAttributeEnum attr);

    /// @brief Returns the pointer's address space.
    /// @return pointer's address space.
    TypeAttributeEnum getAddressSpace() const;

    /// @brief Adds or removes a pointer's qualifier.
    /// @param TypeAttributeEnum qual - qualifier to add/remove.
    /// @param bool enabled - true if qualifier should exist false otherwise.
    ///        default is set to false.
    void setQualifier(TypeAttributeEnum qual, bool enabled);

    /// @brief Checks if the pointer has a certain qualifier.
    /// @param TypeAttributeEnum qual - qualifier to check.
    /// @return true if the qualifier exists and false otherwise.
    bool hasQualifier(TypeAttributeEnum qual) const;

  private:
    /// The type this pointer is pointing at.
    RefParamType m_pType;
    /// Array of the pointer's enabled type qualifiers.
    bool m_qualifiers[ATTR_QUALIFIER_LAST - ATTR_QUALIFIER_FIRST + 1];
    /// Pointer's address space.
    TypeAttributeEnum m_address_space;
  };

  struct VectorType : public ParamType {
    /// An enumeration to identify the type id of this class.
    const static TypeEnum enumTy;

    /// @brief Constructor.
    /// @param RefParamType the type of each scalar element in the vector.
    /// @param int the length of the vector.
    VectorType(const RefParamType type, int len);

    /// Implementation of Abstract Methods ///

    /// @brief Visitor service method. (see TypeVisitor for more details).
    ///        When overridden in subclasses, preform a 'double dispatch' to the
    ///        appropriate visit method in the given visitor.
    /// @param TypeVisitor type visitor.
    MangleError accept(TypeVisitor*) const;

    /// @brief Returns a string representation of the underlying type.
    /// @return type as string.
    std::string toString() const;

    /// @brief Returns true if given param type is equal to this type.
    /// @param ParamType given param type.
    /// @return true if given param type is equal to this type and false otherwise.
    bool equals(const ParamType*) const;

    /// Non-Common Methods ///

    /// @brief Returns the type the vector is packing.
    /// @return scalar type.
    const RefParamType& getScalarType() const {
      return m_pType;
    }

    /// @brief Returns the length of the vector type.
    /// @return vector type length.
    int getLength() const {
      return m_len;
    }

  private:
    /// The scalar type of this vector type.
    RefParamType m_pType;
    /// The length of the vector.
    int m_len;
  };

  struct AtomicType: public ParamType {
    ///an enumeration to identify the type id of this class
    const static TypeEnum enumTy;

    /// @brief Constructor
    /// @param RefParamType the type refernced as atomic.
    AtomicType(const RefParamType type);

    /// Implementation of Abstract Methods ///

    /// @brief visitor service method. (see TypeVisitor for more details).
    ///       When overridden in subclasses, preform a 'double dispatch' to the
    ///       appropriate visit method in the given visitor.
    /// @param TypeVisitor type visitor
    MangleError accept(TypeVisitor*) const;

    /// @brief returns a string representation of the underlying type.
    /// @return type as string
    std::string toString() const;

    /// @brief returns true if given param type is equal to this type.
    /// @param ParamType given param type
    /// @return true if given param type is equal to this type and false otherwise
    bool equals(const ParamType*) const;

    /// Non-Common Methods ///

    /// @brief returns the base type of the atomic parameter.
    /// @return base type
    const RefParamType& getBaseType() const {
      return m_pType;
    }

  private:
    ///the type this pointer is pointing at
    RefParamType m_pType;
  };

  struct BlockType : public ParamType {
    ///an enumeration to identify the type id of this class
    const static TypeEnum enumTy;

    ///@brief Constructor
    BlockType();

    /// Implementation of Abstract Methods ///

    /// @brief visitor service method. (see TypeVisitor for more details).
    ///       When overridden in subclasses, preform a 'double dispatch' to the
    ///       appropriate visit method in the given visitor.
    /// @param TypeVisitor type visitor
    MangleError accept(TypeVisitor*) const;

    /// @brief returns a string representation of the underlying type.
    /// @return type as string
    std::string toString() const;

    /// @brief returns true if given param type is equal to this type.
    /// @param ParamType given param type
    /// @return true if given param type is equal to this type and false otherwise
    bool equals(const ParamType*) const;

    /// Non-Common Methods ///

    /// @brief returns the number of parameters of the block.
    /// @return parameters count
    unsigned int getNumOfParams() const {
      return (unsigned int)m_params.size();
    }

    ///@brief returns the type of parameter "index" of the block.
    // @param index the sequential number of the queried parameter
    ///@return parameter type
    const RefParamType& getParam(unsigned int index) const {
      assert(m_params.size() > index && "index is OOB");
      return m_params[index];
    }

    ///@brief set the type of parameter "index" of the block.
    // @param index the sequential number of the queried parameter
    // @param type the parameter type
    void setParam(unsigned int index, RefParamType type) {
      if(index < getNumOfParams()) {
        m_params[index] = type;
      }
      else if (index == getNumOfParams()) {
        m_params.push_back(type);
      }
      else {
        assert(false && "index is OOB");
      }
    }

  protected:
    ///an enumeration to identify the primitive type
    std::vector<RefParamType> m_params;
  };


  struct UserDefinedType : public ParamType {
    /// An enumeration to identify the type id of this class.
    const static TypeEnum enumTy;

    /// @brief Constructor.
    UserDefinedType(const std::string&);

    /// Implementation of Abstract Methods ///

    /// @brief Visitor service method. (see TypeVisitor for more details).
    ///        When overridden in subclasses, preform a 'double dispatch' to the
    ///        appropriate visit method in the given visitor.
    /// @param TypeVisitor type visitor.
    MangleError accept(TypeVisitor*) const;

    /// @brief Returns a string representation of the underlying type.
    /// @return type as string.
    std::string toString() const;

    /// @brief Returns true if given param type is equal to this type.
    /// @param ParamType given param type.
    /// @return true if given param type is equal to this type and false otherwise.
    bool equals(const ParamType*) const;

  protected:
    /// The name of the user defined type.
    std::string m_name;
  };


  /// @brief Can be overridden so an object of static type Type* will
  ///        dispatch the correct visit method according to its dynamic type.
  struct TypeVisitor{
    SPIRversion spirVer;
    TypeVisitor(SPIRversion ver) : spirVer(ver) {};
    virtual MangleError visit(const PrimitiveType*)   = 0;
    virtual MangleError visit(const VectorType*)      = 0;
    virtual MangleError visit(const PointerType*)     = 0;
    virtual MangleError visit(const AtomicType*)      = 0;
    virtual MangleError visit(const BlockType*)       = 0;
    virtual MangleError visit(const UserDefinedType*) = 0;
  };

  /// @brief Template dynamic cast function for ParamType derived classes.
  /// @param ParamType given param type.
  /// @return required casting type if given param type is an instance if
  //          that type, NULL otherwise.
  template <typename T>
  T* dyn_cast(ParamType* pType) {
    assert(pType && "dyn_cast does not support casting of NULL");
    return (T::enumTy == pType->getTypeId()) ? (T*)pType : NULL;
  }

  /// @brief Template dynamic cast function for ParamType derived classes
  ///        (the constant version).
  /// @param ParamType given param type.
  /// @return required casting type if given param type is an instance if
  //          that type, NULL otherwise.
  template <typename T>
  const T* dyn_cast(const ParamType* pType) {
    assert(pType && "dyn_cast does not support casting of NULL");
    return (T::enumTy == pType->getTypeId()) ? (const T*)pType : NULL;
  }

} // End SPIR namespace
#endif //__PARAMETER_TYPE_H__
