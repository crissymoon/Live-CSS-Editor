# Code Review Report - Code Smells

**Directory:** `/Users/mac/Documents/spreadsheet_tool/my_project`  
**Generated:** 2026-03-10 16:15:51  

---

```
Code Smells scan: /Users/mac/Documents/spreadsheet_tool/my_project
------------------------------------------------------------
[LOW] project-config.js:7  [MAGIC_NUMBER]  Bare numeric literal 8082
[HIGH] index.php:55  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] index.php:56  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] index.php:57  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] index.php:61  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] index.php:62  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] index.php:63  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] index.php:64  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] index.php:65  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] index.php:66  [DEEP_NESTING]  Nesting depth ~8 (max 4)
[HIGH] index.php:67  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] index.php:68  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] index.php:69  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] index.php:70  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] index.php:71  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] index.php:72  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] index.php:73  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] index.php:74  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] index.php:75  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] index.php:76  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] index.php:77  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] index.php:84  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] index.php:100  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] index.php:101  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] index.php:102  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] index.php:108  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] index.php:61  [MAGIC_NUMBER]  Bare numeric literal 9662
[LOW] index.php:100  [MAGIC_NUMBER]  Bare numeric literal 11
[LOW] index.php:101  [MAGIC_NUMBER]  Bare numeric literal 13
[HIGH] js/grid_events.js:3  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/grid_pointer.js:4  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[LOW] js/grid_pointer.js:13  [MAGIC_NUMBER]  Bare numeric literal 30
[LOW] js/grid_pointer.js:14  [MAGIC_NUMBER]  Bare numeric literal 16
[LOW] js/grid_pointer.js:42  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] js/grid_pointer.js:43  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] js/grid_pointer.js:77  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] js/grid_pointer.js:78  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] js/grid_pointer.js:87  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] js/grid_pointer.js:95  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] js/grid_pointer.js:96  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] js/grid_pointer.js:125  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] js/grid_pointer.js:172  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] js/grid.js:12  [MAGIC_NUMBER]  Bare numeric literal 24
[LOW] js/grid.js:13  [MAGIC_NUMBER]  Bare numeric literal 42
[LOW] js/grid.js:14  [MAGIC_NUMBER]  Bare numeric literal 10000
[LOW] js/grid.js:15  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] js/grid.js:16  [MAGIC_NUMBER]  Bare numeric literal 702
[LOW] js/grid.js:17  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] js/grid.js:18  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] js/grid.js:19  [MAGIC_NUMBER]  Bare numeric literal 26
[LOW] js/grid.js:20  [MAGIC_NUMBER]  Bare numeric literal 600
[HIGH] js/grid.js:115  [EMPTY_CATCH]  Empty catch block (swallowed exception)
[HIGH] js/formulas.js:140  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:141  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:163  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:164  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:176  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:177  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:266  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:267  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:287  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:288  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:293  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:294  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:295  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] js/formulas.js:296  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:435  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:436  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] js/formulas.js:16  [MAGIC_NUMBER]  Bare numeric literal 64
[LOW] js/formulas.js:17  [MAGIC_NUMBER]  Bare numeric literal 26
[LOW] js/formulas.js:18  [MAGIC_NUMBER]  Bare numeric literal 65
[LOW] js/formulas.js:19  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] js/formulas.js:219  [MAGIC_NUMBER]  Bare numeric literal 10
[MEDIUM] js/formulas.js:499  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 486-491
[MEDIUM] js/formulas.js:500  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 487-492
[MEDIUM] js/formulas.js:501  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 488-493
[HIGH] js/grid_keyboard.js:86  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/grid_keyboard.js:87  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] js/grid_keyboard.js:10  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] js/app_core.js:13  [MAGIC_NUMBER]  Bare numeric literal 2600
[LOW] js/app_core.js:15  [MAGIC_NUMBER]  Bare numeric literal 800
[LOW] js/app_core.js:16  [MAGIC_NUMBER]  Bare numeric literal 86400000
[HIGH] js/app_core.js:266  [EMPTY_CATCH]  Empty catch block (swallowed exception)
[LOW] js/grid_editor.js:35  [MAGIC_NUMBER]  Bare numeric literal 80
[LOW] js/grid_editor.js:36  [MAGIC_NUMBER]  Bare numeric literal 22
[LOW] js/grid_editor.js:186  [MAGIC_NUMBER]  Bare numeric literal 80
[HIGH] js/app_toolbar.js:3  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] js/app_toolbar.js:51  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] js/app_modal.js:104  [MAGIC_NUMBER]  Bare numeric literal 150
[HIGH] js/app.js:599  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/app.js:606  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] js/app.js:51  [MAGIC_NUMBER]  Bare numeric literal 2600
[LOW] js/app.js:326  [MAGIC_NUMBER]  Bare numeric literal 800
[LOW] js/app.js:465  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] js/app.js:505  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] js/app.js:621  [MAGIC_NUMBER]  Bare numeric literal 150
[LOW] js/app.js:628  [MAGIC_NUMBER]  Bare numeric literal 220
[LOW] js/app.js:681  [MAGIC_NUMBER]  Bare numeric literal 120
[LOW] js/app.js:790  [MAGIC_NUMBER]  Bare numeric literal 86400000
[HIGH] js/app.js:370  [EMPTY_CATCH]  Empty catch block (swallowed exception)
[HIGH] api/index.php:132  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:133  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] api/index.php:134  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:135  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:136  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:137  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:138  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] api/index.php:139  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:140  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:141  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:146  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:147  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:148  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] api/index.php:149  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:151  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:152  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] api/index.php:153  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] api/index.php:154  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] api/index.php:155  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:156  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:157  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] api/index.php:158  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:160  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:161  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:162  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] api/index.php:163  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] api/index.php:164  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] api/index.php:165  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] api/index.php:166  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] api/index.php:167  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] api/index.php:168  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] api/index.php:169  [DEEP_NESTING]  Nesting depth ~8 (max 4)
[HIGH] api/index.php:170  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] api/index.php:171  [DEEP_NESTING]  Nesting depth ~8 (max 4)
[HIGH] api/index.php:172  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] api/index.php:173  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] api/index.php:174  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] api/index.php:175  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] api/index.php:176  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] api/index.php:177  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] api/index.php:178  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:179  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] api/index.php:180  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] api/index.php:181  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:182  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:187  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:188  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:189  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] api/index.php:190  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] api/index.php:191  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] api/index.php:192  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] api/index.php:193  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] api/index.php:194  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:195  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] api/index.php:196  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] api/index.php:197  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:198  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:212  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:223  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:234  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:235  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:238  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:239  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:242  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:243  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:246  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:247  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:251  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:252  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:253  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] api/index.php:254  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:255  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:256  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] api/index.php:257  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:265  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:269  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:274  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:297  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:298  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:299  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:300  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:301  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:324  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:328  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:329  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:330  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:331  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:333  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:334  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] api/index.php:335  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] api/index.php:336  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:337  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:339  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/index.php:340  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] api/index.php:341  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] api/index.php:342  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] api/index.php:343  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] api/index.php:344  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] api/index.php:345  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] api/index.php:346  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] api/index.php:10  [MAGIC_NUMBER]  Bare numeric literal 204
[LOW] api/index.php:269  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] api/index.php:313  [MAGIC_NUMBER]  Bare numeric literal 422
[LOW] api/index.php:357  [MAGIC_NUMBER]  Bare numeric literal 201
[MEDIUM] api/index.php:190  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 174-179
[MEDIUM] api/index.php:191  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 175-180
[MEDIUM] api/index.php:192  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 176-181
[MEDIUM] api/index.php:193  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 177-182
[MEDIUM] api/index.php:194  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 178-183
[MEDIUM] api/index.php:348  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 88-93
[MEDIUM] api/index.php:349  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 89-94
[MEDIUM] api/index.php:350  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 90-95
[MEDIUM] api/index.php:351  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 91-96
[HIGH] api/db.php:50  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/db.php:62  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] api/db.php:16  [MAGIC_NUMBER]  Bare numeric literal 755
------------------------------------------------------------
Total findings: 216
  HIGH: 153
  MEDIUM: 12
  LOW: 51
By type:
  DEEP_NESTING: 150
  MAGIC_NUMBER: 51
  DUPLICATE_CODE: 12
  EMPTY_CATCH: 3
```
