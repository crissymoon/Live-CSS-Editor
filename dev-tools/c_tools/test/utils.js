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

function toLowerCase(str) {
    return str.toLowerCase()
}

function toUpperCase(str) {
    return str.toUpperCase()
}

function trim(str) {
    return str.trim()
}

function padLeft(str, n, char) {
    char = char || " "
    while (str.length < n) {
        str = char + str
    }
    return str
}

function padRight(str, n, char) {
    char = char || " "
    while (str.length < n) {
        str = str + char
    }
    return str
}

function reverse(str) {
    return str.split("").reverse().join("")
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

function average(arr) {
    if (arr.length == 0) return null
    return sum(arr) / arr.length
}

function flatten(arr) {
    var result = []
    for (var i = 0; i < arr.length; i++) {
        if (Array.isArray(arr[i])) {
            result = result.concat(flatten(arr[i]))
        } else {
            result.push(arr[i])
        }
    }
    return result
}

function unique(arr) {
    var result = []
    for (var i = 0; i < arr.length; i++) {
        if (result.indexOf(arr[i]) == -1) {
            result.push(arr[i])
        }
    }
    return result
}

function reverse(arr) {
    var result = []
    for (var i = arr.length - 1; i >= 0; i--) {
        result.push(arr[i])
    }
    return result
}

function contains(arr, item) {
    return arr.indexOf(item) != -1
}

// Date formatting
function formatDate(d) {
    return d.getFullYear() + "-" + (d.getMonth() + 1) + "-" + d.getDate()
}

function formatTime(d) {
    return padLeft(d.getHours().toString(), 2, "0") + ":" + padLeft(d.getMinutes().toString(), 2, "0") + ":" + padLeft(d.getSeconds().toString(), 2, "0")
}

function formatDateTime(d) {
    return formatDate(d) + " " + formatTime(d)
}

// Object utilities
function isEmpty(obj) {
    for (var key in obj) {
        if (obj.hasOwnProperty(key)) {
            return false
        }
    }
    return true
}

function keys(obj) {
    var result = []
    for (var key in obj) {
        if (obj.hasOwnProperty(key)) {
            result.push(key)
        }
    }
    return result
}

function values(obj) {
    var result = []
    for (var key in obj) {
        if (obj.hasOwnProperty(key)) {
            result.push(obj[key])
        }
    }
    return result
}

function merge(obj1, obj2) {
    var result = {}
    for (var key in obj1) {
        result[key] = obj1[key]
    }
    for (var key in obj2) {
        result[key] = obj2[key]
    }
    return result
}