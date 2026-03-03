-- Aggregate Functions for Data Analysis
-- These functions work with groups of rows to produce summary values

-- ============================================
-- BASIC AGGREGATE FUNCTIONS
-- ============================================

-- SUM: Total of all values
-- Example: Calculate total sales
SELECT SUM(amount) as total_sales FROM transactions;

-- AVG: Average (mean) of values
-- Example: Average order value
SELECT AVG(order_total) as average_order_value FROM orders;

-- COUNT: Number of rows
-- Example: Total number of customers
SELECT COUNT(*) as total_customers FROM customers;
SELECT COUNT(DISTINCT customer_id) as unique_customers FROM orders;

-- MIN: Minimum value
-- Example: Lowest price
SELECT MIN(price) as lowest_price FROM products;

-- MAX: Maximum value
-- Example: Highest salary
SELECT MAX(salary) as highest_salary FROM employees;

-- ============================================
-- GROUPED AGGREGATES
-- ============================================

-- Group by category with multiple aggregates
SELECT 
    category,
    COUNT(*) as product_count,
    AVG(price) as avg_price,
    MIN(price) as min_price,
    MAX(price) as max_price,
    SUM(stock_quantity) as total_stock
FROM products
GROUP BY category
ORDER BY product_count DESC;

-- Sales by month
SELECT 
    strftime('%Y-%m', order_date) as month,
    COUNT(*) as order_count,
    SUM(total_amount) as monthly_revenue,
    AVG(total_amount) as avg_order_value
FROM orders
GROUP BY strftime('%Y-%m', order_date)
ORDER BY month;

-- Customer spending analysis
SELECT 
    customer_id,
    COUNT(*) as order_count,
    SUM(amount) as total_spent,
    AVG(amount) as avg_spent,
    MIN(order_date) as first_order,
    MAX(order_date) as last_order
FROM orders
GROUP BY customer_id
HAVING total_spent > 1000
ORDER BY total_spent DESC;

-- ============================================
-- CONDITIONAL AGGREGATES
-- ============================================

-- Count with conditions
SELECT 
    COUNT(CASE WHEN status = 'completed' THEN 1 END) as completed_orders,
    COUNT(CASE WHEN status = 'pending' THEN 1 END) as pending_orders,
    COUNT(CASE WHEN status = 'cancelled' THEN 1 END) as cancelled_orders
FROM orders;

-- Sum with conditions
SELECT 
    SUM(CASE WHEN payment_method = 'cash' THEN amount ELSE 0 END) as cash_total,
    SUM(CASE WHEN payment_method = 'credit' THEN amount ELSE 0 END) as credit_total,
    SUM(CASE WHEN payment_method = 'debit' THEN amount ELSE 0 END) as debit_total
FROM transactions;

-- ============================================
-- PERCENTAGE CALCULATIONS
-- ============================================

-- Calculate percentage of total
SELECT 
    category,
    COUNT(*) as count,
    ROUND(COUNT(*) * 100.0 / (SELECT COUNT(*) FROM products), 2) as percentage
FROM products
GROUP BY category;

-- ============================================
-- RUNNING TOTALS
-- ============================================

-- Running total with window function
SELECT 
    order_date,
    amount,
    SUM(amount) OVER (ORDER BY order_date) as running_total
FROM orders
ORDER BY order_date;

-- ============================================
-- GROUP_CONCAT (String Aggregation)
-- ============================================

-- Concatenate values
SELECT 
    customer_id,
    GROUP_CONCAT(product_name, ', ') as purchased_products
FROM order_items
JOIN products ON order_items.product_id = products.id
GROUP BY customer_id;

-- ============================================
-- TOTAL() Function
-- ============================================

-- TOTAL is like SUM but returns 0.0 instead of NULL for empty sets
SELECT 
    category,
    TOTAL(price) as total_price,
    SUM(price) as sum_price  -- Will return NULL if no rows
FROM products
GROUP BY category;

-- ============================================
-- MEDIAN CALCULATION
-- ============================================

-- Calculate median (50th percentile)
-- Method 1: Using subquery
SELECT AVG(price) as median_price
FROM (
    SELECT price
    FROM products
    ORDER BY price
    LIMIT 2 - (SELECT COUNT(*) FROM products) % 2
    OFFSET (SELECT (COUNT(*) - 1) / 2 FROM products)
);

-- ============================================
-- MODE (Most Common Value)
-- ============================================

-- Find most common value
SELECT category, COUNT(*) as frequency
FROM products
GROUP BY category
ORDER BY frequency DESC
LIMIT 1;

-- ============================================
-- VARIANCE AND STANDARD DEVIATION
-- ============================================

-- Population variance
SELECT 
    category,
    AVG(price) as mean_price,
    AVG(price * price) - AVG(price) * AVG(price) as variance,
    SQRT(AVG(price * price) - AVG(price) * AVG(price)) as std_dev
FROM products
GROUP BY category;

-- ============================================
-- PERCENTILES
-- ============================================

-- 25th, 50th (median), 75th percentile
WITH sorted_data AS (
    SELECT 
        price,
        ROW_NUMBER() OVER (ORDER BY price) as row_num,
        COUNT(*) OVER () as total_count
    FROM products
)
SELECT 
    MAX(CASE WHEN row_num = CAST(total_count * 0.25 AS INTEGER) THEN price END) as percentile_25,
    MAX(CASE WHEN row_num = CAST(total_count * 0.50 AS INTEGER) THEN price END) as percentile_50,
    MAX(CASE WHEN row_num = CAST(total_count * 0.75 AS INTEGER) THEN price END) as percentile_75
FROM sorted_data;

-- ============================================
-- CORRELATION
-- ============================================

-- Correlation between two variables
SELECT 
    (COUNT(*) * SUM(x * y) - SUM(x) * SUM(y)) /
    (SQRT(COUNT(*) * SUM(x * x) - SUM(x) * SUM(x)) *
     SQRT(COUNT(*) * SUM(y * y) - SUM(y) * SUM(y))) as correlation
FROM (
    SELECT price as x, sales_count as y FROM products
);

-- ============================================
-- EXAMPLES WITH REAL SCENARIOS
-- ============================================

-- Sales Performance Dashboard
SELECT 
    'Total Revenue' as metric,
    printf('$%,.2f', SUM(amount)) as value
FROM sales
UNION ALL
SELECT 
    'Average Sale',
    printf('$%,.2f', AVG(amount))
FROM sales
UNION ALL
SELECT 
    'Total Transactions',
    CAST(COUNT(*) AS TEXT)
FROM sales
UNION ALL
SELECT 
    'Unique Customers',
    CAST(COUNT(DISTINCT customer_id) AS TEXT)
FROM sales;

-- Product Performance
SELECT 
    product_name,
    COUNT(*) as times_sold,
    SUM(quantity) as total_quantity,
    SUM(quantity * price) as total_revenue,
    AVG(quantity * price) as avg_transaction_value
FROM order_items
JOIN products ON order_items.product_id = products.id
GROUP BY product_name
ORDER BY total_revenue DESC
LIMIT 10;

-- Customer Segmentation
SELECT 
    CASE 
        WHEN total_spent >= 10000 THEN 'VIP'
        WHEN total_spent >= 5000 THEN 'Premium'
        WHEN total_spent >= 1000 THEN 'Regular'
        ELSE 'New'
    END as segment,
    COUNT(*) as customer_count,
    AVG(total_spent) as avg_spent,
    SUM(total_spent) as segment_revenue
FROM (
    SELECT customer_id, SUM(amount) as total_spent
    FROM orders
    GROUP BY customer_id
)
GROUP BY segment
ORDER BY avg_spent DESC;
