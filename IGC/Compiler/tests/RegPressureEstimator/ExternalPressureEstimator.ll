; UNSUPPORTED: system-windows, llvm-17-plus
; REQUIRES: regkeys
; RUN: igc_opt --typed-pointers --igc-pressure-printer -S --disable-output --regkey=RegPressureVerbocity=1 < %s 2>&1 | FileCheck %s

define spir_func void @baz() nounwind {
entry:
  ret void
}

define spir_func void @bar() nounwind {
entry:
  ret void
}

define spir_func void @main(double addrspace(1)* %base, i64 %offset, i64 %I, i64 %J) nounwind {
entry:
  %baseArith = ptrtoint double addrspace(1)* %base to i64
  %basePtr = mul nuw nsw i64 %offset, 207368
  %offsetI = mul nsw i64 %I, 1288
  %offsetJ = shl nsw i64 %J, 3

  %a0 = add i64 %baseArith, 100780848
  %a1 = add i64 %a0, %basePtr
  %a2 = add i64 %a1, %offsetI
  %a3 = add i64 %a2, %offsetJ
  %a4 = inttoptr i64 %a3 to double addrspace(1)*
  %a5 = inttoptr i64 %a3 to double addrspace(1)*

  call spir_func void @foo();
  call spir_func void @baz();
  call spir_func void @dave();

  %r0 = load double, double addrspace(1)* %a4, align 8
  %rr0 = fmul double %r0, 2.0
  store double %rr0, double addrspace(1)* %a5

  ret void
}

define spir_func void @main_2() nounwind {
entry:
  %alloca0 = alloca i32
  %alloca1 = alloca i32
  %alloca2 = alloca i32
  call spir_func void @dave();
  call spir_func void @foo();
  store i32 0, i32* %alloca0
  store i32 0, i32* %alloca1
  store i32 0, i32* %alloca2

  ret void;
}

define spir_func void @foo() nounwind {
entry:

  %alloca = alloca i32
  call spir_func void @bar();
  call spir_func void @baz();

  store i32 0, i32* %alloca
  ret void
}

define spir_func void @dave() nounwind {
entry:

  %alloca0 = alloca i32
  call spir_func void @foo();
  store i32 0, i32* %alloca0
  ret void
}

;CHECK: SIMD: 8, external pressure: 10
;CHECK: block: entry function: baz
;CHECK: IN:     [       0       ]
;CHECK: OUT:    [       0       ]
;CHECK: N: 0 (0)          ret void
;CHECK: SIMD: 8, external pressure: 10
;CHECK: block: entry function: bar
;CHECK: IN:     [       0       ]
;CHECK: OUT:    [       0       ]
;CHECK: N: 0 (0)          ret void
;CHECK: SIMD: 8, external pressure: 0
;CHECK: block: entry function: main
;CHECK: IN:     [       4       ]
;CHECK: KILL:   [       4       ]
;CHECK: OUT:    [       0       ]
;CHECK: N: 256 (8)        %baseArith = ptrtoint double addrspace(1)* %base to i64
;CHECK: N: 256 (8)        %basePtr = mul nuw nsw i64 %offset, 207368
;CHECK: N: 256 (8)        %offsetI = mul nsw i64 %I, 1288
;CHECK: N: 256 (8)        %offsetJ = shl nsw i64 %J, 3
;CHECK: N: 256 (8)        %a0 = add i64 %baseArith, 100780848
;CHECK: N: 192 (6)        %a1 = add i64 %a0, %basePtr
;CHECK: N: 128 (4)        %a2 = add i64 %a1, %offsetI
;CHECK: N: 64 (2)         %a3 = add i64 %a2, %offsetJ
;CHECK: N: 128 (4)        %a4 = inttoptr i64 %a3 to double addrspace(1)*
;CHECK: N: 128 (4)        %a5 = inttoptr i64 %a3 to double addrspace(1)*
;CHECK: N: 128 (4)        call spir_func void @foo()
;CHECK: N: 128 (4)        call spir_func void @baz()
;CHECK: N: 128 (4)        call spir_func void @dave()
;CHECK: N: 128 (4)        %r0 = load double, double addrspace(1)* %a4, align 8
;CHECK: N: 128 (4)        %rr0 = fmul double %r0, 2.000000e+00
;CHECK: N: 0 (0)          store double %rr0, double addrspace(1)* %a5, align 8
;CHECK: N: 0 (0)          ret void
;CHECK: SIMD: 8, external pressure: 0
;CHECK: block: entry function: main_2
;CHECK: IN:     [       0       ]
;CHECK: OUT:    [       0       ]
;CHECK: N: 64 (2)         %alloca0 = alloca i32, align 4
;CHECK: N: 128 (4)        %alloca1 = alloca i32, align 4
;CHECK: N: 192 (6)        %alloca2 = alloca i32, align 4
;CHECK: N: 192 (6)        call spir_func void @dave()
;CHECK: N: 192 (6)        call spir_func void @foo()
;CHECK: N: 128 (4)        store i32 0, i32* %alloca0, align 4
;CHECK: N: 64 (2)         store i32 0, i32* %alloca1, align 4
;CHECK: N: 0 (0)          store i32 0, i32* %alloca2, align 4
;CHECK: N: 0 (0)          ret void
;CHECK: SIMD: 8, external pressure: 8
;CHECK: block: entry function: foo
;CHECK: IN:     [       0       ]
;CHECK: OUT:    [       0       ]
;CHECK: N: 64 (2)         %alloca = alloca i32, align 4
;CHECK: N: 64 (2)         call spir_func void @bar()
;CHECK: N: 64 (2)         call spir_func void @baz()
;CHECK: N: 0 (0)          store i32 0, i32* %alloca, align 4
;CHECK: N: 0 (0)          ret void
;CHECK: SIMD: 8, external pressure: 6
;CHECK: block: entry function: dave
;CHECK: IN:     [       0       ]
;CHECK: OUT:    [       0       ]
;CHECK: N: 64 (2)         %alloca0 = alloca i32, align 4
;CHECK: N: 64 (2)         call spir_func void @foo()
;CHECK: N: 0 (0)          store i32 0, i32* %alloca0, align 4
;CHECK: N: 0 (0)          ret void
