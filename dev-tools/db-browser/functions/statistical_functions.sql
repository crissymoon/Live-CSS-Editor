-- Statistical Functions for Data Analysis
-- Advanced statistical calculations and probability functions

-- ============================================
-- DESCRIPTIVE STATISTICS
-- ============================================

-- Complete statistical summary
WITH stats AS (
    SELECT 
        COUNT(*) as n,
        AVG(value) as mean,
        MIN(value) as min,
        MAX(value) as max,
        SUM(value) as total
    FROM data_table
)
SELECT 
    n,
    mean,
    min,
    max,
    max - min as range,
    total,
    mean * n as sum_check
FROM stats;

-- ============================================
-- VARIANCE AND STANDARD DEVIATION
-- ============================================

-- Population variance and standard deviation
SELECT 
    COUNT(*) as count,
    AVG(value) as mean,
    -- Population variance
    SUM((value - (SELECT AVG(value) FROM dataset)) * 
        (value - (SELECT AVG(value) FROM dataset))) / COUNT(*) as pop_variance,
    -- Population standard deviation
    SQRT(SUM((value - (SELECT AVG(value) FROM dataset)) * 
             (value - (SELECT AVG(value) FROM dataset))) / COUNT(*)) as pop_std_dev
FROM dataset;

-- Sample variance and standard deviation (Bessel's correction)
SELECT 
    COUNT(*) as count,
    AVG(value) as mean,
    -- Sample variance (n-1 denominator)
    SUM((value - (SELECT AVG(value) FROM dataset)) * 
        (value - (SELECT AVG(value) FROM dataset))) / (COUNT(*) - 1) as sample_variance,
    -- Sample standard deviation
    SQRT(SUM((value - (SELECT AVG(value) FROM dataset)) * 
             (value - (SELECT AVG(value) FROM dataset))) / (COUNT(*) - 1)) as sample_std_dev
FROM dataset;

-- Coefficient of Variation (CV)
SELECT 
    AVG(value) as mean,
    SQRT(AVG(value * value) - AVG(value) * AVG(value)) as std_dev,
    (SQRT(AVG(value * value) - AVG(value) * AVG(value)) / AVG(value)) * 100 as cv_percent
FROM dataset;

-- ============================================
-- QUARTILES AND PERCENTILES
-- ============================================

-- Interquartile Range (IQR)
WITH ranked AS (
    SELECT 
        value,
        ROW_NUMBER() OVER (ORDER BY value) as row_num,
        COUNT(*) OVER () as total_count
    FROM dataset
),
quartiles AS (
    SELECT 
        MAX(CASE WHEN row_num = CAST(total_count * 0.25 AS INTEGER) THEN value END) as q1,
        MAX(CASE WHEN row_num = CAST(total_count * 0.50 AS INTEGER) THEN value END) as q2,
        MAX(CASE WHEN row_num = CAST(total_count * 0.75 AS INTEGER) THEN value END) as q3
    FROM ranked
)
SELECT 
    q1,
    q2 as median,
    q3,
    q3 - q1 as iqr,
    q1 - 1.5 * (q3 - q1) as lower_fence,
    q3 + 1.5 * (q3 - q1) as upper_fence
FROM quartiles;

-- ============================================
-- SKEWNESS
-- ============================================

-- Pearson's coefficient of skewness
SELECT 
    (AVG(value) - 
     (SELECT value FROM (SELECT value FROM dataset ORDER BY value LIMIT 1 OFFSET (SELECT COUNT(*)/2 FROM dataset)))) /
    SQRT(AVG(value * value) - AVG(value) * AVG(value)) as skewness
FROM dataset;

-- ============================================
-- KURTOSIS
-- ============================================

-- Excess kurtosis
WITH moments AS (
    SELECT 
        AVG(value) as mean,
        SQRT(AVG(value * value) - AVG(value) * AVG(value)) as std_dev,
        COUNT(*) as n
    FROM dataset
)
SELECT 
    (SUM(POWER((value - mean) / std_dev, 4)) / n) - 3 as excess_kurtosis
FROM dataset, moments
GROUP BY mean, std_dev, n;

-- ============================================
-- Z-SCORES (STANDARDIZATION)
-- ============================================

-- Calculate z-scores for all values
WITH stats AS (
    SELECT 
        AVG(value) as mean,
        SQRT(AVG(value * value) - AVG(value) * AVG(value)) as std_dev
    FROM dataset
)
SELECT 
    id,
    value,
    (value - mean) / std_dev as z_score,
    CASE 
        WHEN ABS((value - mean) / std_dev) > 3 THEN 'Extreme Outlier'
        WHEN ABS((value - mean) / std_dev) > 2 THEN 'Outlier'
        ELSE 'Normal'
    END as classification
FROM dataset, stats
ORDER BY ABS((value - mean) / std_dev) DESC;

-- ============================================
-- MOVING AVERAGES
-- ============================================

-- Simple Moving Average (SMA)
SELECT 
    date,
    value,
    AVG(value) OVER (
        ORDER BY date 
        ROWS BETWEEN 6 PRECEDING AND CURRENT ROW
    ) as sma_7day,
    AVG(value) OVER (
        ORDER BY date 
        ROWS BETWEEN 29 PRECEDING AND CURRENT ROW
    ) as sma_30day
FROM time_series
ORDER BY date;

-- Exponential Moving Average simulation
WITH RECURSIVE ema AS (
    SELECT 
        date,
        value,
        value as ema_value,
        ROW_NUMBER() OVER (ORDER BY date) as rn
    FROM time_series
    WHERE date = (SELECT MIN(date) FROM time_series)
    
    UNION ALL
    
    SELECT 
        ts.date,
        ts.value,
        (ts.value * 0.2) + (ema.ema_value * 0.8) as ema_value,
        ema.rn + 1
    FROM time_series ts
    JOIN ema ON ts.date > ema.date
    WHERE ts.date = (
        SELECT MIN(date) 
        FROM time_series 
        WHERE date > ema.date
    )
)
SELECT * FROM ema ORDER BY date;

-- ============================================
-- CORRELATION COEFFICIENT
-- ============================================

-- Pearson correlation coefficient
WITH stats AS (
    SELECT 
        COUNT(*) as n,
        AVG(x) as mean_x,
        AVG(y) as mean_y,
        SUM(x) as sum_x,
        SUM(y) as sum_y,
        SUM(x * x) as sum_x2,
        SUM(y * y) as sum_y2,
        SUM(x * y) as sum_xy
    FROM paired_data
)
SELECT 
    (n * sum_xy - sum_x * sum_y) /
    (SQRT(n * sum_x2 - sum_x * sum_x) * 
     SQRT(n * sum_y2 - sum_y * sum_y)) as correlation_coefficient
FROM stats;

-- ============================================
-- LINEAR REGRESSION
-- ============================================

-- Simple linear regression (y = mx + b)
WITH stats AS (
    SELECT 
        COUNT(*) as n,
        AVG(x) as mean_x,
        AVG(y) as mean_y,
        SUM(x * y) as sum_xy,
        SUM(x) as sum_x,
        SUM(y) as sum_y,
        SUM(x * x) as sum_x2
    FROM regression_data
)
SELECT 
    -- Slope (m)
    (n * sum_xy - sum_x * sum_y) / 
    (n * sum_x2 - sum_x * sum_x) as slope,
    -- Intercept (b)
    mean_y - ((n * sum_xy - sum_x * sum_y) / 
              (n * sum_x2 - sum_x * sum_x)) * mean_x as intercept,
    -- R-squared
    POWER(
        (n * sum_xy - sum_x * sum_y) /
        (SQRT(n * sum_x2 - sum_x * sum_x) * 
         SQRT(n * SUM(y * y) - sum_y * sum_y)),
        2
    ) as r_squared
FROM stats, (SELECT SUM(y * y) FROM regression_data);

-- ============================================
-- GROWTH RATE
-- ============================================

-- Period-over-period growth rate
SELECT 
    current_period,
    current_value,
    previous_value,
    ((current_value - previous_value) / previous_value) * 100 as growth_rate_percent,
    current_value - previous_value as absolute_change
FROM (
    SELECT 
        period as current_period,
        value as current_value,
        LAG(value) OVER (ORDER BY period) as previous_value
    FROM time_series_data
)
WHERE previous_value IS NOT NULL;

-- Compound Annual Growth Rate (CAGR)
WITH endpoints AS (
    SELECT 
        MIN(year) as start_year,
        MAX(year) as end_year,
        (SELECT value FROM growth_data WHERE year = MIN(year)) as start_value,
        (SELECT value FROM growth_data WHERE year = MAX(year)) as end_value
    FROM growth_data
)
SELECT 
    start_year,
    end_year,
    end_year - start_year as years,
    start_value,
    end_value,
    (POWER(end_value / start_value, 1.0 / (end_year - start_year)) - 1) * 100 as cagr_percent
FROM endpoints;

-- ============================================
-- CUMULATIVE DISTRIBUTION
-- ============================================

-- Cumulative frequency distribution
SELECT 
    value_bin,
    frequency,
    SUM(frequency) OVER (ORDER BY value_bin) as cumulative_frequency,
    ROUND(SUM(frequency) OVER (ORDER BY value_bin) * 100.0 / 
          SUM(frequency) OVER (), 2) as cumulative_percent
FROM (
    SELECT 
        CAST(value / 10 AS INTEGER) * 10 as value_bin,
        COUNT(*) as frequency
    FROM dataset
    GROUP BY value_bin
)
ORDER BY value_bin;

-- ============================================
-- OUTLIER DETECTION
-- ============================================

-- IQR method for outlier detection
WITH quartiles AS (
    SELECT 
        value,
        NTILE(4) OVER (ORDER BY value) as quartile
    FROM dataset
),
bounds AS (
    SELECT 
        MAX(CASE WHEN quartile = 1 THEN value END) as q1,
        MAX(CASE WHEN quartile = 3 THEN value END) as q3
    FROM quartiles
)
SELECT 
    d.id,
    d.value,
    q1,
    q3,
    q3 - q1 as iqr,
    CASE 
        WHEN d.value < q1 - 1.5 * (q3 - q1) THEN 'Lower Outlier'
        WHEN d.value > q3 + 1.5 * (q3 - q1) THEN 'Upper Outlier'
        ELSE 'Normal'
    END as outlier_status
FROM dataset d, bounds
WHERE d.value < q1 - 1.5 * (q3 - q1) 
   OR d.value > q3 + 1.5 * (q3 - q1);

-- ============================================
-- PROBABILITY CALCULATIONS
-- ============================================

-- Frequency-based probability
SELECT 
    outcome,
    COUNT(*) as frequency,
    COUNT(*) * 1.0 / (SELECT COUNT(*) FROM events) as probability,
    ROUND(COUNT(*) * 100.0 / (SELECT COUNT(*) FROM events), 2) as probability_percent
FROM events
GROUP BY outcome
ORDER BY probability DESC;

-- ============================================
-- HYPOTHESIS TESTING (T-TEST CALCULATION)
-- ============================================

-- Two-sample t-test calculation
WITH group_stats AS (
    SELECT 
        'Group A' as group_name,
        COUNT(*) as n,
        AVG(value) as mean,
        SQRT(SUM((value - (SELECT AVG(value) FROM sample_a)) * 
                 (value - (SELECT AVG(value) FROM sample_a))) / (COUNT(*) - 1)) as std_dev
    FROM sample_a
    UNION ALL
    SELECT 
        'Group B',
        COUNT(*),
        AVG(value),
        SQRT(SUM((value - (SELECT AVG(value) FROM sample_b)) * 
                 (value - (SELECT AVG(value) FROM sample_b))) / (COUNT(*) - 1))
    FROM sample_b
)
SELECT 
    a.mean as mean_a,
    b.mean as mean_b,
    a.mean - b.mean as mean_difference,
    -- T-statistic
    (a.mean - b.mean) / 
    SQRT((a.std_dev * a.std_dev / a.n) + (b.std_dev * b.std_dev / b.n)) as t_statistic,
    -- Degrees of freedom (approximate)
    a.n + b.n - 2 as df
FROM 
    (SELECT * FROM group_stats WHERE group_name = 'Group A') a,
    (SELECT * FROM group_stats WHERE group_name = 'Group B') b;

-- ============================================
-- CONFIDENCE INTERVALS
-- ============================================

-- 95% Confidence Interval (assumes normal distribution, z=1.96)
SELECT 
    AVG(value) as mean,
    COUNT(*) as n,
    SQRT(AVG(value * value) - AVG(value) * AVG(value)) as std_dev,
    AVG(value) - 1.96 * SQRT(AVG(value * value) - AVG(value) * AVG(value)) / SQRT(COUNT(*)) as ci_lower,
    AVG(value) + 1.96 * SQRT(AVG(value * value) - AVG(value) * AVG(value)) / SQRT(COUNT(*)) as ci_upper
FROM dataset;
