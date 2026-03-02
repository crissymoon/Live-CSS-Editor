/**
 * sample.js
 * Test file for the c_tools TUI agent.
 * Ask the agent to refactor, add docs, fix bugs, etc.
 */

// Simple utility functions
function add(a, b) {
    return a + b
}

function subtract(a, b) {
    return a - b
}

function multiply(a, b) {
    return a * b
}

function divide(a, b) {
    if (b == 0) {
        console.log("error: division by zero")
        return null
    }
    return a / b
}

// Format a result
function formatResult(operation, a, b, result) {
    return operation + "(" + a + ", " + b + ") = " + result
}

// Run some calculations
var x = 10
var y = 5

console.log(formatResult("add",      x, y, add(x, y)))
console.log(formatResult("subtract", x, y, subtract(x, y)))
console.log(formatResult("multiply", x, y, multiply(x, y)))
console.log(formatResult("divide",   x, y, divide(x, y)))
console.log(formatResult("divide",   x, 0, divide(x, 0)))
/**
 * correlation.test.js
 * Test file for correlation math functions
 */

// Calculate the mean of an array
function mean(arr) {
    const sum = arr.reduce((a, b) => a + b, 0)
    return sum / arr.length
}

// Calculate the standard deviation
function standardDeviation(arr) {
    const avg = mean(arr)
    const squareDiffs = arr.map(val => Math.pow(val - avg, 2))
    const avgSquareDiff = mean(squareDiffs)
    return Math.sqrt(avgSquareDiff)
}

// Calculate correlation coefficient (Pearson)
function correlation(x, y) {
    if (x.length !== y.length) {
        console.log("error: arrays must have equal length")
        return null
    }
    
    const meanX = mean(x)
    const meanY = mean(y)
    const stdX = standardDeviation(x)
    const stdY = standardDeviation(y)
    
    if (stdX === 0 || stdY === 0) {
        console.log("error: standard deviation cannot be zero")
        return null
    }
    
    let numerator = 0
    for (let i = 0; i < x.length; i++) {
        numerator += (x[i] - meanX) * (y[i] - meanY)
    }
    
    const denominator = (x.length - 1) * stdX * stdY
    return numerator / denominator
}

// Test correlation
const testX = [1, 2, 3, 4, 5]
const testY = [2, 4, 5, 4, 6]

console.log("\n--- Correlation Tests ---")
console.log("Mean of X:", mean(testX))
console.log("Mean of Y:", mean(testY))
console.log("Std Dev of X:", standardDeviation(testX))
console.log("Std Dev of Y:", standardDeviation(testY))
console.log("Correlation coefficient:", correlation(testX, testY))

const perfectX = [1, 2, 3, 4, 5]
const perfectY = [2, 4, 6, 8, 10]
console.log("Correlation (perfect linear):", correlation(perfectX, perfectY))