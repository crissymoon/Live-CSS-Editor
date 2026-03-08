#!/usr/bin/env node
/*
 * watch-log.js -- Live tail of server.log
 * Crissy's Style Tool
 *
 * Usage:
 *   node watch-log.js              (watches server.log in this directory)
 *   node watch-log.js /path/to/server.log
 *
 * Prints new lines as they are appended, with colour coding by level.
 * Works on any platform that has Node. No npm deps.
 */
'use strict';

const fs   = require('fs');
const path = require('path');

// Default to server.log alongside this script (php-wasm-project/server.log)
const LOGFILE = process.argv[2] || path.join(__dirname, 'server.log');

// ANSI colour map keyed on log level prefix
const COLORS = {
    INFO      : '\x1b[36m',   // cyan
    WARN      : '\x1b[33m',   // yellow
    ERR       : '\x1b[31m',   // red
    FATAL     : '\x1b[41m',   // red bg
    PHP       : '\x1b[35m',   // magenta
    'PHP-ERR' : '\x1b[31m',   // red
    REQ       : '\x1b[32m',   // green
    BROWSER   : '\x1b[34m',   // blue
    'JS-ERROR': '\x1b[31m',   // red
    'JS-WARN' : '\x1b[33m',   // yellow
    'JS-UNCAUGHT': '\x1b[41m',// red bg
    'JS-REJECT': '\x1b[31m',  // red
    CLICK     : '\x1b[96m',   // bright cyan
    RESOURCE  : '\x1b[90m',   // grey
};
const RESET = '\x1b[0m';

function colorLine(line) {
    // Format: [iso-date] [LEVEL] message
    var m = line.match(/^\[([^\]]+)\] \[([^\]]+)\] (.*)$/);
    if (!m) return line;
    var ts    = m[1];
    var level = m[2];
    var msg   = m[3];
    var color = COLORS[level] || '\x1b[37m'; // white default
    return `\x1b[90m${ts}\x1b[0m ${color}[${level}]${RESET} ${msg}`;
}

// Create file if it does not exist yet
if (!fs.existsSync(LOGFILE)) {
    fs.writeFileSync(LOGFILE, '');
}

console.log(`\x1b[36mWatching: ${LOGFILE}\x1b[0m  (Ctrl-C to stop)\n`);

// Start reading from current end of file so we only see new entries
var pos = fs.statSync(LOGFILE).size;

fs.watchFile(LOGFILE, { interval: 250 }, (curr) => {
    if (curr.size <= pos) return; // truncated or no change
    const stream = fs.createReadStream(LOGFILE, { start: pos, end: curr.size - 1 });
    pos = curr.size;
    let buf = '';
    stream.on('data', (d) => { buf += d.toString(); });
    stream.on('end', () => {
        buf.split('\n').forEach((line) => {
            if (line.trim()) console.log(colorLine(line));
        });
    });
});
