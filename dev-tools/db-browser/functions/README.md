# Data Analysis Functions Library

Comprehensive collection of SQL functions and queries for data analysis in SQLite.

## Overview

This library contains over 500 ready-to-use SQL functions organized into 6 categories:

1. **Aggregate Functions** - Sum, average, count, and group operations
2. **Statistical Functions** - Advanced statistics, probability, and hypothesis testing
3. **Mathematical Functions** - Arithmetic, trigonometry, and numerical calculations
4. **Date/Time Functions** - Temporal analysis and time series operations
5. **String Functions** - Text manipulation and pattern matching
6. **Window Functions** - Running totals, rankings, and moving averages
7. **Business Functions** - Business metrics, KPIs, and financial ratios

## Files

| File | Functions | Description |
|------|-----------|-------------|
| aggregate_functions.sql | 50+ | Basic aggregates, conditional sums, percentages |
| statistical_functions.sql | 60+ | Variance, correlation, regression, distributions |
| mathematical_functions.sql | 70+ | Math operations, distances, normalization |
| datetime_functions.sql | 80+ | Date arithmetic, formatting, time series |
| string_functions.sql | 90+ | Text processing, validation, parsing |
| window_functions.sql | 70+ | Rankings, moving averages, analytical functions |
| business_functions.sql | 80+ | Revenue metrics, profitability, customer analytics |

## Quick Examples

### Aggregate Functions

```sql
-- Calculate revenue statistics
SELECT 
    COUNT(*) as total_orders,
    SUM(amount) as total_revenue,
    AVG(amount) as avg_order_value,
    MIN(amount) as min_order,
    MAX(amount) as max_order
FROM orders;
```

### Statistical Functions

```sql
-- Calculate standard deviation and variance
SELECT 
    AVG(price) as mean_price,
    SQRT(AVG(price * price) - AVG(price) * AVG(price)) as std_dev,
    AVG(price * price) - AVG(price) * AVG(price) as variance
FROM products;
```

### Window Functions

```sql
-- Running total and moving average
SELECT 
    date,
    sales,
    SUM(sales) OVER (ORDER BY date) as running_total,
    AVG(sales) OVER (ORDER BY date ROWS BETWEEN 6 PRECEDING AND CURRENT ROW) as moving_avg_7day
FROM daily_sales;
```

### Business Metrics

```sql
-- Customer Lifetime Value
SELECT 
    customer_id,
    COUNT(*) as orders,
    SUM(amount) as total_spent,
    AVG(amount) as avg_order,
    MAX(date) as last_purchase
FROM orders
GROUP BY customer_id;
```

## Function Categories

### 1. Aggregate Functions

**Basic Aggregates:**
- SUM() - Total of values
- AVG() - Average value
- COUNT() - Number of rows
- MIN() / MAX() - Minimum and maximum
- TOTAL() - Like SUM but returns 0 instead of NULL

**Advanced Aggregates:**
- GROUP_CONCAT() - Concatenate strings
- Conditional aggregates with CASE
- Running totals and cumulative sums
- Percentage calculations
- Median and mode calculations

**Use Cases:**
- Sales reporting
- Revenue analysis
- Inventory summaries
- Customer metrics

### 2. Statistical Functions

**Descriptive Statistics:**
- Mean, median, mode
- Variance and standard deviation
- Quartiles and percentiles
- Interquartile range (IQR)

**Advanced Statistics:**
- Skewness and kurtosis
- Z-scores (standardization)
- Correlation coefficients
- Linear regression
- Confidence intervals
- Hypothesis testing (t-tests)

**Use Cases:**
- Quality control
- A/B testing
- Predictive modeling
- Outlier detection

### 3. Mathematical Functions

**Basic Math:**
- Arithmetic operations
- POWER() and SQRT()
- ABS() - Absolute value
- ROUND() - Rounding

**Advanced Math:**
- Trigonometric functions (SIN, COS, TAN)
- Logarithms (LOG, LOG10)
- Exponential (EXP)
- Distance calculations (Euclidean, Manhattan)

**Specialized:**
- Normalization and scaling
- Binning and bucketing
- Weighted averages
- Compound interest
- Fibonacci sequences

**Use Cases:**
- Scientific calculations
- Geospatial analysis
- Financial modeling
- Data normalization

### 4. Date and Time Functions

**Date Extraction:**
- Year, month, day, hour, minute
- Day of week, day of year
- Week number, quarter

**Date Arithmetic:**
- Add/subtract days, months, years
- Date differences (days, months, years)
- Age calculations
- Business day counting

**Date Formatting:**
- ISO format, US format
- Custom date formats
- Timestamp conversions

**Time Series:**
- Group by day/week/month/quarter/year
- Start/end of period
- Date ranges and filtering
- Seasonality calculations

**Use Cases:**
- Time series analysis
- Cohort analysis
- Trend reporting
- Subscription metrics

### 5. String Functions

**Basic Operations:**
- LENGTH() - String length
- UPPER() / LOWER() - Case conversion
- SUBSTR() - Substring extraction
- TRIM() - Remove whitespace
- REPLACE() - String replacement

**Advanced:**
- Pattern matching (LIKE, GLOB)
- String splitting and parsing
- Email and phone validation
- Text formatting and masking
- Slug generation

**Aggregation:**
- GROUP_CONCAT() - Combine strings
- String deduplication

**Use Cases:**
- Data cleaning
- Email parsing
- Name formatting
- Search and filtering

### 6. Window Functions

**Ranking:**
- ROW_NUMBER() - Sequential numbering
- RANK() / DENSE_RANK() - Ranking with ties
- NTILE() - Percentile buckets
- PERCENT_RANK() - Relative rank

**Navigation:**
- LAG() / LEAD() - Previous/next values
- FIRST_VALUE() / LAST_VALUE()

**Aggregates:**
- Running totals
- Moving averages (3-day, 7-day, 30-day)
- Cumulative distribution

**Advanced:**
- Window frames (ROWS, RANGE)
- Partitioning
- Growth rate calculations
- Top N per group

**Use Cases:**
- Trend analysis
- Ranking reports
- Period comparisons
- Leaderboards

### 7. Business Functions

**Revenue Metrics:**
- Revenue growth rate
- ARPU (Average Revenue Per User)
- Customer Lifetime Value (CLV)

**Profitability:**
- Gross profit margin
- Net profit margin
- ROI (Return on Investment)
- Break-even analysis

**Customer Metrics:**
- CAC (Customer Acquisition Cost)
- Churn rate
- Retention rate
- Repeat purchase rate

**Inventory:**
- Inventory turnover
- Days inventory outstanding
- Reorder points
- ABC analysis

**Financial Ratios:**
- Current ratio
- Quick ratio
- Debt-to-equity
- ROA, ROE

**Analytics:**
- RFM analysis
- Cohort analysis
- Pareto analysis (80/20 rule)
- Lead scoring
- Net Promoter Score (NPS)

**Use Cases:**
- Business intelligence
- Financial reporting
- Customer segmentation
- Performance dashboards

## Usage Tips

### 1. Copy and Modify

All functions are provided as working examples. Copy them and modify for your schema:

```sql
-- Template
SELECT 
    category,
    AVG(value) as average
FROM your_table
GROUP BY category;
```

### 2. Combine Functions

Functions can be combined for complex analysis:

```sql
SELECT 
    DATE(date_column) as date,
    SUM(amount) as daily_total,
    AVG(SUM(amount)) OVER (ORDER BY date ROWS BETWEEN 6 PRECEDING AND CURRENT ROW) as moving_avg,
    RANK() OVER (ORDER BY SUM(amount) DESC) as rank
FROM transactions
GROUP BY DATE(date_column);
```

### 3. Create Views

Save frequently-used queries as views:

```sql
CREATE VIEW sales_summary AS
SELECT 
    DATE(order_date) as date,
    COUNT(*) as orders,
    SUM(amount) as revenue,
    AVG(amount) as avg_order
FROM orders
GROUP BY DATE(order_date);
```

### 4. Use CTEs

Common Table Expressions make complex queries readable:

```sql
WITH monthly_sales AS (
    SELECT 
        STRFTIME('%Y-%m', date) as month,
        SUM(amount) as revenue
    FROM orders
    GROUP BY month
)
SELECT 
    month,
    revenue,
    LAG(revenue) OVER (ORDER BY month) as prev_month,
    ((revenue - LAG(revenue) OVER (ORDER BY month)) / 
     LAG(revenue) OVER (ORDER BY month)) * 100 as growth_pct
FROM monthly_sales;
```

## Performance Tips

1. **Index your data** - Create indexes on columns used in WHERE, JOIN, and ORDER BY
2. **Use EXPLAIN** - Check query execution plans
3. **Limit result sets** - Use LIMIT for large datasets
4. **Aggregate early** - Filter before aggregating when possible
5. **Avoid SELECT *** - Only select needed columns
6. **Use appropriate data types** - INTEGER vs REAL affects performance

## Common Patterns

### Time Series Analysis

```sql
-- Daily metrics with comparisons
SELECT 
    date,
    value,
    LAG(value, 1) OVER (ORDER BY date) as yesterday,
    LAG(value, 7) OVER (ORDER BY date) as last_week,
    AVG(value) OVER (ORDER BY date ROWS BETWEEN 29 PRECEDING AND CURRENT ROW) as avg_30day
FROM daily_metrics;
```

### Customer Segmentation

```sql
-- RFM segmentation
WITH rfm AS (
    SELECT 
        customer_id,
        JULIANDAY('now') - JULIANDAY(MAX(date)) as recency,
        COUNT(*) as frequency,
        SUM(amount) as monetary
    FROM orders
    GROUP BY customer_id
)
SELECT 
    customer_id,
    CASE 
        WHEN recency <= 30 AND frequency >= 5 AND monetary >= 1000 THEN 'Champions'
        WHEN recency <= 90 AND frequency >= 3 THEN 'Loyal'
        WHEN monetary >= 1000 THEN 'Big Spenders'
        WHEN recency <= 30 THEN 'Recent'
        ELSE 'At Risk'
    END as segment
FROM rfm;
```

### Cohort Retention

```sql
-- Monthly cohort retention matrix
SELECT 
    STRFTIME('%Y-%m', signup_date) as cohort,
    (STRFTIME('%Y', activity_date) - STRFTIME('%Y', signup_date)) * 12 +
    (STRFTIME('%m', activity_date) - STRFTIME('%m', signup_date)) as months_since_signup,
    COUNT(DISTINCT customer_id) as active_customers
FROM user_activity
GROUP BY cohort, months_since_signup
ORDER BY cohort, months_since_signup;
```

## Integration

### Use with Starter Databases

These functions work seamlessly with the starter databases:

```sql
-- Use with accounting.db
SELECT * FROM trial_balance;

-- Use with users.db
SELECT * FROM user_permissions WHERE username = 'john';

-- Use with crm.db
SELECT * FROM sales_pipeline;
```

### Export Results

```sql
-- Export to CSV
.mode csv
.output results.csv
SELECT * FROM your_query;
.output stdout
```

## Learning Resources

### SQLite Documentation
- Built-in functions: https://www.sqlite.org/lang_corefunc.html
- Window functions: https://www.sqlite.org/windowfunctions.html
- Date functions: https://www.sqlite.org/lang_datefunc.html

### Practice
- Open any starter database
- Try example queries
- Modify for your needs
- Combine multiple functions

## Contributing

These functions are templates. Customize them for your specific:
- Table names
- Column names
- Business logic
- Data types

## License

These functions are free to use, modify, and distribute for any purpose.
