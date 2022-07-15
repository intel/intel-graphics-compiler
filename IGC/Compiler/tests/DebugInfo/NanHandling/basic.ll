;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --NanHandling -S < %s | FileCheck %s
; ------------------------------------------------
; NanHandling
; ------------------------------------------------
; This test checks that NanHandling pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define void @test_basic
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]
; CHECK: [[CMP_V:%[0-9]*]] = fcmp
; CHECK-SAME: !dbg [[CMP_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: metadata i1 [[CMP_V]]
; CHECK-SAME: metadata [[CMP_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CMP_LOC]]
; CHECK: br i1 [[CMP_V]]
; CHECK-SAME: !dbg [[BR1_LOC:![0-9]*]]
;
; CHECK: t1:
; CHECK: [[FADD_V:%[0-9]*]] = fadd
; CHECK-SAME: !dbg [[FADD_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: metadata float [[FADD_V]]
; CHECK-SAME: metadata [[FADD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FADD_LOC]]
; CHECK: br {{.*}} !dbg [[BR2_LOC:![0-9]*]]
;
; CHECK: f1:
; CHECK: [[FSUB_V:%[0-9]*]] = fsub
; CHECK-SAME: !dbg [[FSUB_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: metadata float [[FSUB_V]]
; CHECK-SAME: metadata [[FSUB_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FSUB_LOC]]
; CHECK: br {{.*}} !dbg [[BR3_LOC:![0-9]*]]
;
; CHECK: e1:
; CHECK: [[PHI_V:%[0-9]*]] = phi
; CHECK-SAME: !dbg [[PHI_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: metadata float [[PHI_V]]
; CHECK-SAME: metadata [[PHI_MD:![0-9]*]], metadata !DIExpression()), !dbg [[PHI_LOC]]

define void @test_basic(float %a, float %b) !dbg !6 {
entry:
  %0 = fcmp fast oeq float %a, %b, !dbg !14
  call void @llvm.dbg.value(metadata i1 %0, metadata !9, metadata !DIExpression()), !dbg !14
  br i1 %0, label %t1, label %f1, !dbg !15

t1:                                               ; preds = %entry
  %1 = fadd float %a, %b, !dbg !16
  call void @llvm.dbg.value(metadata float %1, metadata !11, metadata !DIExpression()), !dbg !16
  br label %e1, !dbg !17

f1:                                               ; preds = %entry
  %2 = fsub float %a, %b, !dbg !18
  call void @llvm.dbg.value(metadata float %2, metadata !13, metadata !DIExpression()), !dbg !18
  br label %f_temp, !dbg !19

f_temp:                                           ; preds = %f1
  %a.1 = fcmp one float %a, %b
  %a.2 = fcmp one float %a, %b
  %a.3 = fcmp one float %a, %b
  %a.4 = fcmp one float %a, %b
  %a.5 = fcmp one float %a, %b
  %a.6 = fcmp one float %a, %b
  %a.7 = fcmp one float %a, %b
  %a.8 = fcmp one float %a, %b
  %a.9 = fcmp one float %a, %b
  %a.10 = fcmp one float %a, %b
  %a.11 = fcmp one float %a, %b
  %a.12 = fcmp one float %a, %b
  %a.13 = fcmp one float %a, %b
  %a.14 = fcmp one float %a, %b
  %a.15 = fcmp one float %a, %b
  %a.16 = fcmp one float %a, %b
  %a.17 = fcmp one float %a, %b
  %a.18 = fcmp one float %a, %b
  %a.19 = fcmp one float %a, %b
  %a.20 = fcmp one float %a, %b
  %a.21 = fcmp one float %a, %b
  %a.22 = fcmp one float %a, %b
  %a.23 = fcmp one float %a, %b
  %a.24 = fcmp one float %a, %b
  %a.25 = fcmp one float %a, %b
  %a.26 = fcmp one float %a, %b
  %a.27 = fcmp one float %a, %b
  %a.28 = fcmp one float %a, %b
  %a.29 = fcmp one float %a, %b
  %a.30 = fcmp one float %a, %b
  %a.31 = fcmp one float %a, %b
  %a.32 = fcmp one float %a, %b
  %a.33 = fcmp one float %a, %b
  %a.34 = fcmp one float %a, %b
  %a.35 = fcmp one float %a, %b
  %a.36 = fcmp one float %a, %b
  %a.37 = fcmp one float %a, %b
  %a.38 = fcmp one float %a, %b
  %a.39 = fcmp one float %a, %b
  %a.40 = fcmp one float %a, %b
  %a.41 = fcmp one float %a, %b
  %a.42 = fcmp one float %a, %b
  %a.43 = fcmp one float %a, %b
  %a.44 = fcmp one float %a, %b
  %a.45 = fcmp one float %a, %b
  %a.46 = fcmp one float %a, %b
  %a.47 = fcmp one float %a, %b
  %a.48 = fcmp one float %a, %b
  %a.49 = fcmp one float %a, %b
  %a.50 = fcmp one float %a, %b
  %a.51 = fcmp one float %a, %b
  %a.52 = fcmp one float %a, %b
  %a.53 = fcmp one float %a, %b
  %a.54 = fcmp one float %a, %b
  %a.55 = fcmp one float %a, %b
  %a.56 = fcmp one float %a, %b
  %a.57 = fcmp one float %a, %b
  %a.58 = fcmp one float %a, %b
  %a.59 = fcmp one float %a, %b
  %a.60 = fcmp one float %a, %b
  %a.61 = fcmp one float %a, %b
  %a.62 = fcmp one float %a, %b
  %a.63 = fcmp one float %a, %b
  %a.64 = fcmp one float %a, %b
  %a.65 = fcmp one float %a, %b
  %a.66 = fcmp one float %a, %b
  %a.67 = fcmp one float %a, %b
  %a.68 = fcmp one float %a, %b
  %a.69 = fcmp one float %a, %b
  %a.70 = fcmp one float %a, %b
  %a.71 = fcmp one float %a, %b
  %a.72 = fcmp one float %a, %b
  %a.73 = fcmp one float %a, %b
  %a.74 = fcmp one float %a, %b
  %a.75 = fcmp one float %a, %b
  %a.76 = fcmp one float %a, %b
  %a.77 = fcmp one float %a, %b
  %a.78 = fcmp one float %a, %b
  %a.79 = fcmp one float %a, %b
  %a.80 = fcmp one float %a, %b
  %a.81 = fcmp one float %a, %b
  %a.82 = fcmp one float %a, %b
  %a.83 = fcmp one float %a, %b
  %a.84 = fcmp one float %a, %b
  %a.85 = fcmp one float %a, %b
  %a.86 = fcmp one float %a, %b
  %a.87 = fcmp one float %a, %b
  %a.88 = fcmp one float %a, %b
  %a.89 = fcmp one float %a, %b
  %a.90 = fcmp one float %a, %b
  %a.91 = fcmp one float %a, %b
  %a.92 = fcmp one float %a, %b
  %a.93 = fcmp one float %a, %b
  %a.94 = fcmp one float %a, %b
  %a.95 = fcmp one float %a, %b
  %a.96 = fcmp one float %a, %b
  %a.97 = fcmp one float %a, %b
  %a.98 = fcmp one float %a, %b
  %a.99 = fcmp one float %a, %b
  %a.100 = fcmp one float %a, %b
  %a.101 = fcmp one float %a, %b
  %a.102 = fcmp one float %a, %b
  %a.103 = fcmp one float %a, %b
  %a.104 = fcmp one float %a, %b
  %a.105 = fcmp one float %a, %b
  %a.106 = fcmp one float %a, %b
  %a.107 = fcmp one float %a, %b
  %a.108 = fcmp one float %a, %b
  %a.109 = fcmp one float %a, %b
  %a.110 = fcmp one float %a, %b
  %a.111 = fcmp one float %a, %b
  %a.112 = fcmp one float %a, %b
  %a.113 = fcmp one float %a, %b
  %a.114 = fcmp one float %a, %b
  %a.115 = fcmp one float %a, %b
  %a.116 = fcmp one float %a, %b
  %a.117 = fcmp one float %a, %b
  %a.118 = fcmp one float %a, %b
  %a.119 = fcmp one float %a, %b
  %a.120 = fcmp one float %a, %b
  %a.121 = fcmp one float %a, %b
  %a.122 = fcmp one float %a, %b
  %a.123 = fcmp one float %a, %b
  %a.124 = fcmp one float %a, %b
  %a.125 = fcmp one float %a, %b
  %a.126 = fcmp one float %a, %b
  %a.127 = fcmp one float %a, %b
  %a.128 = fcmp one float %a, %b
  %a.129 = fcmp one float %a, %b
  %a.130 = fcmp one float %a, %b
  %a.131 = fcmp one float %a, %b
  %a.132 = fcmp one float %a, %b
  %a.133 = fcmp one float %a, %b
  %a.134 = fcmp one float %a, %b
  %a.135 = fcmp one float %a, %b
  %a.136 = fcmp one float %a, %b
  %a.137 = fcmp one float %a, %b
  %a.138 = fcmp one float %a, %b
  %a.139 = fcmp one float %a, %b
  %a.140 = fcmp one float %a, %b
  %a.141 = fcmp one float %a, %b
  %a.142 = fcmp one float %a, %b
  %a.143 = fcmp one float %a, %b
  %a.144 = fcmp one float %a, %b
  %a.145 = fcmp one float %a, %b
  %a.146 = fcmp one float %a, %b
  %a.147 = fcmp one float %a, %b
  %a.148 = fcmp one float %a, %b
  %a.149 = fcmp one float %a, %b
  %a.150 = fcmp one float %a, %b
  %a.151 = fcmp one float %a, %b
  %a.152 = fcmp one float %a, %b
  %a.153 = fcmp one float %a, %b
  %a.154 = fcmp one float %a, %b
  %a.155 = fcmp one float %a, %b
  %a.156 = fcmp one float %a, %b
  %a.157 = fcmp one float %a, %b
  %a.158 = fcmp one float %a, %b
  %a.159 = fcmp one float %a, %b
  %a.160 = fcmp one float %a, %b
  %a.161 = fcmp one float %a, %b
  %a.162 = fcmp one float %a, %b
  %a.163 = fcmp one float %a, %b
  %a.164 = fcmp one float %a, %b
  %a.165 = fcmp one float %a, %b
  %a.166 = fcmp one float %a, %b
  %a.167 = fcmp one float %a, %b
  %a.168 = fcmp one float %a, %b
  %a.169 = fcmp one float %a, %b
  %a.170 = fcmp one float %a, %b
  %a.171 = fcmp one float %a, %b
  %a.172 = fcmp one float %a, %b
  %a.173 = fcmp one float %a, %b
  %a.174 = fcmp one float %a, %b
  %a.175 = fcmp one float %a, %b
  %a.176 = fcmp one float %a, %b
  %a.177 = fcmp one float %a, %b
  %a.178 = fcmp one float %a, %b
  %a.179 = fcmp one float %a, %b
  %a.180 = fcmp one float %a, %b
  %a.181 = fcmp one float %a, %b
  %a.182 = fcmp one float %a, %b
  %a.183 = fcmp one float %a, %b
  %a.184 = fcmp one float %a, %b
  %a.185 = fcmp one float %a, %b
  %a.186 = fcmp one float %a, %b
  %a.187 = fcmp one float %a, %b
  %a.188 = fcmp one float %a, %b
  %a.189 = fcmp one float %a, %b
  %a.190 = fcmp one float %a, %b
  %a.191 = fcmp one float %a, %b
  %a.192 = fcmp one float %a, %b
  %a.193 = fcmp one float %a, %b
  %a.194 = fcmp one float %a, %b
  %a.195 = fcmp one float %a, %b
  %a.196 = fcmp one float %a, %b
  %a.197 = fcmp one float %a, %b
  %a.198 = fcmp one float %a, %b
  %a.199 = fcmp one float %a, %b
  %a.200 = fcmp one float %a, %b
  %a.201 = fcmp one float %a, %b
  %a.202 = fcmp one float %a, %b
  %a.203 = fcmp one float %a, %b
  %a.204 = fcmp one float %a, %b
  %a.205 = fcmp one float %a, %b
  %a.206 = fcmp one float %a, %b
  %a.207 = fcmp one float %a, %b
  %a.208 = fcmp one float %a, %b
  %a.209 = fcmp one float %a, %b
  %a.210 = fcmp one float %a, %b
  %a.211 = fcmp one float %a, %b
  %a.212 = fcmp one float %a, %b
  %a.213 = fcmp one float %a, %b
  %a.214 = fcmp one float %a, %b
  %a.215 = fcmp one float %a, %b
  %a.216 = fcmp one float %a, %b
  %a.217 = fcmp one float %a, %b
  %a.218 = fcmp one float %a, %b
  %a.219 = fcmp one float %a, %b
  %a.220 = fcmp one float %a, %b
  %a.221 = fcmp one float %a, %b
  %a.222 = fcmp one float %a, %b
  %a.223 = fcmp one float %a, %b
  %a.224 = fcmp one float %a, %b
  %a.225 = fcmp one float %a, %b
  %a.226 = fcmp one float %a, %b
  %a.227 = fcmp one float %a, %b
  %a.228 = fcmp one float %a, %b
  %a.229 = fcmp one float %a, %b
  %a.230 = fcmp one float %a, %b
  %a.231 = fcmp one float %a, %b
  %a.232 = fcmp one float %a, %b
  %a.233 = fcmp one float %a, %b
  %a.234 = fcmp one float %a, %b
  %a.235 = fcmp one float %a, %b
  %a.236 = fcmp one float %a, %b
  %a.237 = fcmp one float %a, %b
  %a.238 = fcmp one float %a, %b
  %a.239 = fcmp one float %a, %b
  %a.240 = fcmp one float %a, %b
  %a.241 = fcmp one float %a, %b
  %a.242 = fcmp one float %a, %b
  %a.243 = fcmp one float %a, %b
  %a.244 = fcmp one float %a, %b
  %a.245 = fcmp one float %a, %b
  %a.246 = fcmp one float %a, %b
  %a.247 = fcmp one float %a, %b
  %a.248 = fcmp one float %a, %b
  %a.249 = fcmp one float %a, %b
  %a.250 = fcmp one float %a, %b
  %a.251 = fcmp one float %a, %b
  %a.252 = fcmp one float %a, %b
  %a.253 = fcmp one float %a, %b
  %a.254 = fcmp one float %a, %b
  %a.255 = fcmp one float %a, %b
  %a.256 = fcmp one float %a, %b
  %a.257 = fcmp one float %a, %b
  %a.258 = fcmp one float %a, %b
  %a.259 = fcmp one float %a, %b
  %a.260 = fcmp one float %a, %b
  %a.261 = fcmp one float %a, %b
  %a.262 = fcmp one float %a, %b
  %a.263 = fcmp one float %a, %b
  %a.264 = fcmp one float %a, %b
  %a.265 = fcmp one float %a, %b
  %a.266 = fcmp one float %a, %b
  %a.267 = fcmp one float %a, %b
  %a.268 = fcmp one float %a, %b
  %a.269 = fcmp one float %a, %b
  %a.270 = fcmp one float %a, %b
  %a.271 = fcmp one float %a, %b
  %a.272 = fcmp one float %a, %b
  %a.273 = fcmp one float %a, %b
  %a.274 = fcmp one float %a, %b
  %a.275 = fcmp one float %a, %b
  %a.276 = fcmp one float %a, %b
  %a.277 = fcmp one float %a, %b
  %a.278 = fcmp one float %a, %b
  %a.279 = fcmp one float %a, %b
  %a.280 = fcmp one float %a, %b
  %a.281 = fcmp one float %a, %b
  %a.282 = fcmp one float %a, %b
  %a.283 = fcmp one float %a, %b
  %a.284 = fcmp one float %a, %b
  %a.285 = fcmp one float %a, %b
  %a.286 = fcmp one float %a, %b
  %a.287 = fcmp one float %a, %b
  %a.288 = fcmp one float %a, %b
  %a.289 = fcmp one float %a, %b
  %a.290 = fcmp one float %a, %b
  %a.291 = fcmp one float %a, %b
  %a.292 = fcmp one float %a, %b
  %a.293 = fcmp one float %a, %b
  %a.294 = fcmp one float %a, %b
  %a.295 = fcmp one float %a, %b
  %a.296 = fcmp one float %a, %b
  %a.297 = fcmp one float %a, %b
  %a.298 = fcmp one float %a, %b
  %a.299 = fcmp one float %a, %b
  %a.300 = fcmp one float %a, %b
  %a.301 = fcmp one float %a, %b
  %a.302 = fcmp one float %a, %b
  %a.303 = fcmp one float %a, %b
  %a.304 = fcmp one float %a, %b
  %a.305 = fcmp one float %a, %b
  %a.306 = fcmp one float %a, %b
  %a.307 = fcmp one float %a, %b
  %a.308 = fcmp one float %a, %b
  %a.309 = fcmp one float %a, %b
  %a.310 = fcmp one float %a, %b
  %a.311 = fcmp one float %a, %b
  %a.312 = fcmp one float %a, %b
  %a.313 = fcmp one float %a, %b
  %a.314 = fcmp one float %a, %b
  %a.315 = fcmp one float %a, %b
  %a.316 = fcmp one float %a, %b
  %a.317 = fcmp one float %a, %b
  %a.318 = fcmp one float %a, %b
  %a.319 = fcmp one float %a, %b
  %a.320 = fcmp one float %a, %b
  %a.321 = fcmp one float %a, %b
  %a.322 = fcmp one float %a, %b
  %a.323 = fcmp one float %a, %b
  %a.324 = fcmp one float %a, %b
  %a.325 = fcmp one float %a, %b
  %a.326 = fcmp one float %a, %b
  %a.327 = fcmp one float %a, %b
  %a.328 = fcmp one float %a, %b
  %a.329 = fcmp one float %a, %b
  %a.330 = fcmp one float %a, %b
  %a.331 = fcmp one float %a, %b
  %a.332 = fcmp one float %a, %b
  %a.333 = fcmp one float %a, %b
  %a.334 = fcmp one float %a, %b
  %a.335 = fcmp one float %a, %b
  %a.336 = fcmp one float %a, %b
  %a.337 = fcmp one float %a, %b
  %a.338 = fcmp one float %a, %b
  %a.339 = fcmp one float %a, %b
  %a.340 = fcmp one float %a, %b
  %a.341 = fcmp one float %a, %b
  %a.342 = fcmp one float %a, %b
  %a.343 = fcmp one float %a, %b
  %a.344 = fcmp one float %a, %b
  %a.345 = fcmp one float %a, %b
  %a.346 = fcmp one float %a, %b
  %a.347 = fcmp one float %a, %b
  %a.348 = fcmp one float %a, %b
  %a.349 = fcmp one float %a, %b
  %a.350 = fcmp one float %a, %b
  %a.351 = fcmp one float %a, %b
  %a.352 = fcmp one float %a, %b
  %a.353 = fcmp one float %a, %b
  %a.354 = fcmp one float %a, %b
  %a.355 = fcmp one float %a, %b
  %a.356 = fcmp one float %a, %b
  %a.357 = fcmp one float %a, %b
  %a.358 = fcmp one float %a, %b
  %a.359 = fcmp one float %a, %b
  %a.360 = fcmp one float %a, %b
  %a.361 = fcmp one float %a, %b
  %a.362 = fcmp one float %a, %b
  %a.363 = fcmp one float %a, %b
  %a.364 = fcmp one float %a, %b
  %a.365 = fcmp one float %a, %b
  %a.366 = fcmp one float %a, %b
  %a.367 = fcmp one float %a, %b
  %a.368 = fcmp one float %a, %b
  %a.369 = fcmp one float %a, %b
  %a.370 = fcmp one float %a, %b
  %a.371 = fcmp one float %a, %b
  %a.372 = fcmp one float %a, %b
  %a.373 = fcmp one float %a, %b
  %a.374 = fcmp one float %a, %b
  %a.375 = fcmp one float %a, %b
  %a.376 = fcmp one float %a, %b
  %a.377 = fcmp one float %a, %b
  %a.378 = fcmp one float %a, %b
  %a.379 = fcmp one float %a, %b
  %a.380 = fcmp one float %a, %b
  %a.381 = fcmp one float %a, %b
  %a.382 = fcmp one float %a, %b
  %a.383 = fcmp one float %a, %b
  %a.384 = fcmp one float %a, %b
  %a.385 = fcmp one float %a, %b
  %a.386 = fcmp one float %a, %b
  %a.387 = fcmp one float %a, %b
  %a.388 = fcmp one float %a, %b
  %a.389 = fcmp one float %a, %b
  %a.390 = fcmp one float %a, %b
  %a.391 = fcmp one float %a, %b
  %a.392 = fcmp one float %a, %b
  %a.393 = fcmp one float %a, %b
  %a.394 = fcmp one float %a, %b
  %a.395 = fcmp one float %a, %b
  %a.396 = fcmp one float %a, %b
  %a.397 = fcmp one float %a, %b
  %a.398 = fcmp one float %a, %b
  %a.399 = fcmp one float %a, %b
  %a.400 = fcmp one float %a, %b
  br label %f2

f2:                                               ; preds = %f_temp
  %3 = fmul float %a, %b, !dbg !20
  call void @llvm.dbg.value(metadata float %3, metadata !21, metadata !DIExpression()), !dbg !20
  br label %e1, !dbg !22

e1:                                               ; preds = %f2, %t1
  %4 = phi float [ %a, %t1 ], [ %3, %f2 ], !dbg !23
  call void @llvm.dbg.value(metadata float %4, metadata !24, metadata !DIExpression()), !dbg !23
  ret void, !dbg !25
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "basic.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_basic", linkageName: "test_basic", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[CMP_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[CMP_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[BR1_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[FADD_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[FADD_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[BR2_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[FSUB_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[FSUB_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[BR3_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[PHI_MD]] = !DILocalVariable(name: "405", scope: [[SCOPE]], file: [[FILE]], line: 410
; CHECK-DAG: [[PHI_LOC]] = !DILocation(line: 410, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}
!igc.functions = !{}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "basic.ll", directory: "/")
!2 = !{}
!3 = !{i32 411}
!4 = !{i32 405}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_basic", linkageName: "test_basic", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 3, type: !12)
!12 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 5, type: !12)
!14 = !DILocation(line: 1, column: 1, scope: !6)
!15 = !DILocation(line: 2, column: 1, scope: !6)
!16 = !DILocation(line: 3, column: 1, scope: !6)
!17 = !DILocation(line: 4, column: 1, scope: !6)
!18 = !DILocation(line: 5, column: 1, scope: !6)
!19 = !DILocation(line: 6, column: 1, scope: !6)
!20 = !DILocation(line: 408, column: 1, scope: !6)
!21 = !DILocalVariable(name: "404", scope: !6, file: !1, line: 408, type: !12)
!22 = !DILocation(line: 409, column: 1, scope: !6)
!23 = !DILocation(line: 410, column: 1, scope: !6)
!24 = !DILocalVariable(name: "405", scope: !6, file: !1, line: 410, type: !12)
!25 = !DILocation(line: 411, column: 1, scope: !6)
