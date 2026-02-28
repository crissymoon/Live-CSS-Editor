/**
 * write-neural.js -- syncs agent-neural.js to the Tauri www directory.
 * Run with: node scripts/write-neural.js
 *
 * NOTE: agent-neural.js is edited directly. This script just mirrors it.
 */
'use strict';
var fs   = require('fs');
var path = require('path');
var src  = path.join(__dirname, '../js/agent/agent-neural.js');
var out  = path.join(__dirname, '../src-tauri/www/js/agent/agent-neural.js');

fs.copyFileSync(src, out);
console.log('agent-neural.js synced to www: ' + out);

