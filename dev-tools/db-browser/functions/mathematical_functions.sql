-- Mathematical Functions for Data Analysis
-- Common mathematical operations and calculations

-- ============================================
-- BASIC ARITHMETIC
-- ============================================

-- Addition, Subtraction, Multiplication, Division
SELECT 
    10 + 5 as addition,
    10 - 5 as subtraction,
    10 * 5 as multiplication,
    10 / 5 as division,
    10.0 / 3.0 as float_division,
    10 % 3 as modulo;

-- ============================================
-- POWER AND ROOTS
-- ============================================

-- Power function
SELECT 
    POWER(2, 3) as two_cubed,
    POWER(10, 2) as ten_squared,
    POWER(5, 0.5) as square_root_of_5;

-- Square root
SELECT 
    SQRT(16) as square_root,
    SQRT(2) as sqrt_2,
    POWER(27, 1.0/3.0) as cube_root;

-- ============================================
-- ABSOLUTE VALUE
-- ============================================

SELECT 
    ABS(-15) as absolute_value,
    ABS(10 - 25) as distance;

-- ============================================
-- ROUNDING FUNCTIONS
-- ============================================

-- Round to nearest integer
SELECT 
    ROUND(3.7) as round_up,
    ROUND(3.2) as round_down,
    ROUND(3.5) as round_half;

-- Round to decimal places
SELECT 
    ROUND(3.14159, 2) as two_decimals,
    ROUND(3.14159, 4) as four_decimals;

-- Ceiling (round up)
SELECT 
    CAST((value + 0.9999) AS INTEGER) as ceiling_value
FROM numbers;

-- Floor (round down)
SELECT 
    CAST(value AS INTEGER) as floor_value
FROM numbers;

-- ============================================
-- LOGARITHMS
-- ============================================

-- Natural logarithm (base e)
SELECT 
    LOG(10) as natural_log,
    LOG(2.718281828) as ln_e;

-- Base-10 logarithm
SELECT 
    LOG10(100) as log10_100,
    LOG10(1000) as log10_1000;

-- Custom base logarithm
SELECT 
    LOG(value) / LOG(2) as log_base_2,
    LOG(value) / LOG(5) as log_base_5
FROM numbers;

-- ============================================
-- EXPONENTIAL
-- ============================================

-- e raised to power
SELECT 
    EXP(1) as e,
    EXP(2) as e_squared;

-- ============================================
-- TRIGONOMETRIC FUNCTIONS
-- ============================================

-- Note: SQLite uses radians, not degrees
-- Convert degrees to radians: degrees * PI / 180

-- Sine
SELECT 
    SIN(0) as sin_0,
    SIN(3.14159265359 / 2) as sin_90_deg;

-- Cosine
SELECT 
    COS(0) as cos_0,
    COS(3.14159265359) as cos_180_deg;

-- Tangent
SELECT 
    TAN(0) as tan_0,
    TAN(3.14159265359 / 4) as tan_45_deg;

-- Arc functions
SELECT 
    ASIN(0.5) as arcsin,
    ACOS(0.5) as arccos,
    ATAN(1) as arctan;

-- Convert degrees to radians
SELECT 
    angle_degrees,
    angle_degrees * 3.14159265359 / 180 as angle_radians
FROM angles;

-- ============================================
-- PI CONSTANT
-- ============================================

SELECT 
    3.14159265359 as pi,
    2 * 3.14159265359 as two_pi,
    3.14159265359 / 2 as pi_over_2;

-- ============================================
-- SIGN FUNCTION
-- ============================================

SELECT 
    value,
    CASE 
        WHEN value > 0 THEN 1
        WHEN value < 0 THEN -1
        ELSE 0
    END as sign
FROM numbers;

-- ============================================
-- RANDOM NUMBERS
-- ============================================

-- Random integer between 0 and max
SELECT 
    ABS(RANDOM()) % 100 as random_0_to_99,
    ABS(RANDOM()) % 1000 as random_0_to_999;

-- Random float between 0 and 1
SELECT 
    (ABS(RANDOM()) % 1000000) / 1000000.0 as random_float;

-- Random integer in range [min, max]
SELECT 
    10 + (ABS(RANDOM()) % (50 - 10 + 1)) as random_10_to_50;

-- ============================================
-- GREATEST AND LEAST
-- ============================================

-- Maximum of multiple values
SELECT 
    MAX(value1, value2, value3) as greatest_value
FROM (
    SELECT 
        value1,
        value2,
        value3,
        CASE 
            WHEN value1 >= value2 AND value1 >= value3 THEN value1
            WHEN value2 >= value1 AND value2 >= value3 THEN value2
            ELSE value3
        END as max
    FROM multi_values
);

-- Minimum of multiple values
SELECT 
    CASE 
        WHEN value1 <= value2 AND value1 <= value3 THEN value1
        WHEN value2 <= value1 AND value2 <= value3 THEN value2
        ELSE value3
    END as least_value
FROM multi_values;

-- ============================================
-- PERCENTAGE CALCULATIONS
-- ============================================

-- Calculate percentage
SELECT 
    part,
    total,
    (part * 100.0 / total) as percentage
FROM calculations;

-- Calculate value from percentage
SELECT 
    total,
    percentage,
    (total * percentage / 100.0) as part_value
FROM calculations;

-- Percentage change
SELECT 
    old_value,
    new_value,
    ((new_value - old_value) / old_value) * 100 as percent_change
FROM comparisons;

-- ============================================
-- COMPOUND INTEREST
-- ============================================

-- Future value with compound interest
-- FV = PV * (1 + r)^n
SELECT 
    principal,
    rate,
    periods,
    principal * POWER(1 + rate, periods) as future_value
FROM investments;

-- ============================================
-- FACTORIAL (for small numbers)
-- ============================================

WITH RECURSIVE factorial(n, result) AS (
    SELECT 0, 1
    UNION ALL
    SELECT n + 1, result * (n + 1)
    FROM factorial
    WHERE n < 10
)
SELECT n, result as factorial FROM factorial;

-- ============================================
-- FIBONACCI SEQUENCE
-- ============================================

WITH RECURSIVE fibonacci(n, current, next) AS (
    SELECT 1, 0, 1
    UNION ALL
    SELECT n + 1, next, current + next
    FROM fibonacci
    WHERE n < 20
)
SELECT n, current as fibonacci_number FROM fibonacci;

-- ============================================
-- DISTANCE CALCULATIONS
-- ============================================

-- Euclidean distance (2D)
SELECT 
    SQRT(POWER(x2 - x1, 2) + POWER(y2 - y1, 2)) as distance_2d
FROM points;

-- Euclidean distance (3D)
SELECT 
    SQRT(POWER(x2 - x1, 2) + POWER(y2 - y1, 2) + POWER(z2 - z1, 2)) as distance_3d
FROM points_3d;

-- Manhattan distance
SELECT 
    ABS(x2 - x1) + ABS(y2 - y1) as manhattan_distance
FROM points;

-- ============================================
-- CIRCLE CALCULATIONS
-- ============================================

-- Area of circle
SELECT 
    radius,
    3.14159265359 * POWER(radius, 2) as area
FROM circles;

-- Circumference of circle
SELECT 
    radius,
    2 * 3.14159265359 * radius as circumference
FROM circles;

-- ============================================
-- TRIANGLE CALCULATIONS
-- ============================================

-- Area using Heron's formula
SELECT 
    side_a,
    side_b,
    side_c,
    (side_a + side_b + side_c) / 2.0 as s,
    SQRT(
        ((side_a + side_b + side_c) / 2.0) *
        (((side_a + side_b + side_c) / 2.0) - side_a) *
        (((side_a + side_b + side_c) / 2.0) - side_b) *
        (((side_a + side_b + side_c) / 2.0) - side_c)
    ) as area
FROM triangles;

-- ============================================
-- RANGE NORMALIZATION
-- ============================================

-- Normalize to 0-1 range
SELECT 
    value,
    (value - (SELECT MIN(value) FROM dataset)) /
    ((SELECT MAX(value) FROM dataset) - (SELECT MIN(value) FROM dataset)) as normalized
FROM dataset;

-- Normalize to custom range [a, b]
SELECT 
    value,
    5 + ((value - (SELECT MIN(value) FROM dataset)) /
         ((SELECT MAX(value) FROM dataset) - (SELECT MIN(value) FROM dataset))) * (10 - 5) as normalized_5_to_10
FROM dataset;

-- ============================================
-- LINEAR INTERPOLATION
-- ============================================

-- Interpolate value at x
SELECT 
    (y1 + (x - x1) * (y2 - y1) / (x2 - x1)) as interpolated_value
FROM (
    SELECT 
        1.0 as x1, 10.0 as y1,
        5.0 as x2, 20.0 as y2,
        3.0 as x
);

-- ============================================
-- BINNING/BUCKETING
-- ============================================

-- Create bins
SELECT 
    value,
    CASE 
        WHEN value < 10 THEN '0-10'
        WHEN value < 20 THEN '10-20'
        WHEN value < 30 THEN '20-30'
        ELSE '30+'
    END as bin
FROM dataset;

-- Equal-width binning
SELECT 
    value,
    CAST(value / 10 AS INTEGER) * 10 as bin_start,
    (CAST(value / 10 AS INTEGER) + 1) * 10 as bin_end
FROM dataset;

-- ============================================
-- RUNNING CALCULATIONS
-- ============================================

-- Running sum
SELECT 
    date,
    value,
    SUM(value) OVER (ORDER BY date ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW) as running_total
FROM time_series;

-- Running average
SELECT 
    date,
    value,
    AVG(value) OVER (ORDER BY date ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW) as running_avg
FROM time_series;

-- Running product
WITH RECURSIVE running_product AS (
    SELECT 
        date,
        value,
        value as product,
        ROW_NUMBER() OVER (ORDER BY date) as rn
    FROM time_series
    WHERE date = (SELECT MIN(date) FROM time_series)
    
    UNION ALL
    
    SELECT 
        ts.date,
        ts.value,
        rp.product * ts.value,
        rp.rn + 1
    FROM time_series ts
    JOIN running_product rp
    WHERE ts.date = (
        SELECT MIN(date) 
        FROM time_series 
        WHERE date > rp.date
    )
)
SELECT * FROM running_product ORDER BY date;

-- ============================================
-- WEIGHTED AVERAGE
-- ============================================

SELECT 
    SUM(value * weight) / SUM(weight) as weighted_average
FROM weighted_data;

-- By category
SELECT 
    category,
    SUM(value * weight) / SUM(weight) as weighted_average
FROM weighted_data
GROUP BY category;
