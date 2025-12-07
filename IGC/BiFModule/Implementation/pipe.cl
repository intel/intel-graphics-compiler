/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// TODO: current producer leaves off PacketAlignment argument.  We don't use
// it but that should probably be fixed at some point.

 #include "../Headers/spirv.h"

 #if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
// Pipe Instructions

/////////////////////////////////////////////////////////////////////
// Pipe Helper Functions (static)
/////////////////////////////////////////////////////////////////////

// Total size:  129 ( + 1 because of a variable length array).
// RT must allocate 128 chars for pipe control at the beginning of
// contiguous memory. This buffer must be aligned by CACHE_LINE.
#define INTEL_PIPE_HEADER_RESERVED_SPACE    128
#define CACHE_LINE 64

typedef struct _tag_pipe_control_intel_t
{
    // The pipe packet size is always passed as an implicit argument (i32 immediate)

    // Total number of packets in the pipe.  This value must be
    // set by the host when the pipe is created. Pipe cannot accommodate
    // more than pipe_max_packets - 1 packets. So RT must allocate memory
    // for one more packet.
    const uint pipe_max_packets;

    // The pipe head and tail must be set by the host when
    // the pipe is created.  They will probably be set to zero,
    // though as long as head equals tail, it doesn't matter
    // what they are initially set to.
    uint head;  // Head Index, for reading: [0, pipe_max_packets)
    uint tail;  // Tail Index, for writing: [0, pipe_max_packets)
    char pad0[CACHE_LINE - 2 * sizeof(uint) - sizeof(uint)];

    // This controls whether the pipe is unlocked, locked for
    // reading, or locked for writing.  If it is zero, the pipe
    // is unlocked.  If it is positive, it is locked for writing.
    // If it is negative, it is locked for reading. This must
    // be set to zero by the host when the pipe is created.
    uint lock;
    char pad1[CACHE_LINE - sizeof(int)];
    // The end of the control structure.

    // Packets storage begins right after the pipe control.
    // This is a compile time shift to that storage (the variable length array).
    char base[1];
} pipe_control_intel_t;

#define rtos(r) ((uint)(__builtin_IB_cast_object_to_generic_ptr((r))))
#define stor(s) (__builtin_IB_convert_object_type_to_spirv_reserveid(((void*)(size_t)(s))))


static void OVERLOADABLE copy_data( __generic void *dst, __generic const void *src, uint numBytes )
{
  uint remBytes = numBytes;
#define CHUNK_SIZE(TYPE) \
  { \
    uint numCopies = remBytes / sizeof(TYPE); \
    TYPE *dstc = (TYPE*)((uchar*)dst); \
    const TYPE *srcc = (const TYPE*)((const uchar*)src); \
    uint offset = (numBytes - remBytes) / sizeof(TYPE); \
    for (uint i=0; i < numCopies; i++) { \
      dstc[i + offset] = srcc[i + offset]; \
    } \
    remBytes -= numCopies * sizeof(TYPE); \
  }

  CHUNK_SIZE(uint4)
  CHUNK_SIZE(uint2)
  CHUNK_SIZE(uint)
  CHUNK_SIZE(ushort)
  CHUNK_SIZE(uchar)

#undef CHUNK_SIZE
}

// TODO: Remove this CTH dependence!

INLINE static __spirv_ReserveId SetInvalidRid()
{
    return CLK_NULL_RESERVE_ID;
}

INLINE static uint advance( __global pipe_control_intel_t* p, uint base, uint stride )
{
    return (p->pipe_max_packets <= base + stride) ?
        (base + stride - p->pipe_max_packets) :
        (base + stride);
}

INLINE static __spirv_ReserveId create_reserve_id( uint idx )
{
  return stor( idx | INTEL_PIPE_RESERVE_ID_VALID_BIT );
}

INLINE static uint extract_index( __spirv_ReserveId rid )
{
  return (uint)(rtos(rid) & ~INTEL_PIPE_RESERVE_ID_VALID_BIT);
}

INLINE static bool intel_lock_pipe_read( __global pipe_control_intel_t* p )
{
    int lock = __spirv_AtomicLoad( ( global int* )&p->lock, Device, Relaxed );
    while( lock <= 0 )
    {
        int newLock = lock - 1;
        if (__spirv_AtomicCompareExchange(
                ( global int* ) &p->lock,
                Device,
                SequentiallyConsistent,
                SequentiallyConsistent,
                newLock,
                lock) == lock)
        {
            return true;
        }
        else
        {
            lock = p->lock;
        }
    }
    return false;
}

static void intel_unlock_pipe_read( __global pipe_control_intel_t* p )
{
    __spirv_AtomicIIncrement(
            ( global int* ) &p->lock,
            Device,
            SequentiallyConsistent );
    // OK to inc, since we must have locked.
}

static bool intel_lock_pipe_write( __global  pipe_control_intel_t* p )
{
    int lock = __spirv_AtomicLoad( ( global int* )&p->lock, Device, Relaxed );

    while( lock >= 0 )
    {
        int newLock = lock + 1;
        if( __spirv_AtomicCompareExchange(
                    ( global int* ) &p->lock,
                    Device,
                    SequentiallyConsistent,
                    SequentiallyConsistent,
                    newLock,
                    lock) == lock )
        {
            return true;
        }
        else
        {
            lock = p->lock;
        }
    }
    return false;
}

static void intel_unlock_pipe_write( __global pipe_control_intel_t* p )
{
    __spirv_AtomicIDecrement(
            ( global int* ) &p->lock,
            Device,
            SequentiallyConsistent );
    // OK to dec, since we must have locked.
}

static uint read_head( __global pipe_control_intel_t* p )
{
    int head = __spirv_AtomicLoad(
            ( global int* )&p->head,
            Device,
            SequentiallyConsistent );

    return head;
}

static uint read_tail( __global pipe_control_intel_t* p )
{
    int tail = __spirv_AtomicLoad(
            ( global int* )&p->tail,
            Device,
            SequentiallyConsistent );

    return tail;
}

bool __intel_is_first_work_group_item( void );

/////////////////////////////////////////////////////////////////////
// END - Pipe Helper Functions (static)
/////////////////////////////////////////////////////////////////////

int __attribute__((overloadable)) __spirv_ReadPipe( __spirv_Pipe_ro Pipe, generic char *Pointer, int PacketSize/*, int PacketAlignment */)
{
    __global pipe_control_intel_t* p = (__global pipe_control_intel_t*)(Pipe);
#if defined(_DEBUG)
    printf( "ENTER: read_pipe\n" );
#endif
    int retVal = -1;

    if( intel_lock_pipe_read( p ) )
    {
        uint head       = read_head( p );
        const uint tail = read_tail( p );

        while( true )
        {
#if defined(_DEBUG)
            printf( "\t read_pipe: Initially, head = %d\n", head );
#endif
            const uint newHead = advance(p, head, 1);
            bool wrap = newHead < head;

            if( !wrap & ( head <= tail & tail < newHead ) )    // Underflow
            {
#if defined(_DEBUG)
                printf( "\t read_pipe: Underflow!  num_packets = %d, head = %d, tail = %d\n",
                        1, head, tail );
#endif
                break;
            }
            else if ( wrap & ( head <= tail | tail < newHead ) )
            {
#if defined(_DEBUG)
                printf( "\t read_pipe: Wrap and Underflow!  num_packets = %d, head = %d, tail = %d\n",
                        1, head, tail );
#endif
                break;
            }

            if( __spirv_AtomicCompareExchange(
                        ( global int* ) &p->head,
                        Device,
                        SequentiallyConsistent,
                        SequentiallyConsistent,
                        newHead,
                        head ) == head )
            {
                copy_data( Pointer, (generic const void*)(p->base + head * PacketSize), PacketSize );
                intel_unlock_pipe_read( p );
                retVal = 0;
                break;  // Success.
            }
            else
            {
#if defined(_DEBUG)
                printf( "\t read_pipe: Iterate!  old head = %d, new head = %d\n", head, newHead );
#endif
                head = p->head;
            }
        }

    }

#if defined(_DEBUG)
    printf( "EXIT: read_pipe returned %d\n", retVal );
#endif
    return retVal;
}


int __attribute__((overloadable)) __spirv_WritePipe( __spirv_Pipe_wo Pipe, generic char *Pointer, int PacketSize/*, int PacketAlignment */)
{
#if defined(_DEBUG)
    printf( "ENTER: write_pipe\n" );
#endif
    __global pipe_control_intel_t* p = (__global pipe_control_intel_t*)Pipe;

    int retVal = -1;
    if( intel_lock_pipe_write( p ) )
    {
        const uint head = read_head( p );
        uint tail       = read_tail( p );

        while( true )
        {
#if defined(_DEBUG)
            printf( "\t write_pipe: Initially, tail = %d\n", tail );
#endif
            const uint newTail = advance(p, tail, 1);
            bool wrap = newTail < tail;

            if( !wrap & ( tail < head & head <= newTail ) ) {
#if defined(_DEBUG)
                printf( "\t write_pipe: Overflow!  num_packets = %d, head = %d, tail = %d\n",
                        1, head, tail );
#endif
                break;
            }
            else if( wrap & ( tail < head | head <= newTail ) )
            {
#if defined(_DEBUG)
                printf( "\t write_pipe: Wrap + Overflow!  num_packets = %d, pipe_max_packets = %d, head = %d, tail = %d\n",
                        1, p->pipe_max_packets, head, tail );
#endif
                break;
            }

            if( __spirv_AtomicCompareExchange(
                        ( global int* ) &p->tail,
                        Device,
                        SequentiallyConsistent,
                        SequentiallyConsistent,
                        newTail,
                        tail ) == tail )

            {
                copy_data( (generic void*)(p->base + tail * PacketSize), (generic const void*)Pointer, PacketSize );
                intel_unlock_pipe_write( p );
                retVal = 0;
                break;  // Success.
            }
            else
            {
#if defined(_DEBUG)
                printf( "\t write_pipe: Iterate!  old tail = %d, got %d\n", tail, newTail );
#endif
                tail = p->tail;
            }
        }
    }

#if defined(_DEBUG)
    printf( "EXIT: write_pipe returned %d\n", retVal );
#endif
    return retVal;
}

int __attribute__((overloadable)) __spirv_ReservedReadPipe( __spirv_Pipe_ro Pipe, __spirv_ReserveId ReserveId, int Index, generic char *Pointer, int PacketSize/*, int PacketAlignment */)
{
#if defined(_DEBUG)
  printf( "ENTER: read_pipe( reserve_id = %08X, index = %d)\n", rtos(ReserveId), Index );
#endif
  __global pipe_control_intel_t* p = (__global pipe_control_intel_t*)Pipe;
  int retVal = -1;

  if( __spirv_IsValidReserveId( ReserveId ) )
  {
    const uint base_idx = extract_index(ReserveId);
    generic const void * src = p->base + PacketSize * advance(p, base_idx, Index);
    copy_data(Pointer, src, PacketSize);
    retVal = 0;
  }

#if defined(_DEBUG)
  printf( "EXIT: read_pipe returned %d\n", retVal );
#endif
  return retVal;
}

// write_pipe with 4 explicit arguments
int __attribute__((overloadable)) __spirv_ReservedWritePipe( __spirv_Pipe_wo Pipe, __spirv_ReserveId ReserveId, int Index, generic char *Pointer, int PacketSize/*, int PacketAlignment */)
{
#if defined(_DEBUG)
  printf( "ENTER: write_pipe( reserve_id = %08X, index = %d)\n", rtos(ReserveId), Index );
#endif
  __global pipe_control_intel_t* p = (__global pipe_control_intel_t*)Pipe;
  int retVal = -1;

  if(__spirv_IsValidReserveId(ReserveId))
  {
    const uint base_idx = extract_index(ReserveId);
    generic void * dst = p->base + PacketSize * advance(p, base_idx, Index);
    copy_data(dst, Pointer, PacketSize);
    retVal = 0;
  }

#if defined(_DEBUG)
  printf( "EXIT: write_pipe returned %d\n", retVal );
#endif
  return retVal;
}

__spirv_ReserveId __attribute__((overloadable)) __spirv_ReserveReadPipePackets( __spirv_Pipe_ro Pipe, int NumPackets, int PacketSize/*, int PacketAlignment */)
{
#if defined(_DEBUG)
    printf( "ENTER: reserve_read_pipe( num_packets = %d)\n", NumPackets );
#endif
    __global pipe_control_intel_t* p = (__global pipe_control_intel_t*)Pipe;
    __spirv_ReserveId retVal = SetInvalidRid();

    // The maximum possible reservation number is (_pipe_max_packets - 1) packets.
    if( ( NumPackets >= p->pipe_max_packets ) | ( 0 == NumPackets ) )
    {
#if defined(_DEBUG)
        printf( "\t reserve_read_pipe: Sanity check failed!  num_packets = %d, pipe_max_packets = %d\n",
                NumPackets, p->pipe_max_packets );
#endif
    }
    else if( intel_lock_pipe_read( p ) )
    {
        uint head = read_head( p );
        const uint tail = read_tail( p );

        while( true )
        {
            const uint newHead = advance(p, head, NumPackets);
            bool wrap = newHead < head;
#if defined(_DEBUG)
            printf( "\t reserve_read_pipe: Initially, head = %d, new head = %d\n", head, newHead );
#endif

            if( !wrap & ( head <= tail & tail < newHead ) )    // Underflow
            {
#if defined(_DEBUG)
                printf( "\t reserve_read_pipe: Underflow!  num_packets = %d, head = %d, tail = %d\n",
                        num_packets, head, tail );
#endif
                break;
            }
            else if ( wrap & ( head <= tail | tail < newHead ) )
            {
#if defined(_DEBUG)
                printf( "\t reserve_read_pipe: Wrap and Underflow!  num_packets = %d, head = %d, tail = %d\n",
                        NumPackets, head, tail );
#endif
                break;
            }

            if( __spirv_AtomicCompareExchange(
                        ( global int* ) &p->head,
                        Device,
                        SequentiallyConsistent,
                        SequentiallyConsistent,
                        newHead,
                        head ) == head )
            {
                retVal = create_reserve_id( head );
                // the lock must be unlocked with following commit
                break;  // Success.
            }
            else
            {
#if defined(_DEBUG)
                printf( "\t read_pipe: Iterate!  old head = %d, new head = %d\n", head, newHead );
#endif
                head = p->head;
            }
        }

        if( !__spirv_IsValidReserveId( retVal ) )
        {
            intel_unlock_pipe_read( p );
        }
        // Else: note, no unlock!  The pipe will be unlocked as part of committing
        // the reservation.
    }

#if defined(_DEBUG)
    printf( "EXIT: reserve_read_pipe returned %08X\n", rtos(retVal) );
#endif
    return retVal;
}


__spirv_ReserveId __attribute__((overloadable)) __spirv_ReserveWritePipePackets(__spirv_Pipe_wo Pipe, int NumPackets, int PacketSize/*, int PacketAlignment */)
{
#if defined(_DEBUG)
    printf( "ENTER: reserve_write_pipe( num_packets = %d)\n", NumPackets );
#endif
    __global pipe_control_intel_t* p = (__global pipe_control_intel_t*)Pipe;
    __spirv_ReserveId retVal = SetInvalidRid();

    if( ( NumPackets >= p->pipe_max_packets ) | ( 0 == NumPackets ) )
    {
#if defined(_DEBUG)
        printf( "\t reserve_write_pipe: Sanity check failed!  num_packets = %d, pipe_max_packets = %d\n",
                NumPackets, p->pipe_max_packets );
#endif
    }
    else if( intel_lock_pipe_write( p ) )
    {
        const uint head = read_head( p );
        uint tail = read_tail( p );

        while( true )
        {
            const uint newTail = advance(p, tail, NumPackets);
#if defined(_DEBUG)
            printf( "\t reserve_write_pipe: Initially, tail = %d, new tail = %d\n", tail, newTail );
#endif
            bool wrap = newTail < tail;

            if( !wrap & ( tail < head & head <= newTail ) ) {
#if defined(_DEBUG)
                printf( "\t reserve_write_pipe: Overflow!  num_packets = %d, head = %d, tail = %d\n",
                        num_packets, head, tail );
#endif
                break;
            }
            else if( wrap & ( tail < head | head <= newTail ) )
            {
#if defined(_DEBUG)
                printf( "\t reserve_write_pipe: Wrap + Overflow!  num_packets = %d, pipe_max_packets = %d, head = %d, tail = %d\n",
                        num_packets, p->pipe_max_packets, head, tail );
#endif
                break;
            }

            if( __spirv_AtomicCompareExchange(
                        ( global int* ) &p->tail,
                        Device,
                        SequentiallyConsistent,
                        SequentiallyConsistent,
                        newTail,
                        tail ) == tail )
            {
                retVal = create_reserve_id(tail);
                break;  // Success.
                // the lock must be unlocked by the following commit
            }
            else
            {
#if defined(_DEBUG)
                printf( "\t reserve_write_pipe: Iterate!  old tail = %d, new tail = %d\n", tail, newTail );
#endif
                tail = p->tail;
            }
        }

        if(!__spirv_IsValidReserveId(retVal) )
        {
            intel_unlock_pipe_write( p );
        }
        // Otherwise, note: No unlock!  The pipe will be unlocked as part of committing
        // the reservation.
    }

#if defined(_DEBUG)
    printf( "EXIT: reserve_write_pipe returned %08X\n", retVal );
#endif
    return retVal;
}


void __attribute__((overloadable)) __spirv_CommitReadPipe( __spirv_Pipe_ro Pipe, __spirv_ReserveId ReserveId, int PacketSize/*, int PacketAlignment */)
{
#if defined(_DEBUG)
    printf( "ENTER: commit_read_pipe( reserve_id = %08X)\n", ReserveId );
#endif
    __global pipe_control_intel_t* p = (__global pipe_control_intel_t*)Pipe;

    intel_unlock_pipe_read( p );
#if defined(_DEBUG)
    printf( "EXIT: commit_read_pipe\n" );
#endif
}


void __attribute__((overloadable)) __spirv_CommitWritePipe( __spirv_Pipe_wo Pipe, __spirv_ReserveId ReserveId, int PacketSize/*, int PacketAlignment */)
{
#if defined(_DEBUG)
    printf( "ENTER: commit_write_pipe( reserve_id = %08X)\n", rtos(ReserveId) );
#endif
    __global pipe_control_intel_t* p = (__global pipe_control_intel_t*)Pipe;

    intel_unlock_pipe_write( p );
#if defined(_DEBUG)
    printf( "EXIT: commit_write_pipe\n");
#endif
}


bool __attribute__((overloadable)) __spirv_IsValidReserveId( __spirv_ReserveId  ReserveId )
{
    return ( rtos(ReserveId) & INTEL_PIPE_RESERVE_ID_VALID_BIT ) != 0;
}

uint __attribute__((overloadable)) __spirv_GetNumPipePackets( __spirv_Pipe_ro Pipe, int PacketSize/*, int PacketAlignment */)
{
  __global pipe_control_intel_t* p = (__global pipe_control_intel_t*)Pipe;

  // load from tail shouldn't be moved before load from head so acquire head first then relaxively load tail
  uint head = read_head( p );
  uint tail = read_tail( p );

  return (head <= tail) ? (tail - head) : (p->pipe_max_packets - head + tail);
}

uint __attribute__((overloadable)) __spirv_GetNumPipePackets( __spirv_Pipe_wo Pipe, int PacketSize/*, int PacketAlignment */)
{
  __global pipe_control_intel_t* p = (__global pipe_control_intel_t*)Pipe;

  // load from tail shouldn't be moved before load from head so acquire head first then relaxively load tail
  uint head = read_head( p );
  uint tail = read_tail( p );

  return (head <= tail) ? (tail - head) : (p->pipe_max_packets - head + tail);
}

uint __attribute__((overloadable)) __spirv_GetMaxPipePackets( __spirv_Pipe_ro Pipe, int PacketSize/*, int PacketAlignment */)
{
  __global pipe_control_intel_t* p = (__global pipe_control_intel_t*)Pipe;
  return p->pipe_max_packets - 1;
}

uint __attribute__((overloadable)) __spirv_GetMaxPipePackets( __spirv_Pipe_wo Pipe, int PacketSize/*, int PacketAlignment */)
{
  __global pipe_control_intel_t* p = (__global pipe_control_intel_t*)Pipe;
  return p->pipe_max_packets - 1;
}

static uint __intel_pipe_broadcast(uint val)
{
  return __spirv_GroupBroadcast(Workgroup, as_int(val), 0);
}

__spirv_ReserveId __attribute__((overloadable)) __spirv_GroupReserveReadPipePackets( int Execution, __spirv_Pipe_ro Pipe, int NumPackets, int PacketSize/*, int PacketAlignment */)
{
    __spirv_ReserveId rid = SetInvalidRid();

    if( Execution == Subgroup )
    {
        if( __spirv_BuiltInSubgroupLocalInvocationId() == 0 )
        {
            rid = __spirv_ReserveReadPipePackets( Pipe, NumPackets, PacketSize/*, PacketAlignment */);
        }

        __spirv_ReserveId result = stor(__spirv_GroupBroadcast(Subgroup, as_int(rtos(rid)), 0));
        return result;
    }
    else
    {
        if( __intel_is_first_work_group_item() )
        {
            rid = __spirv_ReserveReadPipePackets( Pipe, NumPackets, PacketSize/*, PacketAlignment */);
        }

        __spirv_ReserveId result = stor(__intel_pipe_broadcast(rtos(rid)));
        return result;
    }
}


__spirv_ReserveId __attribute__((overloadable)) __spirv_GroupReserveWritePipePackets( int Execution, __spirv_Pipe_wo Pipe, int NumPackets, int PacketSize/*, int PacketAlignment */)
{
    __spirv_ReserveId rid = SetInvalidRid();

    if( Execution == Subgroup )
    {
        if( __spirv_BuiltInSubgroupLocalInvocationId() == 0 )
        {
            rid = __spirv_ReserveWritePipePackets( Pipe, NumPackets, PacketSize/*, PacketAlignment */);
        }

        __spirv_ReserveId result = stor(__spirv_GroupBroadcast(Subgroup, as_int(rtos(rid)), 0));
        return result;
    }
    else
    {
        if( __intel_is_first_work_group_item() )
        {
            rid = __spirv_ReserveWritePipePackets( Pipe, NumPackets, PacketSize/*, PacketAlignment */);
        }

        __spirv_ReserveId result = stor(__intel_pipe_broadcast(rtos(rid)));
        return result;
    }
}


void __attribute__((overloadable)) __spirv_GroupCommitReadPipe( int Execution, __spirv_Pipe_ro Pipe, __spirv_ReserveId ReserveId, int PacketSize/*, int PacketAlignment */)
{
    if( Execution == Subgroup )
    {
        if (__spirv_BuiltInSubgroupLocalInvocationId() == 0)
        {
            __spirv_CommitReadPipe( Pipe, ReserveId, PacketSize/*, PacketAlignment */);
        }
    }
    else
    {
        if( __intel_is_first_work_group_item() )
        {
            __spirv_CommitReadPipe( Pipe, ReserveId, PacketSize/*, PacketAlignment */);
        }
    }

    __spirv_ControlBarrier( Execution, Execution, Relaxed );
}

void __attribute__((overloadable)) __spirv_GroupCommitWritePipe(int Execution, __spirv_Pipe_wo Pipe, __spirv_ReserveId ReserveId, int PacketSize/*, int PacketAlignment*/)
{
    if( Execution == Subgroup )
    {
        if (__spirv_BuiltInSubgroupLocalInvocationId() == 0)
        {
            __spirv_CommitWritePipe( Pipe, ReserveId, PacketSize/*, PacketAlignment */);
        }
    }
    else
    {
        if( __intel_is_first_work_group_item() )
        {
            __spirv_CommitWritePipe( Pipe, ReserveId, PacketSize/*, PacketAlignment */);
        }
    }

    __spirv_ControlBarrier( Execution, Execution, Relaxed );
}

int __attribute__((overloadable)) __spirv_ReadPipe( __spirv_Pipe_ro Pipe, generic char *Pointer, int PacketSize, int PacketAlignment )
{
  return __spirv_ReadPipe( Pipe, Pointer, PacketSize );
}

int __attribute__((overloadable)) __spirv_WritePipe( __spirv_Pipe_wo Pipe, generic char *Pointer, int PacketSize, int PacketAlignment )
{
  return __spirv_WritePipe( Pipe, Pointer, PacketSize );
}

int __attribute__((overloadable)) __spirv_ReservedReadPipe( __spirv_Pipe_ro Pipe, __spirv_ReserveId ReserveId, int Index, generic char *Pointer, int PacketSize, int PacketAlignment )
{
  return __spirv_ReservedReadPipe( Pipe, ReserveId, Index, Pointer, PacketSize );
}

int __attribute__((overloadable)) __spirv_ReservedWritePipe( __spirv_Pipe_wo Pipe, __spirv_ReserveId ReserveId, int Index, generic char *Pointer, int PacketSize, int PacketAlignment )
{
  return __spirv_ReservedWritePipe( Pipe, ReserveId, Index, Pointer, PacketSize );
}

__spirv_ReserveId __attribute__((overloadable)) __spirv_ReserveReadPipePackets( __spirv_Pipe_ro Pipe, int NumPackets, int PacketSize, int PacketAlignment )
{
  return __spirv_ReserveReadPipePackets( Pipe, NumPackets, PacketSize );
}

__spirv_ReserveId __attribute__((overloadable)) __spirv_ReserveWritePipePackets( __spirv_Pipe_wo Pipe, int NumPackets, int PacketSize, int PacketAlignment )
{
  return __spirv_ReserveWritePipePackets( Pipe, NumPackets, PacketSize );
}

void __attribute__((overloadable)) __spirv_CommitReadPipe( __spirv_Pipe_ro Pipe, __spirv_ReserveId ReserveId, int PacketSize, int PacketAlignment )
{
  __spirv_CommitReadPipe( Pipe, ReserveId, PacketSize );
}

void __attribute__((overloadable)) __spirv_CommitWritePipe( __spirv_Pipe_wo Pipe, __spirv_ReserveId ReserveId, int PacketSize, int PacketAlignment )
{
  __spirv_CommitWritePipe( Pipe, ReserveId, PacketSize );
}

uint __attribute__((overloadable)) __spirv_GetNumPipePackets( __spirv_Pipe_ro Pipe, int PacketSize, int PacketAlignment )
{
  return __spirv_GetNumPipePackets( Pipe, PacketSize );
}

uint __attribute__((overloadable)) __spirv_GetNumPipePackets( __spirv_Pipe_wo Pipe, int PacketSize, int PacketAlignment )
{
  return __spirv_GetNumPipePackets( Pipe, PacketSize );
}

uint __attribute__((overloadable)) __spirv_GetMaxPipePackets( __spirv_Pipe_ro Pipe, int PacketSize, int PacketAlignment )
{
  return __spirv_GetMaxPipePackets( Pipe, PacketSize );
}

uint __attribute__((overloadable)) __spirv_GetMaxPipePackets( __spirv_Pipe_wo Pipe, int PacketSize, int PacketAlignment )
{
  return __spirv_GetMaxPipePackets( Pipe, PacketSize );
}

__spirv_ReserveId __attribute__((overloadable)) __spirv_GroupReserveReadPipePackets( int Execution, __spirv_Pipe_ro Pipe, int NumPackets, int PacketSize, int PacketAlignment )
{
  return __spirv_GroupReserveReadPipePackets( Execution, Pipe, NumPackets, PacketSize );
}

__spirv_ReserveId __attribute__((overloadable)) __spirv_GroupReserveWritePipePackets( int Execution, __spirv_Pipe_wo Pipe, int NumPackets, int PacketSize, int PacketAlignment )
{
  return __spirv_GroupReserveWritePipePackets( Execution, Pipe, NumPackets, PacketSize );
}

void __attribute__((overloadable)) __spirv_GroupCommitReadPipe( int Execution, __spirv_Pipe_ro Pipe, __spirv_ReserveId ReserveId, int PacketSize, int PacketAlignment )
{
  __spirv_GroupCommitReadPipe( Execution, Pipe, ReserveId, PacketSize );
}

void __attribute__((overloadable)) __spirv_GroupCommitWritePipe( int Execution, __spirv_Pipe_wo Pipe, __spirv_ReserveId ReserveId, int PacketSize, int PacketAlignment )
{
  __spirv_GroupCommitWritePipe( Execution, Pipe, ReserveId, PacketSize );
}
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
