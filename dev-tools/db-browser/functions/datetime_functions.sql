-- Date and Time Functions for Data Analysis
-- Working with temporal data and time series

-- ============================================
-- CURRENT DATE AND TIME
-- ============================================

SELECT 
    DATE('now') as current_date,
    TIME('now') as current_time,
    DATETIME('now') as current_datetime,
    JULIANDAY('now') as julian_day,
    STRFTIME('%s', 'now') as unix_timestamp;

-- ============================================
-- DATE EXTRACTION
-- ============================================

-- Extract components from date
SELECT 
    date_column,
    STRFTIME('%Y', date_column) as year,
    STRFTIME('%m', date_column) as month,
    STRFTIME('%d', date_column) as day,
    STRFTIME('%H', date_column) as hour,
    STRFTIME('%M', date_column) as minute,
    STRFTIME('%S', date_column) as second,
    STRFTIME('%w', date_column) as day_of_week,  -- 0=Sunday
    STRFTIME('%j', date_column) as day_of_year,
    STRFTIME('%W', date_column) as week_of_year
FROM dates;

-- ============================================
-- DATE FORMATTING
-- ============================================

SELECT 
    STRFTIME('%Y-%m-%d', date_column) as iso_date,
    STRFTIME('%m/%d/%Y', date_column) as us_format,
    STRFTIME('%d-%b-%Y', date_column) as day_month_year,
    STRFTIME('%Y-%m-%d %H:%M:%S', datetime_column) as iso_datetime,
    STRFTIME('%B %d, %Y', date_column) as long_format,
    STRFTIME('%a, %b %d %Y', date_column) as short_format
FROM dates;

-- Format codes:
-- %Y = 4-digit year (2026)
-- %m = month (01-12)
-- %d = day (01-31)
-- %H = hour (00-23)
-- %M = minute (00-59)
-- %S = second (00-59)
-- %w = weekday (0-6, 0=Sunday)
-- %j = day of year (001-366)
-- %W = week of year (00-53)
-- %a = abbreviated weekday (Mon)
-- %A = full weekday (Monday)
-- %b = abbreviated month (Jan)
-- %B = full month (January)

-- ============================================
-- DATE ARITHMETIC
-- ============================================

-- Add/subtract days
SELECT 
    date_column,
    DATE(date_column, '+1 day') as tomorrow,
    DATE(date_column, '-1 day') as yesterday,
    DATE(date_column, '+7 days') as next_week,
    DATE(date_column, '-30 days') as month_ago
FROM dates;

-- Add/subtract months
SELECT 
    DATE(date_column, '+1 month') as next_month,
    DATE(date_column, '-1 month') as last_month,
    DATE(date_column, '+6 months') as six_months_later
FROM dates;

-- Add/subtract years
SELECT 
    DATE(date_column, '+1 year') as next_year,
    DATE(date_column, '-1 year') as last_year
FROM dates;

-- Complex date arithmetic
SELECT 
    DATE(date_column, '+2 years', '-3 months', '+10 days') as complex_date
FROM dates;

-- ============================================
-- DATE DIFFERENCES
-- ============================================

-- Days between dates
SELECT 
    start_date,
    end_date,
    JULIANDAY(end_date) - JULIANDAY(start_date) as days_difference
FROM date_ranges;

-- Months between dates (approximate)
SELECT 
    start_date,
    end_date,
    (STRFTIME('%Y', end_date) - STRFTIME('%Y', start_date)) * 12 +
    (STRFTIME('%m', end_date) - STRFTIME('%m', start_date)) as months_difference
FROM date_ranges;

-- Years between dates
SELECT 
    start_date,
    end_date,
    STRFTIME('%Y', end_date) - STRFTIME('%Y', start_date) as years_difference
FROM date_ranges;

-- Age calculation
SELECT 
    birth_date,
    DATE('now') as today,
    STRFTIME('%Y', 'now') - STRFTIME('%Y', birth_date) -
    (CASE 
        WHEN STRFTIME('%m-%d', 'now') < STRFTIME('%m-%d', birth_date) THEN 1
        ELSE 0
    END) as age
FROM people;

-- ============================================
-- START/END OF PERIOD
-- ============================================

-- Start of month
SELECT 
    date_column,
    DATE(date_column, 'start of month') as month_start
FROM dates;

-- End of month
SELECT 
    date_column,
    DATE(date_column, 'start of month', '+1 month', '-1 day') as month_end
FROM dates;

-- Start of year
SELECT 
    DATE(date_column, 'start of year') as year_start
FROM dates;

-- End of year
SELECT 
    DATE(date_column, 'start of year', '+1 year', '-1 day') as year_end
FROM dates;

-- Start of week (Monday)
SELECT 
    date_column,
    DATE(date_column, 'weekday 1') as week_start_monday
FROM dates;

-- Start of week (Sunday)
SELECT 
    date_column,
    DATE(date_column, 'weekday 0') as week_start_sunday
FROM dates;

-- ============================================
-- DAY OF WEEK CALCULATIONS
-- ============================================

-- Day name
SELECT 
    date_column,
    CASE CAST(STRFTIME('%w', date_column) AS INTEGER)
        WHEN 0 THEN 'Sunday'
        WHEN 1 THEN 'Monday'
        WHEN 2 THEN 'Tuesday'
        WHEN 3 THEN 'Wednesday'
        WHEN 4 THEN 'Thursday'
        WHEN 5 THEN 'Friday'
        WHEN 6 THEN 'Saturday'
    END as day_name
FROM dates;

-- Is weekend?
SELECT 
    date_column,
    CASE 
        WHEN CAST(STRFTIME('%w', date_column) AS INTEGER) IN (0, 6) THEN 'Weekend'
        ELSE 'Weekday'
    END as day_type
FROM dates;

-- Business days between dates
SELECT 
    start_date,
    end_date,
    (JULIANDAY(end_date) - JULIANDAY(start_date)) - 
    ((JULIANDAY(end_date) - JULIANDAY(start_date)) / 7) * 2 as business_days_approx
FROM date_ranges;

-- ============================================
-- QUARTER CALCULATIONS
-- ============================================

-- Get quarter
SELECT 
    date_column,
    CASE 
        WHEN CAST(STRFTIME('%m', date_column) AS INTEGER) BETWEEN 1 AND 3 THEN 'Q1'
        WHEN CAST(STRFTIME('%m', date_column) AS INTEGER) BETWEEN 4 AND 6 THEN 'Q2'
        WHEN CAST(STRFTIME('%m', date_column) AS INTEGER) BETWEEN 7 AND 9 THEN 'Q3'
        ELSE 'Q4'
    END as quarter,
    STRFTIME('%Y', date_column) || '-' ||
    CASE 
        WHEN CAST(STRFTIME('%m', date_column) AS INTEGER) BETWEEN 1 AND 3 THEN 'Q1'
        WHEN CAST(STRFTIME('%m', date_column) AS INTEGER) BETWEEN 4 AND 6 THEN 'Q2'
        WHEN CAST(STRFTIME('%m', date_column) AS INTEGER) BETWEEN 7 AND 9 THEN 'Q3'
        ELSE 'Q4'
    END as year_quarter
FROM dates;

-- ============================================
-- TIME SERIES GROUPING
-- ============================================

-- Group by day
SELECT 
    DATE(datetime_column) as date,
    COUNT(*) as count,
    SUM(amount) as total
FROM transactions
GROUP BY DATE(datetime_column);

-- Group by week
SELECT 
    STRFTIME('%Y-W%W', date_column) as week,
    COUNT(*) as count
FROM events
GROUP BY STRFTIME('%Y-W%W', date_column);

-- Group by month
SELECT 
    STRFTIME('%Y-%m', date_column) as month,
    COUNT(*) as count,
    AVG(value) as average
FROM data
GROUP BY STRFTIME('%Y-%m', date_column);

-- Group by quarter
SELECT 
    STRFTIME('%Y', date_column) || '-Q' ||
    ((CAST(STRFTIME('%m', date_column) AS INTEGER) - 1) / 3 + 1) as quarter,
    COUNT(*) as count
FROM events
GROUP BY quarter;

-- Group by year
SELECT 
    STRFTIME('%Y', date_column) as year,
    COUNT(*) as count
FROM events
GROUP BY STRFTIME('%Y', date_column);

-- ============================================
-- DATE RANGES AND FILTERING
-- ============================================

-- Last 7 days
SELECT * FROM events
WHERE date_column >= DATE('now', '-7 days');

-- Last 30 days
SELECT * FROM events
WHERE date_column >= DATE('now', '-30 days');

-- Last 90 days
SELECT * FROM events
WHERE date_column >= DATE('now', '-90 days');

-- This year
SELECT * FROM events
WHERE date_column >= DATE('now', 'start of year');

-- This month
SELECT * FROM events
WHERE date_column >= DATE('now', 'start of month');

-- This week
SELECT * FROM events
WHERE date_column >= DATE('now', 'weekday 1');

-- Between dates
SELECT * FROM events
WHERE date_column BETWEEN '2026-01-01' AND '2026-12-31';

-- ============================================
-- TIME CALCULATIONS
-- ============================================

-- Add/subtract hours
SELECT 
    DATETIME(datetime_column, '+2 hours') as two_hours_later,
    DATETIME(datetime_column, '-1 hour') as one_hour_ago
FROM times;

-- Add/subtract minutes
SELECT 
    DATETIME(datetime_column, '+30 minutes') as thirty_min_later
FROM times;

-- Time difference in seconds
SELECT 
    (JULIANDAY(end_time) - JULIANDAY(start_time)) * 86400 as seconds_diff
FROM time_ranges;

-- Time difference in minutes
SELECT 
    (JULIANDAY(end_time) - JULIANDAY(start_time)) * 1440 as minutes_diff
FROM time_ranges;

-- Time difference in hours
SELECT 
    (JULIANDAY(end_time) - JULIANDAY(start_time)) * 24 as hours_diff
FROM time_ranges;

-- ============================================
-- TIMESTAMP CONVERSION
-- ============================================

-- Unix timestamp to date
SELECT 
    unix_timestamp,
    DATETIME(unix_timestamp, 'unixepoch') as datetime
FROM timestamps;

-- Date to unix timestamp
SELECT 
    date_column,
    STRFTIME('%s', date_column) as unix_timestamp
FROM dates;

-- ============================================
-- TIME ZONES (local/UTC)
-- ============================================

-- Convert to UTC
SELECT 
    DATETIME(datetime_column, 'utc') as utc_time
FROM local_times;

-- Convert from UTC to local
SELECT 
    DATETIME(datetime_column, 'localtime') as local_time
FROM utc_times;

-- ============================================
-- CALENDAR FUNCTIONS
-- ============================================

-- Is leap year?
SELECT 
    year,
    CASE 
        WHEN year % 400 = 0 THEN 'Yes'
        WHEN year % 100 = 0 THEN 'No'
        WHEN year % 4 = 0 THEN 'Yes'
        ELSE 'No'
    END as is_leap_year
FROM years;

-- Days in month
SELECT 
    date_column,
    CAST(STRFTIME('%d', DATE(date_column, 'start of month', '+1 month', '-1 day')) AS INTEGER) as days_in_month
FROM dates;

-- ============================================
-- AGE BUCKETS
-- ============================================

SELECT 
    birth_date,
    STRFTIME('%Y', 'now') - STRFTIME('%Y', birth_date) as age,
    CASE 
        WHEN STRFTIME('%Y', 'now') - STRFTIME('%Y', birth_date) < 18 THEN 'Under 18'
        WHEN STRFTIME('%Y', 'now') - STRFTIME('%Y', birth_date) < 30 THEN '18-29'
        WHEN STRFTIME('%Y', 'now') - STRFTIME('%Y', birth_date) < 50 THEN '30-49'
        WHEN STRFTIME('%Y', 'now') - STRFTIME('%Y', birth_date) < 65 THEN '50-64'
        ELSE '65+'
    END as age_group
FROM people;

-- ============================================
-- COHORT ANALYSIS BY DATE
-- ============================================

-- Customer cohorts by signup month
SELECT 
    STRFTIME('%Y-%m', signup_date) as cohort_month,
    COUNT(*) as cohort_size,
    SUM(total_spent) as cohort_revenue
FROM customers
GROUP BY cohort_month
ORDER BY cohort_month;

-- ============================================
-- SEASON CALCULATION
-- ============================================

SELECT 
    date_column,
    CASE 
        WHEN CAST(STRFTIME('%m', date_column) AS INTEGER) IN (12, 1, 2) THEN 'Winter'
        WHEN CAST(STRFTIME('%m', date_column) AS INTEGER) IN (3, 4, 5) THEN 'Spring'
        WHEN CAST(STRFTIME('%m', date_column) AS INTEGER) IN (6, 7, 8) THEN 'Summer'
        ELSE 'Fall'
    END as season
FROM dates;
