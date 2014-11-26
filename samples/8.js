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

