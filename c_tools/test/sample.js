/**
 * sample.js
 * Advanced test file for the c_tools TUI agent.
 * Includes calculator, statistics, matrix operations, and data analysis.
 */

/**
 * Basic arithmetic operations with input validation
 */
const Calculator = {
  /**
   * Add two numbers
   * @param {number} a
   * @param {number} b
   * @returns {number}
   * @throws {TypeError} If inputs are not numbers
   */
  add(a, b) {
    this._validateNumbers(a, b);
    return a + b;
  },

  /**
   * Subtract two numbers
   * @param {number} a
   * @param {number} b
   * @returns {number}
   * @throws {TypeError} If inputs are not numbers
   */
  subtract(a, b) {
    this._validateNumbers(a, b);
    return a - b;
  },

  /**
   * Multiply two numbers
   * @param {number} a
   * @param {number} b
   * @returns {number}
   * @throws {TypeError} If inputs are not numbers
   */
  multiply(a, b) {
    this._validateNumbers(a, b);
    return a * b;
  },

  /**
   * Divide two numbers
   * @param {number} a
   * @param {number} b
   * @returns {number}
   * @throws {Error} If division by zero is attempted
   * @throws {TypeError} If inputs are not numbers
   */
  divide(a, b) {
    this._validateNumbers(a, b);
    if (b === 0) {
      throw new Error("Division by zero is not allowed");
    }
    return a / b;
  },

  /**
   * Calculate power
   * @param {number} base
   * @param {number} exponent
   * @returns {number}
   */
  power(base, exponent) {
    this._validateNumbers(base, exponent);
    return Math.pow(base, exponent);
  },

  /**
   * Internal validation helper
   * @private
   * @param {...number} args
   * @throws {TypeError}
   */
  _validateNumbers(...args) {
    args.forEach(arg => {
      if (typeof arg !== 'number' || isNaN(arg)) {
        throw new TypeError(`Expected number, got ${typeof arg}`);
      }
    });
  }
};

/**
 * Advanced matrix operations
 */
const Matrix = {
  /**
   * Create an identity matrix
   * @param {number} size
   * @returns {number[][]}
   */
  identity(size) {
    return Array.from({ length: size }, (_, i) =>
      Array.from({ length: size }, (_, j) => i === j ? 1 : 0)
    );
  },

  /**
   * Multiply two matrices
   * @param {number[][]} a
   * @param {number[][]} b
   * @returns {number[][]}
   * @throws {Error} If matrices are incompatible for multiplication
   */
  multiply(a, b) {
    if (a[0].length !== b.length) {
      throw new Error("Incompatible matrix dimensions for multiplication");
    }
    return a.map((row, i) =>
      b[0].map((_, j) =>
        row.reduce((sum, val, k) => sum + val * b[k][j], 0)
      )
    );
  },

  /**
   * Transpose a matrix
   * @param {number[][]} matrix
   * @returns {number[][]}
   */
  transpose(matrix) {
    return matrix[0].map((_, i) => matrix.map(row => row[i]));
  },

  /**
   * Calculate matrix determinant (2x2 and 3x3)
   * @param {number[][]} matrix
   * @returns {number}
   * @throws {Error} If matrix is not square or unsupported size
   */
  determinant(matrix) {
    const n = matrix.length;
    if (n !== matrix[0].length) {
      throw new Error("Matrix must be square");
    }
    if (n === 2) {
      return matrix[0][0] * matrix[1][1] - matrix[0][1] * matrix[1][0];
    }
    if (n === 3) {
      const [a, b, c] = matrix[0];
      const m1 = matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1];
      const m2 = matrix[1][0] * matrix[2][2] - matrix[1][2] * matrix[2][0];
      const m3 = matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0];
      return a * m1 - b * m2 + c * m3;
    }
    throw new Error("Determinant calculation only supported for 2x2 and 3x3 matrices");
  }
};

/**
 * Format operation result as a string
 * @param {string} operation - Name of the operation
 * @param {number} a - First operand
 * @param {number} b - Second operand
 * @param {number|null} result - Result of the operation
 * @returns {string}
 */
function formatResult(operation, a, b, result) {
  return `${operation}(${a}, ${b}) = ${result}`;
}

/**
 * Run calculator tests with error handling
 */
function runCalculatorTests() {
  console.log("=== Calculator Tests ===");
  const x = 10;
  const y = 5;

  try {
    console.log(formatResult("add", x, y, Calculator.add(x, y)));
    console.log(formatResult("subtract", x, y, Calculator.subtract(x, y)));
    console.log(formatResult("multiply", x, y, Calculator.multiply(x, y)));
    console.log(formatResult("divide", x, y, Calculator.divide(x, y)));
    console.log(formatResult("power", x, y, Calculator.power(x, y)));
  } catch (error) {
    console.error("Calculator error:", error.message);
  }

  try {
    Calculator.divide(x, 0);
  } catch (error) {
    console.log(`Division by zero caught: ${error.message}`);
  }
}

/**
 * Advanced statistical functions for correlation analysis
 */
const Statistics = {
  /**
   * Calculate the mean (average) of an array
   * @param {number[]} arr
   * @returns {number}
   * @throws {Error} If array is empty
   */
  mean(arr) {
    if (arr.length === 0) throw new Error("Cannot calculate mean of empty array");
    const sum = arr.reduce((a, b) => a + b, 0);
    return sum / arr.length;
  },

  /**
   * Calculate the median of an array
   * @param {number[]} arr
   * @returns {number}
   */
  median(arr) {
    const sorted = [...arr].sort((a, b) => a - b);
    const mid = Math.floor(sorted.length / 2);
    return sorted.length % 2 !== 0 ? sorted[mid] : (sorted[mid - 1] + sorted[mid]) / 2;
  },

  /**
   * Calculate the standard deviation of an array
   * @param {number[]} arr
   * @returns {number}
   */
  standardDeviation(arr) {
    const avg = this.mean(arr);
    const squareDiffs = arr.map(val => Math.pow(val - avg, 2));
    const avgSquareDiff = this.mean(squareDiffs);
    return Math.sqrt(avgSquareDiff);
  },

  /**
   * Calculate Pearson correlation coefficient between two arrays
   * @param {number[]} x
   * @param {number[]} y
   * @returns {number}
   * @throws {Error} If arrays have different lengths or invalid input
   */
  correlation(x, y) {
    if (x.length !== y.length) {
      throw new Error("Arrays must have equal length");
    }

    const meanX = this.mean(x);
    const meanY = this.mean(y);
    const stdX = this.standardDeviation(x);
    const stdY = this.standardDeviation(y);

    if (stdX === 0 || stdY === 0) {
      throw new Error("Standard deviation cannot be zero");
    }

    let numerator = 0;
    for (let i = 0; i < x.length; i++) {
      numerator += (x[i] - meanX) * (y[i] - meanY);
    }

    const denominator = (x.length - 1) * stdX * stdY;
    return numerator / denominator;
  },

  /**
   * Calculate covariance between two arrays
   * @param {number[]} x
   * @param {number[]} y
   * @returns {number}
   */
  covariance(x, y) {
    if (x.length !== y.length) {
      throw new Error("Arrays must have equal length");
    }
    const meanX = this.mean(x);
    const meanY = this.mean(y);
    const sum = x.reduce((acc, val, i) => acc + (val - meanX) * (y[i] - meanY), 0);
    return sum / (x.length - 1);
  }
};

/**
 * Run correlation analysis tests
 */
function runCorrelationTests() {
  console.log("\n=== Correlation Tests ===");
  const testX = [1, 2, 3, 4, 5];
  const testY = [2, 4, 5, 4, 6];

  try {
    console.log("Mean of X:", Statistics.mean(testX));
    console.log("Median of X:", Statistics.median(testX));
    console.log("Mean of Y:", Statistics.mean(testY));
    console.log("Median of Y:", Statistics.median(testY));
    console.log("Std Dev of X:", Statistics.standardDeviation(testX));
    console.log("Std Dev of Y:", Statistics.standardDeviation(testY));
    console.log("Correlation coefficient:", Statistics.correlation(testX, testY));
    console.log("Covariance:", Statistics.covariance(testX, testY));

    const perfectX = [1, 2, 3, 4, 5];
    const perfectY = [2, 4, 6, 8, 10];
    console.log("Correlation (perfect linear):", Statistics.correlation(perfectX, perfectY));
  } catch (error) {
    console.error("Statistics error:", error.message);
  }
}

/**
 * Run matrix operation tests
 */
function runMatrixTests() {
  console.log("\n=== Matrix Tests ===");

  try {
    const identity2 = Matrix.identity(2);
    console.log("Identity 2x2:", JSON.stringify(identity2));

    const a = [[1, 2], [3, 4]];
    const b = [[5, 6], [7, 8]];
    const result = Matrix.multiply(a, b);
    console.log("Matrix multiplication result:", JSON.stringify(result));

    console.log("Transpose of a:", JSON.stringify(Matrix.transpose(a)));

    console.log("Determinant of a:", Matrix.determinant(a));

    const matrix3x3 = [[1, 2, 3], [0, 1, 4], [5, 6, 0]];
    console.log("Determinant of 3x3:", Matrix.determinant(matrix3x3));
  } catch (error) {
    console.error("Matrix error:", error.message);
  }
}

// Run all tests
runCalculatorTests();
runCorrelationTests();
runMatrixTests();