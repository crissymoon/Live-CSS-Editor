-- Double Entry Accounting System
-- Based on standard accounting principles

-- Chart of Accounts
CREATE TABLE accounts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    account_code VARCHAR(20) NOT NULL UNIQUE,
    account_name VARCHAR(100) NOT NULL,
    account_type VARCHAR(20) NOT NULL CHECK(account_type IN ('ASSET', 'LIABILITY', 'EQUITY', 'REVENUE', 'EXPENSE')),
    normal_balance VARCHAR(10) NOT NULL CHECK(normal_balance IN ('DEBIT', 'CREDIT')),
    parent_account_id INTEGER,
    is_active BOOLEAN DEFAULT 1,
    description TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (parent_account_id) REFERENCES accounts(id)
);

-- Journal Entries
CREATE TABLE journal_entries (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    entry_number VARCHAR(50) NOT NULL UNIQUE,
    entry_date DATE NOT NULL,
    description TEXT NOT NULL,
    reference VARCHAR(100),
    posted BOOLEAN DEFAULT 0,
    created_by VARCHAR(100),
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    posted_at DATETIME
);

-- Journal Entry Lines (double entry)
CREATE TABLE journal_lines (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    journal_entry_id INTEGER NOT NULL,
    account_id INTEGER NOT NULL,
    debit_amount DECIMAL(15,2) DEFAULT 0.00,
    credit_amount DECIMAL(15,2) DEFAULT 0.00,
    memo TEXT,
    FOREIGN KEY (journal_entry_id) REFERENCES journal_entries(id) ON DELETE CASCADE,
    FOREIGN KEY (account_id) REFERENCES accounts(id),
    CHECK (debit_amount >= 0 AND credit_amount >= 0),
    CHECK ((debit_amount > 0 AND credit_amount = 0) OR (credit_amount > 0 AND debit_amount = 0))
);

-- Fiscal Periods
CREATE TABLE fiscal_periods (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    period_name VARCHAR(50) NOT NULL,
    start_date DATE NOT NULL,
    end_date DATE NOT NULL,
    is_closed BOOLEAN DEFAULT 0,
    closed_at DATETIME
);

-- Sample Chart of Accounts
INSERT INTO accounts (account_code, account_name, account_type, normal_balance, description) VALUES
('1000', 'Assets', 'ASSET', 'DEBIT', 'Root asset account'),
('1100', 'Current Assets', 'ASSET', 'DEBIT', 'Assets convertible to cash within one year'),
('1110', 'Cash', 'ASSET', 'DEBIT', 'Cash on hand and in bank'),
('1120', 'Accounts Receivable', 'ASSET', 'DEBIT', 'Money owed by customers'),
('1130', 'Inventory', 'ASSET', 'DEBIT', 'Goods available for sale'),
('1200', 'Fixed Assets', 'ASSET', 'DEBIT', 'Long-term assets'),
('1210', 'Equipment', 'ASSET', 'DEBIT', 'Office and production equipment'),
('1220', 'Accumulated Depreciation', 'ASSET', 'CREDIT', 'Contra asset account'),

('2000', 'Liabilities', 'LIABILITY', 'CREDIT', 'Root liability account'),
('2100', 'Current Liabilities', 'LIABILITY', 'CREDIT', 'Obligations due within one year'),
('2110', 'Accounts Payable', 'LIABILITY', 'CREDIT', 'Money owed to suppliers'),
('2120', 'Accrued Expenses', 'LIABILITY', 'CREDIT', 'Expenses incurred but not yet paid'),
('2200', 'Long-term Liabilities', 'LIABILITY', 'CREDIT', 'Obligations due after one year'),
('2210', 'Loans Payable', 'LIABILITY', 'CREDIT', 'Long-term debt obligations'),

('3000', 'Equity', 'EQUITY', 'CREDIT', 'Root equity account'),
('3100', 'Owners Equity', 'EQUITY', 'CREDIT', 'Owner investment and retained earnings'),
('3200', 'Retained Earnings', 'EQUITY', 'CREDIT', 'Accumulated profits'),

('4000', 'Revenue', 'REVENUE', 'CREDIT', 'Root revenue account'),
('4100', 'Sales Revenue', 'REVENUE', 'CREDIT', 'Income from primary operations'),
('4200', 'Service Revenue', 'REVENUE', 'CREDIT', 'Income from services provided'),

('5000', 'Expenses', 'EXPENSE', 'DEBIT', 'Root expense account'),
('5100', 'Cost of Goods Sold', 'EXPENSE', 'DEBIT', 'Direct costs of products sold'),
('5200', 'Operating Expenses', 'EXPENSE', 'DEBIT', 'Day-to-day business expenses'),
('5210', 'Salaries and Wages', 'EXPENSE', 'DEBIT', 'Employee compensation'),
('5220', 'Rent Expense', 'EXPENSE', 'DEBIT', 'Facility rental costs'),
('5230', 'Utilities', 'EXPENSE', 'DEBIT', 'Electric, water, gas'),
('5240', 'Office Supplies', 'EXPENSE', 'DEBIT', 'Supplies and materials');

-- Sample fiscal period
INSERT INTO fiscal_periods (period_name, start_date, end_date) VALUES
('Q1 2026', '2026-01-01', '2026-03-31'),
('Q2 2026', '2026-04-01', '2026-06-30'),
('Q3 2026', '2026-07-01', '2026-09-30'),
('Q4 2026', '2026-10-01', '2026-12-31');

-- Indexes for performance
CREATE INDEX idx_journal_lines_journal ON journal_lines(journal_entry_id);
CREATE INDEX idx_journal_lines_account ON journal_lines(account_id);
CREATE INDEX idx_journal_entries_date ON journal_entries(entry_date);
CREATE INDEX idx_accounts_type ON accounts(account_type);

-- View for trial balance
CREATE VIEW trial_balance AS
SELECT 
    a.account_code,
    a.account_name,
    a.account_type,
    SUM(jl.debit_amount) as total_debits,
    SUM(jl.credit_amount) as total_credits,
    CASE 
        WHEN a.normal_balance = 'DEBIT' THEN SUM(jl.debit_amount) - SUM(jl.credit_amount)
        ELSE SUM(jl.credit_amount) - SUM(jl.debit_amount)
    END as balance
FROM accounts a
LEFT JOIN journal_lines jl ON a.id = jl.account_id
LEFT JOIN journal_entries je ON jl.journal_entry_id = je.id
WHERE je.posted = 1 OR je.posted IS NULL
GROUP BY a.id, a.account_code, a.account_name, a.account_type, a.normal_balance
ORDER BY a.account_code;
