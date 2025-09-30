To check for kernel stack overflow, set [configuration flag](https://github.com/intel/intel-graphics-compiler/blob/master/documentation/configuration_flags.md) `IGC_StackOverflowDetection=1`.
Stack overflow can happen if the kernel uses non-inlined stack calls (such as recursion) or the Variable Length Array (VLA) feature.

This adds checks on each stack pointer modification that catch kernel private memory overflow. Note that enabling this feature will reduce kernel performance, so it should be used only for debugging.

If overflow is detected, the message "Stack overflow detected!" will be printed to the console and kernel will throw an assert.

You can change the default stack memory size with the `IGC_ForcePerThreadPrivateMemorySize` flag, using a value between 1024 and 20480. For example `IGC_ForcePerThreadPrivateMemorySize=4096`. Setting a higher value may impact performance.

## Usage examples
### Recursive stack calls
This OpenCL C kernel will lead to stack overflow for sufficiently large `n`. We will detect this stack overflow if the kernel is compiled with `IGC_StackOverflowDetection=1` flag.
```c
int fact(int n) {
  return n < 2 ? 1 : n*fact(n-1);
}
kernel void test_recursive(global int* out, int n) {
  out[0] = fact(n);
}
```
### VLA feature
This uses VLA feature via OpenMP Fortran. Increasing `m` will lead to stack overflow.
```fortran
program test_vla
  implicit none
  integer, parameter :: n = 1000, m = 600
  real, allocatable :: a(:), b(:)
  integer :: i, j

  allocate(a(n), b(m))
  !$omp target teams distribute parallel do private(b)
  do i = 1, n
    b(1) = i
    do j = 2, m
      b(j) = b(j-1) + 1
    enddo
    a(i) = b(m)
  enddo

  do i = 1, n
    write(*,*) 'Index ', i, ' Computed ', a(i)
  enddo

  deallocate(a, b)
end program test_vla
```

Compiling this code with `ifx -fiopenmp -fopenmp-targets=spir64_gen -Xopenmp-target-backend "-device pvc" ./test.F90 -o dynam.AOT` emits following warning.
```
warning: VLA has been detected, the private memory size is set to 4240B.
You can change this size by setting environmental variable IGC_ForcePerThreadPrivateMemorySize to a value in range [1024:20480].
Greater values can affect performance, and lower ones may lead to incorrect results of your program.
To make sure your program runs correctly you can set environmental variable IGC_StackOverflowDetection=1.
This flag will print "Stack overflow detected!" if insufficient memory value has led to stack overflow.
It should be used for debugging only as it affects performance.
```

Compiling with `IGC_StackOverflowDetection=1 ifx -fiopenmp -fopenmp-targets=spir64_gen -Xopenmp-target-backend "-device pvc" ./test.F90 -o dynam.AOT` and then executing it will result in
```
Stack overflow detected!
...
Stack overflow detected!
Stack overflow detected!
Stack overflow detected!
AssertHandler::printMessage
forrtl: error (76): Abort trap signal
```

We can adjust the private kernel memory size by `IGC_ForcePerThreadPrivateMemorySize` so our VLA won't overflow stack anymore. `IGC_StackOverflowDetection=1 IGC_ForcePerThreadPrivateMemorySize=8192 ifx -fiopenmp -fopenmp-targets=spir64_gen -Xopenmp-target-backend "-device pvc" ./test.F90 -o dynam.AOT` results in proper program execution.

```
 Index            1  Computed    600.0000
...
 Index         1000  Computed    1599.000
```