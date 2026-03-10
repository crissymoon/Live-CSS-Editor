# Code Review Report — God Functions

**Directory:** `/Users/mac/Documents/spreadsheet_tool/imgui-browser`  
**Generated:** 2026-03-09 23:39:14  

---

```
============================================================
GOD FUNCTIONS REPORT
============================================================
Files scanned:      789
Functions analyzed: 12359
God functions found: 493
  CRITICAL: 202
  HIGH:     291
By language:
  C: 486
  JavaScript: 4
  PHP: 3
------------------------------------------------------------
DETECTED GOD FUNCTIONS
------------------------------------------------------------
!!! [CRITICAL] gen_executor()
    File: php-wasm-project/php-src/Zend/zend_vm_gen.php
    Language: PHP
    Thresholds exceeded: 4/4
    Reasons:
      - 430 lines (>150)
      - complexity 67 (>20)
      - nesting 8 (>4)
      - 6 params (>5)
!!! [CRITICAL] zend_dump_op()
    File: php-wasm-project/php-src/Zend/Optimizer/zend_dump.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 305 lines (>150)
      - complexity 111 (>20)
      - nesting 5 (>4)
      - 6 params (>5)
!!! [CRITICAL] zend_inference_calc_binary_op_range()
    File: php-wasm-project/php-src/Zend/Optimizer/zend_inference.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 260 lines (>150)
      - complexity 93 (>20)
      - nesting 5 (>4)
      - 6 params (>5)
!!! [CRITICAL] zend_inference_calc_range()
    File: php-wasm-project/php-src/Zend/Optimizer/zend_inference.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 170 lines (>150)
      - complexity 74 (>20)
      - nesting 9 (>4)
      - 6 params (>5)
!!! [CRITICAL] zend_inference_propagate_range()
    File: php-wasm-project/php-src/Zend/Optimizer/zend_inference.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 336 lines (>150)
      - complexity 126 (>20)
      - nesting 6 (>4)
      - 6 params (>5)
!!! [CRITICAL] _zend_update_type_info()
    File: php-wasm-project/php-src/Zend/Optimizer/zend_inference.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 1393 lines (>150)
      - complexity 590 (>20)
      - nesting 8 (>4)
      - 9 params (>5)
!!! [CRITICAL] zend_may_throw_ex()
    File: php-wasm-project/php-src/Zend/Optimizer/zend_inference.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 390 lines (>150)
      - complexity 248 (>20)
      - nesting 5 (>4)
      - 6 params (>5)
!!! [CRITICAL] zend_optimize_block()
    File: php-wasm-project/php-src/Zend/Optimizer/block_pass.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 704 lines (>150)
      - complexity 274 (>20)
      - nesting 7 (>4)
      - 6 params (>5)
!!! [CRITICAL] zend_jit_needs_call_chain()
    File: php-wasm-project/php-src/ext/opcache/jit/zend_jit.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 170 lines (>150)
      - complexity 112 (>20)
      - nesting 6 (>4)
      - 8 params (>5)
!!! [CRITICAL] zend_jit_try_allocate_free_reg()
    File: php-wasm-project/php-src/ext/opcache/jit/zend_jit.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 186 lines (>150)
      - complexity 65 (>20)
      - nesting 5 (>4)
      - 10 params (>5)
!!! [CRITICAL] php_snmp_parse_oid()
    File: php-wasm-project/php-src/ext/snmp/snmp.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 162 lines (>150)
      - complexity 39 (>20)
      - nesting 8 (>4)
      - 9 params (>5)
!!! [CRITICAL] _php_iconv_mime_encode()
    File: php-wasm-project/php-src/ext/iconv/iconv.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 234 lines (>150)
      - complexity 45 (>20)
      - nesting 8 (>4)
      - 10 params (>5)
!!! [CRITICAL] _php_iconv_mime_decode()
    File: php-wasm-project/php-src/ext/iconv/iconv.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 454 lines (>150)
      - complexity 128 (>20)
      - nesting 8 (>4)
      - 6 params (>5)
!!! [CRITICAL] parse_packet_soap()
    File: php-wasm-project/php-src/ext/soap/php_packet_soap.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 349 lines (>150)
      - complexity 115 (>20)
      - nesting 9 (>4)
      - 7 params (>5)
!!! [CRITICAL] make_http_soap_request()
    File: php-wasm-project/php-src/ext/soap/php_http.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 907 lines (>150)
      - complexity 271 (>20)
      - nesting 7 (>4)
      - 6 params (>5)
!!! [CRITICAL] php_sscanf_internal()
    File: php-wasm-project/php-src/ext/standard/scanf.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 430 lines (>150)
      - complexity 172 (>20)
      - nesting 6 (>4)
      - 6 params (>5)
!!! [CRITICAL] php_stream_url_wrap_ftp()
    File: php-wasm-project/php-src/ext/standard/ftp_fopen_wrapper.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 155 lines (>150)
      - complexity 47 (>20)
      - nesting 5 (>4)
      - 6 params (>5)
!!! [CRITICAL] match()
    File: php-wasm-project/php-src/ext/fileinfo/libmagic/softmagic.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 221 lines (>150)
      - complexity 68 (>20)
      - nesting 6 (>4)
      - 14 params (>5)
!!! [CRITICAL] file_ascmagic_with_encoding()
    File: php-wasm-project/php-src/ext/fileinfo/libmagic/ascmagic.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 192 lines (>150)
      - complexity 81 (>20)
      - nesting 5 (>4)
      - 7 params (>5)
!!! [CRITICAL] exif_process_IFD_TAG_impl()
    File: php-wasm-project/php-src/ext/exif/exif.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 254 lines (>150)
      - complexity 75 (>20)
      - nesting 5 (>4)
      - 7 params (>5)
!!! [CRITICAL] gdImageStringFTEx()
    File: php-wasm-project/php-src/ext/gd/libgd/gdft.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 311 lines (>150)
      - complexity 77 (>20)
      - nesting 6 (>4)
      - 10 params (>5)
!!! [CRITICAL] php_pcre_match_impl()
    File: php-wasm-project/php-src/ext/pcre/php_pcre.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 248 lines (>150)
      - complexity 77 (>20)
      - nesting 9 (>4)
      - 8 params (>5)
!!! [CRITICAL] convert_posix()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_convert.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 185 lines (>150)
      - complexity 65 (>20)
      - nesting 6 (>4)
      - 9 params (>5)
!!! [CRITICAL] convert_glob()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_convert.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 202 lines (>150)
      - complexity 57 (>20)
      - nesting 5 (>4)
      - 9 params (>5)
!!! [CRITICAL] internal_dfa_match()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_dfa_match.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 2132 lines (>150)
      - complexity 841 (>20)
      - nesting 8 (>4)
      - 10 params (>5)
!!! [CRITICAL] pcre2_dfa_match()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_dfa_match.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 486 lines (>150)
      - complexity 168 (>20)
      - nesting 9 (>4)
      - 9 params (>5)
!!! [CRITICAL] match()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_match.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 4431 lines (>150)
      - complexity 1465 (>20)
      - nesting 9 (>4)
      - 6 params (>5)
!!! [CRITICAL] pcre2_match()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_match.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 716 lines (>150)
      - complexity 223 (>20)
      - nesting 9 (>4)
      - 7 params (>5)
!!! [CRITICAL] compile_branch()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_compile.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 1567 lines (>150)
      - complexity 473 (>20)
      - nesting 9 (>4)
      - 11 params (>5)
!!! [CRITICAL] compile_regex()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_compile.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 156 lines (>150)
      - complexity 31 (>20)
      - nesting 7 (>4)
      - 12 params (>5)
!!! [CRITICAL] pcre2_compile()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_compile.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 540 lines (>150)
      - complexity 147 (>20)
      - nesting 7 (>4)
      - 6 params (>5)
!!! [CRITICAL] detect_early_fail()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_jit_compile.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 284 lines (>150)
      - complexity 158 (>20)
      - nesting 6 (>4)
      - 6 params (>5)
!!! [CRITICAL] copy_recurse_data()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_jit_compile.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 371 lines (>150)
      - complexity 129 (>20)
      - nesting 6 (>4)
      - 7 params (>5)
!!! [CRITICAL] pcre2_substitute()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_substitute.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 557 lines (>150)
      - complexity 161 (>20)
      - nesting 10 (>4)
      - 11 params (>5)
!!! [CRITICAL] find_minlength()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_study.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 488 lines (>150)
      - complexity 262 (>20)
      - nesting 9 (>4)
      - 7 params (>5)
!!! [CRITICAL] compare_opcodes()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_auto_possess.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 388 lines (>150)
      - complexity 176 (>20)
      - nesting 6 (>4)
      - 7 params (>5)
!!! [CRITICAL] emit_single_op()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativePPC_64.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 295 lines (>150)
      - complexity 105 (>20)
      - nesting 5 (>4)
      - 6 params (>5)
!!! [CRITICAL] sljit_emit_op2()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativePPC_common.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 207 lines (>150)
      - complexity 86 (>20)
      - nesting 5 (>4)
      - 8 params (>5)
!!! [CRITICAL] emit_single_op()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeRISCV_common.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 336 lines (>150)
      - complexity 148 (>20)
      - nesting 5 (>4)
      - 6 params (>5)
!!! [CRITICAL] emit_single_op()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeMIPS_common.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 464 lines (>150)
      - complexity 217 (>20)
      - nesting 5 (>4)
      - 6 params (>5)
!!! [CRITICAL] sljit_emit_enter()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeARM_T2_32.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 177 lines (>150)
      - complexity 42 (>20)
      - nesting 5 (>4)
      - 8 params (>5)
!!! [CRITICAL] sljit_emit_enter()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeX86_32.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 178 lines (>150)
      - complexity 47 (>20)
      - nesting 6 (>4)
      - 8 params (>5)
!!! [CRITICAL] mb_mime_header_encode()
    File: php-wasm-project/php-src/ext/mbstring/mbstring.c
    Language: C
    Thresholds exceeded: 4/4
    Reasons:
      - 198 lines (>150)
      - complexity 62 (>20)
      - nesting 7 (>4)
      - 7 params (>5)
!!! [CRITICAL] zend_scan_escape_string()
    File: php-wasm-project/php-src/Zend/zend_language_scanner.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 180 lines (>150)
      - complexity 54 (>20)
      - nesting 8 (>4)
!!! [CRITICAL] destroy_zend_class()
    File: php-wasm-project/php-src/Zend/zend_opcode.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 200 lines (>150)
      - complexity 54 (>20)
      - nesting 6 (>4)
!!! [CRITICAL] pass_two()
    File: php-wasm-project/php-src/Zend/zend_opcode.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 153 lines (>150)
      - complexity 49 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] yyparse()
    File: php-wasm-project/php-src/Zend/zend_ini_parser.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 457 lines (>150)
      - complexity 97 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] gen_vm()
    File: php-wasm-project/php-src/Zend/zend_vm_gen.php
    Language: PHP
    Thresholds exceeded: 3/4
    Reasons:
      - 362 lines (>150)
      - complexity 121 (>20)
      - nesting 8 (>4)
!!! [CRITICAL] gc_scan_black()
    File: php-wasm-project/php-src/Zend/zend_gc.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 172 lines (>150)
      - complexity 46 (>20)
      - nesting 9 (>4)
!!! [CRITICAL] gc_mark_grey()
    File: php-wasm-project/php-src/Zend/zend_gc.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 162 lines (>150)
      - complexity 40 (>20)
      - nesting 9 (>4)
!!! [CRITICAL] gc_collect_white()
    File: php-wasm-project/php-src/Zend/zend_gc.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 172 lines (>150)
      - complexity 46 (>20)
      - nesting 9 (>4)
!!! [CRITICAL] zend_gc_collect_cycles()
    File: php-wasm-project/php-src/Zend/zend_gc.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 161 lines (>150)
      - complexity 29 (>20)
      - nesting 7 (>4)
!!! [CRITICAL] zend_check_type_slow()
    File: php-wasm-project/php-src/Zend/zend_execute.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 21 (>20)
      - nesting 7 (>4)
      - 6 params (>5)
!!! [CRITICAL] zend_fetch_dimension_address_read()
    File: php-wasm-project/php-src/Zend/zend_execute.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 47 (>20)
      - nesting 7 (>4)
      - 7 params (>5)
!!! [CRITICAL] zend_fetch_property_address()
    File: php-wasm-project/php-src/Zend/zend_execute.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 43 (>20)
      - nesting 7 (>4)
      - 8 params (>5)
!!! [CRITICAL] zend_ast_evaluate_inner()
    File: php-wasm-project/php-src/Zend/zend_ast.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 453 lines (>150)
      - complexity 104 (>20)
      - nesting 7 (>4)
!!! [CRITICAL] yyparse()
    File: php-wasm-project/php-src/Zend/zend_language_parser.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 1879 lines (>150)
      - complexity 583 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] sccp_visit_instr()
    File: php-wasm-project/php-src/Zend/Optimizer/sccp.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 739 lines (>150)
      - complexity 234 (>20)
      - nesting 7 (>4)
!!! [CRITICAL] try_remove_definition()
    File: php-wasm-project/php-src/Zend/Optimizer/sccp.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 207 lines (>150)
      - complexity 116 (>20)
      - nesting 6 (>4)
!!! [CRITICAL] zend_dfa_optimize_jmps()
    File: php-wasm-project/php-src/Zend/Optimizer/dfa_pass.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 262 lines (>150)
      - complexity 85 (>20)
      - nesting 7 (>4)
!!! [CRITICAL] zend_dfa_optimize_op_array()
    File: php-wasm-project/php-src/Zend/Optimizer/dfa_pass.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 488 lines (>150)
      - complexity 230 (>20)
      - nesting 8 (>4)
!!! [CRITICAL] zend_optimizer_compact_literals()
    File: php-wasm-project/php-src/Zend/Optimizer/compact_literals.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 639 lines (>150)
      - complexity 208 (>20)
      - nesting 7 (>4)
!!! [CRITICAL] zend_dump_type_info()
    File: php-wasm-project/php-src/Zend/Optimizer/zend_dump.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 182 lines (>150)
      - complexity 90 (>20)
      - nesting 6 (>4)
!!! [CRITICAL] zend_dump_op_array()
    File: php-wasm-project/php-src/Zend/Optimizer/zend_dump.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 222 lines (>150)
      - complexity 56 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] zend_optimize_func_calls()
    File: php-wasm-project/php-src/Zend/Optimizer/optimize_func_calls.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 186 lines (>150)
      - complexity 75 (>20)
      - nesting 6 (>4)
!!! [CRITICAL] may_have_side_effects()
    File: php-wasm-project/php-src/Zend/Optimizer/dce.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 172 lines (>150)
      - complexity 120 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] assemble_code_blocks()
    File: php-wasm-project/php-src/Zend/Optimizer/block_pass.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 155 lines (>150)
      - complexity 56 (>20)
      - nesting 6 (>4)
!!! [CRITICAL] zend_jmp_optimization()
    File: php-wasm-project/php-src/Zend/Optimizer/block_pass.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 174 lines (>150)
      - complexity 66 (>20)
      - nesting 6 (>4)
!!! [CRITICAL] zend_t_usage()
    File: php-wasm-project/php-src/Zend/Optimizer/block_pass.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 179 lines (>150)
      - complexity 75 (>20)
      - nesting 6 (>4)
!!! [CRITICAL] zend_optimizer_pass1()
    File: php-wasm-project/php-src/Zend/Optimizer/pass1.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 276 lines (>150)
      - complexity 128 (>20)
      - nesting 8 (>4)
!!! [CRITICAL] ssa_verify_integrity()
    File: php-wasm-project/php-src/Zend/Optimizer/ssa_integrity.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 287 lines (>150)
      - complexity 102 (>20)
      - nesting 6 (>4)
!!! [CRITICAL] spl_recursive_it_move_forward_ex()
    File: php-wasm-project/php-src/ext/spl/spl_iterators.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 174 lines (>150)
      - complexity 47 (>20)
      - nesting 7 (>4)
!!! [CRITICAL] ftp_get()
    File: php-wasm-project/php-src/ext/ftp/ftp.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 26 (>20)
      - nesting 5 (>4)
      - 6 params (>5)
!!! [CRITICAL] zend_file_cache_serialize_op_array()
    File: php-wasm-project/php-src/ext/opcache/zend_file_cache.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 166 lines (>150)
      - complexity 44 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] zend_file_cache_serialize_class()
    File: php-wasm-project/php-src/ext/opcache/zend_file_cache.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 154 lines (>150)
      - complexity 25 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] zend_file_cache_unserialize_op_array()
    File: php-wasm-project/php-src/ext/opcache/zend_file_cache.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 161 lines (>150)
      - complexity 46 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] zend_file_cache_unserialize_class()
    File: php-wasm-project/php-src/ext/opcache/zend_file_cache.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 158 lines (>150)
      - complexity 30 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] zend_persist_op_array_ex()
    File: php-wasm-project/php-src/ext/opcache/zend_persist.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 271 lines (>150)
      - complexity 82 (>20)
      - nesting 6 (>4)
!!! [CRITICAL] zend_jit_trace_allocate_registers()
    File: php-wasm-project/php-src/ext/opcache/jit/zend_jit_trace.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 693 lines (>150)
      - complexity 265 (>20)
      - nesting 8 (>4)
!!! [CRITICAL] zend_jit_trace_deoptimization()
    File: php-wasm-project/php-src/ext/opcache/jit/zend_jit_trace.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 53 (>20)
      - nesting 7 (>4)
      - 9 params (>5)
!!! [CRITICAL] zend_jit_dump_trace()
    File: php-wasm-project/php-src/ext/opcache/jit/zend_jit_trace.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 194 lines (>150)
      - complexity 80 (>20)
      - nesting 7 (>4)
!!! [CRITICAL] zend_jit_trace_exit()
    File: php-wasm-project/php-src/ext/opcache/jit/zend_jit_trace.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 179 lines (>150)
      - complexity 72 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] zend_may_overflow()
    File: php-wasm-project/php-src/ext/opcache/jit/zend_jit.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 165 lines (>150)
      - complexity 80 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] zend_jit_compute_liveness()
    File: php-wasm-project/php-src/ext/opcache/jit/zend_jit.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 270 lines (>150)
      - complexity 114 (>20)
      - nesting 9 (>4)
!!! [CRITICAL] zend_jit_allocate_registers()
    File: php-wasm-project/php-src/ext/opcache/jit/zend_jit.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 192 lines (>150)
      - complexity 74 (>20)
      - nesting 11 (>4)
!!! [CRITICAL] zend_jit()
    File: php-wasm-project/php-src/ext/opcache/jit/zend_jit.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 1388 lines (>150)
      - complexity 583 (>20)
      - nesting 11 (>4)
!!! [CRITICAL] luaV_execute()
    File: php-wasm-project/php-src/ext/opcache/jit/dynasm/minilua.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 366 lines (>150)
      - complexity 81 (>20)
      - nesting 6 (>4)
!!! [CRITICAL] ZEND_HASH_FOREACH_VAL()
    File: php-wasm-project/php-src/ext/imap/php_imap.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 241 lines (>150)
      - complexity 65 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] _php_imap_mail()
    File: php-wasm-project/php-src/ext/imap/php_imap.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 154 lines (>150)
      - complexity 41 (>20)
      - 7 params (>5)
!!! [CRITICAL] do_callback()
    File: php-wasm-project/php-src/ext/pdo_sqlite/sqlite_driver.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 27 (>20)
      - nesting 5 (>4)
      - 6 params (>5)
!!! [CRITICAL] php_snmp_internal()
    File: php-wasm-project/php-src/ext/snmp/snmp.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 232 lines (>150)
      - complexity 76 (>20)
      - nesting 9 (>4)
!!! [CRITICAL] _php_iconv_substr()
    File: php-wasm-project/php-src/ext/iconv/iconv.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 32 (>20)
      - nesting 6 (>4)
      - 6 params (>5)
!!! [CRITICAL] _php_iconv_strpos()
    File: php-wasm-project/php-src/ext/iconv/iconv.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 25 (>20)
      - nesting 6 (>4)
      - 8 params (>5)
!!! [CRITICAL] php_iconv_stream_filter_append_bucket()
    File: php-wasm-project/php-src/ext/iconv/iconv.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 26 (>20)
      - nesting 8 (>4)
      - 8 params (>5)
!!! [CRITICAL] grapheme_strpos_utf16()
    File: php-wasm-project/php-src/ext/intl/grapheme/grapheme_util.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 23 (>20)
      - nesting 5 (>4)
      - 8 params (>5)
!!! [CRITICAL] php_dba_open()
    File: php-wasm-project/php-src/ext/dba/dba.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 359 lines (>150)
      - complexity 98 (>20)
      - nesting 6 (>4)
!!! [CRITICAL] _php_ldap_control_from_array()
    File: php-wasm-project/php-src/ext/ldap/ldap.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 308 lines (>150)
      - complexity 71 (>20)
      - nesting 7 (>4)
!!! [CRITICAL] php_ldap_do_search()
    File: php-wasm-project/php-src/ext/ldap/ldap.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 248 lines (>150)
      - complexity 52 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] php_ldap_do_modify()
    File: php-wasm-project/php-src/ext/ldap/ldap.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 155 lines (>150)
      - complexity 27 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] scan()
    File: php-wasm-project/php-src/ext/date/lib/parse_iso_intervals.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 788 lines (>150)
      - complexity 227 (>20)
      - nesting 6 (>4)
!!! [CRITICAL] scan()
    File: php-wasm-project/php-src/ext/date/lib/parse_date.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 23786 lines (>150)
      - complexity 9502 (>20)
      - nesting 8 (>4)
!!! [CRITICAL] php_mysqlnd_rowp_read_binary_protocol()
    File: php-wasm-project/php-src/ext/mysqlnd/mysqlnd_wireprotocol.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 38 (>20)
      - nesting 5 (>4)
      - 6 params (>5)
!!! [CRITICAL] php_mysqlnd_rowp_read_text_protocol()
    File: php-wasm-project/php-src/ext/mysqlnd/mysqlnd_wireprotocol.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 58 (>20)
      - nesting 6 (>4)
      - 6 params (>5)
!!! [CRITICAL] phar_path_check()
    File: php-wasm-project/php-src/ext/phar/phar_path_check.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 209 lines (>150)
      - complexity 63 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] pdo_parse_params()
    File: php-wasm-project/php-src/ext/pdo/pdo_sql_parser.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 262 lines (>150)
      - complexity 72 (>20)
      - nesting 8 (>4)
!!! [CRITICAL] php_bz2_decompress_filter()
    File: php-wasm-project/php-src/ext/bz2/bz2_filter.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 23 (>20)
      - nesting 5 (>4)
      - 6 params (>5)
!!! [CRITICAL] odbc_stmt_param_hook()
    File: php-wasm-project/php-src/ext/pdo_odbc/odbc_stmt.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 203 lines (>150)
      - complexity 59 (>20)
      - nesting 9 (>4)
!!! [CRITICAL] get_sdl_from_cache()
    File: php-wasm-project/php-src/ext/soap/php_sdl.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 223 lines (>150)
      - complexity 39 (>20)
      - nesting 7 (>4)
!!! [CRITICAL] add_sdl_to_cache()
    File: php-wasm-project/php-src/ext/soap/php_sdl.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 243 lines (>150)
      - complexity 33 (>20)
      - nesting 7 (>4)
!!! [CRITICAL] get_sdl()
    File: php-wasm-project/php-src/ext/soap/php_sdl.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 186 lines (>150)
      - complexity 43 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] schema_element()
    File: php-wasm-project/php-src/ext/soap/php_schema.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 196 lines (>150)
      - complexity 57 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] schema_attribute()
    File: php-wasm-project/php-src/ext/soap/php_schema.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 200 lines (>150)
      - complexity 61 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] model_to_xml_object()
    File: php-wasm-project/php-src/ext/soap/php_encoding.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 151 lines (>150)
      - complexity 64 (>20)
      - nesting 8 (>4)
!!! [CRITICAL] to_xml_object()
    File: php-wasm-project/php-src/ext/soap/php_encoding.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 167 lines (>150)
      - complexity 64 (>20)
      - nesting 9 (>4)
!!! [CRITICAL] add_xml_array_elements()
    File: php-wasm-project/php-src/ext/soap/php_encoding.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 27 (>20)
      - nesting 5 (>4)
      - 8 params (>5)
!!! [CRITICAL] to_xml_array()
    File: php-wasm-project/php-src/ext/soap/php_encoding.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 244 lines (>150)
      - complexity 76 (>20)
      - nesting 6 (>4)
!!! [CRITICAL] ValidateFormat()
    File: php-wasm-project/php-src/ext/standard/scanf.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 181 lines (>150)
      - complexity 64 (>20)
      - nesting 6 (>4)
!!! [CRITICAL] php_parserr()
    File: php-wasm-project/php-src/ext/standard/dns_win32.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 163 lines (>150)
      - complexity 31 (>20)
      - nesting 6 (>4)
!!! [CRITICAL] php_array_pick_keys()
    File: php-wasm-project/php-src/ext/standard/array.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 27 (>20)
      - nesting 6 (>4)
      - 6 params (>5)
!!! [CRITICAL] php_stat()
    File: php-wasm-project/php-src/ext/standard/filestat.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 244 lines (>150)
      - complexity 83 (>20)
      - nesting 6 (>4)
!!! [CRITICAL] php_get_windows_name()
    File: php-wasm-project/php-src/ext/standard/info.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 341 lines (>150)
      - complexity 127 (>20)
      - nesting 6 (>4)
!!! [CRITICAL] metaphone()
    File: php-wasm-project/php-src/ext/standard/metaphone.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 221 lines (>150)
      - complexity 95 (>20)
      - nesting 6 (>4)
!!! [CRITICAL] php_str_replace_in_subject()
    File: php-wasm-project/php-src/ext/standard/string.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 27 (>20)
      - nesting 7 (>4)
      - 7 params (>5)
!!! [CRITICAL] php_strip_tags_ex()
    File: php-wasm-project/php-src/ext/standard/string.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 304 lines (>150)
      - complexity 120 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] php_conv_qprint_encode_convert()
    File: php-wasm-project/php-src/ext/standard/filters.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 158 lines (>150)
      - complexity 47 (>20)
      - nesting 8 (>4)
!!! [CRITICAL] php_conv_qprint_decode_convert()
    File: php-wasm-project/php-src/ext/standard/filters.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 153 lines (>150)
      - complexity 52 (>20)
      - nesting 6 (>4)
!!! [CRITICAL] strfilter_convert_append_bucket()
    File: php-wasm-project/php-src/ext/standard/filters.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 27 (>20)
      - nesting 7 (>4)
      - 8 params (>5)
!!! [CRITICAL] php_url_encode_hash_ex()
    File: php-wasm-project/php-src/ext/standard/http.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 26 (>20)
      - nesting 5 (>4)
      - 8 params (>5)
!!! [CRITICAL] get_next_char()
    File: php-wasm-project/php-src/ext/standard/html.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 222 lines (>150)
      - complexity 115 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] php_formatted_print()
    File: php-wasm-project/php-src/ext/standard/formatted_print.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 301 lines (>150)
      - complexity 70 (>20)
      - nesting 10 (>4)
!!! [CRITICAL] php_formatted_print()
    File: php-wasm-project/php-src/ext/standard/formatted_print.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 273 lines (>150)
      - complexity 70 (>20)
      - nesting 7 (>4)
!!! [CRITICAL] pdo_mysql_handle_factory()
    File: php-wasm-project/php-src/ext/pdo_mysql/mysql_driver.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 272 lines (>150)
      - complexity 69 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] ZEND_HASH_FOREACH_KEY_VAL()
    File: php-wasm-project/php-src/ext/curl/interface.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 156 lines (>150)
      - complexity 36 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] yyparse()
    File: php-wasm-project/php-src/ext/json/json_parser.tab.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 380 lines (>150)
      - complexity 72 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] php_json_scan()
    File: php-wasm-project/php-src/ext/json/json_scanner.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 1523 lines (>150)
      - complexity 537 (>20)
      - nesting 6 (>4)
!!! [CRITICAL] file_buffer()
    File: php-wasm-project/php-src/ext/fileinfo/libmagic/funcs.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 160 lines (>150)
      - complexity 55 (>20)
      - 6 params (>5)
!!! [CRITICAL] mcopy()
    File: php-wasm-project/php-src/ext/fileinfo/libmagic/softmagic.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 38 (>20)
      - nesting 6 (>4)
      - 8 params (>5)
!!! [CRITICAL] mget()
    File: php-wasm-project/php-src/ext/fileinfo/libmagic/softmagic.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 355 lines (>150)
      - complexity 134 (>20)
      - 16 params (>5)
!!! [CRITICAL] magiccheck()
    File: php-wasm-project/php-src/ext/fileinfo/libmagic/softmagic.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 332 lines (>150)
      - complexity 108 (>20)
      - nesting 8 (>4)
!!! [CRITICAL] check_format_type()
    File: php-wasm-project/php-src/ext/fileinfo/libmagic/apprentice.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 167 lines (>150)
      - complexity 77 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] php_oci_statement_execute()
    File: php-wasm-project/php-src/ext/oci8/oci8_statement.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 303 lines (>150)
      - complexity 68 (>20)
      - nesting 6 (>4)
!!! [CRITICAL] php_oci_bind_by_name()
    File: php-wasm-project/php-src/ext/oci8/oci8_statement.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 211 lines (>150)
      - complexity 50 (>20)
      - 6 params (>5)
!!! [CRITICAL] get_skip_sym()
    File: php-wasm-project/php-src/ext/ffi/ffi_parser.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 1368 lines (>150)
      - complexity 602 (>20)
      - nesting 7 (>4)
!!! [CRITICAL] parse_array_or_function_declarators()
    File: php-wasm-project/php-src/ext/ffi/ffi_parser.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 194 lines (>150)
      - complexity 62 (>20)
      - nesting 6 (>4)
!!! [CRITICAL] parse_unary_expression()
    File: php-wasm-project/php-src/ext/ffi/ffi_parser.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 165 lines (>150)
      - complexity 43 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] odbc_sqlconnect()
    File: php-wasm-project/php-src/ext/odbc/php_odbc.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 29 (>20)
      - nesting 5 (>4)
      - 6 params (>5)
!!! [CRITICAL] exif_process_IFD_in_TIFF_impl()
    File: php-wasm-project/php-src/ext/exif/exif.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 218 lines (>150)
      - complexity 63 (>20)
      - nesting 10 (>4)
!!! [CRITICAL] php_pgsql_convert()
    File: php-wasm-project/php-src/ext/pgsql/pgsql.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 543 lines (>150)
      - complexity 160 (>20)
      - nesting 7 (>4)
!!! [CRITICAL] gdImageTrueColorToPaletteBody()
    File: php-wasm-project/php-src/ext/gd/libgd/gd_topal.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 187 lines (>150)
      - complexity 39 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] gdft_draw_bitmap()
    File: php-wasm-project/php-src/ext/gd/libgd/gdft.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 34 (>20)
      - nesting 7 (>4)
      - 6 params (>5)
!!! [CRITICAL] gdImageCreateFromGd2PartCtx()
    File: php-wasm-project/php-src/ext/gd/libgd/gd_gd2.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 169 lines (>150)
      - complexity 46 (>20)
      - nesting 8 (>4)
!!! [CRITICAL] main()
    File: php-wasm-project/php-src/ext/gd/libgd/webpng.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 227 lines (>150)
      - complexity 39 (>20)
      - nesting 7 (>4)
!!! [CRITICAL] gdImageScaleBicubicFixed()
    File: php-wasm-project/php-src/ext/gd/libgd/gd_interpolation.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 185 lines (>150)
      - complexity 38 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] gdImageRotateBicubicFixed()
    File: php-wasm-project/php-src/ext/gd/libgd/gd_interpolation.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 227 lines (>150)
      - complexity 50 (>20)
      - nesting 7 (>4)
!!! [CRITICAL] gdTransformAffineCopy()
    File: php-wasm-project/php-src/ext/gd/libgd/gd_interpolation.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 23 (>20)
      - nesting 5 (>4)
      - 6 params (>5)
!!! [CRITICAL] gdImageLine()
    File: php-wasm-project/php-src/ext/gd/libgd/gd.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 32 (>20)
      - nesting 5 (>4)
      - 6 params (>5)
!!! [CRITICAL] gdImageFilledArc()
    File: php-wasm-project/php-src/ext/gd/libgd/gd.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 30 (>20)
      - nesting 7 (>4)
      - 9 params (>5)
!!! [CRITICAL] gdImageCopyResampled()
    File: php-wasm-project/php-src/ext/gd/libgd/gd.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 21 (>20)
      - nesting 7 (>4)
      - 10 params (>5)
!!! [CRITICAL] gdImageCreateFromPngCtx()
    File: php-wasm-project/php-src/ext/gd/libgd/gd_png.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 237 lines (>150)
      - complexity 52 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] gdImagePngCtxEx()
    File: php-wasm-project/php-src/ext/gd/libgd/gd_png.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 214 lines (>150)
      - complexity 53 (>20)
      - nesting 6 (>4)
!!! [CRITICAL] gdImageBmpCtx()
    File: php-wasm-project/php-src/ext/gd/libgd/gd_bmp.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 153 lines (>150)
      - complexity 36 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] show_parsed()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_compile.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 219 lines (>150)
      - complexity 102 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] parse_regex()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_compile.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 1493 lines (>150)
      - complexity 501 (>20)
      - nesting 8 (>4)
!!! [CRITICAL] If()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_compile.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 154 lines (>150)
      - complexity 52 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] add_to_class_internal()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_compile.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 33 (>20)
      - nesting 5 (>4)
      - 6 params (>5)
!!! [CRITICAL] get_branchlength()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_compile.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 271 lines (>150)
      - complexity 109 (>20)
      - nesting 6 (>4)
!!! [CRITICAL] scan_prefix()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_jit_compile.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 356 lines (>150)
      - complexity 144 (>20)
      - nesting 8 (>4)
!!! [CRITICAL] compile_xclass_matchingpath()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_jit_compile.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 632 lines (>150)
      - complexity 158 (>20)
      - nesting 8 (>4)
!!! [CRITICAL] compile_char1_matchingpath()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_jit_compile.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 357 lines (>150)
      - complexity 113 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] compile_assert_matchingpath()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_jit_compile.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 370 lines (>150)
      - complexity 95 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] compile_bracket_matchingpath()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_jit_compile.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 475 lines (>150)
      - complexity 178 (>20)
      - nesting 6 (>4)
!!! [CRITICAL] get_iterator_parameters()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_jit_compile.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 45 (>20)
      - nesting 5 (>4)
      - 7 params (>5)
!!! [CRITICAL] compile_iterator_matchingpath()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_jit_compile.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 440 lines (>150)
      - complexity 109 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] compile_bracket_backtrackingpath()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_jit_compile.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 407 lines (>150)
      - complexity 141 (>20)
      - nesting 6 (>4)
!!! [CRITICAL] compile_recurse()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_jit_compile.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 151 lines (>150)
      - complexity 32 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] jit_compile()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_jit_compile.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 566 lines (>150)
      - complexity 131 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] set_start_bits()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_study.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 502 lines (>150)
      - complexity 230 (>20)
      - nesting 8 (>4)
!!! [CRITICAL] pcre2_printint()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_printint.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 441 lines (>150)
      - complexity 205 (>20)
      - nesting 10 (>4)
!!! [CRITICAL] sljit_emit_enter()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeARM_64.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 32 (>20)
      - nesting 5 (>4)
      - 8 params (>5)
!!! [CRITICAL] emit_op()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeSPARC_common.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 42 (>20)
      - nesting 5 (>4)
      - 9 params (>5)
!!! [CRITICAL] sljit_generate_code()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativePPC_common.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 216 lines (>150)
      - complexity 61 (>20)
      - nesting 7 (>4)
!!! [CRITICAL] sljit_emit_enter()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativePPC_common.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 27 (>20)
      - nesting 5 (>4)
      - 8 params (>5)
!!! [CRITICAL] sljit_emit_enter()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeX86_64.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 155 lines (>150)
      - complexity 30 (>20)
      - 8 params (>5)
!!! [CRITICAL] getput_arg()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeRISCV_common.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 29 (>20)
      - nesting 5 (>4)
      - 7 params (>5)
!!! [CRITICAL] emit_op()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeRISCV_common.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 43 (>20)
      - nesting 5 (>4)
      - 9 params (>5)
!!! [CRITICAL] sljit_generate_code()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeARM_32.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 254 lines (>150)
      - complexity 75 (>20)
      - nesting 7 (>4)
!!! [CRITICAL] emit_single_op()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeARM_32.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 152 lines (>150)
      - complexity 78 (>20)
      - 6 params (>5)
!!! [CRITICAL] emit_op()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeARM_32.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 39 (>20)
      - nesting 5 (>4)
      - 9 params (>5)
!!! [CRITICAL] emit_single_op()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativePPC_32.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 228 lines (>150)
      - complexity 75 (>20)
      - 6 params (>5)
!!! [CRITICAL] sljit_emit_enter()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeMIPS_common.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 168 lines (>150)
      - complexity 58 (>20)
      - 8 params (>5)
!!! [CRITICAL] getput_arg()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeMIPS_common.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 29 (>20)
      - nesting 5 (>4)
      - 7 params (>5)
!!! [CRITICAL] emit_op()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeMIPS_common.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 41 (>20)
      - nesting 5 (>4)
      - 9 params (>5)
!!! [CRITICAL] sljit_generate_code()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeS390X.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 153 lines (>150)
      - complexity 27 (>20)
      - nesting 6 (>4)
!!! [CRITICAL] sljit_emit_op1()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeS390X.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 251 lines (>150)
      - complexity 82 (>20)
      - 6 params (>5)
!!! [CRITICAL] sljit_emit_sub()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeS390X.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 65 (>20)
      - nesting 5 (>4)
      - 8 params (>5)
!!! [CRITICAL] emit_op_imm()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeARM_T2_32.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 312 lines (>150)
      - complexity 152 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] tail_call_with_args()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeX86_32.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 184 lines (>150)
      - complexity 46 (>20)
      - nesting 6 (>4)
!!! [CRITICAL] php_com_do_invoke_byref()
    File: php-wasm-project/php-src/ext/com_dotnet/com_com.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 23 (>20)
      - nesting 7 (>4)
      - 6 params (>5)
!!! [CRITICAL] firebird_stmt_param_hook()
    File: php-wasm-project/php-src/ext/pdo_firebird/firebird_statement.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 208 lines (>150)
      - complexity 63 (>20)
      - nesting 7 (>4)
!!! [CRITICAL] php_mb_parse_encoding_list()
    File: php-wasm-project/php-src/ext/mbstring/mbstring.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - complexity 21 (>20)
      - nesting 6 (>4)
      - 6 params (>5)
!!! [CRITICAL] PHP_FUNCTION()
    File: php-wasm-project/php-src/ext/mbstring/mbstring.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 209 lines (>150)
      - complexity 56 (>20)
      - nesting 7 (>4)
!!! [CRITICAL] mb_fast_check_utf8_default()
    File: php-wasm-project/php-src/ext/mbstring/mbstring.c
    Language: C
    Thresholds exceeded: 3/4
    Reasons:
      - 159 lines (>150)
      - complexity 47 (>20)
      - nesting 5 (>4)
 !  [HIGH] ___syscall_ioctl()
    File: php-wasm-project/wasm/php.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 38 (>20)
      - nesting 6 (>4)
 !  [HIGH] PHP()
    File: php-wasm-project/wasm/php.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 696 (>20)
      - nesting 6 (>4)
 !  [HIGH] _getaddrinfo()
    File: php-wasm-project/wasm/php.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 45 (>20)
      - 6 params (>5)
 !  [HIGH] _strptime()
    File: php-wasm-project/wasm/php.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 68 (>20)
      - nesting 5 (>4)
 !  [HIGH] zend_is_class_subtype_of_type()
    File: php-wasm-project/php-src/Zend/zend_inheritance.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 21 (>20)
      - nesting 5 (>4)
 !  [HIGH] zend_multibyte_detect_unicode()
    File: php-wasm-project/php-src/Zend/zend_language_scanner.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 31 (>20)
      - nesting 7 (>4)
 !  [HIGH] emit_live_range()
    File: php-wasm-project/php-src/Zend/zend_opcode.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 45 (>20)
      - nesting 6 (>4)
 !  [HIGH] zend_calc_live_ranges()
    File: php-wasm-project/php-src/Zend/zend_opcode.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 21 (>20)
      - nesting 6 (>4)
 !  [HIGH] zend_compile_match()
    File: php-wasm-project/php-src/Zend/zend_compile.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 33 (>20)
      - nesting 6 (>4)
 !  [HIGH] zend_shutdown_executor_values()
    File: php-wasm-project/php-src/Zend/zend_execute_API.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 26 (>20)
      - nesting 8 (>4)
 !  [HIGH] yysyntax_error()
    File: php-wasm-project/php-src/Zend/zend_ini_parser.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 22 (>20)
      - nesting 5 (>4)
 !  [HIGH] gc_scan()
    File: php-wasm-project/php-src/Zend/zend_gc.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 38 (>20)
      - nesting 9 (>4)
 !  [HIGH] gc_remove_nested_data_from_buffer()
    File: php-wasm-project/php-src/Zend/zend_gc.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 32 (>20)
      - nesting 7 (>4)
 !  [HIGH] slow_index_convert_w()
    File: php-wasm-project/php-src/Zend/zend_execute.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 23 (>20)
      - nesting 5 (>4)
 !  [HIGH] zend_fetch_dimension_address()
    File: php-wasm-project/php-src/Zend/zend_execute.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 44 (>20)
      - nesting 6 (>4)
 !  [HIGH] zend_error_zstr_at()
    File: php-wasm-project/php-src/Zend/zend.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 37 (>20)
      - nesting 5 (>4)
 !  [HIGH] zend_hash_destroy()
    File: php-wasm-project/php-src/Zend/zend_hash.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 26 (>20)
      - nesting 8 (>4)
 !  [HIGH] zend_hash_clean()
    File: php-wasm-project/php-src/Zend/zend_hash.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 25 (>20)
      - nesting 8 (>4)
 !  [HIGH] zend_array_dup_value()
    File: php-wasm-project/php-src/Zend/zend_hash.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - nesting 5 (>4)
      - 6 params (>5)
 !  [HIGH] zend_hash_merge()
    File: php-wasm-project/php-src/Zend/zend_hash.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 25 (>20)
      - nesting 5 (>4)
 !  [HIGH] zend_hash_compare_impl()
    File: php-wasm-project/php-src/Zend/zend_hash.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 33 (>20)
      - nesting 5 (>4)
 !  [HIGH] zend_objects_clone_members()
    File: php-wasm-project/php-src/Zend/zend_objects.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 22 (>20)
      - nesting 5 (>4)
 !  [HIGH] yysyntax_error()
    File: php-wasm-project/php-src/Zend/zend_language_parser.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 22 (>20)
      - nesting 5 (>4)
 !  [HIGH] yydestruct()
    File: php-wasm-project/php-src/Zend/zend_language_parser.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - 480 lines (>150)
      - complexity 158 (>20)
 !  [HIGH] sccp_mark_feasible_successors()
    File: php-wasm-project/php-src/Zend/Optimizer/sccp.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 40 (>20)
      - nesting 5 (>4)
 !  [HIGH] zend_ssa_remove_nops()
    File: php-wasm-project/php-src/Zend/Optimizer/dfa_pass.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 32 (>20)
      - nesting 7 (>4)
 !  [HIGH] zend_dfa_optimize_calls()
    File: php-wasm-project/php-src/Zend/Optimizer/dfa_pass.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 33 (>20)
      - nesting 8 (>4)
 !  [HIGH] zend_ssa_replace_control_link()
    File: php-wasm-project/php-src/Zend/Optimizer/dfa_pass.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 29 (>20)
      - nesting 6 (>4)
 !  [HIGH] zend_dfa_try_to_replace_result()
    File: php-wasm-project/php-src/Zend/Optimizer/dfa_pass.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 26 (>20)
      - nesting 6 (>4)
 !  [HIGH] zend_optimize_temporary_variables()
    File: php-wasm-project/php-src/Zend/Optimizer/optimize_temp_vars_5.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 32 (>20)
      - nesting 8 (>4)
 !  [HIGH] zend_infer_ranges_warmup()
    File: php-wasm-project/php-src/Zend/Optimizer/zend_inference.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 39 (>20)
      - nesting 7 (>4)
 !  [HIGH] WHILE_WORKLIST()
    File: php-wasm-project/php-src/Zend/Optimizer/zend_inference.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 32 (>20)
      - nesting 5 (>4)
 !  [HIGH] binary_op_result_type()
    File: php-wasm-project/php-src/Zend/Optimizer/zend_inference.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 48 (>20)
      - 6 params (>5)
 !  [HIGH] zend_infer_types_ex()
    File: php-wasm-project/php-src/Zend/Optimizer/zend_inference.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 22 (>20)
      - nesting 7 (>4)
 !  [HIGH] can_convert_to_double()
    File: php-wasm-project/php-src/Zend/Optimizer/zend_inference.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 38 (>20)
      - nesting 5 (>4)
 !  [HIGH] zend_func_return_info()
    File: php-wasm-project/php-src/Zend/Optimizer/zend_inference.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 50 (>20)
      - nesting 8 (>4)
 !  [HIGH] zend_mark_cv_references()
    File: php-wasm-project/php-src/Zend/Optimizer/zend_inference.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 53 (>20)
      - nesting 7 (>4)
 !  [HIGH] zend_optimizer_pass3()
    File: php-wasm-project/php-src/Zend/Optimizer/pass3.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 68 (>20)
      - nesting 5 (>4)
 !  [HIGH] dce_optimize_op_array()
    File: php-wasm-project/php-src/Zend/Optimizer/dce.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 31 (>20)
      - nesting 6 (>4)
 !  [HIGH] scdf_solve()
    File: php-wasm-project/php-src/Zend/Optimizer/scdf.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 23 (>20)
      - nesting 6 (>4)
 !  [HIGH] zend_optimizer_update_op1_const()
    File: php-wasm-project/php-src/Zend/Optimizer/zend_optimizer.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - 155 lines (>150)
      - complexity 71 (>20)
 !  [HIGH] zend_optimizer_update_op2_const()
    File: php-wasm-project/php-src/Zend/Optimizer/zend_optimizer.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - 174 lines (>150)
      - complexity 72 (>20)
 !  [HIGH] zend_optimizer_replace_by_const()
    File: php-wasm-project/php-src/Zend/Optimizer/zend_optimizer.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 33 (>20)
      - nesting 8 (>4)
 !  [HIGH] zend_redo_pass_two()
    File: php-wasm-project/php-src/Zend/Optimizer/zend_optimizer.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 55 (>20)
      - nesting 6 (>4)
 !  [HIGH] zend_redo_pass_two_ex()
    File: php-wasm-project/php-src/Zend/Optimizer/zend_optimizer.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 62 (>20)
      - nesting 6 (>4)
 !  [HIGH] zend_optimize_script()
    File: php-wasm-project/php-src/Zend/Optimizer/zend_optimizer.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 39 (>20)
      - nesting 5 (>4)
 !  [HIGH] zend_analyze_calls()
    File: php-wasm-project/php-src/Zend/Optimizer/zend_call_graph.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 37 (>20)
      - nesting 6 (>4)
 !  [HIGH] zend_optimizer_compact_vars()
    File: php-wasm-project/php-src/Zend/Optimizer/compact_vars.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 22 (>20)
      - nesting 5 (>4)
 !  [HIGH] process_args()
    File: php-wasm-project/php-src/ext/ext_skel.php
    Language: PHP
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 28 (>20)
      - nesting 5 (>4)
 !  [HIGH] spl_dual_it_construct()
    File: php-wasm-project/php-src/ext/spl/spl_iterators.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 34 (>20)
      - nesting 6 (>4)
 !  [HIGH] ftp_login()
    File: php-wasm-project/php-src/ext/ftp/ftp.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 30 (>20)
      - nesting 5 (>4)
 !  [HIGH] ftp_pasv()
    File: php-wasm-project/php-src/ext/ftp/ftp.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 26 (>20)
      - nesting 5 (>4)
 !  [HIGH] ftp_put()
    File: php-wasm-project/php-src/ext/ftp/ftp.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 26 (>20)
      - 6 params (>5)
 !  [HIGH] zend_get_file_handle_timestamp()
    File: php-wasm-project/php-src/ext/opcache/ZendAccelerator.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 22 (>20)
      - nesting 5 (>4)
 !  [HIGH] accel_activate()
    File: php-wasm-project/php-src/ext/opcache/ZendAccelerator.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 24 (>20)
      - nesting 5 (>4)
 !  [HIGH] accel_move_code_to_huge_pages()
    File: php-wasm-project/php-src/ext/opcache/ZendAccelerator.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 21 (>20)
      - nesting 7 (>4)
 !  [HIGH] preload_link()
    File: php-wasm-project/php-src/ext/opcache/ZendAccelerator.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 28 (>20)
      - nesting 6 (>4)
 !  [HIGH] preload_remove_empty_includes()
    File: php-wasm-project/php-src/ext/opcache/ZendAccelerator.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 27 (>20)
      - nesting 8 (>4)
 !  [HIGH] accel_preload()
    File: php-wasm-project/php-src/ext/opcache/ZendAccelerator.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - 173 lines (>150)
      - nesting 6 (>4)
 !  [HIGH] zend_persist_class_method()
    File: php-wasm-project/php-src/ext/opcache/zend_persist.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 21 (>20)
      - nesting 7 (>4)
 !  [HIGH] zend_update_parent_ce()
    File: php-wasm-project/php-src/ext/opcache/zend_persist.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 44 (>20)
      - nesting 6 (>4)
 !  [HIGH] zend_accel_persist_class_table()
    File: php-wasm-project/php-src/ext/opcache/zend_persist.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 23 (>20)
      - nesting 8 (>4)
 !  [HIGH] zend_accel_blacklist_update_regexp()
    File: php-wasm-project/php-src/ext/opcache/zend_accelerator_blacklist.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 30 (>20)
      - nesting 6 (>4)
 !  [HIGH] zend_shared_alloc_startup()
    File: php-wasm-project/php-src/ext/opcache/zend_shared_alloc.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 21 (>20)
      - nesting 5 (>4)
 !  [HIGH] create_segments()
    File: php-wasm-project/php-src/ext/opcache/shared_alloc_mmap.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 28 (>20)
      - nesting 5 (>4)
 !  [HIGH] zend_persist_class_entry_calc()
    File: php-wasm-project/php-src/ext/opcache/zend_persist_calc.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 35 (>20)
      - nesting 6 (>4)
 !  [HIGH] zend_jit_fetch_dim_r_helper()
    File: php-wasm-project/php-src/ext/opcache/jit/zend_jit_helpers.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 34 (>20)
      - nesting 6 (>4)
 !  [HIGH] zend_jit_fetch_dim_is_helper()
    File: php-wasm-project/php-src/ext/opcache/jit/zend_jit_helpers.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 35 (>20)
      - nesting 6 (>4)
 !  [HIGH] zend_jit_fetch_dim_rw_helper()
    File: php-wasm-project/php-src/ext/opcache/jit/zend_jit_helpers.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 34 (>20)
      - nesting 6 (>4)
 !  [HIGH] zend_jit_fetch_dim_w_helper()
    File: php-wasm-project/php-src/ext/opcache/jit/zend_jit_helpers.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 36 (>20)
      - nesting 6 (>4)
 !  [HIGH] zend_jit_fetch_dim_obj_helper()
    File: php-wasm-project/php-src/ext/opcache/jit/zend_jit_helpers.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 27 (>20)
      - nesting 5 (>4)
 !  [HIGH] zend_jit_needs_arg_dtor()
    File: php-wasm-project/php-src/ext/opcache/jit/zend_jit_trace.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 23 (>20)
      - nesting 7 (>4)
 !  [HIGH] zend_jit_trace_add_phis()
    File: php-wasm-project/php-src/ext/opcache/jit/zend_jit_trace.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 21 (>20)
      - nesting 5 (>4)
 !  [HIGH] is_checked_guard()
    File: php-wasm-project/php-src/ext/opcache/jit/zend_jit_trace.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 55 (>20)
      - nesting 7 (>4)
 !  [HIGH] zend_jit_may_skip_comparison()
    File: php-wasm-project/php-src/ext/opcache/jit/zend_jit_trace.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 49 (>20)
      - nesting 6 (>4)
 !  [HIGH] zend_jit_dump_exit_info()
    File: php-wasm-project/php-src/ext/opcache/jit/zend_jit_trace.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 25 (>20)
      - nesting 6 (>4)
 !  [HIGH] zend_jit_compile_side_trace()
    File: php-wasm-project/php-src/ext/opcache/jit/zend_jit_trace.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 21 (>20)
      - nesting 7 (>4)
 !  [HIGH] zend_elf_load_symbols()
    File: php-wasm-project/php-src/ext/opcache/jit/zend_elf.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 23 (>20)
      - nesting 9 (>4)
 !  [HIGH] zend_jit_is_constant_cmp_long_long()
    File: php-wasm-project/php-src/ext/opcache/jit/zend_jit.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 33 (>20)
      - 6 params (>5)
 !  [HIGH] zend_get_known_property_info()
    File: php-wasm-project/php-src/ext/opcache/jit/zend_jit.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 25 (>20)
      - nesting 5 (>4)
 !  [HIGH] zend_jit_op_array()
    File: php-wasm-project/php-src/ext/opcache/jit/zend_jit.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 22 (>20)
      - nesting 5 (>4)
 !  [HIGH] zend_jit_script()
    File: php-wasm-project/php-src/ext/opcache/jit/zend_jit.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 33 (>20)
      - nesting 6 (>4)
 !  [HIGH] zend_jit_disasm_add_symbol()
    File: php-wasm-project/php-src/ext/opcache/jit/zend_jit_disasm.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 21 (>20)
      - nesting 6 (>4)
 !  [HIGH] llex()
    File: php-wasm-project/php-src/ext/opcache/jit/dynasm/minilua.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 27 (>20)
      - nesting 6 (>4)
 !  [HIGH] decode_operand()
    File: php-wasm-project/php-src/ext/opcache/jit/libudis86/decode.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - 154 lines (>150)
      - complexity 67 (>20)
 !  [HIGH] ud_translate_intel()
    File: php-wasm-project/php-src/ext/opcache/jit/libudis86/syn-intel.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 44 (>20)
      - nesting 5 (>4)
 !  [HIGH] pdo_sqlite_stmt_param_hook()
    File: php-wasm-project/php-src/ext/pdo_sqlite/sqlite_statement.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 33 (>20)
      - nesting 6 (>4)
 !  [HIGH] ZEND_HASH_FOREACH_VAL()
    File: php-wasm-project/php-src/ext/snmp/snmp.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 24 (>20)
      - nesting 6 (>4)
 !  [HIGH] php_snmp()
    File: php-wasm-project/php-src/ext/snmp/snmp.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - 157 lines (>150)
      - complexity 22 (>20)
 !  [HIGH] php_libxml_node_free()
    File: php-wasm-project/php-src/ext/libxml/libxml.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 37 (>20)
      - nesting 6 (>4)
 !  [HIGH] get_icu_value_internal()
    File: php-wasm-project/php-src/ext/intl/locale/locale_methods.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 28 (>20)
      - nesting 5 (>4)
 !  [HIGH] lookup_loc_range()
    File: php-wasm-project/php-src/ext/intl/locale/locale_methods.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 23 (>20)
      - nesting 5 (>4)
 !  [HIGH] inifile_delete_replace_append()
    File: php-wasm-project/php-src/ext/dba/libinifile/inifile.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 28 (>20)
      - nesting 5 (>4)
 !  [HIGH] _php_ldap_control_to_array()
    File: php-wasm-project/php-src/ext/ldap/ldap.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 32 (>20)
      - nesting 8 (>4)
 !  [HIGH] on_event()
    File: php-wasm-project/php-src/ext/tokenizer/tokenizer.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - nesting 5 (>4)
      - 6 params (>5)
 !  [HIGH] timelib_dump_date()
    File: php-wasm-project/php-src/ext/date/lib/timelib.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 23 (>20)
      - nesting 5 (>4)
 !  [HIGH] do_adjust_timezone()
    File: php-wasm-project/php-src/ext/date/lib/tm2unixtime.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 22 (>20)
      - nesting 6 (>4)
 !  [HIGH] php_mysqlnd_auth_write()
    File: php-wasm-project/php-src/ext/mysqlnd/mysqlnd_wireprotocol.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 22 (>20)
      - nesting 7 (>4)
 !  [HIGH] write_compressed_packet()
    File: php-wasm-project/php-src/ext/mysqlnd/mysqlnd_protocol_frame_codec.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - nesting 5 (>4)
      - 7 params (>5)
 !  [HIGH] mysqlnd_query_read_result_set_header()
    File: php-wasm-project/php-src/ext/mysqlnd/mysqlnd_result.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 28 (>20)
      - nesting 7 (>4)
 !  [HIGH] mysqlnd_stmt_execute_calculate_param_values_size()
    File: php-wasm-project/php-src/ext/mysqlnd/mysqlnd_ps_codec.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 29 (>20)
      - nesting 7 (>4)
 !  [HIGH] mysqlnd_auth_handshake()
    File: php-wasm-project/php-src/ext/mysqlnd/mysqlnd_auth.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - nesting 5 (>4)
      - 20 params (>5)
 !  [HIGH] mysqlnd_auth_change_user()
    File: php-wasm-project/php-src/ext/mysqlnd/mysqlnd_auth.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 21 (>20)
      - 19 params (>5)
 !  [HIGH] mysqlnd_stmt_execute_parse_response()
    File: php-wasm-project/php-src/ext/mysqlnd/mysqlnd_ps.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 24 (>20)
      - nesting 5 (>4)
 !  [HIGH] phar_add_file()
    File: php-wasm-project/php-src/ext/phar/phar_object.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - nesting 5 (>4)
      - 6 params (>5)
 !  [HIGH] phar_fancy_stat()
    File: php-wasm-project/php-src/ext/phar/func_interceptors.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 31 (>20)
      - nesting 6 (>4)
 !  [HIGH] pdo_stmt_setup_fetch_mode()
    File: php-wasm-project/php-src/ext/pdo/pdo_stmt.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 35 (>20)
      - nesting 6 (>4)
 !  [HIGH] scan()
    File: php-wasm-project/php-src/ext/pdo/pdo_sql_parser.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - 308 lines (>150)
      - complexity 187 (>20)
 !  [HIGH] odbc_stmt_execute()
    File: php-wasm-project/php-src/ext/pdo_odbc/odbc_stmt.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 26 (>20)
      - nesting 6 (>4)
 !  [HIGH] odbc_stmt_get_col()
    File: php-wasm-project/php-src/ext/pdo_odbc/odbc_stmt.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 25 (>20)
      - nesting 6 (>4)
 !  [HIGH] ZEND_HASH_FOREACH_VAL()
    File: php-wasm-project/php-src/ext/soap/soap.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 31 (>20)
      - nesting 6 (>4)
 !  [HIGH] soap_client_call_impl()
    File: php-wasm-project/php-src/ext/soap/soap.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 28 (>20)
      - nesting 5 (>4)
 !  [HIGH] sdl_set_uri_credentials()
    File: php-wasm-project/php-src/ext/soap/php_sdl.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 39 (>20)
      - nesting 6 (>4)
 !  [HIGH] load_wsdl_ex()
    File: php-wasm-project/php-src/ext/soap/php_sdl.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 47 (>20)
      - nesting 5 (>4)
 !  [HIGH] wsdl_soap_binding_header()
    File: php-wasm-project/php-src/ext/soap/php_sdl.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 29 (>20)
      - nesting 5 (>4)
 !  [HIGH] wsdl_soap_binding_body()
    File: php-wasm-project/php-src/ext/soap/php_sdl.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 28 (>20)
      - nesting 7 (>4)
 !  [HIGH] load_schema()
    File: php-wasm-project/php-src/ext/soap/php_schema.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 40 (>20)
      - nesting 5 (>4)
 !  [HIGH] schema_group()
    File: php-wasm-project/php-src/ext/soap/php_schema.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 26 (>20)
      - nesting 5 (>4)
 !  [HIGH] schema_complexType()
    File: php-wasm-project/php-src/ext/soap/php_schema.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 27 (>20)
      - nesting 5 (>4)
 !  [HIGH] http_connect()
    File: php-wasm-project/php-src/ext/soap/php_http.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 29 (>20)
      - nesting 5 (>4)
 !  [HIGH] get_http_body()
    File: php-wasm-project/php-src/ext/soap/php_http.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 40 (>20)
      - nesting 7 (>4)
 !  [HIGH] master_to_xml_int()
    File: php-wasm-project/php-src/ext/soap/php_encoding.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 41 (>20)
      - nesting 7 (>4)
 !  [HIGH] to_xml_string()
    File: php-wasm-project/php-src/ext/soap/php_encoding.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 23 (>20)
      - nesting 5 (>4)
 !  [HIGH] model_to_zval_object()
    File: php-wasm-project/php-src/ext/soap/php_encoding.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 38 (>20)
      - nesting 8 (>4)
 !  [HIGH] to_xml_list()
    File: php-wasm-project/php-src/ext/soap/php_encoding.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 22 (>20)
      - nesting 5 (>4)
 !  [HIGH] php_zlib_decode()
    File: php-wasm-project/php-src/ext/zlib/zlib.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - nesting 5 (>4)
      - 6 params (>5)
 !  [HIGH] _xml_characterDataHandler()
    File: php-wasm-project/php-src/ext/xml/xml.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 29 (>20)
      - nesting 6 (>4)
 !  [HIGH] _start_element_handler_ns()
    File: php-wasm-project/php-src/ext/xml/compat.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - nesting 6 (>4)
      - 9 params (>5)
 !  [HIGH] php_mail()
    File: php-wasm-project/php-src/ext/standard/mail.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - 151 lines (>150)
      - complexity 32 (>20)
 !  [HIGH] php_stream_ftp_mkdir()
    File: php-wasm-project/php-src/ext/standard/ftp_fopen_wrapper.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 24 (>20)
      - nesting 7 (>4)
 !  [HIGH] php_array_walk()
    File: php-wasm-project/php-src/ext/standard/array.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 23 (>20)
      - nesting 5 (>4)
 !  [HIGH] php_print_info()
    File: php-wasm-project/php-src/ext/standard/info.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - 255 lines (>150)
      - complexity 44 (>20)
 !  [HIGH] process_nested_object_data()
    File: php-wasm-project/php-src/ext/standard/var_unserializer.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 40 (>20)
      - nesting 7 (>4)
 !  [HIGH] ZEND_HASH_FOREACH_KEY_VAL()
    File: php-wasm-project/php-src/ext/standard/string.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 38 (>20)
      - nesting 5 (>4)
 !  [HIGH] php_strtr_array()
    File: php-wasm-project/php-src/ext/standard/string.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 24 (>20)
      - nesting 6 (>4)
 !  [HIGH] php_stripcslashes()
    File: php-wasm-project/php-src/ext/standard/string.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 23 (>20)
      - nesting 6 (>4)
 !  [HIGH] php_conv_base64_encode_convert()
    File: php-wasm-project/php-src/ext/standard/filters.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 21 (>20)
      - nesting 5 (>4)
 !  [HIGH] php_dechunk()
    File: php-wasm-project/php-src/ext/standard/filters.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 44 (>20)
      - nesting 5 (>4)
 !  [HIGH] xx_mainloop()
    File: php-wasm-project/php-src/ext/standard/url_scanner_ex.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - 409 lines (>150)
      - complexity 62 (>20)
 !  [HIGH] _crypt_extended_init()
    File: php-wasm-project/php-src/ext/standard/crypt_freesec.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 29 (>20)
      - nesting 5 (>4)
 !  [HIGH] php_sha256_crypt_r()
    File: php-wasm-project/php-src/ext/standard/crypt_sha256.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - 184 lines (>150)
      - complexity 30 (>20)
 !  [HIGH] php_setcookie()
    File: php-wasm-project/php-src/ext/standard/head.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 24 (>20)
      - 9 params (>5)
 !  [HIGH] php_sha512_crypt_r()
    File: php-wasm-project/php-src/ext/standard/crypt_sha512.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - 196 lines (>150)
      - complexity 30 (>20)
 !  [HIGH] find_entity_for_char()
    File: php-wasm-project/php-src/ext/standard/html.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - nesting 5 (>4)
      - 8 params (>5)
 !  [HIGH] php_fputcsv()
    File: php-wasm-project/php-src/ext/standard/file.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - nesting 5 (>4)
      - 6 params (>5)
 !  [HIGH] php_next_meta_token()
    File: php-wasm-project/php-src/ext/standard/file.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 33 (>20)
      - nesting 6 (>4)
 !  [HIGH] php_spintf_appenddouble()
    File: php-wasm-project/php-src/ext/standard/formatted_print.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 27 (>20)
      - nesting 7 (>4)
 !  [HIGH] php_sprintf_appenddouble()
    File: php-wasm-project/php-src/ext/standard/formatted_print.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 27 (>20)
      - 10 params (>5)
 !  [HIGH] ParseIpco()
    File: php-wasm-project/php-src/ext/standard/libavifinfo/avifinfo.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 24 (>20)
      - nesting 5 (>4)
 !  [HIGH] pdo_mysql_stmt_after_execute_prepared()
    File: php-wasm-project/php-src/ext/pdo_mysql/mysql_statement.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 23 (>20)
      - nesting 5 (>4)
 !  [HIGH] yysyntax_error()
    File: php-wasm-project/php-src/ext/json/json_parser.tab.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 22 (>20)
      - nesting 5 (>4)
 !  [HIGH] file_mdump()
    File: php-wasm-project/php-src/ext/fileinfo/libmagic/print.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - 185 lines (>150)
      - complexity 84 (>20)
 !  [HIGH] file_zmagic()
    File: php-wasm-project/php-src/ext/fileinfo/libmagic/compress.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 27 (>20)
      - nesting 5 (>4)
 !  [HIGH] uncompressbuf()
    File: php-wasm-project/php-src/ext/fileinfo/libmagic/compress.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 30 (>20)
      - 6 params (>5)
 !  [HIGH] mprint()
    File: php-wasm-project/php-src/ext/fileinfo/libmagic/softmagic.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - 224 lines (>150)
      - complexity 94 (>20)
 !  [HIGH] mconvert()
    File: php-wasm-project/php-src/ext/fileinfo/libmagic/softmagic.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - 151 lines (>150)
      - complexity 76 (>20)
 !  [HIGH] file_strncmp()
    File: php-wasm-project/php-src/ext/fileinfo/libmagic/softmagic.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 33 (>20)
      - nesting 5 (>4)
 !  [HIGH] cdf_read_property_info()
    File: php-wasm-project/php-src/ext/fileinfo/libmagic/cdf.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 42 (>20)
      - 6 params (>5)
 !  [HIGH] cdf_file_property_info()
    File: php-wasm-project/php-src/ext/fileinfo/libmagic/readcdf.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 49 (>20)
      - nesting 6 (>4)
 !  [HIGH] apprentice_load()
    File: php-wasm-project/php-src/ext/fileinfo/libmagic/apprentice.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 24 (>20)
      - nesting 5 (>4)
 !  [HIGH] parse()
    File: php-wasm-project/php-src/ext/fileinfo/libmagic/apprentice.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - 332 lines (>150)
      - complexity 113 (>20)
 !  [HIGH] getstr()
    File: php-wasm-project/php-src/ext/fileinfo/libmagic/apprentice.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 58 (>20)
      - nesting 6 (>4)
 !  [HIGH] file_encoding()
    File: php-wasm-project/php-src/ext/fileinfo/libmagic/encoding.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 26 (>20)
      - 7 params (>5)
 !  [HIGH] php_oci_statement_fetch()
    File: php-wasm-project/php-src/ext/oci8/oci8_statement.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 28 (>20)
      - nesting 6 (>4)
 !  [HIGH] php_oci_bind_post_exec()
    File: php-wasm-project/php-src/ext/oci8/oci8_statement.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 40 (>20)
      - nesting 6 (>4)
 !  [HIGH] php_oci_bind_array_by_name()
    File: php-wasm-project/php-src/ext/oci8/oci8_statement.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 21 (>20)
      - 7 params (>5)
 !  [HIGH] php_oci_fetch_row()
    File: php-wasm-project/php-src/ext/oci8/oci8.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 41 (>20)
      - nesting 5 (>4)
 !  [HIGH] php_oci_old_create_session()
    File: php-wasm-project/php-src/ext/oci8/oci8.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 26 (>20)
      - 10 params (>5)
 !  [HIGH] php_oci_create_session()
    File: php-wasm-project/php-src/ext/oci8/oci8.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 30 (>20)
      - 11 params (>5)
 !  [HIGH] pgsql_stmt_execute()
    File: php-wasm-project/php-src/ext/pdo_pgsql/pgsql_statement.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 25 (>20)
      - nesting 7 (>4)
 !  [HIGH] pgsql_stmt_param_hook()
    File: php-wasm-project/php-src/ext/pdo_pgsql/pgsql_statement.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 40 (>20)
      - nesting 8 (>4)
 !  [HIGH] odbc_bindcols()
    File: php-wasm-project/php-src/ext/odbc/php_odbc.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 27 (>20)
      - nesting 5 (>4)
 !  [HIGH] odbc_do_connect()
    File: php-wasm-project/php-src/ext/odbc/php_odbc.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 24 (>20)
      - nesting 5 (>4)
 !  [HIGH] php_sqlite3_authorizer()
    File: php-wasm-project/php-src/ext/sqlite3/sqlite3.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - nesting 5 (>4)
      - 6 params (>5)
 !  [HIGH] exif_iif_add_value()
    File: php-wasm-project/php-src/ext/exif/exif.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 34 (>20)
      - 9 params (>5)
 !  [HIGH] add_assoc_image_info()
    File: php-wasm-project/php-src/ext/exif/exif.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 46 (>20)
      - nesting 9 (>4)
 !  [HIGH] exif_scan_JPEG_header()
    File: php-wasm-project/php-src/ext/exif/exif.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 41 (>20)
      - nesting 5 (>4)
 !  [HIGH] php_pgsql_do_connect()
    File: php-wasm-project/php-src/ext/pgsql/pgsql.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 34 (>20)
      - nesting 5 (>4)
 !  [HIGH] php_pgsql_fetch_hash()
    File: php-wasm-project/php-src/ext/pgsql/pgsql.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 26 (>20)
      - nesting 5 (>4)
 !  [HIGH] php_dom_nodelist_get_item_into_zval()
    File: php-wasm-project/php-src/ext/dom/nodelist.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 25 (>20)
      - nesting 9 (>4)
 !  [HIGH] dom_node_prefix_write()
    File: php-wasm-project/php-src/ext/dom/node.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 22 (>20)
      - nesting 6 (>4)
 !  [HIGH] _gd2GetHeader()
    File: php-wasm-project/php-src/ext/gd/libgd/gd_gd2.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 31 (>20)
      - 9 params (>5)
 !  [HIGH] gdImageCreateFromGd2Ctx()
    File: php-wasm-project/php-src/ext/gd/libgd/gd_gd2.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 27 (>20)
      - nesting 8 (>4)
 !  [HIGH] _gdImageGd2()
    File: php-wasm-project/php-src/ext/gd/libgd/gd_gd2.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 30 (>20)
      - nesting 7 (>4)
 !  [HIGH] main()
    File: php-wasm-project/php-src/ext/gd/libgd/gdtest.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - 258 lines (>150)
      - complexity 21 (>20)
 !  [HIGH] gdImageSelectiveBlur()
    File: php-wasm-project/php-src/ext/gd/libgd/gd_filter.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 29 (>20)
      - nesting 7 (>4)
 !  [HIGH] gdImageDashedLine()
    File: php-wasm-project/php-src/ext/gd/libgd/gd.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - nesting 5 (>4)
      - 6 params (>5)
 !  [HIGH] gdImageFillToBorder()
    File: php-wasm-project/php-src/ext/gd/libgd/gd.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 34 (>20)
      - nesting 5 (>4)
 !  [HIGH] gdImageFill()
    File: php-wasm-project/php-src/ext/gd/libgd/gd.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 24 (>20)
      - nesting 5 (>4)
 !  [HIGH] gdImageCopy()
    File: php-wasm-project/php-src/ext/gd/libgd/gd.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - nesting 6 (>4)
      - 8 params (>5)
 !  [HIGH] gdImageCopyMergeGray()
    File: php-wasm-project/php-src/ext/gd/libgd/gd.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - nesting 6 (>4)
      - 9 params (>5)
 !  [HIGH] gdImageCopyResized()
    File: php-wasm-project/php-src/ext/gd/libgd/gd.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - nesting 8 (>4)
      - 10 params (>5)
 !  [HIGH] read_image_tga()
    File: php-wasm-project/php-src/ext/gd/libgd/gd_tga.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 24 (>20)
      - nesting 5 (>4)
 !  [HIGH] DetectKanjiCode()
    File: php-wasm-project/php-src/ext/gd/libgd/gdkanji.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 90 (>20)
      - nesting 7 (>4)
 !  [HIGH] do_convert()
    File: php-wasm-project/php-src/ext/gd/libgd/gdkanji.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 31 (>20)
      - nesting 5 (>4)
 !  [HIGH] bmp_read_direct()
    File: php-wasm-project/php-src/ext/gd/libgd/gd_bmp.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 29 (>20)
      - nesting 5 (>4)
 !  [HIGH] bmp_read_4bit()
    File: php-wasm-project/php-src/ext/gd/libgd/gd_bmp.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 23 (>20)
      - nesting 5 (>4)
 !  [HIGH] bmp_read_8bit()
    File: php-wasm-project/php-src/ext/gd/libgd/gd_bmp.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 21 (>20)
      - nesting 5 (>4)
 !  [HIGH] bmp_read_rle()
    File: php-wasm-project/php-src/ext/gd/libgd/gd_bmp.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 36 (>20)
      - nesting 6 (>4)
 !  [HIGH] populate_subpat_array()
    File: php-wasm-project/php-src/ext/pcre/php_pcre.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - nesting 6 (>4)
      - 8 params (>5)
 !  [HIGH] preg_replace_func_impl()
    File: php-wasm-project/php-src/ext/pcre/php_pcre.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - nesting 5 (>4)
      - 9 params (>5)
 !  [HIGH] php_pcre_split_impl()
    File: php-wasm-project/php-src/ext/pcre/php_pcre.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 43 (>20)
      - nesting 7 (>4)
 !  [HIGH] convert_glob_parse_range()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_convert.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 41 (>20)
      - 8 params (>5)
 !  [HIGH] pcre2_pattern_convert()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_convert.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 24 (>20)
      - 6 params (>5)
 !  [HIGH] match_ref()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_match.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 24 (>20)
      - nesting 6 (>4)
 !  [HIGH] read_name()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_compile.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 21 (>20)
      - 9 params (>5)
 !  [HIGH] next_opcode()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_jit_compile.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - 192 lines (>150)
      - complexity 171 (>20)
 !  [HIGH] detect_repeat()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_jit_compile.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 28 (>20)
      - nesting 5 (>4)
 !  [HIGH] set_private_data_ptrs()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_jit_compile.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - 156 lines (>150)
      - complexity 54 (>20)
 !  [HIGH] get_framesize()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_jit_compile.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - 162 lines (>150)
      - complexity 94 (>20)
 !  [HIGH] get_recurse_data_length()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_jit_compile.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - 215 lines (>150)
      - complexity 85 (>20)
 !  [HIGH] read_char()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_jit_compile.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - 176 lines (>150)
      - complexity 43 (>20)
 !  [HIGH] fast_forward_first_n_chars()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_jit_compile.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - 185 lines (>150)
      - complexity 53 (>20)
 !  [HIGH] fast_forward_newline()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_jit_compile.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - 151 lines (>150)
      - complexity 27 (>20)
 !  [HIGH] compile_simple_assertion_matchingpath()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_jit_compile.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - 255 lines (>150)
      - complexity 39 (>20)
 !  [HIGH] compile_ref_iterator_matchingpath()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_jit_compile.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - 181 lines (>150)
      - complexity 37 (>20)
 !  [HIGH] compile_bracketpos_matchingpath()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_jit_compile.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - 246 lines (>150)
      - complexity 55 (>20)
 !  [HIGH] compile_matchingpath()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_jit_compile.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - 288 lines (>150)
      - complexity 179 (>20)
 !  [HIGH] compile_backtrackingpath()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_jit_compile.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - 174 lines (>150)
      - complexity 121 (>20)
 !  [HIGH] get_chr_property_list()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/pcre2_auto_possess.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - 156 lines (>150)
      - complexity 66 (>20)
 !  [HIGH] sljit_generate_code()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeARM_64.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 34 (>20)
      - nesting 5 (>4)
 !  [HIGH] emit_op_imm()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeARM_64.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - 254 lines (>150)
      - complexity 111 (>20)
 !  [HIGH] emit_op_mem()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeARM_64.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 23 (>20)
      - 6 params (>5)
 !  [HIGH] sljit_emit_op1()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeARM_64.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 30 (>20)
      - 6 params (>5)
 !  [HIGH] sljit_emit_shift_into()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeARM_64.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 26 (>20)
      - 7 params (>5)
 !  [HIGH] sljit_generate_code()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeX86_common.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 29 (>20)
      - nesting 6 (>4)
 !  [HIGH] emit_mov_byte()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeX86_common.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 37 (>20)
      - 6 params (>5)
 !  [HIGH] emit_clz_ctz()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeX86_common.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 22 (>20)
      - 6 params (>5)
 !  [HIGH] sljit_emit_op1()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeX86_common.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 64 (>20)
      - 6 params (>5)
 !  [HIGH] emit_mul()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeX86_common.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 28 (>20)
      - 7 params (>5)
 !  [HIGH] emit_shift()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeX86_common.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 29 (>20)
      - 8 params (>5)
 !  [HIGH] sljit_emit_op2()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeX86_common.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 29 (>20)
      - 8 params (>5)
 !  [HIGH] sljit_emit_shift_into()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeX86_common.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 36 (>20)
      - 7 params (>5)
 !  [HIGH] sljit_generate_code()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeSPARC_common.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 34 (>20)
      - nesting 5 (>4)
 !  [HIGH] getput_arg()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeSPARC_common.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 26 (>20)
      - 7 params (>5)
 !  [HIGH] sljit_emit_op1()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativePPC_common.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 39 (>20)
      - 6 params (>5)
 !  [HIGH] sljit_emit_shift_into()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativePPC_common.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 34 (>20)
      - 7 params (>5)
 !  [HIGH] sljit_generate_code()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeRISCV_common.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 36 (>20)
      - nesting 5 (>4)
 !  [HIGH] sljit_emit_enter()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeRISCV_common.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 22 (>20)
      - 8 params (>5)
 !  [HIGH] sljit_emit_op1()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeRISCV_common.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 29 (>20)
      - 6 params (>5)
 !  [HIGH] sljit_emit_op2()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeRISCV_common.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 27 (>20)
      - 8 params (>5)
 !  [HIGH] sljit_emit_fop1_cmp()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeRISCV_common.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 21 (>20)
      - 6 params (>5)
 !  [HIGH] sljit_emit_cmp()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeRISCV_common.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 22 (>20)
      - 6 params (>5)
 !  [HIGH] inline_set_jump_addr()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeARM_32.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 22 (>20)
      - nesting 5 (>4)
 !  [HIGH] sljit_emit_enter()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeARM_32.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 30 (>20)
      - 8 params (>5)
 !  [HIGH] emit_stack_frame_release()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeARM_32.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 41 (>20)
      - nesting 5 (>4)
 !  [HIGH] generate_int()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeARM_32.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 21 (>20)
      - nesting 5 (>4)
 !  [HIGH] emit_op_mem()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeARM_32.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 29 (>20)
      - 6 params (>5)
 !  [HIGH] softfloat_call_with_args()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeARM_32.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 26 (>20)
      - nesting 6 (>4)
 !  [HIGH] sljit_generate_code()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeMIPS_common.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 29 (>20)
      - nesting 5 (>4)
 !  [HIGH] sljit_emit_op1()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeMIPS_common.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 29 (>20)
      - 6 params (>5)
 !  [HIGH] sljit_emit_op2()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeMIPS_common.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 28 (>20)
      - 8 params (>5)
 !  [HIGH] sljit_emit_fop1_cmp()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeMIPS_common.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 22 (>20)
      - 6 params (>5)
 !  [HIGH] sljit_emit_cmp()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeMIPS_common.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 49 (>20)
      - 6 params (>5)
 !  [HIGH] emit_commutative()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeS390X.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 33 (>20)
      - 7 params (>5)
 !  [HIGH] sljit_emit_add()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeS390X.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 27 (>20)
      - 8 params (>5)
 !  [HIGH] sljit_emit_bitwise()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeS390X.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 23 (>20)
      - 7 params (>5)
 !  [HIGH] sljit_emit_shift()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeS390X.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 29 (>20)
      - 7 params (>5)
 !  [HIGH] sljit_emit_op2()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeS390X.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 23 (>20)
      - 8 params (>5)
 !  [HIGH] sljit_emit_shift_into()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeS390X.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 31 (>20)
      - 7 params (>5)
 !  [HIGH] sljit_emit_fop2()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeS390X.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - nesting 5 (>4)
      - 8 params (>5)
 !  [HIGH] check_sljit_emit_op2()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitLir.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 37 (>20)
      - 9 params (>5)
 !  [HIGH] sljit_generate_code()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeARM_T2_32.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 21 (>20)
      - nesting 5 (>4)
 !  [HIGH] emit_op_mem()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeARM_T2_32.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 38 (>20)
      - 6 params (>5)
 !  [HIGH] emit_stack_frame_release()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeARM_T2_32.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 46 (>20)
      - nesting 5 (>4)
 !  [HIGH] sljit_emit_op1()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeARM_T2_32.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 25 (>20)
      - 6 params (>5)
 !  [HIGH] sljit_emit_shift_into()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeARM_T2_32.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 23 (>20)
      - 7 params (>5)
 !  [HIGH] softfloat_call_with_args()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeARM_T2_32.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 26 (>20)
      - nesting 6 (>4)
 !  [HIGH] emit_single_op()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeSPARC_32.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 33 (>20)
      - 6 params (>5)
 !  [HIGH] call_with_args()
    File: php-wasm-project/php-src/ext/pcre/pcre2lib/sljit/sljitNativeSPARC_32.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 26 (>20)
      - nesting 5 (>4)
 !  [HIGH] sxe_prop_dim_exists()
    File: php-wasm-project/php-src/ext/simplexml/simplexml.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 43 (>20)
      - nesting 7 (>4)
 !  [HIGH] sxe_prop_dim_delete()
    File: php-wasm-project/php-src/ext/simplexml/simplexml.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 33 (>20)
      - nesting 7 (>4)
 !  [HIGH] php_random_bytes()
    File: php-wasm-project/php-src/ext/random/csprng.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 41 (>20)
      - nesting 6 (>4)
 !  [HIGH] php_com_process_typeinfo()
    File: php-wasm-project/php-src/ext/com_dotnet/com_typeinfo.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 26 (>20)
      - nesting 8 (>4)
 !  [HIGH] php_com_invoke_helper()
    File: php-wasm-project/php-src/ext/com_dotnet/com_com.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - nesting 5 (>4)
      - 7 params (>5)
 !  [HIGH] disp_invokeex()
    File: php-wasm-project/php-src/ext/com_dotnet/com_wrapper.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - nesting 7 (>4)
      - 8 params (>5)
 !  [HIGH] php_openssl_make_REQ()
    File: php-wasm-project/php-src/ext/openssl/openssl.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 31 (>20)
      - nesting 6 (>4)
 !  [HIGH] PHP_FUNCTION()
    File: php-wasm-project/php-src/ext/openssl/openssl.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - 157 lines (>150)
      - complexity 35 (>20)
 !  [HIGH] _bc_do_compare()
    File: php-wasm-project/php-src/ext/bcmath/libbcmath/src/compare.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 31 (>20)
      - nesting 6 (>4)
 !  [HIGH] bc_divide()
    File: php-wasm-project/php-src/ext/bcmath/libbcmath/src/div.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 27 (>20)
      - nesting 6 (>4)
 !  [HIGH] pdo_dblib_stmt_get_col()
    File: php-wasm-project/php-src/ext/pdo_dblib/dblib_stmt.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 32 (>20)
      - nesting 6 (>4)
 !  [HIGH] pdo_dblib_handle_factory()
    File: php-wasm-project/php-src/ext/pdo_dblib/dblib_driver.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 28 (>20)
      - nesting 5 (>4)
 !  [HIGH] getToken()
    File: php-wasm-project/php-src/ext/pdo_firebird/firebird_driver.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 43 (>20)
      - nesting 5 (>4)
 !  [HIGH] preprocess()
    File: php-wasm-project/php-src/ext/pdo_firebird/firebird_driver.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 37 (>20)
      - nesting 5 (>4)
 !  [HIGH] firebird_stmt_get_col()
    File: php-wasm-project/php-src/ext/pdo_firebird/firebird_statement.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 30 (>20)
      - nesting 6 (>4)
 !  [HIGH] init_candidate_array()
    File: php-wasm-project/php-src/ext/mbstring/mbstring.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - nesting 6 (>4)
      - 8 params (>5)
 !  [HIGH] mb_guess_encoding_for_strings()
    File: php-wasm-project/php-src/ext/mbstring/mbstring.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - nesting 5 (>4)
      - 7 params (>5)
 !  [HIGH] html_numeric_entity_decode()
    File: php-wasm-project/php-src/ext/mbstring/mbstring.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 43 (>20)
      - nesting 7 (>4)
 !  [HIGH] _php_mbstr_parse_mail_headers()
    File: php-wasm-project/php-src/ext/mbstring/mbstring.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 36 (>20)
      - nesting 6 (>4)
 !  [HIGH] mime_header_decode_encoded_word()
    File: php-wasm-project/php-src/ext/mbstring/mbstring.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 39 (>20)
      - nesting 6 (>4)
 !  [HIGH] mb_mime_header_decode()
    File: php-wasm-project/php-src/ext/mbstring/mbstring.c
    Language: C
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 33 (>20)
      - nesting 7 (>4)
============================================================
```
