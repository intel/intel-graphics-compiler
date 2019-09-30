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
#pragma once

#include "ShaderInstruction.h"
#include "usc_gen7_types.h"

namespace USC
{

struct s_shader
{
    // version
    SHADER_VERSION_TYPE version;
    
    // shader type
    SHADER_TYPE shader_type;

    // hash code
    mutable unsigned long long hash_code;
    
    // input and output channel declarations
    struct common_decl_channel {
        char                           is_active;
        char                           is_indexed;
        SHADER_USAGE                   usage;
        unsigned int                   usage_index;
    };

    // input channel declarations
    struct in_decl_channel : public common_decl_channel {
        SHADER_INTERPOLATION_MODE      interpolation_mode;
    };

    // output channel declarations
    struct out_decl_channel : public common_decl_channel {
        char is_invariant;
    };

    // common data
    struct common_data {
        unsigned int        count;
        unsigned int        capacity;
        unsigned int        last;
    };

    // input register declarations
    struct in_decl : public common_data {
        in_decl_channel               **p_decl;
    };

    // output register declarations
    struct out_decl : public common_data {
        out_decl_channel              **p_decl;
    };

    in_decl     register_input,
                register_patch_constant_input,
                register_output_control_point_input;
    out_decl    register_output,
                register_patch_constant;

    // temporary registers
    struct bool_array : public common_data {
        char                           *p_is_declared;
    };
    bool_array register_temp;

    // pixel to sample inter phase registers
    bool_array register_pixel_to_sample;
    
    // indexed temporary registers
    struct unsigned_short_array : public common_data {
        unsigned short                 *p_size;
    } register_indexed_temp;

    // unsigned int arrays
    struct unsigned_int_array : public common_data {
        unsigned int                   *p_size;
    };

    // resources and views
    struct resource_decl {
        SHADER_RESOURCE_TYPE           data_type;
        SHADER_RESOURCE_RETURN_TYPE    return_type;
        SURFACE_FORMAT                 surface_format;
        SHADER_UAV_ACCESS_MODE         access_mode;
        unsigned int                   stride;
        unsigned int                   size_or_count;
        unsigned int                   offset;
        char                           access_coherency;
    };
    struct resource_array : public common_data {
        resource_decl                 **p_decl;
    } resource,
    view;

    // active samplers
    struct sampler_decl {
        SHADER_SAMPLER_TYPE             type;
    };
    struct sampler_array : public common_data {
            sampler_decl              **p_decl;
    } sampler;

    // active TGSM regions
    struct tgsm_decl {
        unsigned int                    byte_count;
        unsigned int                    alignment;
    };
    struct tgsm_array : public common_data {
            tgsm_decl                 **p_decl;
    } tgsm;

    // active constant registers and constant buffers
    bool_array constant_register,
    constant_buffer;

    // immediate constant buffer data
    struct imm_const_buffer {
        unsigned int                *p_data;
        unsigned int                size;
    } imm_cb;

    // signatures
    in_decl     signature_input,
                signature_patch_constant_input,
                signature_output_control_point_input;
    out_decl    signature_output,
                signature_patch_constant;

    // subroutine information for needed declarations
    struct interface_decl_info {
        unsigned int number;
        unsigned int        array_size;
        //unsigned int array_index;
        unsigned int        num_call_sites;
        unsigned int        num_func_tables;
        const unsigned int* func_tables;
    };
    
    struct interface_decl_array : public common_data {
        interface_decl_info *p_decl;
    };
    struct function_table_decl_info {
        unsigned int        num_func_bodies;
        const unsigned int* func_bodies;
    };
    struct function_table_decl_array : public common_data {
        function_table_decl_info *p_decl;
    };

    struct subroutine_decl_info {
        bool_array                function_bodies;
        function_table_decl_array function_tables;
        interface_decl_array      interfaces;
    } subroutine_info;

    struct float_denorm_mode {
        USC_FLOAT_DENORM_MODE float16_denorm_mode;
        USC_FLOAT_DENORM_MODE float32_denorm_mode;
        USC_FLOAT_DENORM_MODE float64_denorm_mode;
    } denorm_modes;

   USC_EARLY_DEPTH_STENCIL_TEST_MODE early_depth_stencil_test_mode;

    // instruction pointer stream
    struct stream : public common_data
    {
        void                       *p_data;
        unsigned int                token_count;
        unsigned char               needs_precise; //usage: boolean <0, 1>
    } instruction_stream;
    
    static const unsigned int MAX_NUM_SHADING_PHASES = 1;

    // shader-dependent context
    union
    {
        // context: hull shader
        struct
        {
            unsigned int                        input_control_point_count;
            unsigned int                        output_control_point_count;
            char                                declares_input_primitive_id;
            char                                declares_output_control_point_id;
            USC::TESSELLATOR_PARTITIONING_TYPE  tess_partitioning;
        } hull_shader;
        // context: domain shader
        struct
        {
            unsigned int                            input_control_point_count;
            USC::TESSELLATOR_DOMAIN_TYPE            tess_domain;
            SHADER_MASK                             domain_mask;
            USC::TESSELLATOR_PARTITIONING_TYPE      tess_partitioning;
            USC::TESSELLATOR_OUTPUT_PRIMITIVE_TYPE  tess_outputprimitive;
            char                                    declares_input_primitive_id;
        } domain_shader;
        // context: geometry shader
        struct
        {
            unsigned int                        max_output_vertex_count;
            unsigned int                        invocation_count;
            PRIMITIVE_TOPOLOGY_TYPE             output_type;
            GSHADER_INPUT_PRIMITIVE_TYPE        input_type;
            char                                declares_input_primitive_id;
            char                                declares_invocation_id;
        } geometry_shader;
        // context: pixel shader
        struct
        {
            char position_center;
            char position_inverted;
            CLEAR_PS clear_shader_type;
            char declared_ostencil;
            // multi rate shading
            bool is_multi_rate_shader;
            bool is_shading_phase;
            USC_SHADING_RATE shading_rate;
            unsigned int num_shading_phases;
            s_shader* shading_phase[MAX_NUM_SHADING_PHASES];
        } pixel_shader;
        // context: compute shader
        struct
        {
            unsigned int thread_group_size_dim_x;
            unsigned int thread_group_size_dim_y;
            unsigned int thread_group_size_dim_z;
        } compute_shader;
    } context;
};

// internal function for declaring register type 
char __fastcall InternalDeclareIn( 
                                        s_shader::in_decl *const       container, 
                                        unsigned int                   at,
                                        SHADER_CHANNEL                 channel, 
                                        SHADER_INTERPOLATION_MODE      interpolation_mode, 
                                        SHADER_USAGE                   usage, 
                                        unsigned int                   usage_index );

// internal function for declaring register type 
char __fastcall InternalDeclareOut( 
                                        s_shader::out_decl *const      container, 
                                        unsigned int                   at,
                                        SHADER_CHANNEL                 channel, 
                                        SHADER_USAGE                   usage, 
                                        unsigned int                   usage_index );


// internal function for declaring bool type 
char __fastcall InternalDeclareBool(
                                      s_shader::bool_array *const       container, 
                                      unsigned int                      at,
                                      char                              is_declared );

// internal function for declaring resource 
char __fastcall InternalDeclareResourceView(
                                               s_shader::resource_array *const  container, 
                                               unsigned int                     at,
                                               SHADER_RESOURCE_TYPE             data_type,
                                               SHADER_RESOURCE_RETURN_TYPE      return_type,
                                               unsigned int                     stride,
                                               unsigned int                     size_or_count,
                                               unsigned int                     offset,
                                               char                             access_coherency );

// internal function for declaring typed resource view
char __fastcall InternalDeclareTypedView(
                                               s_shader::resource_array *const container, 
                                               unsigned int                     at,
                                               SHADER_RESOURCE_TYPE             data_type,
                                               SURFACE_FORMAT                   surface_format,
                                               SHADER_UAV_ACCESS_MODE           access_mode,
                                               SHADER_RESOURCE_RETURN_TYPE      return_type,
                                               char                             access_coherency );

char __fastcall InternalDeclareSampler(
                    s_shader::sampler_array *const         container, 
                    unsigned int                           at,
                    SHADER_SAMPLER_TYPE                    type );

char __fastcall InternalDeclareTGSM(
                    s_shader::tgsm_array *const            container, 
                    unsigned int                           at,
                    unsigned int                           byte_count,
                    unsigned int                           alignment );

// internal function for declaring a function body
char __fastcall InternalDeclareFunctionBody(
                    s_shader::bool_array                 *const container,
                    unsigned int                          at);

// internal function for declaring a function table
char __fastcall InternalDeclareFunctionTable(
                    s_shader::function_table_decl_array *const container,
                    unsigned int                          at,
                    unsigned int                          num_func_bodies,
                    const unsigned int                   *func_bodies);

// internal function for declaring an interface
char __fastcall InternalDeclareInterface(
                    s_shader::interface_decl_array *const container,
                    unsigned int                          at,
                    unsigned int                          array_size,
                    unsigned int                          num_call_sites,
                    unsigned int                          num_func_tables,
                    const unsigned int                   *func_tables);

// internal function for declaring denorm handling mode in s_shader
char __fastcall InternalDeclareFloatDenormMode(
                    s_shader::float_denorm_mode * modes,
                    USC::USC_FLOAT_PRECISION floatPrecision,
                    USC::USC_FLOAT_DENORM_MODE denormMode);

// internal function for declaring early depth-stencil test handling mode in s_shader
char __fastcall InternalDeclareEarlyDepthStencilTestMode( 
                    USC_EARLY_DEPTH_STENCIL_TEST_MODE * p_testMode,
                    USC_EARLY_DEPTH_STENCIL_TEST_MODE testMode);

// internal function for declaring immediate constant buffer with inlined data in s_shader
char __fastcall InternalDeclareImmConstantBuffer(
                    s_shader::imm_const_buffer *const   container,
                    const unsigned int                  size,
                    const unsigned int*                 buffer);

// declare input register 
inline char DeclareRegisterInput(
                                   s_shader *const             p_shader,
                                   unsigned int                at,
                                   SHADER_CHANNEL              channel, 
                                   SHADER_INTERPOLATION_MODE   interpolation_mode, 
                                   SHADER_USAGE                usage, 
                                   unsigned int                usage_index ) 
{
    return 0!=p_shader
    ? InternalDeclareIn( &p_shader->register_input, at, channel, interpolation_mode, usage, usage_index )
    : 0;
};


// declare output register 
inline char DeclareRegisterOutput(
                                    s_shader *const                p_shader,
                                    unsigned int                   at,
                                    SHADER_CHANNEL                 channel, 
                                    SHADER_USAGE                   usage, 
                                    unsigned int                   usage_index )
{
    return 0!=p_shader
    ? InternalDeclareOut( &p_shader->register_output, at, channel, usage, usage_index )
    : 0;
};


// declare patch constant input
inline char DeclarePatchConstantInput(
                                        s_shader *const               p_shader,
                                        unsigned int                  at,
                                        SHADER_CHANNEL                channel, 
                                        SHADER_INTERPOLATION_MODE     interpolation_mode, 
                                        SHADER_USAGE                  usage, 
                                        unsigned int                  usage_index )
{
    return 0!=p_shader
    ? InternalDeclareIn( &p_shader->register_patch_constant_input, at, channel, interpolation_mode, usage, usage_index )
    : 0;
};


// declare patch constant output
inline char DeclarePatchConstantOutput(
                                   s_shader *const               p_shader,
                                   unsigned int                  at,
                                   SHADER_CHANNEL                channel, 
                                   SHADER_USAGE                  usage, 
                                   unsigned int                  usage_index )
{
    return 0!=p_shader
    ? InternalDeclareOut( &p_shader->register_patch_constant, at, channel, usage, usage_index )
    : 0;
};


// declare temp register 
inline char DeclareRegisterTemp(
                                  s_shader *const   p_shader,
                                  unsigned int      at )
{
    return 0!=p_shader
    ? InternalDeclareBool( &p_shader->register_temp, at, 1 )
    : 0;
};

// declare pixel to sample inter phase register
inline char DeclarePixelToSample(
                                s_shader *const   p_shader,
                                unsigned int      at )
{
    return 0!=p_shader
    ? InternalDeclareBool( &p_shader->register_pixel_to_sample, at, 1 )
    : 0;
}

// declare temp register 
char __fastcall DeclareRegisterIndexedTemp(
                                              s_shader *const   p_shader,
                                              unsigned int      at,
                                              unsigned int      size );

// declare sampler 
inline char DeclareResource(
                             s_shader *const               p_shader,
                             unsigned int                  at,
                             SHADER_RESOURCE_TYPE          data_type,
                             SHADER_RESOURCE_RETURN_TYPE   return_type,
                             unsigned int                  stride,
                             unsigned int                  size_or_count,
                             unsigned int                  offset,
                             char                          access_coherency )
{
    return 0!=p_shader
    ? InternalDeclareResourceView( &p_shader->resource, at,data_type, return_type, stride, size_or_count, offset, access_coherency )
    : 0;
};

// declare view 
inline char DeclareView(
                         s_shader *const               p_shader,
                         unsigned int                  at,
                         SHADER_RESOURCE_TYPE          data_type,
                         SHADER_RESOURCE_RETURN_TYPE   return_type,
                         unsigned int                  stride,
                         unsigned int                  size_or_count,
                         unsigned int                  offset,
                         char                          access_coherency )
{
    return 0!=p_shader
    ? InternalDeclareResourceView( &p_shader->view, at,data_type, return_type, stride, size_or_count, offset, access_coherency )
    : 0;
};

// declare typed view 
inline char DeclareTypedView(
                         s_shader *const               p_shader,
                         unsigned int                  at,
                         SHADER_RESOURCE_TYPE          data_type,
                         const SURFACE_FORMAT          surface_format,
                         const SHADER_UAV_ACCESS_MODE  access_mode,
                         SHADER_RESOURCE_RETURN_TYPE   return_type,
                         char                          access_coherency )
{
    return 0!=p_shader
    ? InternalDeclareTypedView( &p_shader->view, at, data_type, surface_format, access_mode, return_type, access_coherency )
    : 0;
};

// declare sampler 
inline char DeclareSampler(
                            s_shader *const p_shader,
                            unsigned int    at,
                            SHADER_SAMPLER_TYPE type )
{
    return 0!=p_shader
    ? InternalDeclareSampler( &p_shader->sampler, at, type )
    : 0;
};

// declare constant register 
inline char DeclareConstantRegister(
                                      s_shader *const   p_shader,
                                      unsigned int      at )
{
    return 0!=p_shader
    ? InternalDeclareBool( &p_shader->constant_register, at, 1 )
    : 0;
};

// declare constant buffer 
inline char DeclareConstantBuffer(
                                    s_shader *const p_shader,
                                    unsigned int    at )
{
    return 0!=p_shader
    ? InternalDeclareBool( &p_shader->constant_buffer, at, 1 )
    : 0;
};

// declare constant buffer 
inline char DeclareImmConstantBuffer(
                                    s_shader *const p_shader,
                                    const unsigned int sizeInUints,
                                    const unsigned int* srcBuffer )
{
    return 0!=p_shader
    ? InternalDeclareImmConstantBuffer(&p_shader->imm_cb, sizeInUints, srcBuffer)
    : 0;
};

// declare Thread Group Shared Memory region    
inline char DeclareRegisterTGSM(
                                  s_shader *const p_shader,
                                  unsigned int    at,
                                  unsigned int    byte_count,
                                  unsigned int    alignment ) 
{
    return 0!=p_shader
    ? InternalDeclareTGSM( &p_shader->tgsm, at, byte_count, alignment )
    : 0;
};

// declare input signature 
inline char DeclareSignatureInput(
                                    s_shader *const            p_shader,
                                    unsigned int               at,
                                    SHADER_CHANNEL             channel, 
                                    SHADER_INTERPOLATION_MODE  interpolation_mode, 
                                    SHADER_USAGE               usage, 
                                    unsigned int               usage_index )
{
    return 0!=p_shader
    ? InternalDeclareIn( &p_shader->signature_input, at, channel, interpolation_mode, usage, usage_index )
    : 0;
};

// declare output signature 
inline char DeclareSignatureOutput(
                                     s_shader *const               p_shader,
                                     unsigned int                  at,
                                     SHADER_CHANNEL                channel, 
                                     SHADER_USAGE                  usage, 
                                     unsigned int                  usage_index )
{
    return 0!=p_shader
    ? InternalDeclareOut( &p_shader->signature_output, at, channel, usage, usage_index )
    : 0;
};

// declare patch constant input signature
inline char DeclareSignaturePatchConstantInput(
                                                 s_shader *const               p_shader,
                                                 unsigned int                  at,
                                                 SHADER_CHANNEL                channel, 
                                                 SHADER_INTERPOLATION_MODE     interpolation_mode, 
                                                 SHADER_USAGE                  usage, 
                                                 unsigned int                  usage_index )
{
    return 0!=p_shader
    ? InternalDeclareIn( &p_shader->signature_patch_constant_input, at, channel, interpolation_mode, usage, usage_index )
    : 0;
};

// declare patch constant output signature
inline char DeclareSignaturePatchConstant( 
                                             s_shader *const               p_shader,
                                             unsigned int                  at,
                                             SHADER_CHANNEL                channel, 
                                             SHADER_USAGE                  usage, 
                                             unsigned int                  usage_index )
{
    return 0!=p_shader
    ? InternalDeclareOut( &p_shader->signature_patch_constant, at, channel, usage, usage_index )
    : 0;
};

// declare function body
inline char DeclareFunctionBody(
                                     s_shader *const               p_shader,
                                     unsigned int                  at)
{
    return 0!=p_shader
    ? InternalDeclareFunctionBody( &p_shader->subroutine_info.function_bodies, at )
    : 0;
}

// declare function table
inline char DeclareFunctionTable(
                                     s_shader *const               p_shader,
                                     unsigned int                  at,
                                     unsigned int                  num_func_bodies,
                                     const unsigned int           *func_bodies)
{
    return 0!=p_shader
    ? InternalDeclareFunctionTable( &p_shader->subroutine_info.function_tables, at, num_func_bodies, func_bodies )
    : 0;
}

// declare interface
inline char DeclareInterface(
                                     s_shader *const               p_shader,
                                     unsigned int                  at,
                                     unsigned int                  array_size,
                                     unsigned int                  num_call_sites,
                                     unsigned int                  num_func_tables,
                                     const unsigned int           *func_tables)
{
    return 0!=p_shader
    ? InternalDeclareInterface( &p_shader->subroutine_info.interfaces, at, array_size, num_call_sites, num_func_tables, func_tables )
    : 0;
};

// declare the way of handling of floating point denormalized numbers
inline char DeclareFloatDenormMode( 
                                     s_shader * const p_shader,
                                     const USC_FLOAT_PRECISION precision,
                                     const USC_FLOAT_DENORM_MODE denormMode)
{
    return 0 != p_shader ?
        InternalDeclareFloatDenormMode( &p_shader->denorm_modes, precision, denormMode ) :
        0;
}


inline char DeclareEarlyDepthStencilTestMode(
    s_shader * const p_shader,
    const USC_EARLY_DEPTH_STENCIL_TEST_MODE testMode)
{
    return 0 != p_shader ?
        InternalDeclareEarlyDepthStencilTestMode(&p_shader->early_depth_stencil_test_mode, testMode) :
        0;
}


// return hash code for shader
unsigned long long __fastcall Hash(
                                   const s_shader *const p_shader );



} // namespace USC
