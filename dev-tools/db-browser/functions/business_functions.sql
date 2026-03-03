-- Business Logic Functions
-- Common business calculations and analytics

-- ============================================
-- REVENUE METRICS
-- ============================================

-- Revenue growth rate
SELECT 
    current_period,
    current_revenue,
    previous_revenue,
    ((current_revenue - previous_revenue) / previous_revenue) * 100 as revenue_growth_pct
FROM (
    SELECT 
        period as current_period,
        revenue as current_revenue,
        LAG(revenue) OVER (ORDER BY period) as previous_revenue
    FROM period_revenue
);

-- Average Revenue Per User (ARPU)
SELECT 
    period,
    total_revenue,
    active_users,
    total_revenue / active_users as arpu
FROM user_metrics;

-- Customer Lifetime Value (CLV)
SELECT 
    customer_id,
    SUM(purchase_amount) as total_spent,
    COUNT(DISTINCT order_id) as purchase_count,
    AVG(purchase_amount) as avg_order_value,
    SUM(purchase_amount) / COUNT(DISTINCT order_id) as avg_transaction_value
FROM orders
GROUP BY customer_id;

-- ============================================
-- PROFITABILITY METRICS
-- ============================================

-- Gross Profit Margin
SELECT 
    product_id,
    revenue,
    cost_of_goods,
    revenue - cost_of_goods as gross_profit,
    ((revenue - cost_of_goods) / revenue) * 100 as gross_margin_pct
FROM product_financials;

-- Net Profit Margin
SELECT 
    period,
    revenue,
    total_expenses,
    revenue - total_expenses as net_profit,
    ((revenue - total_expenses) / revenue) * 100 as net_margin_pct
FROM financial_summary;

-- Return on Investment (ROI)
SELECT 
    investment_id,
    gain,
    cost,
    ((gain - cost) / cost) * 100 as roi_pct
FROM investments;

-- Break-even Point
SELECT 
    fixed_costs,
    price_per_unit,
    variable_cost_per_unit,
    fixed_costs / (price_per_unit - variable_cost_per_unit) as breakeven_units
FROM product_costs;

-- ============================================
-- CUSTOMER METRICS
-- ============================================

-- Customer Acquisition Cost (CAC)
SELECT 
    period,
    marketing_spend,
    new_customers,
    marketing_spend / new_customers as cac
FROM acquisition_data;

-- Customer Churn Rate
SELECT 
    period,
    customers_start,
    customers_lost,
    (customers_lost * 100.0 / customers_start) as churn_rate_pct,
    100 - (customers_lost * 100.0 / customers_start) as retention_rate_pct
FROM customer_lifecycle;

-- Customer Retention Rate
SELECT 
    cohort_month,
    initial_customers,
    retained_customers,
    (retained_customers * 100.0 / initial_customers) as retention_rate
FROM cohort_analysis;

-- Repeat Purchase Rate
SELECT 
    customer_id,
    COUNT(DISTINCT order_id) as order_count,
    CASE 
        WHEN COUNT(DISTINCT order_id) > 1 THEN 'Repeat'
        ELSE 'One-time'
    END as customer_type
FROM orders
GROUP BY customer_id;

-- ============================================
-- CONVERSION METRICS
-- ============================================

-- Conversion Rate
SELECT 
    stage,
    count,
    LAG(count) OVER (ORDER BY stage_order) as previous_stage_count,
    (count * 100.0 / LAG(count) OVER (ORDER BY stage_order)) as conversion_rate
FROM funnel_stages;

-- Cart Abandonment Rate
SELECT 
    carts_created,
    orders_completed,
    carts_created - orders_completed as carts_abandoned,
    ((carts_created - orders_completed) * 100.0 / carts_created) as abandonment_rate
FROM cart_metrics;

-- ============================================
-- INVENTORY METRICS
-- ============================================

-- Inventory Turnover Ratio
SELECT 
    product_id,
    cost_of_goods_sold,
    avg_inventory_value,
    cost_of_goods_sold / avg_inventory_value as inventory_turnover
FROM inventory_metrics;

-- Days Inventory Outstanding (DIO)
SELECT 
    365.0 / inventory_turnover as days_inventory_outstanding
FROM (
    SELECT cost_of_goods_sold / avg_inventory_value as inventory_turnover
    FROM inventory_metrics
);

-- Stock-to-Sales Ratio
SELECT 
    period,
    ending_inventory_value,
    sales_value,
    ending_inventory_value / sales_value as stock_to_sales_ratio
FROM inventory_sales;

-- Reorder Point
SELECT 
    product_id,
    daily_usage_rate,
    lead_time_days,
    safety_stock,
    (daily_usage_rate * lead_time_days) + safety_stock as reorder_point
FROM inventory_params;

-- ============================================
-- SALES METRICS
-- ============================================

-- Sales per Square Foot
SELECT 
    store_id,
    total_sales,
    square_footage,
    total_sales / square_footage as sales_per_sqft
FROM store_metrics;

-- Average Transaction Value (ATV)
SELECT 
    period,
    total_sales,
    transaction_count,
    total_sales / transaction_count as avg_transaction_value
FROM sales_summary;

-- Units Per Transaction (UPT)
SELECT 
    period,
    units_sold,
    transaction_count,
    units_sold / transaction_count as units_per_transaction
FROM sales_metrics;

-- ============================================
-- PRICING METRICS
-- ============================================

-- Price Elasticity (approximation)
SELECT 
    product_id,
    ((new_quantity - old_quantity) / old_quantity) /
    ((new_price - old_price) / old_price) as price_elasticity
FROM price_changes;

-- Discount Impact
SELECT 
    discount_pct,
    AVG(order_value) as avg_order_value,
    COUNT(*) as order_count,
    SUM(order_value) as total_revenue,
    SUM(order_value * discount_pct / 100) as revenue_lost_to_discount
FROM orders
GROUP BY discount_pct;

-- ============================================
-- FINANCIAL RATIOS
-- ============================================

-- Current Ratio (Liquidity)
SELECT 
    current_assets,
    current_liabilities,
    current_assets / current_liabilities as current_ratio
FROM balance_sheet;

-- Quick Ratio (Acid Test)
SELECT 
    current_assets,
    inventory,
    current_liabilities,
    (current_assets - inventory) / current_liabilities as quick_ratio
FROM balance_sheet;

-- Debt-to-Equity Ratio
SELECT 
    total_debt,
    total_equity,
    total_debt / total_equity as debt_to_equity_ratio
FROM balance_sheet;

-- Return on Assets (ROA)
SELECT 
    net_income,
    total_assets,
    (net_income / total_assets) * 100 as roa_pct
FROM financial_statements;

-- Return on Equity (ROE)
SELECT 
    net_income,
    shareholders_equity,
    (net_income / shareholders_equity) * 100 as roe_pct
FROM financial_statements;

-- ============================================
-- PRODUCTIVITY METRICS
-- ============================================

-- Revenue Per Employee
SELECT 
    period,
    total_revenue,
    employee_count,
    total_revenue / employee_count as revenue_per_employee
FROM company_metrics;

-- Profit Per Employee
SELECT 
    period,
    net_profit,
    employee_count,
    net_profit / employee_count as profit_per_employee
FROM company_metrics;

-- ============================================
-- MARKET SHARE
-- ============================================

-- Market Share Calculation
SELECT 
    company,
    company_revenue,
    total_market_revenue,
    (company_revenue * 100.0 / total_market_revenue) as market_share_pct
FROM (
    SELECT 
        company,
        revenue as company_revenue,
        (SELECT SUM(revenue) FROM market_data) as total_market_revenue
    FROM market_data
);

-- ============================================
-- COHORT ANALYSIS
-- ============================================

-- Monthly Cohort Retention
SELECT 
    cohort_month,
    months_since_signup,
    COUNT(DISTINCT customer_id) as retained_customers,
    (COUNT(DISTINCT customer_id) * 100.0 / 
     MAX(COUNT(DISTINCT customer_id)) OVER (PARTITION BY cohort_month)) as retention_pct
FROM (
    SELECT 
        customer_id,
        STRFTIME('%Y-%m', signup_date) as cohort_month,
        (STRFTIME('%Y', purchase_date) - STRFTIME('%Y', signup_date)) * 12 +
        (STRFTIME('%m', purchase_date) - STRFTIME('%m', signup_date)) as months_since_signup
    FROM customer_purchases
)
GROUP BY cohort_month, months_since_signup
ORDER BY cohort_month, months_since_signup;

-- ============================================
-- RFM ANALYSIS (Recency, Frequency, Monetary)
-- ============================================

SELECT 
    customer_id,
    -- Recency (days since last purchase)
    JULIANDAY('now') - JULIANDAY(MAX(purchase_date)) as recency_days,
    -- Frequency (number of purchases)
    COUNT(DISTINCT order_id) as frequency,
    -- Monetary (total spent)
    SUM(order_amount) as monetary_value,
    -- RFM Score (simple 1-5 scale)
    CASE 
        WHEN JULIANDAY('now') - JULIANDAY(MAX(purchase_date)) <= 30 THEN 5
        WHEN JULIANDAY('now') - JULIANDAY(MAX(purchase_date)) <= 90 THEN 4
        WHEN JULIANDAY('now') - JULIANDAY(MAX(purchase_date)) <= 180 THEN 3
        WHEN JULIANDAY('now') - JULIANDAY(MAX(purchase_date)) <= 365 THEN 2
        ELSE 1
    END as recency_score,
    CASE 
        WHEN COUNT(DISTINCT order_id) >= 10 THEN 5
        WHEN COUNT(DISTINCT order_id) >= 7 THEN 4
        WHEN COUNT(DISTINCT order_id) >= 4 THEN 3
        WHEN COUNT(DISTINCT order_id) >= 2 THEN 2
        ELSE 1
    END as frequency_score,
    NTILE(5) OVER (ORDER BY SUM(order_amount)) as monetary_score
FROM orders
GROUP BY customer_id;

-- ============================================
-- ABC ANALYSIS (Inventory Classification)
-- ============================================

WITH product_value AS (
    SELECT 
        product_id,
        SUM(quantity * price) as total_value
    FROM sales
    GROUP BY product_id
),
cumulative_value AS (
    SELECT 
        product_id,
        total_value,
        SUM(total_value) OVER (ORDER BY total_value DESC) as cumulative_value,
        SUM(total_value) OVER () as total_value_all,
        (SUM(total_value) OVER (ORDER BY total_value DESC) * 100.0 / 
         SUM(total_value) OVER ()) as cumulative_pct
    FROM product_value
)
SELECT 
    product_id,
    total_value,
    cumulative_pct,
    CASE 
        WHEN cumulative_pct <= 80 THEN 'A'
        WHEN cumulative_pct <= 95 THEN 'B'
        ELSE 'C'
    END as abc_classification
FROM cumulative_value;

-- ============================================
-- PARETO ANALYSIS (80/20 Rule)
-- ============================================

WITH ranked_items AS (
    SELECT 
        item,
        value,
        SUM(value) OVER (ORDER BY value DESC) as running_total,
        SUM(value) OVER () as total_value,
        (SUM(value) OVER (ORDER BY value DESC) * 100.0 / SUM(value) OVER ()) as cumulative_pct
    FROM items
)
SELECT 
    item,
    value,
    cumulative_pct,
    CASE WHEN cumulative_pct <= 80 THEN 'Top 80%' ELSE 'Bottom 20%' END as pareto_group
FROM ranked_items;

-- ============================================
-- SEASONALITY INDEX
-- ============================================

-- Monthly seasonality calculation
WITH monthly_avg AS (
    SELECT 
        STRFTIME('%m', date) as month,
        AVG(sales) as avg_monthly_sales
    FROM daily_sales
    GROUP BY STRFTIME('%m', date)
),
overall_avg AS (
    SELECT AVG(sales) as overall_avg_sales FROM daily_sales
)
SELECT 
    month,
    avg_monthly_sales,
    overall_avg_sales,
    (avg_monthly_sales / overall_avg_sales) * 100 as seasonality_index
FROM monthly_avg, overall_avg
ORDER BY month;

-- ============================================
-- FORECAST ACCURACY
-- ============================================

-- Mean Absolute Percentage Error (MAPE)
SELECT 
    AVG(ABS((actual - forecast) / actual)) * 100 as mape_pct
FROM forecasts
WHERE actual > 0;

-- Mean Absolute Error (MAE)
SELECT 
    AVG(ABS(actual - forecast)) as mae
FROM forecasts;

-- Root Mean Square Error (RMSE)
SELECT 
    SQRT(AVG((actual - forecast) * (actual - forecast))) as rmse
FROM forecasts;

-- ============================================
-- LEAD SCORING
-- ============================================

SELECT 
    lead_id,
    -- Demographic score
    CASE 
        WHEN company_size > 1000 THEN 20
        WHEN company_size > 100 THEN 15
        WHEN company_size > 10 THEN 10
        ELSE 5
    END +
    -- Engagement score
    CASE 
        WHEN email_opens > 10 THEN 20
        WHEN email_opens > 5 THEN 15
        WHEN email_opens > 0 THEN 10
        ELSE 0
    END +
    -- Behavior score
    CASE 
        WHEN website_visits > 10 THEN 30
        WHEN website_visits > 5 THEN 20
        WHEN website_visits > 1 THEN 10
        ELSE 0
    END as lead_score
FROM leads;

-- ============================================
-- NET PROMOTER SCORE (NPS)
-- ============================================

SELECT 
    COUNT(CASE WHEN score >= 9 THEN 1 END) * 100.0 / COUNT(*) as promoters_pct,
    COUNT(CASE WHEN score <= 6 THEN 1 END) * 100.0 / COUNT(*) as detractors_pct,
    (COUNT(CASE WHEN score >= 9 THEN 1 END) - 
     COUNT(CASE WHEN score <= 6 THEN 1 END)) * 100.0 / COUNT(*) as nps_score
FROM survey_responses;
