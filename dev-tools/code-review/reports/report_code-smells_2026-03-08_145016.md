# Code Review Report — Code Smells

**Directory:** `/Users/mac/Desktop/api_keeper`  
**Generated:** 2026-03-08 14:50:16  

---

```
Code Smells scan: /Users/mac/Desktop/api_keeper
------------------------------------------------------------
[LOW] to-webp.py:9  [MAGIC_NUMBER]  Bare numeric literal 85
[LOW] to-webp.py:10  [MAGIC_NUMBER]  Bare numeric literal 90
[LOW] to-webp.py:11  [MAGIC_NUMBER]  Bare numeric literal 85
[HIGH] db-backup.php:167  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] db-backup.php:27  [MAGIC_NUMBER]  Bare numeric literal 30
[LOW] db-backup.php:48  [MAGIC_NUMBER]  Bare numeric literal 3306
[LOW] db-backup.php:84  [MAGIC_NUMBER]  Bare numeric literal 755
[LOW] db-backup.php:226  [MAGIC_NUMBER]  Bare numeric literal 1024
[LOW] db-backup.php:227  [MAGIC_NUMBER]  Bare numeric literal 1048576
[LOW] db-backup.php:228  [MAGIC_NUMBER]  Bare numeric literal 1048576
[MEDIUM] .deploy_staging/config/models.php:15  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 7–12
[MEDIUM] .deploy_staging/config/models.php:23  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 7–12
[MEDIUM] .deploy_staging/config/models.php:31  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 7–12
[MEDIUM] .deploy_staging/config/models.php:50  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 42–47
[MEDIUM] .deploy_staging/config/models.php:58  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 42–47
[HIGH] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:382  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:383  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:384  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:385  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:386  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:387  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:388  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:389  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:390  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:391  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:459  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:463  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:464  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:468  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:469  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:470  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:471  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:472  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:473  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:474  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:475  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:476  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:479  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:480  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:481  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:488  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:489  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:490  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:491  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:492  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:493  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:7  [MAGIC_NUMBER]  Bare numeric literal 32
[LOW] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:48  [MAGIC_NUMBER]  Bare numeric literal 555
[LOW] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:52  [MAGIC_NUMBER]  Bare numeric literal 108
[LOW] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:53  [MAGIC_NUMBER]  Bare numeric literal 108
[LOW] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:197  [MAGIC_NUMBER]  Bare numeric literal 2000
[LOW] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:204  [MAGIC_NUMBER]  Bare numeric literal 2000
[LOW] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:251  [MAGIC_NUMBER]  Bare numeric literal 1000
[LOW] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:270  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:437  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] .deploy_staging/lead-pulls/schedule_a_consult/quote/index.php:441  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] .deploy_staging/lead-pulls/schedule_a_consult/quote/config.php:38  [MAGIC_NUMBER]  Bare numeric literal 3600
[LOW] .deploy_staging/lead-pulls/schedule_a_consult/quote/submit.php:44  [MAGIC_NUMBER]  Bare numeric literal 403
[LOW] .deploy_staging/lead-pulls/schedule_a_consult/quote/submit.php:50  [MAGIC_NUMBER]  Bare numeric literal 32
[LOW] .deploy_staging/lead-pulls/schedule_a_consult/quote/submit.php:71  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/lead-pulls/schedule_a_consult/quote/submit.php:99  [MAGIC_NUMBER]  Bare numeric literal 403
[LOW] .deploy_staging/lead-pulls/schedule_a_consult/quote/submit.php:114  [MAGIC_NUMBER]  Bare numeric literal 755
[LOW] .deploy_staging/lead-pulls/schedule_a_consult/quote/submit.php:133  [MAGIC_NUMBER]  Bare numeric literal 60
[LOW] .deploy_staging/lead-pulls/schedule_a_consult/quote/submit.php:134  [MAGIC_NUMBER]  Bare numeric literal 429
[HIGH] .deploy_staging/schedule_a_consult/quote/index.php:395  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/schedule_a_consult/quote/index.php:396  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/schedule_a_consult/quote/index.php:397  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/schedule_a_consult/quote/index.php:398  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/schedule_a_consult/quote/index.php:399  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/schedule_a_consult/quote/index.php:400  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/schedule_a_consult/quote/index.php:401  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/schedule_a_consult/quote/index.php:402  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/schedule_a_consult/quote/index.php:403  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/schedule_a_consult/quote/index.php:404  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/schedule_a_consult/quote/index.php:437  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/schedule_a_consult/quote/index.php:438  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/schedule_a_consult/quote/index.php:439  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/schedule_a_consult/quote/index.php:478  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/schedule_a_consult/quote/index.php:482  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/schedule_a_consult/quote/index.php:483  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/schedule_a_consult/quote/index.php:487  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/schedule_a_consult/quote/index.php:488  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/schedule_a_consult/quote/index.php:489  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/schedule_a_consult/quote/index.php:490  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/schedule_a_consult/quote/index.php:491  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/schedule_a_consult/quote/index.php:492  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/schedule_a_consult/quote/index.php:493  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/schedule_a_consult/quote/index.php:494  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/schedule_a_consult/quote/index.php:495  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/schedule_a_consult/quote/index.php:498  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/schedule_a_consult/quote/index.php:499  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/schedule_a_consult/quote/index.php:500  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/schedule_a_consult/quote/index.php:507  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/schedule_a_consult/quote/index.php:508  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/schedule_a_consult/quote/index.php:509  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/schedule_a_consult/quote/index.php:510  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/schedule_a_consult/quote/index.php:511  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/schedule_a_consult/quote/index.php:512  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] .deploy_staging/schedule_a_consult/quote/index.php:7  [MAGIC_NUMBER]  Bare numeric literal 32
[LOW] .deploy_staging/schedule_a_consult/quote/index.php:48  [MAGIC_NUMBER]  Bare numeric literal 555
[LOW] .deploy_staging/schedule_a_consult/quote/index.php:52  [MAGIC_NUMBER]  Bare numeric literal 108
[LOW] .deploy_staging/schedule_a_consult/quote/index.php:53  [MAGIC_NUMBER]  Bare numeric literal 108
[LOW] .deploy_staging/schedule_a_consult/quote/index.php:197  [MAGIC_NUMBER]  Bare numeric literal 2000
[LOW] .deploy_staging/schedule_a_consult/quote/index.php:204  [MAGIC_NUMBER]  Bare numeric literal 2000
[LOW] .deploy_staging/schedule_a_consult/quote/index.php:251  [MAGIC_NUMBER]  Bare numeric literal 1000
[LOW] .deploy_staging/schedule_a_consult/quote/index.php:270  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] .deploy_staging/schedule_a_consult/quote/index.php:456  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] .deploy_staging/schedule_a_consult/quote/index.php:460  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] .deploy_staging/schedule_a_consult/quote/config.php:60  [MAGIC_NUMBER]  Bare numeric literal 3600
[LOW] .deploy_staging/schedule_a_consult/quote/submit.php:44  [MAGIC_NUMBER]  Bare numeric literal 403
[LOW] .deploy_staging/schedule_a_consult/quote/submit.php:50  [MAGIC_NUMBER]  Bare numeric literal 32
[LOW] .deploy_staging/schedule_a_consult/quote/submit.php:71  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/schedule_a_consult/quote/submit.php:99  [MAGIC_NUMBER]  Bare numeric literal 403
[LOW] .deploy_staging/schedule_a_consult/quote/submit.php:114  [MAGIC_NUMBER]  Bare numeric literal 755
[LOW] .deploy_staging/schedule_a_consult/quote/submit.php:133  [MAGIC_NUMBER]  Bare numeric literal 60
[LOW] .deploy_staging/schedule_a_consult/quote/submit.php:134  [MAGIC_NUMBER]  Bare numeric literal 429
[HIGH] .deploy_staging/routes/api.php:85  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/routes/api.php:86  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/routes/api.php:87  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] .deploy_staging/routes/api.php:88  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] .deploy_staging/routes/api.php:89  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] .deploy_staging/routes/api.php:90  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/routes/api.php:91  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/routes/api.php:95  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/routes/api.php:96  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/routes/api.php:97  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/routes/api.php:98  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/routes/api.php:99  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/routes/api.php:757  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/routes/api.php:758  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/routes/api.php:759  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/routes/api.php:760  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/routes/api.php:799  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/routes/api.php:800  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/routes/api.php:801  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/routes/api.php:802  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/routes/api.php:816  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/routes/api.php:817  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/routes/api.php:818  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/routes/api.php:825  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/routes/api.php:826  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/routes/api.php:850  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/routes/api.php:1236  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/routes/api.php:1237  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] .deploy_staging/routes/api.php:52  [MAGIC_NUMBER]  Bare numeric literal 502
[LOW] .deploy_staging/routes/api.php:58  [MAGIC_NUMBER]  Bare numeric literal 502
[LOW] .deploy_staging/routes/api.php:131  [MAGIC_NUMBER]  Bare numeric literal 502
[LOW] .deploy_staging/routes/api.php:137  [MAGIC_NUMBER]  Bare numeric literal 502
[LOW] .deploy_staging/routes/api.php:200  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:211  [MAGIC_NUMBER]  Bare numeric literal 201
[LOW] .deploy_staging/routes/api.php:217  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] .deploy_staging/routes/api.php:237  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] .deploy_staging/routes/api.php:255  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] .deploy_staging/routes/api.php:268  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] .deploy_staging/routes/api.php:364  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:368  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:378  [MAGIC_NUMBER]  Bare numeric literal 409
[LOW] .deploy_staging/routes/api.php:383  [MAGIC_NUMBER]  Bare numeric literal 12
[LOW] .deploy_staging/routes/api.php:416  [MAGIC_NUMBER]  Bare numeric literal 300
[LOW] .deploy_staging/routes/api.php:443  [MAGIC_NUMBER]  Bare numeric literal 201
[LOW] .deploy_staging/routes/api.php:457  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:461  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:470  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] .deploy_staging/routes/api.php:489  [MAGIC_NUMBER]  Bare numeric literal 300
[LOW] .deploy_staging/routes/api.php:529  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:538  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:543  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:550  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:560  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:570  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] .deploy_staging/routes/api.php:616  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] .deploy_staging/routes/api.php:618  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] .deploy_staging/routes/api.php:654  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:664  [MAGIC_NUMBER]  Bare numeric literal 300
[LOW] .deploy_staging/routes/api.php:672  [MAGIC_NUMBER]  Bare numeric literal 300
[LOW] .deploy_staging/routes/api.php:702  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] .deploy_staging/routes/api.php:730  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:735  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:742  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:759  [MAGIC_NUMBER]  Bare numeric literal 403
[LOW] .deploy_staging/routes/api.php:860  [MAGIC_NUMBER]  Bare numeric literal 502
[LOW] .deploy_staging/routes/api.php:940  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:947  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:955  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:969  [MAGIC_NUMBER]  Bare numeric literal 502
[LOW] .deploy_staging/routes/api.php:993  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:1010  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:1018  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:1022  [MAGIC_NUMBER]  Bare numeric literal 201
[LOW] .deploy_staging/routes/api.php:1031  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:1110  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] .deploy_staging/routes/api.php:1118  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:1137  [MAGIC_NUMBER]  Bare numeric literal 402
[LOW] .deploy_staging/routes/api.php:1144  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] .deploy_staging/routes/api.php:1163  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] .deploy_staging/routes/api.php:1171  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:1182  [MAGIC_NUMBER]  Bare numeric literal 402
[LOW] .deploy_staging/routes/api.php:1195  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] .deploy_staging/routes/api.php:1202  [MAGIC_NUMBER]  Bare numeric literal 30
[LOW] .deploy_staging/routes/api.php:1205  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:1224  [MAGIC_NUMBER]  Bare numeric literal 201
[LOW] .deploy_staging/routes/api.php:1307  [MAGIC_NUMBER]  Bare numeric literal 300
[LOW] .deploy_staging/routes/api.php:1341  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:1345  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:1353  [MAGIC_NUMBER]  Bare numeric literal 409
[LOW] .deploy_staging/routes/api.php:1357  [MAGIC_NUMBER]  Bare numeric literal 12
[LOW] .deploy_staging/routes/api.php:1386  [MAGIC_NUMBER]  Bare numeric literal 300
[LOW] .deploy_staging/routes/api.php:1388  [MAGIC_NUMBER]  Bare numeric literal 201
[LOW] .deploy_staging/routes/api.php:1406  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:1415  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] .deploy_staging/routes/api.php:1431  [MAGIC_NUMBER]  Bare numeric literal 300
[LOW] .deploy_staging/routes/api.php:1453  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:1469  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:1473  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:1478  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:1487  [MAGIC_NUMBER]  Bare numeric literal 429
[LOW] .deploy_staging/routes/api.php:1495  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] .deploy_staging/routes/api.php:1558  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:1567  [MAGIC_NUMBER]  Bare numeric literal 300
[LOW] .deploy_staging/routes/api.php:1657  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:1661  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:1670  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] .deploy_staging/routes/api.php:1675  [MAGIC_NUMBER]  Bare numeric literal 12
[LOW] .deploy_staging/routes/api.php:1735  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:1744  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:1765  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:1774  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:1795  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:1844  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:1852  [MAGIC_NUMBER]  Bare numeric literal 403
[LOW] .deploy_staging/routes/api.php:1872  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:1888  [MAGIC_NUMBER]  Bare numeric literal 403
[LOW] .deploy_staging/routes/api.php:1905  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:1914  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:1936  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:1940  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:1952  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:1956  [MAGIC_NUMBER]  Bare numeric literal 201
[LOW] .deploy_staging/routes/api.php:1969  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:1989  [MAGIC_NUMBER]  Bare numeric literal 403
[LOW] .deploy_staging/routes/api.php:2000  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:2004  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:2022  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:2026  [MAGIC_NUMBER]  Bare numeric literal 201
[LOW] .deploy_staging/routes/api.php:2037  [MAGIC_NUMBER]  Bare numeric literal 403
[LOW] .deploy_staging/routes/api.php:2045  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:2054  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] .deploy_staging/routes/api.php:2078  [MAGIC_NUMBER]  Bare numeric literal 400
[MEDIUM] .deploy_staging/routes/api.php:125  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 46–51
[MEDIUM] .deploy_staging/routes/api.php:126  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 47–52
[MEDIUM] .deploy_staging/routes/api.php:127  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 48–53
[MEDIUM] .deploy_staging/routes/api.php:128  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 49–54
[MEDIUM] .deploy_staging/routes/api.php:129  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 50–55
[MEDIUM] .deploy_staging/routes/api.php:130  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 51–56
[MEDIUM] .deploy_staging/routes/api.php:131  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 52–57
[MEDIUM] .deploy_staging/routes/api.php:132  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 53–58
[MEDIUM] .deploy_staging/routes/api.php:133  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 54–59
[MEDIUM] .deploy_staging/routes/api.php:134  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 55–60
[MEDIUM] .deploy_staging/routes/api.php:135  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 56–61
[MEDIUM] .deploy_staging/routes/api.php:136  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 57–62
[MEDIUM] .deploy_staging/routes/api.php:235  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 215–220
[MEDIUM] .deploy_staging/routes/api.php:236  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 216–221
[MEDIUM] .deploy_staging/routes/api.php:237  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 217–222
[MEDIUM] .deploy_staging/routes/api.php:253  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 215–220
[MEDIUM] .deploy_staging/routes/api.php:254  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 216–221
[MEDIUM] .deploy_staging/routes/api.php:266  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 215–220
[MEDIUM] .deploy_staging/routes/api.php:484  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 411–416
[MEDIUM] .deploy_staging/routes/api.php:485  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 412–417
[MEDIUM] .deploy_staging/routes/api.php:486  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 413–418
[MEDIUM] .deploy_staging/routes/api.php:487  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 414–419
[MEDIUM] .deploy_staging/routes/api.php:488  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 415–420
[MEDIUM] .deploy_staging/routes/api.php:489  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 416–421
[MEDIUM] .deploy_staging/routes/api.php:490  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 417–422
[MEDIUM] .deploy_staging/routes/api.php:491  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 418–423
[MEDIUM] .deploy_staging/routes/api.php:492  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 419–424
[MEDIUM] .deploy_staging/routes/api.php:493  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 420–425
[MEDIUM] .deploy_staging/routes/api.php:494  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 421–426
[MEDIUM] .deploy_staging/routes/api.php:495  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 422–427
[MEDIUM] .deploy_staging/routes/api.php:496  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 423–428
[MEDIUM] .deploy_staging/routes/api.php:497  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 424–429
[MEDIUM] .deploy_staging/routes/api.php:498  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 425–430
[MEDIUM] .deploy_staging/routes/api.php:499  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 426–431
[MEDIUM] .deploy_staging/routes/api.php:631  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 508–513
[MEDIUM] .deploy_staging/routes/api.php:632  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 509–514
[MEDIUM] .deploy_staging/routes/api.php:633  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 510–515
[MEDIUM] .deploy_staging/routes/api.php:655  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 530–535
[MEDIUM] .deploy_staging/routes/api.php:669  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 413–418
[MEDIUM] .deploy_staging/routes/api.php:670  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 414–419
[MEDIUM] .deploy_staging/routes/api.php:671  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 415–420
[MEDIUM] .deploy_staging/routes/api.php:672  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 416–421
[MEDIUM] .deploy_staging/routes/api.php:673  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 417–422
[MEDIUM] .deploy_staging/routes/api.php:674  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 418–423
[MEDIUM] .deploy_staging/routes/api.php:675  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 419–424
[MEDIUM] .deploy_staging/routes/api.php:676  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 420–425
[MEDIUM] .deploy_staging/routes/api.php:677  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 421–426
[MEDIUM] .deploy_staging/routes/api.php:678  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 422–427
[MEDIUM] .deploy_staging/routes/api.php:679  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 423–428
[MEDIUM] .deploy_staging/routes/api.php:680  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 424–429
[MEDIUM] .deploy_staging/routes/api.php:681  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 425–430
[MEDIUM] .deploy_staging/routes/api.php:682  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 426–431
[MEDIUM] .deploy_staging/routes/api.php:683  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 500–505
[MEDIUM] .deploy_staging/routes/api.php:700  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 215–220
[MEDIUM] .deploy_staging/routes/api.php:1032  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 994–999
[MEDIUM] .deploy_staging/routes/api.php:1108  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 215–220
[MEDIUM] .deploy_staging/routes/api.php:1142  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 215–220
[MEDIUM] .deploy_staging/routes/api.php:1143  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 216–221
[MEDIUM] .deploy_staging/routes/api.php:1161  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 215–220
[MEDIUM] .deploy_staging/routes/api.php:1193  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 215–220
[MEDIUM] .deploy_staging/routes/api.php:1339  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 362–367
[MEDIUM] .deploy_staging/routes/api.php:1340  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 363–368
[MEDIUM] .deploy_staging/routes/api.php:1341  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 364–369
[MEDIUM] .deploy_staging/routes/api.php:1342  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 365–370
[MEDIUM] .deploy_staging/routes/api.php:1343  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 366–371
[MEDIUM] .deploy_staging/routes/api.php:1421  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 477–482
[MEDIUM] .deploy_staging/routes/api.php:1422  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 478–483
[MEDIUM] .deploy_staging/routes/api.php:1423  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 479–484
[MEDIUM] .deploy_staging/routes/api.php:1451  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 527–532
[MEDIUM] .deploy_staging/routes/api.php:1454  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1407–1412
[MEDIUM] .deploy_staging/routes/api.php:1455  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1408–1413
[MEDIUM] .deploy_staging/routes/api.php:1559  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1407–1412
[MEDIUM] .deploy_staging/routes/api.php:1560  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1408–1413
[MEDIUM] .deploy_staging/routes/api.php:1757  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1727–1732
[MEDIUM] .deploy_staging/routes/api.php:1758  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1728–1733
[MEDIUM] .deploy_staging/routes/api.php:1759  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1729–1734
[MEDIUM] .deploy_staging/routes/api.php:1760  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1730–1735
[MEDIUM] .deploy_staging/routes/api.php:1761  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1731–1736
[MEDIUM] .deploy_staging/routes/api.php:1762  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1732–1737
[MEDIUM] .deploy_staging/routes/api.php:1763  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1733–1738
[MEDIUM] .deploy_staging/routes/api.php:1764  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1734–1739
[MEDIUM] .deploy_staging/routes/api.php:1765  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1735–1740
[MEDIUM] .deploy_staging/routes/api.php:1772  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1742–1747
[MEDIUM] .deploy_staging/routes/api.php:1786  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1705–1710
[MEDIUM] .deploy_staging/routes/api.php:1793  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1742–1747
[MEDIUM] .deploy_staging/routes/api.php:1901  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1868–1873
[MEDIUM] .deploy_staging/routes/api.php:1902  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1869–1874
[MEDIUM] .deploy_staging/routes/api.php:1903  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1870–1875
[MEDIUM] .deploy_staging/routes/api.php:1912  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1742–1747
[MEDIUM] .deploy_staging/routes/api.php:1966  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1555–1560
[MEDIUM] .deploy_staging/routes/api.php:1967  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1556–1561
[MEDIUM] .deploy_staging/routes/api.php:1998  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1934–1939
[MEDIUM] .deploy_staging/routes/api.php:1999  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1935–1940
[MEDIUM] .deploy_staging/routes/api.php:2000  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1936–1941
[MEDIUM] .deploy_staging/routes/api.php:2001  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1937–1942
[MEDIUM] .deploy_staging/routes/api.php:2002  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1938–1943
[MEDIUM] .deploy_staging/routes/api.php:2003  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1939–1944
[MEDIUM] .deploy_staging/routes/api.php:2004  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1940–1945
[MEDIUM] .deploy_staging/routes/api.php:2020  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1742–1747
[MEDIUM] .deploy_staging/routes/api.php:2035  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1987–1992
[MEDIUM] .deploy_staging/routes/api.php:2043  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1842–1847
[MEDIUM] .deploy_staging/routes/api.php:2044  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1843–1848
[MEDIUM] .deploy_staging/routes/api.php:2045  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1844–1849
[MEDIUM] .deploy_staging/routes/api.php:2052  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1742–1747
[LOW] .deploy_staging/src/Logger.php:30  [MAGIC_NUMBER]  Bare numeric literal 755
[LOW] .deploy_staging/src/Response.php:55  [MAGIC_NUMBER]  Bare numeric literal 400
[MEDIUM] .deploy_staging/src/Response.php:63  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 49–54
[MEDIUM] .deploy_staging/src/Response.php:80  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 49–54
[LOW] .deploy_staging/src/Bootstrap.php:102  [MAGIC_NUMBER]  Bare numeric literal 755
[HIGH] .deploy_staging/src/Request.php:120  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Request.php:121  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Router.php:120  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Router.php:121  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] .deploy_staging/src/Router.php:54  [MAGIC_NUMBER]  Bare numeric literal 204
[MEDIUM] .deploy_staging/src/Router.php:83  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 75–80
[LOW] .deploy_staging/src/Middleware/SessionMiddleware.php:59  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] .deploy_staging/src/Middleware/SessionMiddleware.php:67  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] .deploy_staging/src/Middleware/AuthMiddleware.php:78  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] .deploy_staging/src/Middleware/AuthMiddleware.php:84  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] .deploy_staging/src/Middleware/AuthMiddleware.php:97  [MAGIC_NUMBER]  Bare numeric literal 429
[LOW] .deploy_staging/src/Middleware/AuthMiddleware.php:107  [MAGIC_NUMBER]  Bare numeric literal 429
[LOW] .deploy_staging/src/Auth/RateLimiter.php:23  [MAGIC_NUMBER]  Bare numeric literal 60
[LOW] .deploy_staging/src/Auth/RateLimiter.php:24  [MAGIC_NUMBER]  Bare numeric literal 1000
[LOW] .deploy_staging/src/Auth/RateLimiter.php:49  [MAGIC_NUMBER]  Bare numeric literal 3600
[LOW] .deploy_staging/src/Auth/RateLimiter.php:55  [MAGIC_NUMBER]  Bare numeric literal 60
[LOW] .deploy_staging/src/Auth/RateLimiter.php:65  [MAGIC_NUMBER]  Bare numeric literal 60
[LOW] .deploy_staging/src/Auth/RateLimiter.php:77  [MAGIC_NUMBER]  Bare numeric literal 3600
[LOW] .deploy_staging/src/Auth/RateLimiter.php:121  [MAGIC_NUMBER]  Bare numeric literal 3600
[LOW] .deploy_staging/src/Auth/RateLimiter.php:125  [MAGIC_NUMBER]  Bare numeric literal 60
[MEDIUM] .deploy_staging/src/Auth/RateLimiter.php:104  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 34–39
[HIGH] .deploy_staging/src/Auth/SessionManager.php:136  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Auth/SessionManager.php:137  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] .deploy_staging/src/Auth/SessionManager.php:32  [MAGIC_NUMBER]  Bare numeric literal 86400
[LOW] .deploy_staging/src/Auth/SessionManager.php:33  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] .deploy_staging/src/Auth/SessionManager.php:47  [MAGIC_NUMBER]  Bare numeric literal 64
[LOW] .deploy_staging/src/Auth/SessionManager.php:49  [MAGIC_NUMBER]  Bare numeric literal 24
[LOW] .deploy_staging/src/Auth/ApiKeyManager.php:25  [MAGIC_NUMBER]  Bare numeric literal 64
[LOW] .deploy_staging/src/Auth/ApiKeyManager.php:40  [MAGIC_NUMBER]  Bare numeric literal 16
[HIGH] .deploy_staging/src/Providers/OpenAIProvider.php:127  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Providers/OpenAIProvider.php:128  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Providers/OpenAIProvider.php:133  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Providers/OpenAIProvider.php:134  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] .deploy_staging/src/Providers/OpenAIProvider.php:65  [MAGIC_NUMBER]  Bare numeric literal 120
[LOW] .deploy_staging/src/Providers/OpenAIProvider.php:137  [MAGIC_NUMBER]  Bare numeric literal 120
[MEDIUM] .deploy_staging/src/Providers/OpenAIProvider.php:98  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 25–30
[MEDIUM] .deploy_staging/src/Providers/OpenAIProvider.php:99  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 26–31
[MEDIUM] .deploy_staging/src/Providers/OpenAIProvider.php:100  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 27–32
[MEDIUM] .deploy_staging/src/Providers/OpenAIProvider.php:101  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 28–33
[MEDIUM] .deploy_staging/src/Providers/OpenAIProvider.php:102  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 29–34
[MEDIUM] .deploy_staging/src/Providers/OpenAIProvider.php:103  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 30–35
[MEDIUM] .deploy_staging/src/Providers/OpenAIProvider.php:104  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 31–36
[MEDIUM] .deploy_staging/src/Providers/OpenAIProvider.php:111  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 37–42
[MEDIUM] .deploy_staging/src/Providers/OpenAIProvider.php:115  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 57–62
[MEDIUM] .deploy_staging/src/Providers/OpenAIProvider.php:116  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 58–63
[MEDIUM] .deploy_staging/src/Providers/OpenAIProvider.php:117  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 59–64
[HIGH] .deploy_staging/src/Providers/AnthropicProvider.php:80  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Providers/AnthropicProvider.php:150  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Providers/AnthropicProvider.php:151  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/src/Providers/AnthropicProvider.php:152  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/src/Providers/AnthropicProvider.php:153  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Providers/AnthropicProvider.php:155  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Providers/AnthropicProvider.php:156  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/src/Providers/AnthropicProvider.php:157  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/src/Providers/AnthropicProvider.php:158  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] .deploy_staging/src/Providers/AnthropicProvider.php:69  [MAGIC_NUMBER]  Bare numeric literal 120
[LOW] .deploy_staging/src/Providers/AnthropicProvider.php:161  [MAGIC_NUMBER]  Bare numeric literal 120
[MEDIUM] .deploy_staging/src/Providers/AnthropicProvider.php:114  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 30–35
[MEDIUM] .deploy_staging/src/Providers/AnthropicProvider.php:115  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 31–36
[MEDIUM] .deploy_staging/src/Providers/AnthropicProvider.php:116  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 32–37
[MEDIUM] .deploy_staging/src/Providers/AnthropicProvider.php:117  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 33–38
[MEDIUM] .deploy_staging/src/Providers/AnthropicProvider.php:118  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 34–39
[MEDIUM] .deploy_staging/src/Providers/AnthropicProvider.php:119  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 35–40
[MEDIUM] .deploy_staging/src/Providers/AnthropicProvider.php:120  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 36–41
[MEDIUM] .deploy_staging/src/Providers/AnthropicProvider.php:121  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 37–42
[MEDIUM] .deploy_staging/src/Providers/AnthropicProvider.php:122  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 38–43
[MEDIUM] .deploy_staging/src/Providers/AnthropicProvider.php:129  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 44–49
[MEDIUM] .deploy_staging/src/Providers/AnthropicProvider.php:130  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 45–50
[MEDIUM] .deploy_staging/src/Providers/AnthropicProvider.php:131  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 46–51
[MEDIUM] .deploy_staging/src/Providers/AnthropicProvider.php:132  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 47–52
[MEDIUM] .deploy_staging/src/Providers/AnthropicProvider.php:133  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 48–53
[MEDIUM] .deploy_staging/src/Providers/AnthropicProvider.php:137  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 60–65
[MEDIUM] .deploy_staging/src/Providers/AnthropicProvider.php:138  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 61–66
[MEDIUM] .deploy_staging/src/Providers/AnthropicProvider.php:139  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 62–67
[MEDIUM] .deploy_staging/src/Providers/AnthropicProvider.php:140  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 63–68
[HIGH] .deploy_staging/src/Providers/BaseProvider.php:84  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Providers/BaseProvider.php:85  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Providers/BaseProvider.php:89  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Providers/BaseProvider.php:90  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Providers/BaseProvider.php:91  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Providers/BaseProvider.php:106  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Providers/BaseProvider.php:107  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Providers/BaseProvider.php:108  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Providers/BaseProvider.php:156  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Providers/BaseProvider.php:157  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Providers/BaseProvider.php:158  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Providers/BaseProvider.php:159  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Providers/BaseProvider.php:160  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/src/Providers/BaseProvider.php:161  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Providers/BaseProvider.php:162  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Providers/BaseProvider.php:163  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/src/Providers/BaseProvider.php:164  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/src/Providers/BaseProvider.php:165  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/src/Providers/BaseProvider.php:166  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] .deploy_staging/src/Providers/BaseProvider.php:167  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/src/Providers/BaseProvider.php:168  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] .deploy_staging/src/Providers/BaseProvider.php:37  [MAGIC_NUMBER]  Bare numeric literal 1000
[LOW] .deploy_staging/src/Providers/BaseProvider.php:38  [MAGIC_NUMBER]  Bare numeric literal 1000
[LOW] .deploy_staging/src/Providers/BaseProvider.php:48  [MAGIC_NUMBER]  Bare numeric literal 120
[LOW] .deploy_staging/src/Providers/BaseProvider.php:84  [MAGIC_NUMBER]  Bare numeric literal 100000
[LOW] .deploy_staging/src/Providers/BaseProvider.php:96  [MAGIC_NUMBER]  Bare numeric literal 100000
[LOW] .deploy_staging/src/Providers/BaseProvider.php:104  [MAGIC_NUMBER]  Bare numeric literal 300
[LOW] .deploy_staging/src/Providers/BaseProvider.php:133  [MAGIC_NUMBER]  Bare numeric literal 120
[MEDIUM] .deploy_staging/src/Providers/BaseProvider.php:135  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 50–55
[MEDIUM] .deploy_staging/src/Providers/BaseProvider.php:136  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 51–56
[HIGH] .deploy_staging/src/Providers/ProviderFactory.php:89  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Providers/ProviderFactory.php:90  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Providers/DeepseekProvider.php:131  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Providers/DeepseekProvider.php:132  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Providers/DeepseekProvider.php:137  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Providers/DeepseekProvider.php:138  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Providers/DeepseekProvider.php:143  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Providers/DeepseekProvider.php:144  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] .deploy_staging/src/Providers/DeepseekProvider.php:62  [MAGIC_NUMBER]  Bare numeric literal 120
[LOW] .deploy_staging/src/Providers/DeepseekProvider.php:147  [MAGIC_NUMBER]  Bare numeric literal 120
[MEDIUM] .deploy_staging/src/Providers/DeepseekProvider.php:106  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 26–31
[MEDIUM] .deploy_staging/src/Providers/DeepseekProvider.php:107  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 27–32
[MEDIUM] .deploy_staging/src/Providers/DeepseekProvider.php:108  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 28–33
[MEDIUM] .deploy_staging/src/Providers/DeepseekProvider.php:115  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 34–39
[MEDIUM] .deploy_staging/src/Providers/DeepseekProvider.php:119  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 54–59
[MEDIUM] .deploy_staging/src/Providers/DeepseekProvider.php:120  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 55–60
[MEDIUM] .deploy_staging/src/Providers/DeepseekProvider.php:121  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 56–61
[LOW] .deploy_staging/src/Storage/DatabaseStorage.php:26  [MAGIC_NUMBER]  Bare numeric literal 3306
[LOW] .deploy_staging/src/Storage/DatabaseStorage.php:149  [MAGIC_NUMBER]  Bare numeric literal 64
[LOW] .deploy_staging/src/Storage/DatabaseStorage.php:150  [MAGIC_NUMBER]  Bare numeric literal 255
[LOW] .deploy_staging/src/Storage/FileStorage.php:21  [MAGIC_NUMBER]  Bare numeric literal 755
[LOW] .deploy_staging/src/Storage/FileStorage.php:29  [MAGIC_NUMBER]  Bare numeric literal 755
[LOW] .deploy_staging/src/Storage/FileStorage.php:115  [MAGIC_NUMBER]  Bare numeric literal 755
[HIGH] .deploy_staging/src/Storage/StorageFactory.php:36  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/RAGService.php:52  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/RAGService.php:53  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/RAGService.php:54  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/RAGService.php:55  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/RAGService.php:56  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/RAGService.php:57  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/RAGService.php:156  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/RAGService.php:157  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/RAGService.php:158  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/RAGService.php:159  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/RAGService.php:244  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/RAGService.php:245  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/RAGService.php:324  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] .deploy_staging/src/Services/RAGService.php:282  [MAGIC_NUMBER]  Bare numeric literal 50
[MEDIUM] .deploy_staging/src/Services/RAGService.php:108  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 66–71
[MEDIUM] .deploy_staging/src/Services/RAGService.php:109  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 67–72
[MEDIUM] .deploy_staging/src/Services/RAGService.php:110  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 68–73
[MEDIUM] .deploy_staging/src/Services/RAGService.php:111  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 69–74
[MEDIUM] .deploy_staging/src/Services/RAGService.php:112  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 70–75
[MEDIUM] .deploy_staging/src/Services/RAGService.php:113  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 71–76
[LOW] .deploy_staging/src/Services/PaymentService.php:208  [MAGIC_NUMBER]  Bare numeric literal 16
[LOW] .deploy_staging/src/Services/PaymentService.php:228  [MAGIC_NUMBER]  Bare numeric literal 22
[LOW] .deploy_staging/src/Services/PaymentService.php:271  [MAGIC_NUMBER]  Bare numeric literal 300
[HIGH] .deploy_staging/src/Services/LicenseService.php:227  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/LicenseService.php:228  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/LicenseService.php:229  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/LicenseService.php:230  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/LicenseService.php:238  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/LicenseService.php:239  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/LicenseService.php:240  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/LicenseService.php:241  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/LicenseService.php:242  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/LicenseService.php:243  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/LicenseService.php:244  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/LicenseService.php:245  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/LicenseService.php:246  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/LicenseService.php:314  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/LicenseService.php:315  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/LicenseService.php:316  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/LicenseService.php:317  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/LicenseService.php:318  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/LicenseService.php:319  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] .deploy_staging/src/Services/LicenseService.php:73  [MAGIC_NUMBER]  Bare numeric literal 86400
[LOW] .deploy_staging/src/Services/LicenseService.php:221  [MAGIC_NUMBER]  Bare numeric literal 86400
[LOW] .deploy_staging/src/Services/LicenseService.php:235  [MAGIC_NUMBER]  Bare numeric literal 86400
[LOW] .deploy_staging/src/Services/LicenseService.php:299  [MAGIC_NUMBER]  Bare numeric literal 86400
[LOW] .deploy_staging/src/Services/LicenseService.php:583  [MAGIC_NUMBER]  Bare numeric literal 365
[LOW] .deploy_staging/src/Services/LicenseService.php:586  [MAGIC_NUMBER]  Bare numeric literal 730
[LOW] .deploy_staging/src/Services/LicenseService.php:602  [MAGIC_NUMBER]  Bare numeric literal 16
[LOW] .deploy_staging/src/Services/LicenseService.php:651  [MAGIC_NUMBER]  Bare numeric literal 300
[MEDIUM] .deploy_staging/src/Services/LicenseService.php:368  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 287–292
[MEDIUM] .deploy_staging/src/Services/LicenseService.php:369  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 288–293
[MEDIUM] .deploy_staging/src/Services/LicenseService.php:476  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 287–292
[MEDIUM] .deploy_staging/src/Services/LicenseService.php:477  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 288–293
[MEDIUM] .deploy_staging/src/Services/LicenseService.php:478  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 370–375
[MEDIUM] .deploy_staging/src/Services/SubscriptionService.php:66  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 42–47
[MEDIUM] .deploy_staging/src/Services/SubscriptionService.php:67  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 43–48
[HIGH] .deploy_staging/src/Services/TemporalService.php:161  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/TemporalService.php:162  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/TemporalService.php:163  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/TemporalService.php:173  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/TemporalService.php:174  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/src/Services/TemporalService.php:175  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/src/Services/TemporalService.php:176  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] .deploy_staging/src/Services/TemporalService.php:177  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] .deploy_staging/src/Services/TemporalService.php:178  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] .deploy_staging/src/Services/TemporalService.php:101  [MAGIC_NUMBER]  Bare numeric literal 60
[LOW] .deploy_staging/src/Services/TemporalService.php:104  [MAGIC_NUMBER]  Bare numeric literal 3600
[LOW] .deploy_staging/src/Services/TemporalService.php:105  [MAGIC_NUMBER]  Bare numeric literal 60
[LOW] .deploy_staging/src/Services/TemporalService.php:108  [MAGIC_NUMBER]  Bare numeric literal 86400
[LOW] .deploy_staging/src/Services/TemporalService.php:109  [MAGIC_NUMBER]  Bare numeric literal 3600
[LOW] .deploy_staging/src/Services/TemporalService.php:112  [MAGIC_NUMBER]  Bare numeric literal 604800
[LOW] .deploy_staging/src/Services/TemporalService.php:113  [MAGIC_NUMBER]  Bare numeric literal 86400
[LOW] .deploy_staging/src/Services/TemporalService.php:144  [MAGIC_NUMBER]  Bare numeric literal 17
[LOW] .deploy_staging/src/Services/EmailService.php:153  [MAGIC_NUMBER]  Bare numeric literal 465
[LOW] .deploy_staging/src/Services/EmailService.php:158  [MAGIC_NUMBER]  Bare numeric literal 15
[MEDIUM] .deploy_staging/src/Services/EmailService.php:84  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 63–68
[LOW] .deploy_staging/src/Services/ConsultService.php:101  [MAGIC_NUMBER]  Bare numeric literal 255
[LOW] .deploy_staging/src/Services/ConsultService.php:102  [MAGIC_NUMBER]  Bare numeric literal 255
[LOW] .deploy_staging/src/Services/ConsultService.php:103  [MAGIC_NUMBER]  Bare numeric literal 255
[LOW] .deploy_staging/src/Services/ConsultService.php:106  [MAGIC_NUMBER]  Bare numeric literal 45
[LOW] .deploy_staging/src/Services/ConsultService.php:197  [MAGIC_NUMBER]  Bare numeric literal 3306
[MEDIUM] config/models.php:15  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 7–12
[MEDIUM] config/models.php:23  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 7–12
[MEDIUM] config/models.php:31  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 7–12
[MEDIUM] config/models.php:50  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 42–47
[MEDIUM] config/models.php:58  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 42–47
[HIGH] lead-pulls/howdymail/public_html/default.php:100  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:105  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:108  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:109  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:118  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:119  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:148  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:149  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:150  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:151  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:152  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:153  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:160  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:161  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:162  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:163  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:165  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:166  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:167  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:168  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:169  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:170  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:171  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:172  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:173  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:176  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:177  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:178  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:179  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:181  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:182  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:183  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:184  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:185  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:186  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:187  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:188  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/howdymail/public_html/default.php:189  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] lead-pulls/howdymail/public_html/default.php:12  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] lead-pulls/howdymail/public_html/default.php:30  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] lead-pulls/howdymail/public_html/default.php:35  [MAGIC_NUMBER]  Bare numeric literal 727586
[LOW] lead-pulls/howdymail/public_html/default.php:46  [MAGIC_NUMBER]  Bare numeric literal 700
[LOW] lead-pulls/howdymail/public_html/default.php:60  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/howdymail/public_html/default.php:75  [MAGIC_NUMBER]  Bare numeric literal 700
[LOW] lead-pulls/howdymail/public_html/default.php:100  [MAGIC_NUMBER]  Bare numeric literal 80
[LOW] lead-pulls/howdymail/public_html/default.php:117  [MAGIC_NUMBER]  Bare numeric literal 150
[LOW] lead-pulls/howdymail/public_html/default.php:118  [MAGIC_NUMBER]  Bare numeric literal 793
[LOW] lead-pulls/howdymail/public_html/default.php:119  [MAGIC_NUMBER]  Bare numeric literal 188531
[LOW] lead-pulls/howdymail/public_html/default.php:128  [MAGIC_NUMBER]  Bare numeric literal 650
[LOW] lead-pulls/howdymail/public_html/default.php:130  [MAGIC_NUMBER]  Bare numeric literal 2000
[LOW] lead-pulls/howdymail/public_html/default.php:131  [MAGIC_NUMBER]  Bare numeric literal 1999
[LOW] lead-pulls/howdymail/public_html/default.php:132  [MAGIC_NUMBER]  Bare numeric literal 171
[LOW] lead-pulls/howdymail/public_html/default.php:133  [MAGIC_NUMBER]  Bare numeric literal 106
[LOW] lead-pulls/howdymail/public_html/default.php:134  [MAGIC_NUMBER]  Bare numeric literal 171
[LOW] lead-pulls/howdymail/public_html/default.php:135  [MAGIC_NUMBER]  Bare numeric literal 145
[LOW] lead-pulls/howdymail/public_html/default.php:136  [MAGIC_NUMBER]  Bare numeric literal 1415
[LOW] lead-pulls/howdymail/public_html/default.php:137  [MAGIC_NUMBER]  Bare numeric literal 7718
[LOW] lead-pulls/howdymail/public_html/default.php:138  [MAGIC_NUMBER]  Bare numeric literal 402
[LOW] lead-pulls/howdymail/public_html/default.php:139  [MAGIC_NUMBER]  Bare numeric literal 3747
[LOW] lead-pulls/howdymail/public_html/default.php:140  [MAGIC_NUMBER]  Bare numeric literal 682
[LOW] lead-pulls/howdymail/public_html/default.php:141  [MAGIC_NUMBER]  Bare numeric literal 311
[LOW] lead-pulls/howdymail/public_html/default.php:142  [MAGIC_NUMBER]  Bare numeric literal 199
[LOW] lead-pulls/howdymail/public_html/default.php:146  [MAGIC_NUMBER]  Bare numeric literal 294456
[LOW] lead-pulls/howdymail/public_html/default.php:161  [MAGIC_NUMBER]  Bare numeric literal 4455931
[LOW] lead-pulls/howdymail/public_html/default.php:166  [MAGIC_NUMBER]  Bare numeric literal 21
[LOW] lead-pulls/howdymail/public_html/default.php:170  [MAGIC_NUMBER]  Bare numeric literal 3333
[LOW] lead-pulls/howdymail/public_html/default.php:177  [MAGIC_NUMBER]  Bare numeric literal 3220304
[LOW] lead-pulls/howdymail/public_html/default.php:182  [MAGIC_NUMBER]  Bare numeric literal 21
[LOW] lead-pulls/howdymail/public_html/default.php:186  [MAGIC_NUMBER]  Bare numeric literal 3333
[MEDIUM] lead-pulls/howdymail/public_html/default.php:182  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 166–171
[MEDIUM] lead-pulls/howdymail/public_html/default.php:183  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 167–172
[MEDIUM] lead-pulls/howdymail/public_html/default.php:184  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 168–173
[MEDIUM] lead-pulls/howdymail/public_html/default.php:185  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 169–174
[HIGH] lead-pulls/howdymail/public_html/app.js:46  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/howdymail/public_html/app.js:47  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/howdymail/public_html/app.js:48  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/howdymail/public_html/app.js:63  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/howdymail/public_html/app.js:64  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/howdymail/public_html/app.js:65  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/howdymail/public_html/app.js:66  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/howdymail/public_html/app.js:67  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/howdymail/public_html/app.js:110  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/howdymail/public_html/app.js:111  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/howdymail/public_html/app.js:112  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/howdymail/public_html/app.js:113  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/howdymail/public_html/app.js:114  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/howdymail/public_html/app.js:115  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/howdymail/public_html/app.js:116  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/howdymail/public_html/app.js:117  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/howdymail/public_html/app.js:118  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/howdymail/public_html/app.js:119  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/howdymail/public_html/app.js:120  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/howdymail/public_html/app.js:121  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/howdymail/public_html/app.js:122  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] lead-pulls/howdymail/public_html/app.js:16  [MAGIC_NUMBER]  Bare numeric literal 5000
[LOW] lead-pulls/howdymail/public_html/app.js:65  [MAGIC_NUMBER]  Bare numeric literal 60
[LOW] lead-pulls/howdymail/public_html/app.js:96  [MAGIC_NUMBER]  Bare numeric literal 2000
[LOW] lead-pulls/howdymail/public_html/app.js:154  [MAGIC_NUMBER]  Bare numeric literal 2000
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:51  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:58  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:62  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:63  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:64  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:65  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:66  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:67  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:68  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:113  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:114  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:115  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:116  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:142  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:151  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:158  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:159  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:160  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:161  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:162  [DEEP_NESTING]  Nesting depth ~8 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:163  [DEEP_NESTING]  Nesting depth ~8 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:164  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:165  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:166  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:167  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:175  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:176  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:177  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:250  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:251  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1027  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1028  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1031  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1032  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1035  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1036  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1039  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1040  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1048  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1049  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1050  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1051  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1052  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1053  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1054  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1055  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1056  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1057  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1058  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1059  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1060  [DEEP_NESTING]  Nesting depth ~8 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1061  [DEEP_NESTING]  Nesting depth ~10 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1062  [DEEP_NESTING]  Nesting depth ~9 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1063  [DEEP_NESTING]  Nesting depth ~8 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1064  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1065  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1066  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1067  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1068  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1069  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1070  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1071  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1072  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1073  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1074  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1075  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1076  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1077  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1078  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1086  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1087  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1088  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1089  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1095  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1096  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1097  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1098  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1099  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1105  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1106  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1107  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1108  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1109  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1110  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1111  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1112  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1113  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1114  [DEEP_NESTING]  Nesting depth ~8 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1115  [DEEP_NESTING]  Nesting depth ~8 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1116  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1117  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1118  [DEEP_NESTING]  Nesting depth ~8 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1119  [DEEP_NESTING]  Nesting depth ~8 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1120  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1121  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1122  [DEEP_NESTING]  Nesting depth ~8 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1123  [DEEP_NESTING]  Nesting depth ~8 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1124  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1125  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1126  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1127  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1128  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1129  [DEEP_NESTING]  Nesting depth ~8 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1130  [DEEP_NESTING]  Nesting depth ~9 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1131  [DEEP_NESTING]  Nesting depth ~10 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1132  [DEEP_NESTING]  Nesting depth ~10 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1133  [DEEP_NESTING]  Nesting depth ~9 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1134  [DEEP_NESTING]  Nesting depth ~8 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1135  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1136  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1137  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1138  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1139  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1140  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1141  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1142  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1143  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1144  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1150  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1151  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1152  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1153  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1154  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1155  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1156  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1157  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1158  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1159  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1160  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1161  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1162  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1163  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1164  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:91  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:105  [MAGIC_NUMBER]  Bare numeric literal 5000
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:142  [MAGIC_NUMBER]  Bare numeric literal 15
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:200  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:333  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:335  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:336  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:364  [MAGIC_NUMBER]  Bare numeric literal 708090
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:377  [MAGIC_NUMBER]  Bare numeric literal 26
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:383  [MAGIC_NUMBER]  Bare numeric literal 333333
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:428  [MAGIC_NUMBER]  Bare numeric literal 26
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:433  [MAGIC_NUMBER]  Bare numeric literal 333333
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:456  [MAGIC_NUMBER]  Bare numeric literal 600
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:463  [MAGIC_NUMBER]  Bare numeric literal 333333
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:465  [MAGIC_NUMBER]  Bare numeric literal 26
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:480  [MAGIC_NUMBER]  Bare numeric literal 600
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:505  [MAGIC_NUMBER]  Bare numeric literal 708090
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:507  [MAGIC_NUMBER]  Bare numeric literal 708090
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:512  [MAGIC_NUMBER]  Bare numeric literal 708090
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:513  [MAGIC_NUMBER]  Bare numeric literal 708090
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:517  [MAGIC_NUMBER]  Bare numeric literal 26
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:521  [MAGIC_NUMBER]  Bare numeric literal 333333
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:536  [MAGIC_NUMBER]  Bare numeric literal 600
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:567  [MAGIC_NUMBER]  Bare numeric literal 333333
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:569  [MAGIC_NUMBER]  Bare numeric literal 42
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:603  [MAGIC_NUMBER]  Bare numeric literal 600
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:624  [MAGIC_NUMBER]  Bare numeric literal 444444
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:711  [MAGIC_NUMBER]  Bare numeric literal 255
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:726  [MAGIC_NUMBER]  Bare numeric literal 600
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1032  [MAGIC_NUMBER]  Bare numeric literal 1024
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1071  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1072  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1073  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:1115  [MAGIC_NUMBER]  Bare numeric literal 1024
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/index.php:227  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 194–199
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/serve.php:3  [MAGIC_NUMBER]  Bare numeric literal 8080
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/config.php:9  [MAGIC_NUMBER]  Bare numeric literal 300
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/config.php:14  [MAGIC_NUMBER]  Bare numeric literal 1024
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/config.php:15  [MAGIC_NUMBER]  Bare numeric literal 1024
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:506  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:507  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:510  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:511  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:514  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:515  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:518  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:519  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:533  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:534  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:535  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:616  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:617  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:644  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:645  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:646  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:647  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:648  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:649  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:650  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:651  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:652  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:653  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:654  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:655  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:656  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:657  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:661  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:662  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:663  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:672  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:673  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:676  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:704  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:705  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:706  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:707  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:708  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:709  [DEEP_NESTING]  Nesting depth ~8 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:710  [DEEP_NESTING]  Nesting depth ~8 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:711  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:712  [DEEP_NESTING]  Nesting depth ~8 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:713  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:714  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:715  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:716  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:717  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:718  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:719  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:720  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:721  [DEEP_NESTING]  Nesting depth ~8 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:722  [DEEP_NESTING]  Nesting depth ~8 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:723  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:724  [DEEP_NESTING]  Nesting depth ~8 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:725  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:726  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:727  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:739  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:740  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:741  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:742  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:743  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:744  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:745  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:746  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:747  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:748  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:749  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:751  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:752  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:753  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:754  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:755  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:756  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:757  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:758  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:759  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:760  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:761  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:762  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:763  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:764  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:765  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:766  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:767  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:777  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:790  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:791  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:793  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:794  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:799  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:800  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:801  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:803  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:804  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:805  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:817  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:818  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:46  [MAGIC_NUMBER]  Bare numeric literal 65
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:47  [MAGIC_NUMBER]  Bare numeric literal 65
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:50  [MAGIC_NUMBER]  Bare numeric literal 65
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:53  [MAGIC_NUMBER]  Bare numeric literal 65
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:54  [MAGIC_NUMBER]  Bare numeric literal 65
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:57  [MAGIC_NUMBER]  Bare numeric literal 65
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:58  [MAGIC_NUMBER]  Bare numeric literal 65
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:59  [MAGIC_NUMBER]  Bare numeric literal 65
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:60  [MAGIC_NUMBER]  Bare numeric literal 65
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:61  [MAGIC_NUMBER]  Bare numeric literal 65
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:62  [MAGIC_NUMBER]  Bare numeric literal 65
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:63  [MAGIC_NUMBER]  Bare numeric literal 65
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:64  [MAGIC_NUMBER]  Bare numeric literal 65
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:65  [MAGIC_NUMBER]  Bare numeric literal 65
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:66  [MAGIC_NUMBER]  Bare numeric literal 65
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:67  [MAGIC_NUMBER]  Bare numeric literal 65
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:68  [MAGIC_NUMBER]  Bare numeric literal 65
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:69  [MAGIC_NUMBER]  Bare numeric literal 65
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:70  [MAGIC_NUMBER]  Bare numeric literal 65
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:71  [MAGIC_NUMBER]  Bare numeric literal 65
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:72  [MAGIC_NUMBER]  Bare numeric literal 65
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:73  [MAGIC_NUMBER]  Bare numeric literal 65
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:74  [MAGIC_NUMBER]  Bare numeric literal 65
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:87  [MAGIC_NUMBER]  Bare numeric literal 708090
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:98  [MAGIC_NUMBER]  Bare numeric literal 26
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:104  [MAGIC_NUMBER]  Bare numeric literal 333333
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:117  [MAGIC_NUMBER]  Bare numeric literal 600
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:159  [MAGIC_NUMBER]  Bare numeric literal 26
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:162  [MAGIC_NUMBER]  Bare numeric literal 333333
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:167  [MAGIC_NUMBER]  Bare numeric literal 42
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:170  [MAGIC_NUMBER]  Bare numeric literal 333333
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:178  [MAGIC_NUMBER]  Bare numeric literal 600
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:187  [MAGIC_NUMBER]  Bare numeric literal 600
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:212  [MAGIC_NUMBER]  Bare numeric literal 708090
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:214  [MAGIC_NUMBER]  Bare numeric literal 708090
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:219  [MAGIC_NUMBER]  Bare numeric literal 708090
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:220  [MAGIC_NUMBER]  Bare numeric literal 708090
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:243  [MAGIC_NUMBER]  Bare numeric literal 333333
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:260  [MAGIC_NUMBER]  Bare numeric literal 333333
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:279  [MAGIC_NUMBER]  Bare numeric literal 42
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:283  [MAGIC_NUMBER]  Bare numeric literal 333333
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:302  [MAGIC_NUMBER]  Bare numeric literal 255
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:318  [MAGIC_NUMBER]  Bare numeric literal 600
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:492  [MAGIC_NUMBER]  Bare numeric literal 600
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:507  [MAGIC_NUMBER]  Bare numeric literal 1024
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:638  [MAGIC_NUMBER]  Bare numeric literal 500000
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:651  [MAGIC_NUMBER]  Bare numeric literal 1000000
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:660  [MAGIC_NUMBER]  Bare numeric literal 1000000
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:677  [MAGIC_NUMBER]  Bare numeric literal 3000
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:749  [MAGIC_NUMBER]  Bare numeric literal 2000
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:767  [MAGIC_NUMBER]  Bare numeric literal 2000
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:778  [MAGIC_NUMBER]  Bare numeric literal 2000
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:717  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 705–710
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:718  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 706–711
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:719  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 707–712
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:720  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 708–713
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/view.php:721  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 709–714
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/operating_systems_development.c:6  [MAGIC_NUMBER]  Bare numeric literal 16
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/operating_systems_development.c:55  [MAGIC_NUMBER]  Bare numeric literal 13
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/operating_systems_development.c:56  [MAGIC_NUMBER]  Bare numeric literal 13
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/operating_systems_development.c:59  [MAGIC_NUMBER]  Bare numeric literal 510
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/operating_systems_development.c:63  [MAGIC_NUMBER]  Bare numeric literal 16
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/operating_systems_development.c:86  [MAGIC_NUMBER]  Bare numeric literal 32
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/operating_systems_development.c:116  [MAGIC_NUMBER]  Bare numeric literal 15
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/operating_systems_development.c:117  [MAGIC_NUMBER]  Bare numeric literal 15
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/operating_systems_development.c:118  [MAGIC_NUMBER]  Bare numeric literal 16
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/operating_systems_development.c:120  [MAGIC_NUMBER]  Bare numeric literal 16
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/operating_systems_development.c:121  [MAGIC_NUMBER]  Bare numeric literal 24
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/operating_systems_development.c:305  [MAGIC_NUMBER]  Bare numeric literal 16
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/operating_systems_development.c:319  [MAGIC_NUMBER]  Bare numeric literal 13
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/operating_systems_development.c:320  [MAGIC_NUMBER]  Bare numeric literal 14
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/operating_systems_development.c:323  [MAGIC_NUMBER]  Bare numeric literal 32
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/operating_systems_development.c:324  [MAGIC_NUMBER]  Bare numeric literal 33
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/operating_systems_development.c:376  [MAGIC_NUMBER]  Bare numeric literal 1000
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/operating_systems_development.c:386  [MAGIC_NUMBER]  Bare numeric literal 256
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/operating_systems_development.c:609  [MAGIC_NUMBER]  Bare numeric literal 256
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:653  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:655  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:656  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:657  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:488  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:489  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:510  [MAGIC_NUMBER]  Bare numeric literal 42
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:565  [MAGIC_NUMBER]  Bare numeric literal 16
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:713  [MAGIC_NUMBER]  Bare numeric literal 128
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:468  [EMPTY_CATCH]  Empty except block (swallowed exception)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:470  [EMPTY_CATCH]  Empty except block (swallowed exception)
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:79  [DEAD_IMPORT]  'struct' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:82  [DEAD_IMPORT]  'codecs' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:93  [DEAD_IMPORT]  'calendar' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:101  [DEAD_IMPORT]  'winsound' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:130  [DEAD_IMPORT]  'headers' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:131  [DEAD_IMPORT]  'simple_server' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:137  [DEAD_IMPORT]  'docutils' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:138  [DEAD_IMPORT]  'token' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:139  [DEAD_IMPORT]  'tokenize' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:140  [DEAD_IMPORT]  'keyword' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:142  [DEAD_IMPORT]  'symbol' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:143  [DEAD_IMPORT]  'ast' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:144  [DEAD_IMPORT]  'dis' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:145  [DEAD_IMPORT]  'code' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:146  [DEAD_IMPORT]  'codeop' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:147  [DEAD_IMPORT]  'py_compile' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:148  [DEAD_IMPORT]  'compileall' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:149  [DEAD_IMPORT]  'zipimport' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:150  [DEAD_IMPORT]  'msvcrt' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:153  [DEAD_IMPORT]  'nis' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:154  [DEAD_IMPORT]  'syslog' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:160  [DEAD_IMPORT]  'fpectl' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:164  [DEAD_IMPORT]  'heappush' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:164  [DEAD_IMPORT]  'heappop' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:164  [DEAD_IMPORT]  'heapify' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:164  [DEAD_IMPORT]  'nlargest' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:164  [DEAD_IMPORT]  'nsmallest' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:165  [DEAD_IMPORT]  'bisect_left' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:165  [DEAD_IMPORT]  'bisect_right' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:165  [DEAD_IMPORT]  'insort_left' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:165  [DEAD_IMPORT]  'insort_right' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:167  [DEAD_IMPORT]  'Queue' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:167  [DEAD_IMPORT]  'LifoQueue' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:167  [DEAD_IMPORT]  'PriorityQueue' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:167  [DEAD_IMPORT]  'SimpleQueue' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:168  [DEAD_IMPORT]  'IntEnum' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:168  [DEAD_IMPORT]  'Flag' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:168  [DEAD_IMPORT]  'IntFlag' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:169  [DEAD_IMPORT]  'fields' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:169  [DEAD_IMPORT]  'asdict' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:169  [DEAD_IMPORT]  'astuple' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:170  [DEAD_IMPORT]  'List' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:170  [DEAD_IMPORT]  'Dict' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:170  [DEAD_IMPORT]  'Set' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:170  [DEAD_IMPORT]  'Tuple' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:170  [DEAD_IMPORT]  'Optional' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:170  [DEAD_IMPORT]  'Union' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:170  [DEAD_IMPORT]  'Any' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:170  [DEAD_IMPORT]  'Callable' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:170  [DEAD_IMPORT]  'TypeVar' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:183  [DEAD_IMPORT]  'dircache' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:188  [DEAD_IMPORT]  'difflib' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:189  [DEAD_IMPORT]  'textwrap' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:198  [DEAD_IMPORT]  'shelve' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:199  [DEAD_IMPORT]  'marshal' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:200  [DEAD_IMPORT]  'dbm' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:204  [DEAD_IMPORT]  'zlib' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:206  [DEAD_IMPORT]  'bz2' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:207  [DEAD_IMPORT]  'lzma' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:247  [DEAD_IMPORT]  'contextvars' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:252  [DEAD_IMPORT]  'select' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:253  [DEAD_IMPORT]  'selectors' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:254  [DEAD_IMPORT]  'asyncore' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:255  [DEAD_IMPORT]  'asynchat' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:256  [DEAD_IMPORT]  'signal' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:262  [DEAD_IMPORT]  'mailcap' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:266  [DEAD_IMPORT]  'binhex' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:268  [DEAD_IMPORT]  'quopri' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:269  [DEAD_IMPORT]  'uu' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:295  [DEAD_IMPORT]  'robotparser' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:299  [DEAD_IMPORT]  'cookies' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:300  [DEAD_IMPORT]  'cookiejar' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:304  [DEAD_IMPORT]  'nntplib' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:306  [DEAD_IMPORT]  'smtpd' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:309  [DEAD_IMPORT]  'socketserver' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:313  [DEAD_IMPORT]  'ipaddress' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:322  [DEAD_IMPORT]  'imghdr' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:323  [DEAD_IMPORT]  'sndhdr' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:324  [DEAD_IMPORT]  'ossaudiodev' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:329  [DEAD_IMPORT]  'idna' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:330  [DEAD_IMPORT]  'utf_8_sig' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:331  [DEAD_IMPORT]  'unicode_escape' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:332  [DEAD_IMPORT]  'raw_unicode_escape' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:341  [DEAD_IMPORT]  'ttk' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:342  [DEAD_IMPORT]  'tix' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:343  [DEAD_IMPORT]  'scrolledtext' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:350  [DEAD_IMPORT]  'mock' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:352  [DEAD_IMPORT]  'support' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:355  [DEAD_IMPORT]  'bdb' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:356  [DEAD_IMPORT]  'faulthandler' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:360  [DEAD_IMPORT]  'pstats' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:362  [DEAD_IMPORT]  'trace' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:363  [DEAD_IMPORT]  'tracemalloc' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:374  [DEAD_IMPORT]  'builtins' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:375  [DEAD_IMPORT]  '__main__' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:380  [DEAD_IMPORT]  'atexit' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:382  [DEAD_IMPORT]  '__future__' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:399  [DEAD_IMPORT]  'machinery' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:438  [DEAD_IMPORT]  'handlers' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:457  [DEAD_IMPORT]  'ExitStack' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:457  [DEAD_IMPORT]  'suppress' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:457  [DEAD_IMPORT]  'redirect_stdout' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:457  [DEAD_IMPORT]  'redirect_stderr' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:460  [DEAD_IMPORT]  'singledispatch' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:460  [DEAD_IMPORT]  'cached_property' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:463  [DEAD_IMPORT]  'TYPE_CHECKING' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:463  [DEAD_IMPORT]  'overload' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:463  [DEAD_IMPORT]  'final' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:463  [DEAD_IMPORT]  'Literal' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:463  [DEAD_IMPORT]  'TypedDict' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:563  [DEAD_IMPORT]  'numpy' imported but never used
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:276  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 119–124
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:277  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 120–125
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:431  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 224–229
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:439  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 230–235
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:440  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 231–236
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:441  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 232–237
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_standard_library_complete.py:442  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 233–238
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/kernel_systems_programming.c:439  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/kernel_systems_programming.c:445  [DEEP_NESTING]  Nesting depth ~8 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/kernel_systems_programming.c:446  [DEEP_NESTING]  Nesting depth ~8 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/kernel_systems_programming.c:486  [DEEP_NESTING]  Nesting depth ~9 (max 4)
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/kernel_systems_programming.c:41  [MAGIC_NUMBER]  Bare numeric literal 256
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/kernel_systems_programming.c:136  [MAGIC_NUMBER]  Bare numeric literal 1024
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/kernel_systems_programming.c:143  [MAGIC_NUMBER]  Bare numeric literal 1024
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/kernel_systems_programming.c:149  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/kernel_systems_programming.c:289  [MAGIC_NUMBER]  Bare numeric literal 64
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/kernel_systems_programming.c:291  [MAGIC_NUMBER]  Bare numeric literal 32
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/kernel_systems_programming.c:335  [MAGIC_NUMBER]  Bare numeric literal 1000
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/kernel_systems_programming.c:375  [MAGIC_NUMBER]  Bare numeric literal 5000
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/kernel_systems_programming.c:384  [MAGIC_NUMBER]  Bare numeric literal 5000
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/kernel_systems_programming.c:403  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/kernel_systems_programming.c:453  [MAGIC_NUMBER]  Bare numeric literal 664
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/kernel_systems_programming.c:617  [MAGIC_NUMBER]  Bare numeric literal 32
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/algorithms.py:27  [MAGIC_NUMBER]  Bare numeric literal 45
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/algorithms.py:28  [MAGIC_NUMBER]  Bare numeric literal 45
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/algorithms.py:11  [DEAD_IMPORT]  'annotations' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:1023  [MAGIC_NUMBER]  Bare numeric literal 10
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:349  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 339–344
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:382  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 369–374
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:383  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 370–375
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:384  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 371–376
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:385  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 372–377
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:407  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 395–400
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:408  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 396–401
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:409  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 397–402
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:514  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 442–447
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:535  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 442–447
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:591  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 442–447
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:757  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 728–733
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:758  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 729–734
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:759  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 442–447
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:780  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 728–733
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:781  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 729–734
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:799  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 728–733
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:800  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 729–734
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:801  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 442–447
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:820  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 728–733
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:821  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 729–734
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:822  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 442–447
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:874  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 747–752
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:875  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 748–753
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:880  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 442–447
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:1006  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 990–995
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:1054  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 706–711
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:1070  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1046–1051
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:1071  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1047–1052
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:1078  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 706–711
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:1099  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 706–711
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:1162  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 706–711
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:1185  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1132–1137
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:1186  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1133–1138
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:1187  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1134–1139
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:1188  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1135–1140
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:1189  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1136–1141
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:1190  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1137–1142
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:1191  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1138–1143
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:1192  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1139–1144
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:1193  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1140–1145
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:1194  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1141–1146
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:1203  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1154–1159
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:1204  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1155–1160
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:1213  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 706–711
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:1401  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1211–1216
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:1402  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1212–1217
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:1440  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1337–1342
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:1441  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1338–1343
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:1454  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1337–1342
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:1455  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1338–1343
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:1516  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 706–711
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:1580  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 728–733
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:1581  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 729–734
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:1584  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 706–711
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js:1611  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1601–1606
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_marketing_metrics_kpis.py:474  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_marketing_metrics_kpis.py:475  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_marketing_metrics_kpis.py:476  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_marketing_metrics_kpis.py:477  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_marketing_metrics_kpis.py:478  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_marketing_metrics_kpis.py:479  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_marketing_metrics_kpis.py:480  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_marketing_metrics_kpis.py:481  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_marketing_metrics_kpis.py:489  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_marketing_metrics_kpis.py:494  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_marketing_metrics_kpis.py:52  [MAGIC_NUMBER]  Bare numeric literal 1000
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_marketing_metrics_kpis.py:308  [MAGIC_NUMBER]  Bare numeric literal 12
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_marketing_metrics_kpis.py:344  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_marketing_metrics_kpis.py:562  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_marketing_metrics_kpis.py:566  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_marketing_metrics_kpis.py:592  [MAGIC_NUMBER]  Bare numeric literal 11
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_marketing_metrics_kpis.py:595  [MAGIC_NUMBER]  Bare numeric literal 42
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_marketing_metrics_kpis.py:604  [MAGIC_NUMBER]  Bare numeric literal 42
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_marketing_metrics_kpis.py:677  [MAGIC_NUMBER]  Bare numeric literal 60
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_marketing_metrics_kpis.py:773  [MAGIC_NUMBER]  Bare numeric literal 12
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_marketing_metrics_kpis.py:778  [MAGIC_NUMBER]  Bare numeric literal 45
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_marketing_metrics_kpis.py:792  [MAGIC_NUMBER]  Bare numeric literal 15
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_marketing_metrics_kpis.py:840  [MAGIC_NUMBER]  Bare numeric literal 50000
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_marketing_metrics_kpis.py:841  [MAGIC_NUMBER]  Bare numeric literal 200000
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_marketing_metrics_kpis.py:842  [MAGIC_NUMBER]  Bare numeric literal 24
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_marketing_metrics_kpis.py:4  [DEAD_IMPORT]  'pandas' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_marketing_metrics_kpis.py:5  [DEAD_IMPORT]  'numpy' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_marketing_metrics_kpis.py:6  [DEAD_IMPORT]  'pyplot' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_marketing_metrics_kpis.py:7  [DEAD_IMPORT]  'seaborn' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_marketing_metrics_kpis.py:8  [DEAD_IMPORT]  'timedelta' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/typescript_modern_web_development.ts:92  [MAGIC_NUMBER]  Bare numeric literal 300000
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/typescript_modern_web_development.ts:260  [MAGIC_NUMBER]  Bare numeric literal 30000
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/typescript_modern_web_development.ts:331  [MAGIC_NUMBER]  Bare numeric literal 30000
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/typescript_modern_web_development.ts:669  [MAGIC_NUMBER]  Bare numeric literal 20
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/typescript_modern_web_development.ts:314  [EMPTY_CATCH]  Empty catch block (swallowed exception)
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/typescript_modern_web_development.ts:937  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 910–915
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/typescript_modern_web_development.ts:938  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 911–916
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_basic_loops.py:4  [MAGIC_NUMBER]  Bare numeric literal 11
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_basic_loops.py:8  [MAGIC_NUMBER]  Bare numeric literal 51
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_basic_loops.py:16  [MAGIC_NUMBER]  Bare numeric literal 11
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_basic_loops.py:20  [MAGIC_NUMBER]  Bare numeric literal 16
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_basic_loops.py:24  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_basic_loops.py:29  [MAGIC_NUMBER]  Bare numeric literal 101
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_basic_loops.py:34  [MAGIC_NUMBER]  Bare numeric literal 21
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_basic_loops.py:38  [MAGIC_NUMBER]  Bare numeric literal 13
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_patterns.py:4  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_patterns.py:5  [MAGIC_NUMBER]  Bare numeric literal 11
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_patterns.py:6  [MAGIC_NUMBER]  Bare numeric literal 51
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_patterns.py:7  [MAGIC_NUMBER]  Bare numeric literal 95
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_patterns.py:10  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_patterns.py:13  [MAGIC_NUMBER]  Bare numeric literal 51
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_patterns.py:16  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_patterns.py:20  [MAGIC_NUMBER]  Bare numeric literal 51
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_patterns.py:21  [MAGIC_NUMBER]  Bare numeric literal 11
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_patterns.py:22  [MAGIC_NUMBER]  Bare numeric literal 21
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_patterns.py:35  [MAGIC_NUMBER]  Bare numeric literal 101
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_patterns.py:42  [MAGIC_NUMBER]  Bare numeric literal 51
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/php_programming_patterns.php:91  [MAGIC_NUMBER]  Bare numeric literal 25
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/php_programming_patterns.php:92  [MAGIC_NUMBER]  Bare numeric literal 30
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/php_programming_patterns.php:93  [MAGIC_NUMBER]  Bare numeric literal 35
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/php_programming_patterns.php:98  [MAGIC_NUMBER]  Bare numeric literal 18
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/machine_learning_algorithms.py:101  [LONG_PARAM_LIST]  6 parameters (max 5)
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/machine_learning_algorithms.py:20  [MAGIC_NUMBER]  Bare numeric literal 1000
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/machine_learning_algorithms.py:1  [DEAD_IMPORT]  'numpy' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/machine_learning_algorithms.py:2  [DEAD_IMPORT]  'pandas' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/machine_learning_algorithms.py:4  [DEAD_IMPORT]  'RandomForestClassifier' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/machine_learning_algorithms.py:5  [DEAD_IMPORT]  'train_test_split' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/machine_learning_algorithms.py:6  [DEAD_IMPORT]  'mean_squared_error' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/cryptography.py:35  [MAGIC_NUMBER]  Bare numeric literal 128
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/cryptography.py:37  [MAGIC_NUMBER]  Bare numeric literal 128
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/cryptography.py:49  [MAGIC_NUMBER]  Bare numeric literal 128
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/cryptography.py:50  [MAGIC_NUMBER]  Bare numeric literal 128
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/cryptography.py:51  [MAGIC_NUMBER]  Bare numeric literal 128
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/cryptography.py:59  [MAGIC_NUMBER]  Bare numeric literal 128
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/cryptography.py:60  [MAGIC_NUMBER]  Bare numeric literal 256
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/cryptography.py:61  [MAGIC_NUMBER]  Bare numeric literal 256
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/cryptography.py:69  [MAGIC_NUMBER]  Bare numeric literal 128
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/cryptography.py:70  [MAGIC_NUMBER]  Bare numeric literal 128
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/cryptography.py:86  [MAGIC_NUMBER]  Bare numeric literal 48
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/cryptography.py:98  [MAGIC_NUMBER]  Bare numeric literal 48
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/cryptography.py:106  [MAGIC_NUMBER]  Bare numeric literal 256
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/cryptography.py:112  [MAGIC_NUMBER]  Bare numeric literal 16
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/cryptography.py:113  [MAGIC_NUMBER]  Bare numeric literal 128
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/cryptography.py:128  [MAGIC_NUMBER]  Bare numeric literal 128
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/cryptography.py:129  [MAGIC_NUMBER]  Bare numeric literal 128
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/cryptography.py:5  [DEAD_IMPORT]  'annotations' imported but never used
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/cryptography.py:71  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 38–43
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/cryptography.py:72  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 39–44
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/cryptography.py:73  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 40–45
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/cryptography.py:74  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 41–46
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/cryptography.py:120  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 41–46
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:127  [DEEP_NESTING]  Nesting depth ~9 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:142  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:151  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:152  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:109  [LONG_METHOD]  Method body ~71 lines (max 60)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:399  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:639  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:640  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1059  [LONG_METHOD]  Method body ~61 lines (max 60)
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:100  [MAGIC_NUMBER]  Bare numeric literal 25
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:102  [MAGIC_NUMBER]  Bare numeric literal 25
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:103  [MAGIC_NUMBER]  Bare numeric literal 25
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:207  [MAGIC_NUMBER]  Bare numeric literal 11
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:209  [MAGIC_NUMBER]  Bare numeric literal 11
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:269  [MAGIC_NUMBER]  Bare numeric literal 11
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:270  [MAGIC_NUMBER]  Bare numeric literal 11
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:271  [MAGIC_NUMBER]  Bare numeric literal 11
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:334  [MAGIC_NUMBER]  Bare numeric literal 12
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:452  [MAGIC_NUMBER]  Bare numeric literal 15
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:457  [MAGIC_NUMBER]  Bare numeric literal 15
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:695  [MAGIC_NUMBER]  Bare numeric literal 13
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:904  [MAGIC_NUMBER]  Bare numeric literal 25
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:906  [MAGIC_NUMBER]  Bare numeric literal 25
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:907  [MAGIC_NUMBER]  Bare numeric literal 25
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1112  [MAGIC_NUMBER]  Bare numeric literal 25
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1114  [MAGIC_NUMBER]  Bare numeric literal 25
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1115  [MAGIC_NUMBER]  Bare numeric literal 25
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:253  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 189–194
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:544  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 82–87
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:569  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 529–534
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:570  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 530–535
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:571  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 531–536
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:572  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 532–537
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:598  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 555–560
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:778  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 675–680
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:779  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 676–681
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:780  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 677–682
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:781  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 678–683
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:812  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 675–680
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:813  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 676–681
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:814  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 677–682
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:815  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 678–683
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:840  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 789–794
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:883  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 77–82
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:884  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 78–83
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:885  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 79–84
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:886  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 80–85
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1047  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1032–1037
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1062  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1032–1037
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1081  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 868–873
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1088  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 874–879
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1089  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 875–880
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1124  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 916–921
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1151  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1096–1101
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1196  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1169–1174
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1197  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1170–1175
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1198  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1171–1176
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1199  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1172–1177
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1200  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1173–1178
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1201  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1174–1179
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1202  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1175–1180
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1203  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1176–1181
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1204  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1177–1182
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1205  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1178–1183
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1206  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1179–1184
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1216  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 483–488
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1217  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 484–489
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1218  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 485–490
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1219  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 486–491
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1220  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 487–492
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1221  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 488–493
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1222  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 489–494
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1223  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 490–495
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1224  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 491–496
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1225  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 492–497
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1226  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 493–498
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1227  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 494–499
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1228  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 495–500
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1229  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 496–501
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1230  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 497–502
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1231  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 498–503
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1232  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 499–504
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1233  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 500–505
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1256  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 964–969
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1257  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 965–970
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1258  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 966–971
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1332  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1040–1045
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1333  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1041–1046
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1334  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1042–1047
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1335  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1043–1048
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1336  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1044–1049
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1337  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1045–1050
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1345  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1053–1058
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1346  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1054–1059
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1347  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1055–1060
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1348  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1056–1061
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1349  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1057–1062
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1350  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1058–1063
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1351  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1059–1064
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1352  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1060–1065
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/geometry.py:1354  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1324–1329
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/machine_learning.py:58  [LONG_METHOD]  Method body ~160 lines (max 60)
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/machine_learning.py:219  [LONG_METHOD]  Method body ~64 lines (max 60)
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/machine_learning.py:437  [LONG_METHOD]  Method body ~164 lines (max 60)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/machine_learning.py:664  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/machine_learning.py:602  [LONG_METHOD]  Method body ~67 lines (max 60)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/machine_learning.py:671  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/machine_learning.py:674  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/machine_learning.py:679  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/machine_learning.py:682  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/machine_learning.py:693  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/machine_learning.py:694  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/machine_learning.py:695  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/machine_learning.py:696  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/machine_learning.py:834  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/machine_learning.py:835  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/machine_learning.py:710  [LONG_METHOD]  Method body ~166 lines (max 60)
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/machine_learning.py:457  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/machine_learning.py:512  [MAGIC_NUMBER]  Bare numeric literal 24
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/machine_learning.py:535  [MAGIC_NUMBER]  Bare numeric literal 24
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/machine_learning.py:575  [MAGIC_NUMBER]  Bare numeric literal 16
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/machine_learning.py:617  [MAGIC_NUMBER]  Bare numeric literal 17
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/neural_networks.py:46  [LONG_METHOD]  Method body ~70 lines (max 60)
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/neural_networks.py:136  [LONG_METHOD]  Method body ~95 lines (max 60)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/neural_networks.py:318  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/neural_networks.py:35  [MAGIC_NUMBER]  Bare numeric literal 54
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/neural_networks.py:36  [MAGIC_NUMBER]  Bare numeric literal 128
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/neural_networks.py:39  [MAGIC_NUMBER]  Bare numeric literal 128
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/neural_networks.py:84  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/neural_networks.py:85  [MAGIC_NUMBER]  Bare numeric literal 128
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/neural_networks.py:88  [MAGIC_NUMBER]  Bare numeric literal 128
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/neural_networks.py:192  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/neural_networks.py:193  [MAGIC_NUMBER]  Bare numeric literal 128
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/neural_networks.py:194  [MAGIC_NUMBER]  Bare numeric literal 128
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/neural_networks.py:197  [MAGIC_NUMBER]  Bare numeric literal 128
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/neural_networks.py:149  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 96–101
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/neural_networks.py:150  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 97–102
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/neural_networks.py:151  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 98–103
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/neural_networks.py:210  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 99–104
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/neural_networks.py:211  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 100–105
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/neural_networks.py:226  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 111–116
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/neural_networks.py:227  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 112–117
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/neural_networks.py:228  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 113–118
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/neural_networks.py:229  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 114–119
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/neural_networks.py:230  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 115–120
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/neural_networks.py:272  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 71–76
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/neural_networks.py:273  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 72–77
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/neural_networks.py:274  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 73–78
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/neural_networks.py:275  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 74–79
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/neural_networks.py:276  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 75–80
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/neural_networks.py:277  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 76–81
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_ml_ai_frameworks.py:268  [MAGIC_NUMBER]  Bare numeric literal 42
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_ml_ai_frameworks.py:289  [MAGIC_NUMBER]  Bare numeric literal 10
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:411  [LONG_METHOD]  Method body ~82 lines (max 60)
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:12  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:13  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:28  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:30  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:32  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:34  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:35  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:112  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:114  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:116  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:136  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:138  [MAGIC_NUMBER]  Bare numeric literal 666
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:140  [MAGIC_NUMBER]  Bare numeric literal 666
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:141  [MAGIC_NUMBER]  Bare numeric literal 666
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:143  [MAGIC_NUMBER]  Bare numeric literal 666
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:144  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:148  [MAGIC_NUMBER]  Bare numeric literal 666
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:386  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:389  [MAGIC_NUMBER]  Bare numeric literal 11
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:392  [MAGIC_NUMBER]  Bare numeric literal 11
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:393  [MAGIC_NUMBER]  Bare numeric literal 12
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:396  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:397  [MAGIC_NUMBER]  Bare numeric literal 11
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:399  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:419  [MAGIC_NUMBER]  Bare numeric literal 77345112
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:422  [MAGIC_NUMBER]  Bare numeric literal 5555
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:424  [MAGIC_NUMBER]  Bare numeric literal 192
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:426  [MAGIC_NUMBER]  Bare numeric literal 77
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:427  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:430  [MAGIC_NUMBER]  Bare numeric literal 12
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:442  [MAGIC_NUMBER]  Bare numeric literal 192
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:449  [MAGIC_NUMBER]  Bare numeric literal 77345112
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:450  [MAGIC_NUMBER]  Bare numeric literal 192
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:455  [MAGIC_NUMBER]  Bare numeric literal 12
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:457  [MAGIC_NUMBER]  Bare numeric literal 77345112
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:458  [MAGIC_NUMBER]  Bare numeric literal 192
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:462  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:465  [MAGIC_NUMBER]  Bare numeric literal 77345112
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:466  [MAGIC_NUMBER]  Bare numeric literal 192
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:474  [MAGIC_NUMBER]  Bare numeric literal 5555
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:482  [MAGIC_NUMBER]  Bare numeric literal 192
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:490  [MAGIC_NUMBER]  Bare numeric literal 77345112
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:1  [DEAD_IMPORT]  'annotations' imported but never used
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:149  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 121–126
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:226  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 195–200
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:239  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 195–200
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:265  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 238–243
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:266  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 195–200
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:292  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 238–243
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:293  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 195–200
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/data_structures.py:340  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 195–200
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:264  [LONG_METHOD]  Method body ~62 lines (max 60)
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:395  [LONG_METHOD]  Method body ~71 lines (max 60)
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:734  [LONG_METHOD]  Method body ~66 lines (max 60)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:888  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:905  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:925  [LONG_METHOD]  Method body ~118 lines (max 60)
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1165  [LONG_METHOD]  Method body ~62 lines (max 60)
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1641  [LONG_PARAM_LIST]  7 parameters (max 5)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1799  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1800  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1801  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1802  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1803  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1804  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1805  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1807  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1808  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1809  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1810  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1811  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1812  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1813  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1823  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1824  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1825  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1826  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1827  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1828  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1829  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1831  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1832  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1833  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1834  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1835  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1836  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1837  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1842  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1843  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1844  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1845  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1846  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1847  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1848  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1850  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1851  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1852  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1853  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1854  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1855  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1856  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1787  [LONG_METHOD]  Method body ~82 lines (max 60)
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:28  [MAGIC_NUMBER]  Bare numeric literal 25
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:29  [MAGIC_NUMBER]  Bare numeric literal 625
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:43  [MAGIC_NUMBER]  Bare numeric literal 8333333333
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:47  [MAGIC_NUMBER]  Bare numeric literal 8333
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:64  [MAGIC_NUMBER]  Bare numeric literal 25
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:65  [MAGIC_NUMBER]  Bare numeric literal 38961843444
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:93  [MAGIC_NUMBER]  Bare numeric literal 75
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:95  [MAGIC_NUMBER]  Bare numeric literal 31622776601
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:165  [MAGIC_NUMBER]  Bare numeric literal 21
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:166  [MAGIC_NUMBER]  Bare numeric literal 13
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:187  [MAGIC_NUMBER]  Bare numeric literal 25
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:188  [MAGIC_NUMBER]  Bare numeric literal 25
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:235  [MAGIC_NUMBER]  Bare numeric literal 54
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:236  [MAGIC_NUMBER]  Bare numeric literal 36
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:271  [MAGIC_NUMBER]  Bare numeric literal 40
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:272  [MAGIC_NUMBER]  Bare numeric literal 60
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:274  [MAGIC_NUMBER]  Bare numeric literal 40
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:275  [MAGIC_NUMBER]  Bare numeric literal 48
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:277  [MAGIC_NUMBER]  Bare numeric literal 40
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:278  [MAGIC_NUMBER]  Bare numeric literal 60
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:281  [MAGIC_NUMBER]  Bare numeric literal 40
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:282  [MAGIC_NUMBER]  Bare numeric literal 56
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:409  [MAGIC_NUMBER]  Bare numeric literal 25
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:410  [MAGIC_NUMBER]  Bare numeric literal 35
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:411  [MAGIC_NUMBER]  Bare numeric literal 45
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:412  [MAGIC_NUMBER]  Bare numeric literal 55
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:413  [MAGIC_NUMBER]  Bare numeric literal 65
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:416  [MAGIC_NUMBER]  Bare numeric literal 1071
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:417  [MAGIC_NUMBER]  Bare numeric literal 30
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:420  [MAGIC_NUMBER]  Bare numeric literal 35
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:421  [MAGIC_NUMBER]  Bare numeric literal 484
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:422  [MAGIC_NUMBER]  Bare numeric literal 30
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:426  [MAGIC_NUMBER]  Bare numeric literal 35
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:427  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:428  [MAGIC_NUMBER]  Bare numeric literal 37
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:532  [MAGIC_NUMBER]  Bare numeric literal 75
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:534  [MAGIC_NUMBER]  Bare numeric literal 3720238095238095
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:541  [MAGIC_NUMBER]  Bare numeric literal 3720238095238095
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:555  [MAGIC_NUMBER]  Bare numeric literal 67
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:579  [MAGIC_NUMBER]  Bare numeric literal 25
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:581  [MAGIC_NUMBER]  Bare numeric literal 25
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:588  [MAGIC_NUMBER]  Bare numeric literal 25
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:598  [MAGIC_NUMBER]  Bare numeric literal 13
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:614  [MAGIC_NUMBER]  Bare numeric literal 75
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:615  [MAGIC_NUMBER]  Bare numeric literal 810874155219827
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:632  [MAGIC_NUMBER]  Bare numeric literal 75
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:633  [MAGIC_NUMBER]  Bare numeric literal 986893273527251
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:656  [MAGIC_NUMBER]  Bare numeric literal 75
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:753  [MAGIC_NUMBER]  Bare numeric literal 42
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:756  [MAGIC_NUMBER]  Bare numeric literal 17495
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:769  [MAGIC_NUMBER]  Bare numeric literal 90475
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:865  [MAGIC_NUMBER]  Bare numeric literal 12
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:877  [MAGIC_NUMBER]  Bare numeric literal 106
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:878  [MAGIC_NUMBER]  Bare numeric literal 3838
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:879  [MAGIC_NUMBER]  Bare numeric literal 4258865685331
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:880  [MAGIC_NUMBER]  Bare numeric literal 499
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:881  [MAGIC_NUMBER]  Bare numeric literal 26818732
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:886  [MAGIC_NUMBER]  Bare numeric literal 15
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:887  [MAGIC_NUMBER]  Bare numeric literal 16
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:896  [MAGIC_NUMBER]  Bare numeric literal 3400218741872791
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:897  [MAGIC_NUMBER]  Bare numeric literal 499
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:898  [MAGIC_NUMBER]  Bare numeric literal 33
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:903  [MAGIC_NUMBER]  Bare numeric literal 35
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:904  [MAGIC_NUMBER]  Bare numeric literal 32
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:971  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:976  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:978  [MAGIC_NUMBER]  Bare numeric literal 11
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:980  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:984  [MAGIC_NUMBER]  Bare numeric literal 9
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:985  [MAGIC_NUMBER]  Bare numeric literal 31
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:986  [MAGIC_NUMBER]  Bare numeric literal 70
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:987  [MAGIC_NUMBER]  Bare numeric literal 111
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:988  [MAGIC_NUMBER]  Bare numeric literal 125
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:989  [MAGIC_NUMBER]  Bare numeric literal 110
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:990  [MAGIC_NUMBER]  Bare numeric literal 86
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:991  [MAGIC_NUMBER]  Bare numeric literal 68
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:992  [MAGIC_NUMBER]  Bare numeric literal 59
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:993  [MAGIC_NUMBER]  Bare numeric literal 66
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:994  [MAGIC_NUMBER]  Bare numeric literal 82
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:995  [MAGIC_NUMBER]  Bare numeric literal 82
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:996  [MAGIC_NUMBER]  Bare numeric literal 58
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:997  [MAGIC_NUMBER]  Bare numeric literal 28
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:998  [MAGIC_NUMBER]  Bare numeric literal 9
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:999  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1006  [MAGIC_NUMBER]  Bare numeric literal 22
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1012  [MAGIC_NUMBER]  Bare numeric literal 2018
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1021  [MAGIC_NUMBER]  Bare numeric literal 10419
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1096  [MAGIC_NUMBER]  Bare numeric literal 8675309
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1097  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1170  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1171  [MAGIC_NUMBER]  Bare numeric literal 99
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1281  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1282  [MAGIC_NUMBER]  Bare numeric literal 99
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1297  [MAGIC_NUMBER]  Bare numeric literal 8035050657330205
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1452  [MAGIC_NUMBER]  Bare numeric literal 25
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1453  [MAGIC_NUMBER]  Bare numeric literal 19
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1458  [MAGIC_NUMBER]  Bare numeric literal 1000
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1459  [MAGIC_NUMBER]  Bare numeric literal 1000
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1465  [MAGIC_NUMBER]  Bare numeric literal 63
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1470  [MAGIC_NUMBER]  Bare numeric literal 6963
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1590  [MAGIC_NUMBER]  Bare numeric literal 25
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1646  [MAGIC_NUMBER]  Bare numeric literal 31
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1659  [MAGIC_NUMBER]  Bare numeric literal 45
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1769  [MAGIC_NUMBER]  Bare numeric literal 512
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1777  [MAGIC_NUMBER]  Bare numeric literal 537
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1795  [MAGIC_NUMBER]  Bare numeric literal 425
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1796  [MAGIC_NUMBER]  Bare numeric literal 180625
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1849  [MAGIC_NUMBER]  Bare numeric literal 15
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1598  [EMPTY_CATCH]  Empty except block (swallowed exception)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1868  [EMPTY_CATCH]  Empty except block (swallowed exception)
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:366  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 340–345
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:367  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 341–346
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:386  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 338–343
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1066  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1056–1061
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1067  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1057–1062
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1102  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1025–1030
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1103  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1026–1031
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1104  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1027–1032
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1105  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1028–1033
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1106  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1029–1034
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1107  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1030–1035
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1108  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1031–1036
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/statistics.py:1109  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1032–1037
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:44  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:45  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:62  [LONG_PARAM_LIST]  6 parameters (max 5)
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:86  [LONG_PARAM_LIST]  6 parameters (max 5)
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:108  [LONG_PARAM_LIST]  6 parameters (max 5)
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:124  [LONG_PARAM_LIST]  7 parameters (max 5)
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:292  [LONG_PARAM_LIST]  6 parameters (max 5)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:438  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:439  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:440  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:460  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:108  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:119  [MAGIC_NUMBER]  Bare numeric literal 42
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:127  [MAGIC_NUMBER]  Bare numeric literal 42
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:158  [MAGIC_NUMBER]  Bare numeric literal 12
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:175  [MAGIC_NUMBER]  Bare numeric literal 30
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:191  [MAGIC_NUMBER]  Bare numeric literal 60
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:205  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:207  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:209  [MAGIC_NUMBER]  Bare numeric literal 25
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:214  [MAGIC_NUMBER]  Bare numeric literal 32
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:240  [MAGIC_NUMBER]  Bare numeric literal 12
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:256  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:272  [MAGIC_NUMBER]  Bare numeric literal 42
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:292  [MAGIC_NUMBER]  Bare numeric literal 128
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:317  [MAGIC_NUMBER]  Bare numeric literal 32
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:376  [MAGIC_NUMBER]  Bare numeric literal 5000
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:388  [MAGIC_NUMBER]  Bare numeric literal 1000
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:391  [MAGIC_NUMBER]  Bare numeric literal 42
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:398  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:444  [MAGIC_NUMBER]  Bare numeric literal 800
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:453  [MAGIC_NUMBER]  Bare numeric literal 42
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:477  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:478  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:479  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:499  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:530  [MAGIC_NUMBER]  Bare numeric literal 42
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:547  [MAGIC_NUMBER]  Bare numeric literal 42
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:6  [DEAD_IMPORT]  'LabelEncoder' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:6  [DEAD_IMPORT]  'OneHotEncoder' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:7  [DEAD_IMPORT]  'train_test_split' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:8  [DEAD_IMPORT]  'GradientBoostingRegressor' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:9  [DEAD_IMPORT]  'LogisticRegression' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:9  [DEAD_IMPORT]  'Ridge' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:9  [DEAD_IMPORT]  'Lasso' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:9  [DEAD_IMPORT]  'ElasticNet' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:10  [DEAD_IMPORT]  'SVC' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:10  [DEAD_IMPORT]  'SVR' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:11  [DEAD_IMPORT]  'MLPClassifier' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:11  [DEAD_IMPORT]  'MLPRegressor' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:12  [DEAD_IMPORT]  'KMeans' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:12  [DEAD_IMPORT]  'DBSCAN' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:12  [DEAD_IMPORT]  'AgglomerativeClustering' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:13  [DEAD_IMPORT]  'ICA' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:13  [DEAD_IMPORT]  'NMF' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:14  [DEAD_IMPORT]  'SelectKBest' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:14  [DEAD_IMPORT]  'RFE' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:14  [DEAD_IMPORT]  'SelectFromModel' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:15  [DEAD_IMPORT]  'accuracy_score' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:15  [DEAD_IMPORT]  'precision_score' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:15  [DEAD_IMPORT]  'recall_score' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:15  [DEAD_IMPORT]  'f1_score' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:16  [DEAD_IMPORT]  'mean_absolute_error' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:16  [DEAD_IMPORT]  'r2_score' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:17  [DEAD_IMPORT]  'pandas' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:18  [DEAD_IMPORT]  'numpy' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:19  [DEAD_IMPORT]  'pyplot' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:20  [DEAD_IMPORT]  'seaborn' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:110  [DEAD_IMPORT]  'Real' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:110  [DEAD_IMPORT]  'Integer' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:110  [DEAD_IMPORT]  'Categorical' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:417  [DEAD_IMPORT]  'graph_objects' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:450  [DEAD_IMPORT]  'express' imported but never used
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:544  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 527–532
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:545  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 528–533
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:546  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 529–534
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:547  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 530–535
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_data_science_techniques.py:548  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 531–536
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_stdlib.py:10  [DEAD_IMPORT]  'string' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_stdlib.py:13  [DEAD_IMPORT]  'namedtuple' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_stdlib.py:13  [DEAD_IMPORT]  'deque' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_stdlib.py:13  [DEAD_IMPORT]  'Counter' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_stdlib.py:13  [DEAD_IMPORT]  'OrderedDict' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_stdlib.py:13  [DEAD_IMPORT]  'defaultdict' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_stdlib.py:13  [DEAD_IMPORT]  'ChainMap' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_stdlib.py:14  [DEAD_IMPORT]  'heappush' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_stdlib.py:14  [DEAD_IMPORT]  'heappop' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_stdlib.py:14  [DEAD_IMPORT]  'heapify' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_stdlib.py:15  [DEAD_IMPORT]  'Queue' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_stdlib.py:15  [DEAD_IMPORT]  'LifoQueue' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_stdlib.py:15  [DEAD_IMPORT]  'PriorityQueue' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_stdlib.py:16  [DEAD_IMPORT]  'Enum' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_stdlib.py:16  [DEAD_IMPORT]  'IntEnum' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_stdlib.py:16  [DEAD_IMPORT]  'Flag' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_stdlib.py:16  [DEAD_IMPORT]  'IntFlag' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_stdlib.py:25  [DEAD_IMPORT]  'gzip' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_stdlib.py:31  [DEAD_IMPORT]  'request' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_stdlib.py:46  [DEAD_IMPORT]  'configparser' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_stdlib.py:52  [DEAD_IMPORT]  'List' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_stdlib.py:52  [DEAD_IMPORT]  'Dict' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_stdlib.py:52  [DEAD_IMPORT]  'Set' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_stdlib.py:52  [DEAD_IMPORT]  'Tuple' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_stdlib.py:52  [DEAD_IMPORT]  'Optional' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_stdlib.py:52  [DEAD_IMPORT]  'Union' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_stdlib.py:52  [DEAD_IMPORT]  'Any' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_stdlib.py:52  [DEAD_IMPORT]  'Callable' imported but never used
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_physics_computational_methods.py:44  [LONG_PARAM_LIST]  7 parameters (max 5)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_physics_computational_methods.py:54  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_physics_computational_methods.py:164  [LONG_PARAM_LIST]  7 parameters (max 5)
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_physics_computational_methods.py:276  [LONG_PARAM_LIST]  6 parameters (max 5)
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_physics_computational_methods.py:421  [LONG_PARAM_LIST]  6 parameters (max 5)
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_physics_computational_methods.py:444  [LONG_PARAM_LIST]  6 parameters (max 5)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_physics_computational_methods.py:496  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_physics_computational_methods.py:20  [MAGIC_NUMBER]  Bare numeric literal 1000
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_physics_computational_methods.py:295  [MAGIC_NUMBER]  Bare numeric literal 15
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_physics_computational_methods.py:296  [MAGIC_NUMBER]  Bare numeric literal 17
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_physics_computational_methods.py:297  [MAGIC_NUMBER]  Bare numeric literal 711
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_physics_computational_methods.py:298  [MAGIC_NUMBER]  Bare numeric literal 23
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_physics_computational_methods.py:299  [MAGIC_NUMBER]  Bare numeric literal 11
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_physics_computational_methods.py:335  [MAGIC_NUMBER]  Bare numeric literal 24
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_physics_computational_methods.py:336  [MAGIC_NUMBER]  Bare numeric literal 2200
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_physics_computational_methods.py:368  [MAGIC_NUMBER]  Bare numeric literal 25
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_physics_computational_methods.py:470  [MAGIC_NUMBER]  Bare numeric literal 1000
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_physics_computational_methods.py:491  [MAGIC_NUMBER]  Bare numeric literal 1000
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_physics_computational_methods.py:496  [MAGIC_NUMBER]  Bare numeric literal 25
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_physics_computational_methods.py:522  [MAGIC_NUMBER]  Bare numeric literal 1000
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_physics_computational_methods.py:4  [DEAD_IMPORT]  'numpy' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_physics_computational_methods.py:5  [DEAD_IMPORT]  'pyplot' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_physics_computational_methods.py:6  [DEAD_IMPORT]  'optimize' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_physics_computational_methods.py:7  [DEAD_IMPORT]  'spherical_jn' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_physics_computational_methods.py:9  [DEAD_IMPORT]  'diff' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_physics_computational_methods.py:9  [DEAD_IMPORT]  'sp_integrate' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_physics_computational_methods.py:9  [DEAD_IMPORT]  'solve' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_physics_computational_methods.py:9  [DEAD_IMPORT]  'Matrix' imported but never used
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/advanced_physics_computational_methods.py:10  [DEAD_IMPORT]  'pandas' imported but never used
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:38  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:41  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:44  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:166  [LONG_METHOD]  Method body ~81 lines (max 60)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:289  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:368  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:372  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:373  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:374  [DEEP_NESTING]  Nesting depth ~9 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:378  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:389  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:390  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:392  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:393  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:394  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:395  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:397  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:398  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:399  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:400  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:401  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:402  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:403  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:404  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:422  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:435  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:542  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:543  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:544  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:637  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:641  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:642  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:643  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:660  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:669  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:671  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:673  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:675  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:677  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:679  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:51  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:54  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:56  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:87  [MAGIC_NUMBER]  Bare numeric literal 42
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:108  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:112  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:117  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:122  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:134  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:136  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:211  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:229  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:252  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:257  [MAGIC_NUMBER]  Bare numeric literal 42
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:297  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:298  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:300  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:301  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:303  [MAGIC_NUMBER]  Bare numeric literal 87
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:304  [MAGIC_NUMBER]  Bare numeric literal 87
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:306  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:310  [MAGIC_NUMBER]  Bare numeric literal 42
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:335  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:338  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:341  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:351  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:352  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:354  [MAGIC_NUMBER]  Bare numeric literal 87
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:355  [MAGIC_NUMBER]  Bare numeric literal 87
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:357  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:359  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:367  [MAGIC_NUMBER]  Bare numeric literal 13
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:368  [MAGIC_NUMBER]  Bare numeric literal 13
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:377  [MAGIC_NUMBER]  Bare numeric literal 13
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:379  [MAGIC_NUMBER]  Bare numeric literal 31
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:409  [MAGIC_NUMBER]  Bare numeric literal 32
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:410  [MAGIC_NUMBER]  Bare numeric literal 32
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:413  [MAGIC_NUMBER]  Bare numeric literal 32
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:417  [MAGIC_NUMBER]  Bare numeric literal 65
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:421  [MAGIC_NUMBER]  Bare numeric literal 65
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:430  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:434  [MAGIC_NUMBER]  Bare numeric literal 19
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:457  [MAGIC_NUMBER]  Bare numeric literal 14
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:460  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:462  [MAGIC_NUMBER]  Bare numeric literal 14
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:463  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:465  [MAGIC_NUMBER]  Bare numeric literal 12
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:466  [MAGIC_NUMBER]  Bare numeric literal 65
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:467  [MAGIC_NUMBER]  Bare numeric literal 64
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:468  [MAGIC_NUMBER]  Bare numeric literal 14
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:469  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:470  [MAGIC_NUMBER]  Bare numeric literal 64
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:471  [MAGIC_NUMBER]  Bare numeric literal 12
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:501  [MAGIC_NUMBER]  Bare numeric literal 11
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:502  [MAGIC_NUMBER]  Bare numeric literal 11
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:505  [MAGIC_NUMBER]  Bare numeric literal 101
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:509  [MAGIC_NUMBER]  Bare numeric literal 99
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:511  [MAGIC_NUMBER]  Bare numeric literal 101
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:513  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:516  [MAGIC_NUMBER]  Bare numeric literal 19
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:517  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:519  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:520  [MAGIC_NUMBER]  Bare numeric literal 18
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:521  [MAGIC_NUMBER]  Bare numeric literal 19
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:522  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:529  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:538  [MAGIC_NUMBER]  Bare numeric literal 32
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:544  [MAGIC_NUMBER]  Bare numeric literal 63
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:545  [MAGIC_NUMBER]  Bare numeric literal 63
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:564  [MAGIC_NUMBER]  Bare numeric literal 1000
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:572  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:576  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:579  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:581  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:582  [MAGIC_NUMBER]  Bare numeric literal 30
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:588  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:594  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:602  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:604  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:610  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:618  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:622  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:626  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:631  [MAGIC_NUMBER]  Bare numeric literal 18
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:632  [MAGIC_NUMBER]  Bare numeric literal 19
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:633  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:674  [MAGIC_NUMBER]  Bare numeric literal 201
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:675  [MAGIC_NUMBER]  Bare numeric literal 201
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:676  [MAGIC_NUMBER]  Bare numeric literal 201
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:677  [MAGIC_NUMBER]  Bare numeric literal 201
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:678  [MAGIC_NUMBER]  Bare numeric literal 201
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:679  [MAGIC_NUMBER]  Bare numeric literal 201
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:695  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:696  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:697  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:698  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:699  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:700  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:716  [MAGIC_NUMBER]  Bare numeric literal 10
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:137  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 57–62
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:138  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 58–63
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:139  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 59–64
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:140  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 60–65
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:194  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 176–181
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:195  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 177–182
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:196  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 178–183
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:197  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 179–184
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:198  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 180–185
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:199  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 181–186
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:200  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 182–187
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:201  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 183–188
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:202  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 184–189
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:203  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 185–190
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:204  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 186–191
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:205  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 187–192
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:215  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 178–183
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:216  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 179–184
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:232  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 213–218
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:233  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 214–219
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:234  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 178–183
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:235  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 179–184
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:236  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 217–222
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:237  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 218–223
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:238  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 219–224
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:239  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 220–225
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:240  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 221–226
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:241  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 222–227
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:242  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 223–228
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:435  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 422–427
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:597  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 589–594
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:598  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 590–595
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:614  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 590–595
[MEDIUM] lead-pulls/timetechpro/public_html/xcm-datasets/datasets/python_range_examples.py:622  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 572–577
[HIGH] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:203  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:204  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:205  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:210  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:211  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:212  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:215  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:216  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:217  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:220  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:221  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:222  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:225  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:226  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:227  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:230  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:231  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:232  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:235  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:236  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:237  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:23  [MAGIC_NUMBER]  Bare numeric literal 15
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:51  [MAGIC_NUMBER]  Bare numeric literal 180
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:52  [MAGIC_NUMBER]  Bare numeric literal 5
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:92  [MAGIC_NUMBER]  Bare numeric literal 102
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:101  [MAGIC_NUMBER]  Bare numeric literal 102
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:107  [MAGIC_NUMBER]  Bare numeric literal 102
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:113  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:114  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:117  [MAGIC_NUMBER]  Bare numeric literal 255
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:118  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:119  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:125  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:129  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:137  [MAGIC_NUMBER]  Bare numeric literal 102
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:183  [MAGIC_NUMBER]  Bare numeric literal 3600
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:199  [MAGIC_NUMBER]  Bare numeric literal 25
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:211  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:226  [MAGIC_NUMBER]  Bare numeric literal 45
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:277  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:280  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:281  [MAGIC_NUMBER]  Bare numeric literal 4
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:282  [MAGIC_NUMBER]  Bare numeric literal 62
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:283  [MAGIC_NUMBER]  Bare numeric literal 145
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:306  [MAGIC_NUMBER]  Bare numeric literal 2025
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:307  [MAGIC_NUMBER]  Bare numeric literal 2025
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:309  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:310  [MAGIC_NUMBER]  Bare numeric literal 12
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:311  [MAGIC_NUMBER]  Bare numeric literal 22
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:312  [MAGIC_NUMBER]  Bare numeric literal 24
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:313  [MAGIC_NUMBER]  Bare numeric literal 32
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:314  [MAGIC_NUMBER]  Bare numeric literal 35
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:315  [MAGIC_NUMBER]  Bare numeric literal 37
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:320  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:322  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:324  [MAGIC_NUMBER]  Bare numeric literal 25
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:326  [MAGIC_NUMBER]  Bare numeric literal 40
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:328  [MAGIC_NUMBER]  Bare numeric literal 55
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:330  [MAGIC_NUMBER]  Bare numeric literal 70
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:332  [MAGIC_NUMBER]  Bare numeric literal 360
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:333  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:334  [MAGIC_NUMBER]  Bare numeric literal 432
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:335  [MAGIC_NUMBER]  Bare numeric literal 11
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:336  [MAGIC_NUMBER]  Bare numeric literal 721
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:337  [MAGIC_NUMBER]  Bare numeric literal 12
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:343  [MAGIC_NUMBER]  Bare numeric literal 21
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:345  [MAGIC_NUMBER]  Bare numeric literal 21
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:347  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:349  [MAGIC_NUMBER]  Bare numeric literal 80
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:351  [MAGIC_NUMBER]  Bare numeric literal 111
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:353  [MAGIC_NUMBER]  Bare numeric literal 141
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:355  [MAGIC_NUMBER]  Bare numeric literal 721
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:356  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:357  [MAGIC_NUMBER]  Bare numeric literal 865
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:358  [MAGIC_NUMBER]  Bare numeric literal 11
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:359  [MAGIC_NUMBER]  Bare numeric literal 442
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:360  [MAGIC_NUMBER]  Bare numeric literal 12
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:363  [MAGIC_NUMBER]  Bare numeric literal 2025
[LOW] lead-pulls/timetechpro/public_html/timeclock/dashboard.php:366  [MAGIC_NUMBER]  Bare numeric literal 12
[HIGH] lead-pulls/timetechpro/public_html/crons/timeclockbu.php:57  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/crons/timeclockbu.php:58  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/crons/timeclockbu.php:63  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/crons/timeclockbu.php:64  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/crons/timeclockbu.php:65  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/timetechpro/public_html/crons/timeclockbu.php:66  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:438  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:439  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:442  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:443  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:446  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:447  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:450  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:451  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:454  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:455  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:458  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:459  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:507  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:508  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:511  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:512  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:515  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:516  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:519  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:520  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:521  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:522  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:523  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:539  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:540  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:617  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:618  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:621  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:622  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:625  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:626  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:629  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:630  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:633  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:634  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:635  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:636  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:637  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:885  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:899  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:900  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:904  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:919  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:920  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:921  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:928  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:943  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:944  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:945  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:946  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:1043  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] lead-pulls/xcaliburmoon/landing_mgr/app.js:888  [MAGIC_NUMBER]  Bare numeric literal 255
[LOW] lead-pulls/xcaliburmoon/landing_mgr/app.js:897  [MAGIC_NUMBER]  Bare numeric literal 255
[LOW] lead-pulls/xcaliburmoon/landing_mgr/app.js:920  [MAGIC_NUMBER]  Bare numeric literal 255
[LOW] lead-pulls/xcaliburmoon/landing_mgr/app.js:944  [MAGIC_NUMBER]  Bare numeric literal 255
[LOW] lead-pulls/xcaliburmoon/landing_mgr/app.js:967  [MAGIC_NUMBER]  Bare numeric literal 255
[LOW] lead-pulls/xcaliburmoon/landing_mgr/app.js:995  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] lead-pulls/xcaliburmoon/landing_mgr/app.js:1000  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] lead-pulls/xcaliburmoon/landing_mgr/app.js:1001  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] lead-pulls/xcaliburmoon/landing_mgr/app.js:1008  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] lead-pulls/xcaliburmoon/landing_mgr/app.js:1009  [MAGIC_NUMBER]  Bare numeric literal 111
[LOW] lead-pulls/xcaliburmoon/landing_mgr/app.js:1016  [MAGIC_NUMBER]  Bare numeric literal 1100
[LOW] lead-pulls/xcaliburmoon/landing_mgr/app.js:1026  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] lead-pulls/xcaliburmoon/landing_mgr/app.js:1027  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] lead-pulls/xcaliburmoon/landing_mgr/app.js:1028  [MAGIC_NUMBER]  Bare numeric literal 255
[LOW] lead-pulls/xcaliburmoon/landing_mgr/app.js:1030  [MAGIC_NUMBER]  Bare numeric literal 255
[HIGH] lead-pulls/xcaliburmoon/landing_mgr/app.js:118  [EMPTY_CATCH]  Empty catch block (swallowed exception)
[MEDIUM] lead-pulls/xcaliburmoon/landing_mgr/app.js:188  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 176–181
[MEDIUM] lead-pulls/xcaliburmoon/landing_mgr/app.js:189  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 177–182
[MEDIUM] lead-pulls/xcaliburmoon/landing_mgr/app.js:635  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 521–526
[MEDIUM] lead-pulls/xcaliburmoon/landing_mgr/app.js:636  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 522–527
[MEDIUM] lead-pulls/xcaliburmoon/landing_mgr/app.js:637  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 523–528
[MEDIUM] lead-pulls/xcaliburmoon/landing_mgr/app.js:674  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 583–588
[MEDIUM] lead-pulls/xcaliburmoon/landing_mgr/app.js:745  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 176–181
[MEDIUM] lead-pulls/xcaliburmoon/landing_mgr/app.js:746  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 177–182
[HIGH] schedule_a_consult/quote/index.php:430  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] schedule_a_consult/quote/index.php:431  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] schedule_a_consult/quote/index.php:432  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] schedule_a_consult/quote/index.php:433  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] schedule_a_consult/quote/index.php:434  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] schedule_a_consult/quote/index.php:435  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] schedule_a_consult/quote/index.php:436  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] schedule_a_consult/quote/index.php:437  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] schedule_a_consult/quote/index.php:438  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] schedule_a_consult/quote/index.php:439  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] schedule_a_consult/quote/index.php:484  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] schedule_a_consult/quote/index.php:485  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] schedule_a_consult/quote/index.php:486  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] schedule_a_consult/quote/index.php:525  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] schedule_a_consult/quote/index.php:529  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] schedule_a_consult/quote/index.php:530  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] schedule_a_consult/quote/index.php:534  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] schedule_a_consult/quote/index.php:535  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] schedule_a_consult/quote/index.php:536  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] schedule_a_consult/quote/index.php:537  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] schedule_a_consult/quote/index.php:538  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] schedule_a_consult/quote/index.php:539  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] schedule_a_consult/quote/index.php:540  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] schedule_a_consult/quote/index.php:541  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] schedule_a_consult/quote/index.php:542  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] schedule_a_consult/quote/index.php:545  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] schedule_a_consult/quote/index.php:546  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] schedule_a_consult/quote/index.php:547  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] schedule_a_consult/quote/index.php:554  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] schedule_a_consult/quote/index.php:555  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] schedule_a_consult/quote/index.php:556  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] schedule_a_consult/quote/index.php:557  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] schedule_a_consult/quote/index.php:558  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] schedule_a_consult/quote/index.php:559  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] schedule_a_consult/quote/index.php:7  [MAGIC_NUMBER]  Bare numeric literal 32
[LOW] schedule_a_consult/quote/index.php:61  [MAGIC_NUMBER]  Bare numeric literal 555
[LOW] schedule_a_consult/quote/index.php:65  [MAGIC_NUMBER]  Bare numeric literal 108
[LOW] schedule_a_consult/quote/index.php:66  [MAGIC_NUMBER]  Bare numeric literal 108
[LOW] schedule_a_consult/quote/index.php:177  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] schedule_a_consult/quote/index.php:242  [MAGIC_NUMBER]  Bare numeric literal 2000
[LOW] schedule_a_consult/quote/index.php:249  [MAGIC_NUMBER]  Bare numeric literal 2000
[LOW] schedule_a_consult/quote/index.php:296  [MAGIC_NUMBER]  Bare numeric literal 1000
[LOW] schedule_a_consult/quote/index.php:315  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] schedule_a_consult/quote/index.php:503  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] schedule_a_consult/quote/index.php:507  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] schedule_a_consult/quote/config.php:60  [MAGIC_NUMBER]  Bare numeric literal 3600
[LOW] schedule_a_consult/quote/submit.php:44  [MAGIC_NUMBER]  Bare numeric literal 403
[LOW] schedule_a_consult/quote/submit.php:50  [MAGIC_NUMBER]  Bare numeric literal 32
[LOW] schedule_a_consult/quote/submit.php:71  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] schedule_a_consult/quote/submit.php:99  [MAGIC_NUMBER]  Bare numeric literal 403
[LOW] schedule_a_consult/quote/submit.php:114  [MAGIC_NUMBER]  Bare numeric literal 755
[LOW] schedule_a_consult/quote/submit.php:133  [MAGIC_NUMBER]  Bare numeric literal 60
[LOW] schedule_a_consult/quote/submit.php:134  [MAGIC_NUMBER]  Bare numeric literal 429
[LOW] ai-chat/reset-password.php:34  [MAGIC_NUMBER]  Bare numeric literal 12
[HIGH] ai-chat/public/chat.php:60  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:61  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:64  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:65  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:73  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:74  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:77  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:78  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:81  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:82  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:85  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:86  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:87  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:88  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:89  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:90  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:91  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:100  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:101  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:104  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:105  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:109  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:110  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:126  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:127  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:128  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:145  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:146  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:149  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:150  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:170  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:173  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:174  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:175  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:176  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:177  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:178  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:179  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:180  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:181  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:188  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:189  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:190  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:191  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:198  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:199  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:200  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:201  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:208  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:209  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:210  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:218  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:219  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:220  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:221  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:222  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:223  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:226  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:227  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:228  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:229  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] ai-chat/public/chat.php:230  [DEEP_NESTING]  Nesting depth ~8 (max 4)
[HIGH] ai-chat/public/chat.php:231  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] ai-chat/public/chat.php:232  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] ai-chat/public/chat.php:233  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:234  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:235  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:238  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:239  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:257  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:258  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:259  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:260  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:261  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:262  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:263  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:264  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:265  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:266  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:267  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:268  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:269  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] ai-chat/public/chat.php:270  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] ai-chat/public/chat.php:271  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] ai-chat/public/chat.php:272  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] ai-chat/public/chat.php:273  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] ai-chat/public/chat.php:274  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:275  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:279  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:280  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:281  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:282  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:283  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] ai-chat/public/chat.php:284  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] ai-chat/public/chat.php:285  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] ai-chat/public/chat.php:286  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] ai-chat/public/chat.php:287  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] ai-chat/public/chat.php:288  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] ai-chat/public/chat.php:289  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] ai-chat/public/chat.php:290  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] ai-chat/public/chat.php:291  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] ai-chat/public/chat.php:292  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] ai-chat/public/chat.php:293  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:294  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:295  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:296  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:297  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:298  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:299  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:300  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:301  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:302  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:303  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:304  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:305  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:306  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:307  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:308  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:312  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:313  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:314  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:315  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:316  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:317  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:318  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:319  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:320  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:321  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:322  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:323  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:324  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:328  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:329  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:350  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:351  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:374  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:375  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:376  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/chat.php:377  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:378  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:381  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/chat.php:382  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] ai-chat/public/chat.php:88  [MAGIC_NUMBER]  Bare numeric literal 19
[LOW] ai-chat/public/chat.php:89  [MAGIC_NUMBER]  Bare numeric literal 49
[LOW] ai-chat/public/chat.php:90  [MAGIC_NUMBER]  Bare numeric literal 149
[LOW] ai-chat/public/chat.php:157  [MAGIC_NUMBER]  Bare numeric literal 18
[LOW] ai-chat/public/chat.php:198  [MAGIC_NUMBER]  Bare numeric literal 24
[LOW] ai-chat/public/chat.php:199  [MAGIC_NUMBER]  Bare numeric literal 44
[LOW] ai-chat/public/chat.php:219  [MAGIC_NUMBER]  Bare numeric literal 24
[LOW] ai-chat/public/chat.php:229  [MAGIC_NUMBER]  Bare numeric literal 24
[LOW] ai-chat/public/chat.php:230  [MAGIC_NUMBER]  Bare numeric literal 44
[HIGH] ai-chat/public/assets/js/chat.js:753  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:754  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:755  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:756  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:757  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:760  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:763  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:764  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1103  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1104  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1471  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1472  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1475  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1476  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1481  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1482  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1483  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1571  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1572  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1574  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1622  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1623  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1624  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1625  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1639  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1640  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1643  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1644  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1645  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1646  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1647  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1651  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1652  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1653  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1654  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1655  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1656  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1683  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1684  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1685  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1686  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1687  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1688  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1692  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1693  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1694  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1695  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1698  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1704  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1705  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1706  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1757  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1758  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1759  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1760  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1761  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1762  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1763  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/public/assets/js/chat.js:1764  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] ai-chat/public/assets/js/chat.js:35  [MAGIC_NUMBER]  Bare numeric literal 4096
[LOW] ai-chat/public/assets/js/chat.js:338  [MAGIC_NUMBER]  Bare numeric literal 300
[LOW] ai-chat/public/assets/js/chat.js:348  [MAGIC_NUMBER]  Bare numeric literal 60
[LOW] ai-chat/public/assets/js/chat.js:362  [MAGIC_NUMBER]  Bare numeric literal 1000
[LOW] ai-chat/public/assets/js/chat.js:416  [MAGIC_NUMBER]  Bare numeric literal 300
[LOW] ai-chat/public/assets/js/chat.js:470  [MAGIC_NUMBER]  Bare numeric literal 300
[LOW] ai-chat/public/assets/js/chat.js:841  [MAGIC_NUMBER]  Bare numeric literal 60
[LOW] ai-chat/public/assets/js/chat.js:950  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] ai-chat/public/assets/js/chat.js:987  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] ai-chat/public/assets/js/chat.js:1231  [MAGIC_NUMBER]  Bare numeric literal 160
[LOW] ai-chat/public/assets/js/chat.js:1246  [MAGIC_NUMBER]  Bare numeric literal 1200
[LOW] ai-chat/public/assets/js/chat.js:1263  [MAGIC_NUMBER]  Bare numeric literal 1200
[LOW] ai-chat/public/assets/js/chat.js:1274  [MAGIC_NUMBER]  Bare numeric literal 86400000
[LOW] ai-chat/public/assets/js/chat.js:1277  [MAGIC_NUMBER]  Bare numeric literal 604800000
[LOW] ai-chat/public/assets/js/chat.js:1362  [MAGIC_NUMBER]  Bare numeric literal 12000
[LOW] ai-chat/public/assets/js/chat.js:1363  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] ai-chat/public/assets/js/chat.js:1407  [MAGIC_NUMBER]  Bare numeric literal 1024
[LOW] ai-chat/public/assets/js/chat.js:1408  [MAGIC_NUMBER]  Bare numeric literal 1024
[LOW] ai-chat/public/assets/js/chat.js:1409  [MAGIC_NUMBER]  Bare numeric literal 1024
[LOW] ai-chat/public/assets/js/chat.js:1470  [MAGIC_NUMBER]  Bare numeric literal 5000
[LOW] ai-chat/public/assets/js/chat.js:1471  [MAGIC_NUMBER]  Bare numeric literal 5000
[LOW] ai-chat/public/assets/js/chat.js:1556  [MAGIC_NUMBER]  Bare numeric literal 60
[LOW] ai-chat/public/assets/js/chat.js:1613  [MAGIC_NUMBER]  Bare numeric literal 256
[LOW] ai-chat/public/assets/js/chat.js:1647  [MAGIC_NUMBER]  Bare numeric literal 256
[LOW] ai-chat/public/assets/js/chat.js:1757  [MAGIC_NUMBER]  Bare numeric literal 60
[LOW] ai-chat/public/assets/js/chat.js:1837  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] ai-chat/public/assets/js/chat.js:1979  [MAGIC_NUMBER]  Bare numeric literal 1500
[MEDIUM] ai-chat/public/assets/js/chat.js:1257  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1240–1245
[MEDIUM] ai-chat/public/assets/js/chat.js:1258  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1241–1246
[MEDIUM] ai-chat/public/assets/js/chat.js:1706  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 906–911
[MEDIUM] ai-chat/public/assets/js/chat.js:1707  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 907–912
[MEDIUM] ai-chat/public/assets/js/chat.js:1708  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 908–913
[MEDIUM] ai-chat/public/assets/js/chat.js:1709  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 909–914
[MEDIUM] ai-chat/public/assets/js/chat.js:1710  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 910–915
[MEDIUM] ai-chat/public/assets/js/chat.js:1711  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 911–916
[LOW] ai-chat/src/UserManager.php:22  [MAGIC_NUMBER]  Bare numeric literal 755
[LOW] ai-chat/src/UserManager.php:50  [MAGIC_NUMBER]  Bare numeric literal 12
[LOW] ai-chat/src/UserManager.php:231  [MAGIC_NUMBER]  Bare numeric literal 755
[MEDIUM] ai-chat/src/UserManager.php:121  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 107–112
[MEDIUM] ai-chat/src/UserManager.php:137  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 107–112
[MEDIUM] ai-chat/src/UserManager.php:181  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 107–112
[HIGH] ai-chat/src/ChatRouter.php:55  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:56  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:58  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:59  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:61  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:62  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:64  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:65  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:67  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:68  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:70  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:71  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:73  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:74  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:76  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:77  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:79  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:80  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:82  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:83  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:85  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:86  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:88  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:89  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:91  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:92  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:94  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:95  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:97  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:98  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:100  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:101  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:103  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:104  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:106  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:107  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:109  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:110  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:112  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:113  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:115  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:116  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:118  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:119  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:121  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:122  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:124  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:125  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:127  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:128  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:130  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:244  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:245  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:246  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:247  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:265  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:266  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:267  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:268  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:269  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:642  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:643  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:871  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:872  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:873  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:874  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:875  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:889  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:890  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:891  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:892  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:893  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:894  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:895  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:896  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:897  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:898  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:899  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:900  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:922  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:923  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:924  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:925  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:926  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:927  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:940  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:941  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:942  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:943  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:944  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:969  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:972  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:973  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:974  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:975  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:976  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ChatRouter.php:977  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] ai-chat/src/ChatRouter.php:130  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] ai-chat/src/ChatRouter.php:157  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] ai-chat/src/ChatRouter.php:162  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] ai-chat/src/ChatRouter.php:170  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] ai-chat/src/ChatRouter.php:200  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] ai-chat/src/ChatRouter.php:205  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] ai-chat/src/ChatRouter.php:213  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] ai-chat/src/ChatRouter.php:281  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] ai-chat/src/ChatRouter.php:286  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] ai-chat/src/ChatRouter.php:297  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] ai-chat/src/ChatRouter.php:358  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] ai-chat/src/ChatRouter.php:400  [MAGIC_NUMBER]  Bare numeric literal 42000
[LOW] ai-chat/src/ChatRouter.php:468  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] ai-chat/src/ChatRouter.php:547  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] ai-chat/src/ChatRouter.php:566  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] ai-chat/src/ChatRouter.php:580  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] ai-chat/src/ChatRouter.php:617  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] ai-chat/src/ChatRouter.php:622  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] ai-chat/src/ChatRouter.php:700  [MAGIC_NUMBER]  Bare numeric literal 502
[LOW] ai-chat/src/ChatRouter.php:744  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] ai-chat/src/ChatRouter.php:749  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] ai-chat/src/ChatRouter.php:751  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] ai-chat/src/ChatRouter.php:759  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] ai-chat/src/ChatRouter.php:798  [MAGIC_NUMBER]  Bare numeric literal 502
[LOW] ai-chat/src/ChatRouter.php:804  [MAGIC_NUMBER]  Bare numeric literal 502
[LOW] ai-chat/src/ChatRouter.php:821  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] ai-chat/src/ChatRouter.php:826  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] ai-chat/src/ChatRouter.php:828  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] ai-chat/src/ChatRouter.php:860  [MAGIC_NUMBER]  Bare numeric literal 8859
[LOW] ai-chat/src/ChatRouter.php:902  [MAGIC_NUMBER]  Bare numeric literal 501
[LOW] ai-chat/src/ChatRouter.php:954  [MAGIC_NUMBER]  Bare numeric literal 415
[LOW] ai-chat/src/ChatRouter.php:1078  [MAGIC_NUMBER]  Bare numeric literal 401
[MEDIUM] ai-chat/src/ChatRouter.php:198  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 155–160
[MEDIUM] ai-chat/src/ChatRouter.php:199  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 156–161
[MEDIUM] ai-chat/src/ChatRouter.php:200  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 157–162
[MEDIUM] ai-chat/src/ChatRouter.php:201  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 158–163
[MEDIUM] ai-chat/src/ChatRouter.php:202  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 159–164
[MEDIUM] ai-chat/src/ChatRouter.php:203  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 160–165
[MEDIUM] ai-chat/src/ChatRouter.php:357  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 285–290
[MEDIUM] ai-chat/src/ChatRouter.php:358  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 286–291
[MEDIUM] ai-chat/src/ChatRouter.php:359  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 287–292
[MEDIUM] ai-chat/src/ChatRouter.php:364  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 300–305
[MEDIUM] ai-chat/src/ChatRouter.php:365  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 301–306
[MEDIUM] ai-chat/src/ChatRouter.php:366  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 302–307
[MEDIUM] ai-chat/src/ChatRouter.php:564  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 545–550
[MEDIUM] ai-chat/src/ChatRouter.php:575  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 561–566
[MEDIUM] ai-chat/src/ChatRouter.php:576  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 562–567
[MEDIUM] ai-chat/src/ChatRouter.php:577  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 563–568
[MEDIUM] ai-chat/src/ChatRouter.php:578  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 545–550
[MEDIUM] ai-chat/src/ChatRouter.php:615  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 545–550
[MEDIUM] ai-chat/src/ChatRouter.php:918  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 867–872
[MEDIUM] ai-chat/src/ChatRouter.php:919  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 868–873
[MEDIUM] ai-chat/src/ChatRouter.php:920  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 869–874
[MEDIUM] ai-chat/src/ChatRouter.php:921  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 870–875
[MEDIUM] ai-chat/src/ChatRouter.php:937  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 889–894
[MEDIUM] ai-chat/src/ChatRouter.php:938  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 890–895
[MEDIUM] ai-chat/src/ChatRouter.php:939  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 891–896
[LOW] ai-chat/src/Bootstrap.php:81  [MAGIC_NUMBER]  Bare numeric literal 755
[HIGH] ai-chat/src/ApiClient.php:107  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ApiClient.php:108  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ApiClient.php:109  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/src/ApiClient.php:110  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ApiClient.php:111  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/ApiClient.php:112  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/src/ApiClient.php:113  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/src/ApiClient.php:114  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/src/ApiClient.php:115  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] ai-chat/src/ApiClient.php:116  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] ai-chat/src/ApiClient.php:117  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] ai-chat/src/ApiClient.php:22  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] ai-chat/src/ApiClient.php:23  [MAGIC_NUMBER]  Bare numeric literal 120
[MEDIUM] ai-chat/src/ApiClient.php:345  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 88–93
[LOW] ai-chat/src/ChatHistory.php:23  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] ai-chat/src/ChatHistory.php:27  [MAGIC_NUMBER]  Bare numeric literal 755
[LOW] ai-chat/src/ChatHistory.php:262  [MAGIC_NUMBER]  Bare numeric literal 755
[LOW] ai-chat/src/ChatHistory.php:312  [MAGIC_NUMBER]  Bare numeric literal 755
[LOW] ai-chat/src/ChatHistory.php:385  [MAGIC_NUMBER]  Bare numeric literal 60
[LOW] ai-chat/src/ChatHistory.php:386  [MAGIC_NUMBER]  Bare numeric literal 57
[LOW] ai-chat/src/ChatHistory.php:393  [MAGIC_NUMBER]  Bare numeric literal 12
[MEDIUM] ai-chat/src/ChatHistory.php:308  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 258–263
[MEDIUM] ai-chat/src/ChatHistory.php:309  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 259–264
[HIGH] ai-chat/src/DeviceTracker.php:64  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/DeviceTracker.php:65  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/DeviceTracker.php:201  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] ai-chat/src/DeviceTracker.php:204  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] ai-chat/src/DeviceTracker.php:22  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] ai-chat/src/DeviceTracker.php:25  [MAGIC_NUMBER]  Bare numeric literal 755
[LOW] ai-chat/src/DeviceTracker.php:237  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] ai-chat/src/DeviceTracker.php:238  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] ai-chat/src/TwoFactorAuth.php:26  [MAGIC_NUMBER]  Bare numeric literal 300
[LOW] ai-chat/src/TwoFactorAuth.php:30  [MAGIC_NUMBER]  Bare numeric literal 755
[LOW] ai-chat/src/EmailService.php:34  [MAGIC_NUMBER]  Bare numeric literal 755
[LOW] ai-chat/src/EmailService.php:186  [MAGIC_NUMBER]  Bare numeric literal 465
[LOW] ai-chat/src/EmailService.php:193  [MAGIC_NUMBER]  Bare numeric literal 15
[MEDIUM] ai-chat/src/EmailService.php:77  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 48–53
[MEDIUM] ai-chat/src/EmailService.php:91  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 54–59
[MEDIUM] ai-chat/src/EmailService.php:92  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 55–60
[MEDIUM] ai-chat/src/EmailService.php:124  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 55–60
[HIGH] xcm-builder/bridge.py:120  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] xcm-builder/bridge.py:122  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[MEDIUM] xcm-builder/bridge.py:269  [LONG_METHOD]  Method body ~61 lines (max 60)
[LOW] xcm-builder/bridge.py:88  [MAGIC_NUMBER]  Bare numeric literal 85
[LOW] xcm-builder/bridge.py:273  [MAGIC_NUMBER]  Bare numeric literal 8765
[LOW] xcm-builder/bridge.py:283  [MAGIC_NUMBER]  Bare numeric literal 82
[LOW] xcm-builder/bridge.py:290  [MAGIC_NUMBER]  Bare numeric literal 8765
[LOW] xcm-builder/bridge.py:359  [MAGIC_NUMBER]  Bare numeric literal 85
[HIGH] xcm-builder/bridge.py:300  [EMPTY_CATCH]  Empty except block (swallowed exception)
[HIGH] routes/api.php:85  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] routes/api.php:86  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] routes/api.php:87  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] routes/api.php:88  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] routes/api.php:89  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] routes/api.php:90  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] routes/api.php:91  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] routes/api.php:95  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] routes/api.php:96  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] routes/api.php:97  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] routes/api.php:98  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] routes/api.php:99  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] routes/api.php:757  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] routes/api.php:758  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] routes/api.php:759  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] routes/api.php:760  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] routes/api.php:799  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] routes/api.php:800  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] routes/api.php:801  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] routes/api.php:802  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] routes/api.php:816  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] routes/api.php:817  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] routes/api.php:818  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] routes/api.php:825  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] routes/api.php:826  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] routes/api.php:850  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] routes/api.php:1236  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] routes/api.php:1237  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] routes/api.php:52  [MAGIC_NUMBER]  Bare numeric literal 502
[LOW] routes/api.php:58  [MAGIC_NUMBER]  Bare numeric literal 502
[LOW] routes/api.php:131  [MAGIC_NUMBER]  Bare numeric literal 502
[LOW] routes/api.php:137  [MAGIC_NUMBER]  Bare numeric literal 502
[LOW] routes/api.php:200  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:211  [MAGIC_NUMBER]  Bare numeric literal 201
[LOW] routes/api.php:217  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] routes/api.php:237  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] routes/api.php:255  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] routes/api.php:268  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] routes/api.php:364  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:368  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:378  [MAGIC_NUMBER]  Bare numeric literal 409
[LOW] routes/api.php:383  [MAGIC_NUMBER]  Bare numeric literal 12
[LOW] routes/api.php:416  [MAGIC_NUMBER]  Bare numeric literal 300
[LOW] routes/api.php:443  [MAGIC_NUMBER]  Bare numeric literal 201
[LOW] routes/api.php:457  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:461  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:470  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] routes/api.php:489  [MAGIC_NUMBER]  Bare numeric literal 300
[LOW] routes/api.php:529  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:538  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:543  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:550  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:560  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:570  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] routes/api.php:616  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] routes/api.php:618  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] routes/api.php:654  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:664  [MAGIC_NUMBER]  Bare numeric literal 300
[LOW] routes/api.php:672  [MAGIC_NUMBER]  Bare numeric literal 300
[LOW] routes/api.php:702  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] routes/api.php:730  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:735  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:742  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:759  [MAGIC_NUMBER]  Bare numeric literal 403
[LOW] routes/api.php:860  [MAGIC_NUMBER]  Bare numeric literal 502
[LOW] routes/api.php:940  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:947  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:955  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:969  [MAGIC_NUMBER]  Bare numeric literal 502
[LOW] routes/api.php:993  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:1010  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:1018  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:1022  [MAGIC_NUMBER]  Bare numeric literal 201
[LOW] routes/api.php:1031  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:1110  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] routes/api.php:1118  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:1137  [MAGIC_NUMBER]  Bare numeric literal 402
[LOW] routes/api.php:1144  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] routes/api.php:1163  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] routes/api.php:1171  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:1182  [MAGIC_NUMBER]  Bare numeric literal 402
[LOW] routes/api.php:1195  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] routes/api.php:1202  [MAGIC_NUMBER]  Bare numeric literal 30
[LOW] routes/api.php:1205  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:1224  [MAGIC_NUMBER]  Bare numeric literal 201
[LOW] routes/api.php:1307  [MAGIC_NUMBER]  Bare numeric literal 300
[LOW] routes/api.php:1341  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:1345  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:1353  [MAGIC_NUMBER]  Bare numeric literal 409
[LOW] routes/api.php:1357  [MAGIC_NUMBER]  Bare numeric literal 12
[LOW] routes/api.php:1386  [MAGIC_NUMBER]  Bare numeric literal 300
[LOW] routes/api.php:1388  [MAGIC_NUMBER]  Bare numeric literal 201
[LOW] routes/api.php:1406  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:1415  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] routes/api.php:1431  [MAGIC_NUMBER]  Bare numeric literal 300
[LOW] routes/api.php:1453  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:1469  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:1473  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:1478  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:1487  [MAGIC_NUMBER]  Bare numeric literal 429
[LOW] routes/api.php:1495  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] routes/api.php:1558  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:1567  [MAGIC_NUMBER]  Bare numeric literal 300
[LOW] routes/api.php:1657  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:1661  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:1670  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] routes/api.php:1675  [MAGIC_NUMBER]  Bare numeric literal 12
[LOW] routes/api.php:1735  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:1744  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:1765  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:1774  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:1795  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:1844  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:1852  [MAGIC_NUMBER]  Bare numeric literal 403
[LOW] routes/api.php:1872  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:1888  [MAGIC_NUMBER]  Bare numeric literal 403
[LOW] routes/api.php:1905  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:1914  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:1936  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:1940  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:1952  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:1956  [MAGIC_NUMBER]  Bare numeric literal 201
[LOW] routes/api.php:1969  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:1989  [MAGIC_NUMBER]  Bare numeric literal 403
[LOW] routes/api.php:2000  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:2004  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:2022  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:2026  [MAGIC_NUMBER]  Bare numeric literal 201
[LOW] routes/api.php:2037  [MAGIC_NUMBER]  Bare numeric literal 403
[LOW] routes/api.php:2045  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:2054  [MAGIC_NUMBER]  Bare numeric literal 400
[LOW] routes/api.php:2078  [MAGIC_NUMBER]  Bare numeric literal 400
[MEDIUM] routes/api.php:125  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 46–51
[MEDIUM] routes/api.php:126  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 47–52
[MEDIUM] routes/api.php:127  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 48–53
[MEDIUM] routes/api.php:128  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 49–54
[MEDIUM] routes/api.php:129  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 50–55
[MEDIUM] routes/api.php:130  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 51–56
[MEDIUM] routes/api.php:131  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 52–57
[MEDIUM] routes/api.php:132  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 53–58
[MEDIUM] routes/api.php:133  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 54–59
[MEDIUM] routes/api.php:134  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 55–60
[MEDIUM] routes/api.php:135  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 56–61
[MEDIUM] routes/api.php:136  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 57–62
[MEDIUM] routes/api.php:235  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 215–220
[MEDIUM] routes/api.php:236  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 216–221
[MEDIUM] routes/api.php:237  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 217–222
[MEDIUM] routes/api.php:253  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 215–220
[MEDIUM] routes/api.php:254  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 216–221
[MEDIUM] routes/api.php:266  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 215–220
[MEDIUM] routes/api.php:484  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 411–416
[MEDIUM] routes/api.php:485  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 412–417
[MEDIUM] routes/api.php:486  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 413–418
[MEDIUM] routes/api.php:487  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 414–419
[MEDIUM] routes/api.php:488  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 415–420
[MEDIUM] routes/api.php:489  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 416–421
[MEDIUM] routes/api.php:490  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 417–422
[MEDIUM] routes/api.php:491  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 418–423
[MEDIUM] routes/api.php:492  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 419–424
[MEDIUM] routes/api.php:493  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 420–425
[MEDIUM] routes/api.php:494  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 421–426
[MEDIUM] routes/api.php:495  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 422–427
[MEDIUM] routes/api.php:496  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 423–428
[MEDIUM] routes/api.php:497  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 424–429
[MEDIUM] routes/api.php:498  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 425–430
[MEDIUM] routes/api.php:499  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 426–431
[MEDIUM] routes/api.php:631  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 508–513
[MEDIUM] routes/api.php:632  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 509–514
[MEDIUM] routes/api.php:633  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 510–515
[MEDIUM] routes/api.php:655  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 530–535
[MEDIUM] routes/api.php:669  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 413–418
[MEDIUM] routes/api.php:670  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 414–419
[MEDIUM] routes/api.php:671  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 415–420
[MEDIUM] routes/api.php:672  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 416–421
[MEDIUM] routes/api.php:673  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 417–422
[MEDIUM] routes/api.php:674  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 418–423
[MEDIUM] routes/api.php:675  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 419–424
[MEDIUM] routes/api.php:676  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 420–425
[MEDIUM] routes/api.php:677  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 421–426
[MEDIUM] routes/api.php:678  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 422–427
[MEDIUM] routes/api.php:679  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 423–428
[MEDIUM] routes/api.php:680  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 424–429
[MEDIUM] routes/api.php:681  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 425–430
[MEDIUM] routes/api.php:682  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 426–431
[MEDIUM] routes/api.php:683  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 500–505
[MEDIUM] routes/api.php:700  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 215–220
[MEDIUM] routes/api.php:1032  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 994–999
[MEDIUM] routes/api.php:1108  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 215–220
[MEDIUM] routes/api.php:1142  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 215–220
[MEDIUM] routes/api.php:1143  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 216–221
[MEDIUM] routes/api.php:1161  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 215–220
[MEDIUM] routes/api.php:1193  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 215–220
[MEDIUM] routes/api.php:1339  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 362–367
[MEDIUM] routes/api.php:1340  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 363–368
[MEDIUM] routes/api.php:1341  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 364–369
[MEDIUM] routes/api.php:1342  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 365–370
[MEDIUM] routes/api.php:1343  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 366–371
[MEDIUM] routes/api.php:1421  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 477–482
[MEDIUM] routes/api.php:1422  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 478–483
[MEDIUM] routes/api.php:1423  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 479–484
[MEDIUM] routes/api.php:1451  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 527–532
[MEDIUM] routes/api.php:1454  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1407–1412
[MEDIUM] routes/api.php:1455  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1408–1413
[MEDIUM] routes/api.php:1559  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1407–1412
[MEDIUM] routes/api.php:1560  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1408–1413
[MEDIUM] routes/api.php:1757  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1727–1732
[MEDIUM] routes/api.php:1758  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1728–1733
[MEDIUM] routes/api.php:1759  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1729–1734
[MEDIUM] routes/api.php:1760  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1730–1735
[MEDIUM] routes/api.php:1761  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1731–1736
[MEDIUM] routes/api.php:1762  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1732–1737
[MEDIUM] routes/api.php:1763  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1733–1738
[MEDIUM] routes/api.php:1764  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1734–1739
[MEDIUM] routes/api.php:1765  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1735–1740
[MEDIUM] routes/api.php:1772  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1742–1747
[MEDIUM] routes/api.php:1786  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1705–1710
[MEDIUM] routes/api.php:1793  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1742–1747
[MEDIUM] routes/api.php:1901  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1868–1873
[MEDIUM] routes/api.php:1902  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1869–1874
[MEDIUM] routes/api.php:1903  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1870–1875
[MEDIUM] routes/api.php:1912  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1742–1747
[MEDIUM] routes/api.php:1966  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1555–1560
[MEDIUM] routes/api.php:1967  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1556–1561
[MEDIUM] routes/api.php:1998  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1934–1939
[MEDIUM] routes/api.php:1999  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1935–1940
[MEDIUM] routes/api.php:2000  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1936–1941
[MEDIUM] routes/api.php:2001  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1937–1942
[MEDIUM] routes/api.php:2002  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1938–1943
[MEDIUM] routes/api.php:2003  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1939–1944
[MEDIUM] routes/api.php:2004  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1940–1945
[MEDIUM] routes/api.php:2020  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1742–1747
[MEDIUM] routes/api.php:2035  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1987–1992
[MEDIUM] routes/api.php:2043  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1842–1847
[MEDIUM] routes/api.php:2044  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1843–1848
[MEDIUM] routes/api.php:2045  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1844–1849
[MEDIUM] routes/api.php:2052  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 1742–1747
[LOW] src/Logger.php:30  [MAGIC_NUMBER]  Bare numeric literal 755
[LOW] src/Response.php:55  [MAGIC_NUMBER]  Bare numeric literal 400
[MEDIUM] src/Response.php:63  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 49–54
[MEDIUM] src/Response.php:80  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 49–54
[LOW] src/Bootstrap.php:102  [MAGIC_NUMBER]  Bare numeric literal 755
[HIGH] src/Request.php:120  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Request.php:121  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Router.php:120  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Router.php:121  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] src/Router.php:54  [MAGIC_NUMBER]  Bare numeric literal 204
[MEDIUM] src/Router.php:83  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 75–80
[LOW] src/Middleware/SessionMiddleware.php:59  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] src/Middleware/SessionMiddleware.php:67  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] src/Middleware/AuthMiddleware.php:78  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] src/Middleware/AuthMiddleware.php:84  [MAGIC_NUMBER]  Bare numeric literal 401
[LOW] src/Middleware/AuthMiddleware.php:97  [MAGIC_NUMBER]  Bare numeric literal 429
[LOW] src/Middleware/AuthMiddleware.php:107  [MAGIC_NUMBER]  Bare numeric literal 429
[LOW] src/Auth/RateLimiter.php:23  [MAGIC_NUMBER]  Bare numeric literal 60
[LOW] src/Auth/RateLimiter.php:24  [MAGIC_NUMBER]  Bare numeric literal 1000
[LOW] src/Auth/RateLimiter.php:49  [MAGIC_NUMBER]  Bare numeric literal 3600
[LOW] src/Auth/RateLimiter.php:55  [MAGIC_NUMBER]  Bare numeric literal 60
[LOW] src/Auth/RateLimiter.php:65  [MAGIC_NUMBER]  Bare numeric literal 60
[LOW] src/Auth/RateLimiter.php:77  [MAGIC_NUMBER]  Bare numeric literal 3600
[LOW] src/Auth/RateLimiter.php:121  [MAGIC_NUMBER]  Bare numeric literal 3600
[LOW] src/Auth/RateLimiter.php:125  [MAGIC_NUMBER]  Bare numeric literal 60
[MEDIUM] src/Auth/RateLimiter.php:104  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 34–39
[HIGH] src/Auth/SessionManager.php:136  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Auth/SessionManager.php:137  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] src/Auth/SessionManager.php:32  [MAGIC_NUMBER]  Bare numeric literal 86400
[LOW] src/Auth/SessionManager.php:33  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] src/Auth/SessionManager.php:47  [MAGIC_NUMBER]  Bare numeric literal 64
[LOW] src/Auth/SessionManager.php:49  [MAGIC_NUMBER]  Bare numeric literal 24
[LOW] src/Auth/ApiKeyManager.php:25  [MAGIC_NUMBER]  Bare numeric literal 64
[LOW] src/Auth/ApiKeyManager.php:40  [MAGIC_NUMBER]  Bare numeric literal 16
[HIGH] src/Providers/OpenAIProvider.php:127  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Providers/OpenAIProvider.php:128  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Providers/OpenAIProvider.php:133  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Providers/OpenAIProvider.php:134  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] src/Providers/OpenAIProvider.php:65  [MAGIC_NUMBER]  Bare numeric literal 120
[LOW] src/Providers/OpenAIProvider.php:137  [MAGIC_NUMBER]  Bare numeric literal 120
[MEDIUM] src/Providers/OpenAIProvider.php:98  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 25–30
[MEDIUM] src/Providers/OpenAIProvider.php:99  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 26–31
[MEDIUM] src/Providers/OpenAIProvider.php:100  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 27–32
[MEDIUM] src/Providers/OpenAIProvider.php:101  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 28–33
[MEDIUM] src/Providers/OpenAIProvider.php:102  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 29–34
[MEDIUM] src/Providers/OpenAIProvider.php:103  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 30–35
[MEDIUM] src/Providers/OpenAIProvider.php:104  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 31–36
[MEDIUM] src/Providers/OpenAIProvider.php:111  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 37–42
[MEDIUM] src/Providers/OpenAIProvider.php:115  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 57–62
[MEDIUM] src/Providers/OpenAIProvider.php:116  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 58–63
[MEDIUM] src/Providers/OpenAIProvider.php:117  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 59–64
[HIGH] src/Providers/AnthropicProvider.php:80  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Providers/AnthropicProvider.php:150  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Providers/AnthropicProvider.php:151  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] src/Providers/AnthropicProvider.php:152  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] src/Providers/AnthropicProvider.php:153  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Providers/AnthropicProvider.php:155  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Providers/AnthropicProvider.php:156  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] src/Providers/AnthropicProvider.php:157  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] src/Providers/AnthropicProvider.php:158  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] src/Providers/AnthropicProvider.php:69  [MAGIC_NUMBER]  Bare numeric literal 120
[LOW] src/Providers/AnthropicProvider.php:161  [MAGIC_NUMBER]  Bare numeric literal 120
[MEDIUM] src/Providers/AnthropicProvider.php:114  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 30–35
[MEDIUM] src/Providers/AnthropicProvider.php:115  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 31–36
[MEDIUM] src/Providers/AnthropicProvider.php:116  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 32–37
[MEDIUM] src/Providers/AnthropicProvider.php:117  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 33–38
[MEDIUM] src/Providers/AnthropicProvider.php:118  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 34–39
[MEDIUM] src/Providers/AnthropicProvider.php:119  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 35–40
[MEDIUM] src/Providers/AnthropicProvider.php:120  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 36–41
[MEDIUM] src/Providers/AnthropicProvider.php:121  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 37–42
[MEDIUM] src/Providers/AnthropicProvider.php:122  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 38–43
[MEDIUM] src/Providers/AnthropicProvider.php:129  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 44–49
[MEDIUM] src/Providers/AnthropicProvider.php:130  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 45–50
[MEDIUM] src/Providers/AnthropicProvider.php:131  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 46–51
[MEDIUM] src/Providers/AnthropicProvider.php:132  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 47–52
[MEDIUM] src/Providers/AnthropicProvider.php:133  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 48–53
[MEDIUM] src/Providers/AnthropicProvider.php:137  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 60–65
[MEDIUM] src/Providers/AnthropicProvider.php:138  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 61–66
[MEDIUM] src/Providers/AnthropicProvider.php:139  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 62–67
[MEDIUM] src/Providers/AnthropicProvider.php:140  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 63–68
[HIGH] src/Providers/BaseProvider.php:84  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Providers/BaseProvider.php:85  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Providers/BaseProvider.php:89  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Providers/BaseProvider.php:90  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Providers/BaseProvider.php:91  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Providers/BaseProvider.php:106  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Providers/BaseProvider.php:107  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Providers/BaseProvider.php:108  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Providers/BaseProvider.php:156  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Providers/BaseProvider.php:157  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Providers/BaseProvider.php:158  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Providers/BaseProvider.php:159  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Providers/BaseProvider.php:160  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] src/Providers/BaseProvider.php:161  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Providers/BaseProvider.php:162  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Providers/BaseProvider.php:163  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] src/Providers/BaseProvider.php:164  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] src/Providers/BaseProvider.php:165  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] src/Providers/BaseProvider.php:166  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] src/Providers/BaseProvider.php:167  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] src/Providers/BaseProvider.php:168  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] src/Providers/BaseProvider.php:37  [MAGIC_NUMBER]  Bare numeric literal 1000
[LOW] src/Providers/BaseProvider.php:38  [MAGIC_NUMBER]  Bare numeric literal 1000
[LOW] src/Providers/BaseProvider.php:48  [MAGIC_NUMBER]  Bare numeric literal 120
[LOW] src/Providers/BaseProvider.php:84  [MAGIC_NUMBER]  Bare numeric literal 100000
[LOW] src/Providers/BaseProvider.php:96  [MAGIC_NUMBER]  Bare numeric literal 100000
[LOW] src/Providers/BaseProvider.php:104  [MAGIC_NUMBER]  Bare numeric literal 300
[LOW] src/Providers/BaseProvider.php:133  [MAGIC_NUMBER]  Bare numeric literal 120
[MEDIUM] src/Providers/BaseProvider.php:135  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 50–55
[MEDIUM] src/Providers/BaseProvider.php:136  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 51–56
[HIGH] src/Providers/ProviderFactory.php:89  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Providers/ProviderFactory.php:90  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Providers/DeepseekProvider.php:131  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Providers/DeepseekProvider.php:132  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Providers/DeepseekProvider.php:137  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Providers/DeepseekProvider.php:138  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Providers/DeepseekProvider.php:143  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Providers/DeepseekProvider.php:144  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] src/Providers/DeepseekProvider.php:62  [MAGIC_NUMBER]  Bare numeric literal 120
[LOW] src/Providers/DeepseekProvider.php:147  [MAGIC_NUMBER]  Bare numeric literal 120
[MEDIUM] src/Providers/DeepseekProvider.php:106  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 26–31
[MEDIUM] src/Providers/DeepseekProvider.php:107  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 27–32
[MEDIUM] src/Providers/DeepseekProvider.php:108  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 28–33
[MEDIUM] src/Providers/DeepseekProvider.php:115  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 34–39
[MEDIUM] src/Providers/DeepseekProvider.php:119  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 54–59
[MEDIUM] src/Providers/DeepseekProvider.php:120  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 55–60
[MEDIUM] src/Providers/DeepseekProvider.php:121  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 56–61
[LOW] src/Storage/DatabaseStorage.php:26  [MAGIC_NUMBER]  Bare numeric literal 3306
[LOW] src/Storage/DatabaseStorage.php:149  [MAGIC_NUMBER]  Bare numeric literal 64
[LOW] src/Storage/DatabaseStorage.php:150  [MAGIC_NUMBER]  Bare numeric literal 255
[LOW] src/Storage/FileStorage.php:21  [MAGIC_NUMBER]  Bare numeric literal 755
[LOW] src/Storage/FileStorage.php:29  [MAGIC_NUMBER]  Bare numeric literal 755
[LOW] src/Storage/FileStorage.php:115  [MAGIC_NUMBER]  Bare numeric literal 755
[HIGH] src/Storage/StorageFactory.php:36  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/RAGService.php:52  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/RAGService.php:53  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/RAGService.php:54  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/RAGService.php:55  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/RAGService.php:56  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/RAGService.php:57  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/RAGService.php:156  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/RAGService.php:157  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/RAGService.php:158  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/RAGService.php:159  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/RAGService.php:244  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/RAGService.php:245  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/RAGService.php:324  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] src/Services/RAGService.php:282  [MAGIC_NUMBER]  Bare numeric literal 50
[MEDIUM] src/Services/RAGService.php:108  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 66–71
[MEDIUM] src/Services/RAGService.php:109  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 67–72
[MEDIUM] src/Services/RAGService.php:110  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 68–73
[MEDIUM] src/Services/RAGService.php:111  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 69–74
[MEDIUM] src/Services/RAGService.php:112  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 70–75
[MEDIUM] src/Services/RAGService.php:113  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 71–76
[LOW] src/Services/PaymentService.php:208  [MAGIC_NUMBER]  Bare numeric literal 16
[LOW] src/Services/PaymentService.php:228  [MAGIC_NUMBER]  Bare numeric literal 22
[LOW] src/Services/PaymentService.php:271  [MAGIC_NUMBER]  Bare numeric literal 300
[HIGH] src/Services/LicenseService.php:227  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/LicenseService.php:228  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/LicenseService.php:229  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/LicenseService.php:230  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/LicenseService.php:238  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/LicenseService.php:239  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/LicenseService.php:240  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/LicenseService.php:241  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/LicenseService.php:242  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/LicenseService.php:243  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/LicenseService.php:244  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/LicenseService.php:245  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/LicenseService.php:246  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/LicenseService.php:314  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/LicenseService.php:315  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/LicenseService.php:316  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/LicenseService.php:317  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/LicenseService.php:318  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/LicenseService.php:319  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] src/Services/LicenseService.php:73  [MAGIC_NUMBER]  Bare numeric literal 86400
[LOW] src/Services/LicenseService.php:221  [MAGIC_NUMBER]  Bare numeric literal 86400
[LOW] src/Services/LicenseService.php:235  [MAGIC_NUMBER]  Bare numeric literal 86400
[LOW] src/Services/LicenseService.php:299  [MAGIC_NUMBER]  Bare numeric literal 86400
[LOW] src/Services/LicenseService.php:583  [MAGIC_NUMBER]  Bare numeric literal 365
[LOW] src/Services/LicenseService.php:586  [MAGIC_NUMBER]  Bare numeric literal 730
[LOW] src/Services/LicenseService.php:602  [MAGIC_NUMBER]  Bare numeric literal 16
[LOW] src/Services/LicenseService.php:651  [MAGIC_NUMBER]  Bare numeric literal 300
[MEDIUM] src/Services/LicenseService.php:368  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 287–292
[MEDIUM] src/Services/LicenseService.php:369  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 288–293
[MEDIUM] src/Services/LicenseService.php:476  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 287–292
[MEDIUM] src/Services/LicenseService.php:477  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 288–293
[MEDIUM] src/Services/LicenseService.php:478  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 370–375
[MEDIUM] src/Services/SubscriptionService.php:66  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 42–47
[MEDIUM] src/Services/SubscriptionService.php:67  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 43–48
[HIGH] src/Services/TemporalService.php:161  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/TemporalService.php:162  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/TemporalService.php:163  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/TemporalService.php:173  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/TemporalService.php:174  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] src/Services/TemporalService.php:175  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] src/Services/TemporalService.php:176  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] src/Services/TemporalService.php:177  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/Services/TemporalService.php:178  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] src/Services/TemporalService.php:101  [MAGIC_NUMBER]  Bare numeric literal 60
[LOW] src/Services/TemporalService.php:104  [MAGIC_NUMBER]  Bare numeric literal 3600
[LOW] src/Services/TemporalService.php:105  [MAGIC_NUMBER]  Bare numeric literal 60
[LOW] src/Services/TemporalService.php:108  [MAGIC_NUMBER]  Bare numeric literal 86400
[LOW] src/Services/TemporalService.php:109  [MAGIC_NUMBER]  Bare numeric literal 3600
[LOW] src/Services/TemporalService.php:112  [MAGIC_NUMBER]  Bare numeric literal 604800
[LOW] src/Services/TemporalService.php:113  [MAGIC_NUMBER]  Bare numeric literal 86400
[LOW] src/Services/TemporalService.php:144  [MAGIC_NUMBER]  Bare numeric literal 17
[LOW] src/Services/EmailService.php:153  [MAGIC_NUMBER]  Bare numeric literal 465
[LOW] src/Services/EmailService.php:158  [MAGIC_NUMBER]  Bare numeric literal 15
[MEDIUM] src/Services/EmailService.php:84  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 63–68
[LOW] src/Services/ConsultService.php:101  [MAGIC_NUMBER]  Bare numeric literal 255
[LOW] src/Services/ConsultService.php:102  [MAGIC_NUMBER]  Bare numeric literal 255
[LOW] src/Services/ConsultService.php:103  [MAGIC_NUMBER]  Bare numeric literal 255
[LOW] src/Services/ConsultService.php:106  [MAGIC_NUMBER]  Bare numeric literal 45
[LOW] src/Services/ConsultService.php:197  [MAGIC_NUMBER]  Bare numeric literal 3306
------------------------------------------------------------
Total findings: 3121
  HIGH: 1154
  MEDIUM: 641
  LOW: 1326
By type:
  DEEP_NESTING: 1147
  MAGIC_NUMBER: 1130
  DUPLICATE_CODE: 611
  DEAD_IMPORT: 196
  LONG_METHOD: 18
  LONG_PARAM_LIST: 12
  EMPTY_CATCH: 7
```
