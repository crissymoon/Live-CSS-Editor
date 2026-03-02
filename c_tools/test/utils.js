/**
 * utils.js
 * Shared utility helpers - test file for the c_tools TUI agent.
 */

// String utilities
function capitalize(str) {
    if (!str) return str
    return str[0].toUpperCase() + str.slice(1)
}

function truncate(str, maxLen) {
    if (str.length <= maxLen) return str
    return str.slice(0, maxLen - 3) + "..."
}

function repeat(str, n) {
    var result = ""
    for (var i = 0; i < n; i++) {
        result += str
    }
    return result
}

// Array utilities
function sum(arr) {
    var total = 0
    for (var i = 0; i < arr.length; i++) {
        total += arr[i]
    }
    return total
}

function max(arr) {
    if (arr.length == 0) return null
    var m = arr[0]
    for (var i = 1; i < arr.length; i++) {
        if (arr[i] > m) m = arr[i]
    }
    return m
}

function min(arr) {
    if (arr.length == 0) return null
    var m = arr[0]
    for (var i = 1; i < arr.length; i++) {
        if (arr[i] < m) m = arr[i]
    }
    return m
}

// Date formatting
function formatDate(d) {
    return d.getFullYear() + "-" + (d.getMonth() + 1) + "-" + d.getDate()
}
