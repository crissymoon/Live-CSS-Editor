# Code Review Report — Code Smells

**Directory:** `/Users/mac/Documents/spreadsheet_tool/spreadsheet_tool`  
**Generated:** 2026-03-09 23:41:37  

---

```
Code Smells scan: /Users/mac/Documents/spreadsheet_tool/spreadsheet_tool
------------------------------------------------------------
[HIGH] index.php:55  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] index.php:56  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] index.php:57  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] index.php:60  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] index.php:66  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] index.php:82  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] index.php:83  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] index.php:84  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] index.php:85  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] index.php:86  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] index.php:87  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] index.php:88  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] index.php:89  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] index.php:90  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] index.php:91  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] index.php:106  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] index.php:165  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[LOW] index.php:82  [MAGIC_NUMBER]  Bare numeric literal 11
[LOW] index.php:83  [MAGIC_NUMBER]  Bare numeric literal 12
[LOW] index.php:84  [MAGIC_NUMBER]  Bare numeric literal 13
[LOW] index.php:85  [MAGIC_NUMBER]  Bare numeric literal 14
[LOW] index.php:86  [MAGIC_NUMBER]  Bare numeric literal 16
[LOW] index.php:87  [MAGIC_NUMBER]  Bare numeric literal 18
[LOW] index.php:88  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] index.php:89  [MAGIC_NUMBER]  Bare numeric literal 24
[LOW] index.php:90  [MAGIC_NUMBER]  Bare numeric literal 28
[LOW] index.php:91  [MAGIC_NUMBER]  Bare numeric literal 32
[LOW] index.php:164  [MAGIC_NUMBER]  Bare numeric literal 10
[HIGH] js/grid_events.js:3  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/grid_pointer.js:4  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[LOW] js/grid_pointer.js:39  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] js/grid_pointer.js:40  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] js/grid_pointer.js:74  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] js/grid_pointer.js:75  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] js/grid_pointer.js:84  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] js/grid_pointer.js:92  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] js/grid_pointer.js:93  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] js/grid_pointer.js:122  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] js/grid_pointer.js:142  [MAGIC_NUMBER]  Bare numeric literal 30
[LOW] js/grid_pointer.js:169  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] js/grid_pointer.js:189  [MAGIC_NUMBER]  Bare numeric literal 16
[HIGH] js/grid.js:225  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/grid.js:226  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] js/grid.js:227  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] js/grid.js:228  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/grid.js:229  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/grid.js:231  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/grid.js:329  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/grid.js:330  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] js/grid.js:331  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/grid.js:385  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/grid.js:386  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/grid.js:387  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/grid.js:388  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/grid.js:476  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/grid.js:477  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/grid.js:484  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/grid.js:485  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/grid.js:601  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/grid.js:602  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] js/grid.js:603  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] js/grid.js:604  [DEEP_NESTING]  Nesting depth ~8 (max 4)
[HIGH] js/grid.js:605  [DEEP_NESTING]  Nesting depth ~8 (max 4)
[HIGH] js/grid.js:606  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] js/grid.js:607  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] js/grid.js:608  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/grid.js:609  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] js/grid.js:610  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/grid.js:614  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] js/grid.js:12  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] js/grid.js:13  [MAGIC_NUMBER]  Bare numeric literal 26
[LOW] js/grid.js:30  [MAGIC_NUMBER]  Bare numeric literal 24
[LOW] js/grid.js:31  [MAGIC_NUMBER]  Bare numeric literal 42
[LOW] js/grid.js:32  [MAGIC_NUMBER]  Bare numeric literal 10000
[LOW] js/grid.js:33  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] js/grid.js:99  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] js/grid.js:100  [MAGIC_NUMBER]  Bare numeric literal 26
[LOW] js/grid.js:118  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] js/grid.js:119  [MAGIC_NUMBER]  Bare numeric literal 26
[LOW] js/grid.js:491  [MAGIC_NUMBER]  Bare numeric literal 600
[HIGH] js/grid.js:110  [EMPTY_CATCH]  Empty catch block (swallowed exception)
[MEDIUM] js/grid.js:561  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 378–383
[HIGH] js/formulas.js:123  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:183  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:184  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:206  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:207  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:219  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:220  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:309  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:310  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:330  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:331  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:336  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:337  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:338  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] js/formulas.js:339  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:407  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:408  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:409  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:442  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:443  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:444  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:464  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:465  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:466  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:467  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] js/formulas.js:468  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] js/formulas.js:469  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] js/formulas.js:470  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] js/formulas.js:471  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:492  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:493  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:494  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/formulas.js:495  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] js/formulas.js:16  [MAGIC_NUMBER]  Bare numeric literal 64
[LOW] js/formulas.js:25  [MAGIC_NUMBER]  Bare numeric literal 26
[LOW] js/formulas.js:34  [MAGIC_NUMBER]  Bare numeric literal 26
[LOW] js/formulas.js:35  [MAGIC_NUMBER]  Bare numeric literal 65
[LOW] js/formulas.js:36  [MAGIC_NUMBER]  Bare numeric literal 26
[LOW] js/formulas.js:45  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] js/formulas.js:262  [MAGIC_NUMBER]  Bare numeric literal 10
[MEDIUM] js/formulas.js:443  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 408–413
[MEDIUM] js/formulas.js:457  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 435–440
[MEDIUM] js/formulas.js:483  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 455–460
[MEDIUM] js/formulas.js:484  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 456–461
[MEDIUM] js/formulas.js:485  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 435–440
[MEDIUM] js/formulas.js:492  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 467–472
[MEDIUM] js/formulas.js:496  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 472–477
[MEDIUM] js/formulas.js:497  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 473–478
[MEDIUM] js/formulas.js:498  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 474–479
[MEDIUM] js/formulas.js:499  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 475–480
[HIGH] js/grid_keyboard.js:86  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/grid_keyboard.js:87  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/grid_keyboard.js:93  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/grid_keyboard.js:94  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] js/grid_keyboard.js:76  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] js/grid_keyboard.js:81  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] js/app_core.js:62  [MAGIC_NUMBER]  Bare numeric literal 2600
[LOW] js/app_core.js:218  [MAGIC_NUMBER]  Bare numeric literal 800
[LOW] js/app_core.js:334  [MAGIC_NUMBER]  Bare numeric literal 86400000
[HIGH] js/app_core.js:260  [EMPTY_CATCH]  Empty catch block (swallowed exception)
[LOW] js/grid_editor.js:35  [MAGIC_NUMBER]  Bare numeric literal 80
[LOW] js/grid_editor.js:36  [MAGIC_NUMBER]  Bare numeric literal 22
[LOW] js/grid_editor.js:186  [MAGIC_NUMBER]  Bare numeric literal 80
[HIGH] js/app_toolbar.js:3  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] js/app_toolbar.js:51  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] js/app_modal.js:104  [MAGIC_NUMBER]  Bare numeric literal 150
[HIGH] js/app.js:544  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] js/app.js:551  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] js/app.js:51  [MAGIC_NUMBER]  Bare numeric literal 2600
[LOW] js/app.js:322  [MAGIC_NUMBER]  Bare numeric literal 800
[LOW] js/app.js:461  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] js/app.js:562  [MAGIC_NUMBER]  Bare numeric literal 150
[LOW] js/app.js:709  [MAGIC_NUMBER]  Bare numeric literal 86400000
[HIGH] js/app.js:366  [EMPTY_CATCH]  Empty catch block (swallowed exception)
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
[MEDIUM] api/index.php:190  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 174–179
[MEDIUM] api/index.php:191  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 175–180
[MEDIUM] api/index.php:192  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 176–181
[MEDIUM] api/index.php:193  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 177–182
[MEDIUM] api/index.php:194  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 178–183
[MEDIUM] api/index.php:348  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 88–93
[MEDIUM] api/index.php:349  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 89–94
[MEDIUM] api/index.php:350  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 90–95
[MEDIUM] api/index.php:351  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 91–96
[HIGH] api/db.php:50  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] api/db.php:62  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] api/db.php:16  [MAGIC_NUMBER]  Bare numeric literal 755
------------------------------------------------------------
Total findings: 272
  HIGH: 192
  MEDIUM: 20
  LOW: 60
By type:
  DEEP_NESTING: 189
  MAGIC_NUMBER: 60
  DUPLICATE_CODE: 20
  EMPTY_CATCH: 3
```
