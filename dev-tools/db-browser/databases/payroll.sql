-- Payroll Management System
-- Employee compensation and payroll processing

-- Employees
CREATE TABLE employees (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    employee_number VARCHAR(20) NOT NULL UNIQUE,
    first_name VARCHAR(50) NOT NULL,
    last_name VARCHAR(50) NOT NULL,
    email VARCHAR(100),
    phone VARCHAR(20),
    hire_date DATE NOT NULL,
    termination_date DATE,
    status VARCHAR(20) DEFAULT 'ACTIVE' CHECK(status IN ('ACTIVE', 'INACTIVE', 'TERMINATED')),
    job_title VARCHAR(100),
    department VARCHAR(100),
    pay_type VARCHAR(20) NOT NULL CHECK(pay_type IN ('HOURLY', 'SALARY', 'CONTRACT')),
    pay_rate DECIMAL(10,2) NOT NULL,
    pay_frequency VARCHAR(20) CHECK(pay_frequency IN ('WEEKLY', 'BIWEEKLY', 'SEMIMONTHLY', 'MONTHLY')),
    tax_id VARCHAR(20),
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- Time Entries (for hourly employees)
CREATE TABLE time_entries (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    employee_id INTEGER NOT NULL,
    entry_date DATE NOT NULL,
    clock_in DATETIME,
    clock_out DATETIME,
    hours_worked DECIMAL(5,2),
    overtime_hours DECIMAL(5,2) DEFAULT 0,
    break_hours DECIMAL(5,2) DEFAULT 0,
    approved BOOLEAN DEFAULT 0,
    approved_by INTEGER,
    notes TEXT,
    FOREIGN KEY (employee_id) REFERENCES employees(id),
    FOREIGN KEY (approved_by) REFERENCES employees(id)
);

-- Pay Periods
CREATE TABLE pay_periods (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    period_name VARCHAR(50) NOT NULL,
    start_date DATE NOT NULL,
    end_date DATE NOT NULL,
    pay_date DATE NOT NULL,
    status VARCHAR(20) DEFAULT 'OPEN' CHECK(status IN ('OPEN', 'PROCESSING', 'CLOSED')),
    closed_at DATETIME
);

-- Payroll Runs
CREATE TABLE payroll_runs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    pay_period_id INTEGER NOT NULL,
    employee_id INTEGER NOT NULL,
    gross_pay DECIMAL(10,2) NOT NULL,
    regular_hours DECIMAL(5,2) DEFAULT 0,
    overtime_hours DECIMAL(5,2) DEFAULT 0,
    federal_tax DECIMAL(10,2) DEFAULT 0,
    state_tax DECIMAL(10,2) DEFAULT 0,
    social_security DECIMAL(10,2) DEFAULT 0,
    medicare DECIMAL(10,2) DEFAULT 0,
    retirement_401k DECIMAL(10,2) DEFAULT 0,
    health_insurance DECIMAL(10,2) DEFAULT 0,
    other_deductions DECIMAL(10,2) DEFAULT 0,
    net_pay DECIMAL(10,2) NOT NULL,
    payment_method VARCHAR(20) CHECK(payment_method IN ('DIRECT_DEPOSIT', 'CHECK', 'CASH')),
    processed BOOLEAN DEFAULT 0,
    processed_at DATETIME,
    FOREIGN KEY (pay_period_id) REFERENCES pay_periods(id),
    FOREIGN KEY (employee_id) REFERENCES employees(id)
);

-- Benefits
CREATE TABLE benefits (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    benefit_name VARCHAR(100) NOT NULL,
    benefit_type VARCHAR(50) CHECK(benefit_type IN ('HEALTH', 'DENTAL', 'VISION', 'RETIREMENT', 'LIFE', 'DISABILITY', 'OTHER')),
    provider VARCHAR(100),
    employee_cost DECIMAL(10,2) DEFAULT 0,
    employer_cost DECIMAL(10,2) DEFAULT 0,
    is_active BOOLEAN DEFAULT 1
);

-- Employee Benefits (enrollment)
CREATE TABLE employee_benefits (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    employee_id INTEGER NOT NULL,
    benefit_id INTEGER NOT NULL,
    enrollment_date DATE NOT NULL,
    termination_date DATE,
    coverage_level VARCHAR(50),
    beneficiary VARCHAR(200),
    FOREIGN KEY (employee_id) REFERENCES employees(id),
    FOREIGN KEY (benefit_id) REFERENCES benefits(id)
);

-- Paid Time Off
CREATE TABLE pto_requests (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    employee_id INTEGER NOT NULL,
    request_type VARCHAR(20) CHECK(request_type IN ('VACATION', 'SICK', 'PERSONAL', 'HOLIDAY', 'UNPAID')),
    start_date DATE NOT NULL,
    end_date DATE NOT NULL,
    hours_requested DECIMAL(5,2) NOT NULL,
    status VARCHAR(20) DEFAULT 'PENDING' CHECK(status IN ('PENDING', 'APPROVED', 'DENIED')),
    approved_by INTEGER,
    notes TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (employee_id) REFERENCES employees(id),
    FOREIGN KEY (approved_by) REFERENCES employees(id)
);

-- PTO Balances
CREATE TABLE pto_balances (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    employee_id INTEGER NOT NULL,
    pto_type VARCHAR(20) CHECK(pto_type IN ('VACATION', 'SICK', 'PERSONAL')),
    hours_available DECIMAL(5,2) DEFAULT 0,
    hours_used DECIMAL(5,2) DEFAULT 0,
    year INTEGER NOT NULL,
    FOREIGN KEY (employee_id) REFERENCES employees(id),
    UNIQUE(employee_id, pto_type, year)
);

-- Sample Data
INSERT INTO employees (employee_number, first_name, last_name, email, hire_date, job_title, department, pay_type, pay_rate, pay_frequency) VALUES
('EMP001', 'John', 'Smith', 'john.smith@company.com', '2024-01-15', 'Software Engineer', 'Engineering', 'SALARY', 85000.00, 'BIWEEKLY'),
('EMP002', 'Jane', 'Doe', 'jane.doe@company.com', '2024-03-01', 'Product Manager', 'Product', 'SALARY', 95000.00, 'BIWEEKLY'),
('EMP003', 'Bob', 'Johnson', 'bob.johnson@company.com', '2025-06-10', 'Support Specialist', 'Support', 'HOURLY', 25.00, 'BIWEEKLY'),
('EMP004', 'Alice', 'Williams', 'alice.williams@company.com', '2023-09-20', 'Senior Developer', 'Engineering', 'SALARY', 110000.00, 'BIWEEKLY');

INSERT INTO benefits (benefit_name, benefit_type, provider, employee_cost, employer_cost) VALUES
('Health Insurance PPO', 'HEALTH', 'BlueCross BlueShield', 150.00, 450.00),
('Dental Insurance', 'DENTAL', 'Delta Dental', 25.00, 35.00),
('Vision Insurance', 'VISION', 'VSP', 10.00, 15.00),
('401k Plan', 'RETIREMENT', 'Fidelity', 0.00, 0.00),
('Life Insurance', 'LIFE', 'MetLife', 0.00, 50.00);

INSERT INTO pay_periods (period_name, start_date, end_date, pay_date, status) VALUES
('PP-2026-01', '2026-01-01', '2026-01-15', '2026-01-20', 'CLOSED'),
('PP-2026-02', '2026-01-16', '2026-01-31', '2026-02-05', 'CLOSED'),
('PP-2026-03', '2026-02-01', '2026-02-15', '2026-02-20', 'CLOSED'),
('PP-2026-04', '2026-02-16', '2026-02-28', '2026-03-05', 'OPEN');

-- Indexes
CREATE INDEX idx_employees_status ON employees(status);
CREATE INDEX idx_time_entries_employee ON time_entries(employee_id);
CREATE INDEX idx_time_entries_date ON time_entries(entry_date);
CREATE INDEX idx_payroll_runs_period ON payroll_runs(pay_period_id);
CREATE INDEX idx_payroll_runs_employee ON payroll_runs(employee_id);
CREATE INDEX idx_pto_requests_employee ON pto_requests(employee_id);

-- View for active employees
CREATE VIEW active_employees AS
SELECT * FROM employees WHERE status = 'ACTIVE';

-- View for employee summary
CREATE VIEW employee_summary AS
SELECT 
    e.employee_number,
    e.first_name || ' ' || e.last_name as full_name,
    e.department,
    e.job_title,
    e.pay_type,
    e.pay_rate,
    e.hire_date,
    COUNT(DISTINCT pr.id) as total_paychecks,
    COALESCE(SUM(pr.net_pay), 0) as total_paid_ytd
FROM employees e
LEFT JOIN payroll_runs pr ON e.id = pr.employee_id
WHERE e.status = 'ACTIVE'
GROUP BY e.id;
