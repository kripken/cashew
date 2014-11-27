function a() {
 f((HEAPU8[10202] | 0) + 5 | 0);
 f((HEAPU8[10202] | 0) | 0);
 f(347 | 0);
 f(347 | 12);
 f(347 & 12);
 HEAP[4096 >> 2] = 5;
 HEAP[(4096 & 8191) >> 2] = 5;
 whee(12, 13) | 0;
 +whee(12, 13);
 f((g = t(), (g+g)|0)|0);
 // always coerce function calls in asm
 f() | 0;
 f((h() | 0) + 5 | 0);
 f(((x + y) | 0) + z | 0);
 +f();
 f(+(+h() + 5));
 $140 = $p_3_i + (-$mantSize_0_i | 0) | 0;
 f(g() | 0 | 0);
 f(g() | 0 & -1);
 f((g() | 0) >> 2);
 $56 = (_fcntl() | 0) | 1;
 FUNCTION_TABLE_ii[55 & 127]() | 0;
}
function b($this, $__n) {
 $this = $this | 0;
 $__n = $__n | 0;
 var $4 = 0, $5 = 0, $10 = 0, $13 = 0, $14 = 0, $15 = 0, $23 = 0, $30 = 0, $38 = 0, $40 = 0;
 if (($__n | 0) == 0) {
  return;
 }
 $4 = $this;
 $5 = HEAP8[$4 & 16777215] | 0;
 if (($5 & 1) << 24 >> 24 == 0) {
  $14 = 10;
  $13 = $5;
 } else {
  $10 = HEAP32[(($this | 0) & 16777215) >> 2] | 0;
  $14 = ($10 & -2) - 1 | 0;
  $13 = $10 & 255;
 }
 $15 = $13 & 255;
 if (($15 & 1 | 0) == 0) {
  $23 = $15 >>> 1;
 } else {
  $23 = HEAP32[(($this + 4 | 0) & 16777215) >> 2] | 0;
 }
 if (($14 - $23 | 0) >>> 0 < $__n >>> 0) {
  __ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE9__grow_byEjjjjjj($this, $14, ($__n - $14 | 0) + $23 | 0, $23, $23);
  $30 = HEAP8[$4 & 16777215] | 0;
 } else {
  $30 = $13;
 }
 if (($30 & 1) << 24 >> 24 == 0) {
  $38 = $this + 1 | 0;
 } else {
  $38 = HEAP32[(($this + 8 | 0) & 16777215) >> 2] | 0;
 }
 _memset($38 + $23 | 0 | 0 | 0, 0 | 0 | 0, $__n | 0 | 0, 1 | 0 | 0, 1213141516);
 $40 = $23 + $__n | 0;
 if (((HEAP8[$4 & 16777215] | 0) & 1) << 24 >> 24 == 0) {
  HEAP8[$4 & 16777215] = $40 << 1 & 255;
 } else {
  HEAP32[(($this + 4 | 0) & 16777215) >> 2] = $40;
 }
 HEAP8[($38 + $40 | 0) & 16777215] = 0;
 // Eliminate the |0.
 HEAP32[$4] = ((~(HEAP32[$5]|0))|0);
 // Eliminate the &255
 HEAP8[$4] = HEAP32[$5]&255;
 // Eliminate the &65535
 HEAP16[$4] = HEAP32[$5]&65535;
 // Rewrite to ~.
 HEAP32[$4] = HEAP32[$5]^-1;
 // Rewrite to ~ and eliminate the |0.
 HEAP32[$4] = ((HEAP32[$5]|0)^-1)|0;
 h((~~g) ^ -1); // do NOT convert this, as it would lead to ~~~ which is confusing in asm, given the role of ~~
 return;
}
function i32_8() {
 if (((HEAP8[$4 & 16777215] | 0) << 24 >> 24) == 0) {
  print(5);
 }
 if ((HEAP8[$5 & 16777215] << 24 >> 24) == 0) {
  print(5);
 }
 if (((HEAPU8[$6 & 16777215] | 0) << 24 >> 24) == 0) {
  print(5);
 }
 if ((HEAPU8[$7 & 16777215] << 24 >> 24) == 0) {
  print(5);
 }
 // non-valid
 if ((HEAPU8[$8 & 16777215] << 24 >> 16) == 0) {
  print(5);
 }
 if ((HEAPU8[$9 & 16777215] << 16 >> 16) == 0) {
  print(5);
 }
}
function sign_extension_simplification() {
 if ((HEAP8[$4 & 16777215] & 127) << 24 >> 24 == 0) {
  print(5);
 }
 if ((HEAP8[$4 & 16777215] & 128) << 24 >> 24 == 0) {
  print(5);
 }
 if ((HEAP32[$5 & 16777215] & 32767) << 16 >> 16 == 0) {
  print(5);
 }
 if ((HEAP32[$5 & 16777215] & 32768) << 16 >> 16 == 0) {
  print(5);
 }
}
function compare_result_simplification() {
 f(((a > b)&1) + 1 | 0);
 f(((a > b)&1) | z);
 f(((a > b)&1) | (c > d & 1));
 // Don't eliminate these '&1's.
 HEAP32[$4] = (HEAP32[$5] < HEAP32[$6]) & 1;
 var x = (HEAP32[$5] != HEAP32[$6]) & 1;
}

