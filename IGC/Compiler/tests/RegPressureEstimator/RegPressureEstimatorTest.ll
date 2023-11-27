; UNSUPPORTED: system-windows
; REQUIRES: regkeys
; RUN: igc_opt --igc-df-liveness -S --disable-output --regkey=RegPressureVerbocity=5 < %s 2>&1 | FileCheck %s

define void @main() {
entry:
  %a = alloca i32, align 4
  store i32 5, i32* %a, align 4
  %val = load i32, i32* %a, align 4
  %cmp = icmp eq i32 %val, 5
  br i1 %cmp, label %if.then, label %if.else

if.then:
  store i32 10, i32* %a, align 4
  br label %if.end

if.else:
  %i = alloca i32, align 4
  store i32 0, i32* %i, align 4
  br label %loop.cond

loop.cond:
  %ival = load i32, i32* %i, align 4
  %loopcmp = icmp slt i32 %ival, 5
  br i1 %loopcmp, label %loop.body, label %if.end

loop.body:
  %tmp = load i32, i32* %a, align 4
  %newtmp = add i32 %tmp, 1
  store i32 %newtmp, i32* %a, align 4
  %next = add i32 %ival, 1
  store i32 %next, i32* %i, align 4
  br label %loop.cond

if.end:
  %retval = load i32, i32* %a, align 4
  ret void
}


;CHECK: block: entry function: main
;CHECK: IN:   [  0  ]
;CHECK: DEF:  [  1  ] %a(6)[4],
;CHECK: OUT:  [  1  ] %a,
;CHECK: N: 128 (4)        %a = alloca i32, align 4
;CHECK: N: 128 (4)        store i32 5, i32* %a, align 4
;CHECK: N: 192 (6)        %val = load i32, i32* %a, align 4
;CHECK: N: 130 (5)        %cmp = icmp eq i32 %val, 5
;CHECK: N: 128 (4)        br i1 %cmp, label %if.then, label %if.else
;CHECK: block: if.then function: main
;CHECK: IN:   [  1  ] %a,
;CHECK: OUT:  [  1  ] %a,
;CHECK: N: 128 (4)        store i32 10, i32* %a, align 4
;CHECK: N: 128 (4)        br label %if.end
;CHECK: block: if.else function: main
;CHECK: IN:   [  1  ] %a,
;CHECK: DEF:  [  1  ] %i(3)[3],
;CHECK: OUT:  [  2  ] %i, %a,
;CHECK: N: 256 (8)        %i = alloca i32, align 4
;CHECK: N: 256 (8)        store i32 0, i32* %i, align 4
;CHECK: N: 256 (8)        br label %loop.cond
;CHECK: block: loop.cond function: main
;CHECK: IN:   [  2  ] %i, %a,
;CHECK: DEF:  [  1  ] %ival(2)[2],
;CHECK: OUT:  [  3  ] %i, %ival, %a,
;CHECK: N: 320 (10)        %ival = load i32, i32* %i, align 4
;CHECK: N: 322 (11)        %loopcmp = icmp slt i32 %ival, 5
;CHECK: N: 320 (10)        br i1 %loopcmp, label %loop.body, label %if.end
;CHECK: block: loop.body function: main
;CHECK: IN:   [  3  ] %i, %a, %ival,
;CHECK: KILL: [  1  ] %ival,
;CHECK: OUT:  [  2  ] %i, %a,
;CHECK: N: 384 (12)        %tmp = load i32, i32* %a, align 4
;CHECK: N: 384 (12)        %newtmp = add i32 %tmp, 1
;CHECK: N: 320 (10)        store i32 %newtmp, i32* %a, align 4
;CHECK: N: 320 (10)        %next = add i32 %ival, 1
;CHECK: N: 256 (8)        store i32 %next, i32* %i, align 4
;CHECK: N: 256 (8)        br label %loop.cond
;CHECK: block: if.end function: main
;CHECK: IN:   [  1  ] %a,
;CHECK: KILL: [  1  ] %a,
;CHECK: OUT:  [  0  ]
;CHECK: N: 0 (0)        %retval = load i32, i32* %a, align 4
;CHECK: N: 0 (0)        ret void
