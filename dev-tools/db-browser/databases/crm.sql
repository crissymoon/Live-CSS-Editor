-- CRM (Customer Relationship Management)
-- Sales pipeline, contacts, and customer management

-- Companies
CREATE TABLE companies (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    company_name VARCHAR(200) NOT NULL,
    website VARCHAR(200),
    industry VARCHAR(100),
    employee_count INTEGER,
    annual_revenue DECIMAL(15,2),
    phone VARCHAR(20),
    email VARCHAR(100),
    billing_address TEXT,
    shipping_address TEXT,
    company_type VARCHAR(50) CHECK(company_type IN ('PROSPECT', 'CUSTOMER', 'PARTNER', 'VENDOR', 'COMPETITOR')),
    status VARCHAR(20) DEFAULT 'ACTIVE' CHECK(status IN ('ACTIVE', 'INACTIVE', 'CLOSED')),
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- Contacts
CREATE TABLE contacts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    company_id INTEGER,
    first_name VARCHAR(50) NOT NULL,
    last_name VARCHAR(50) NOT NULL,
    title VARCHAR(100),
    email VARCHAR(100),
    phone VARCHAR(20),
    mobile VARCHAR(20),
    department VARCHAR(100),
    is_primary BOOLEAN DEFAULT 0,
    is_active BOOLEAN DEFAULT 1,
    linkedin_url VARCHAR(200),
    notes TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (company_id) REFERENCES companies(id)
);

-- Sales Opportunities/Deals
CREATE TABLE opportunities (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    opportunity_name VARCHAR(200) NOT NULL,
    company_id INTEGER NOT NULL,
    contact_id INTEGER,
    amount DECIMAL(15,2),
    stage VARCHAR(50) NOT NULL CHECK(stage IN ('PROSPECTING', 'QUALIFICATION', 'PROPOSAL', 'NEGOTIATION', 'CLOSED_WON', 'CLOSED_LOST')),
    probability INTEGER CHECK(probability >= 0 AND probability <= 100),
    expected_close_date DATE,
    actual_close_date DATE,
    owner_id INTEGER,
    lead_source VARCHAR(100),
    next_step TEXT,
    description TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (company_id) REFERENCES companies(id),
    FOREIGN KEY (contact_id) REFERENCES contacts(id)
);

-- Activities (calls, meetings, emails, tasks)
CREATE TABLE activities (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    activity_type VARCHAR(50) NOT NULL CHECK(activity_type IN ('CALL', 'MEETING', 'EMAIL', 'TASK', 'NOTE')),
    subject VARCHAR(200) NOT NULL,
    description TEXT,
    company_id INTEGER,
    contact_id INTEGER,
    opportunity_id INTEGER,
    assigned_to INTEGER,
    status VARCHAR(20) DEFAULT 'PENDING' CHECK(status IN ('PENDING', 'COMPLETED', 'CANCELLED')),
    priority VARCHAR(20) CHECK(priority IN ('LOW', 'MEDIUM', 'HIGH')),
    due_date DATETIME,
    completed_at DATETIME,
    duration_minutes INTEGER,
    outcome TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (company_id) REFERENCES companies(id),
    FOREIGN KEY (contact_id) REFERENCES contacts(id),
    FOREIGN KEY (opportunity_id) REFERENCES opportunities(id)
);

-- Products/Services
CREATE TABLE products (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    product_code VARCHAR(50) NOT NULL UNIQUE,
    product_name VARCHAR(200) NOT NULL,
    description TEXT,
    category VARCHAR(100),
    unit_price DECIMAL(10,2),
    cost DECIMAL(10,2),
    is_active BOOLEAN DEFAULT 1,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- Opportunity Products
CREATE TABLE opportunity_products (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    opportunity_id INTEGER NOT NULL,
    product_id INTEGER NOT NULL,
    quantity INTEGER DEFAULT 1,
    unit_price DECIMAL(10,2),
    discount_percent DECIMAL(5,2) DEFAULT 0,
    total_price DECIMAL(15,2),
    FOREIGN KEY (opportunity_id) REFERENCES opportunities(id) ON DELETE CASCADE,
    FOREIGN KEY (product_id) REFERENCES products(id)
);

-- Lead Sources
CREATE TABLE lead_sources (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    source_name VARCHAR(100) NOT NULL UNIQUE,
    description TEXT
);

-- Sales Pipeline Stages
CREATE TABLE pipeline_stages (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    stage_name VARCHAR(50) NOT NULL UNIQUE,
    stage_order INTEGER NOT NULL,
    probability INTEGER DEFAULT 0,
    is_closed BOOLEAN DEFAULT 0
);

-- Email Templates
CREATE TABLE email_templates (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    template_name VARCHAR(100) NOT NULL,
    subject VARCHAR(200),
    body TEXT,
    template_type VARCHAR(50),
    is_active BOOLEAN DEFAULT 1,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- Notes
CREATE TABLE notes (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    note_text TEXT NOT NULL,
    company_id INTEGER,
    contact_id INTEGER,
    opportunity_id INTEGER,
    created_by INTEGER,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (company_id) REFERENCES companies(id),
    FOREIGN KEY (contact_id) REFERENCES contacts(id),
    FOREIGN KEY (opportunity_id) REFERENCES opportunities(id)
);

-- Sample Data
INSERT INTO companies (company_name, website, industry, phone, email, company_type, status) VALUES
('Acme Corporation', 'www.acme.com', 'Technology', '555-0001', 'sales@acme.com', 'CUSTOMER', 'ACTIVE'),
('TechStart Inc', 'www.techstart.io', 'Software', '555-0002', 'info@techstart.io', 'PROSPECT', 'ACTIVE'),
('Global Solutions', 'www.globalsolutions.com', 'Consulting', '555-0003', 'contact@globalsolutions.com', 'CUSTOMER', 'ACTIVE'),
('Innovation Labs', 'www.innovationlabs.net', 'Research', '555-0004', 'hello@innovationlabs.net', 'PROSPECT', 'ACTIVE');

INSERT INTO contacts (company_id, first_name, last_name, title, email, phone, is_primary) VALUES
(1, 'John', 'Smith', 'CEO', 'john.smith@acme.com', '555-0101', 1),
(1, 'Sarah', 'Johnson', 'CTO', 'sarah.johnson@acme.com', '555-0102', 0),
(2, 'Mike', 'Williams', 'Founder', 'mike@techstart.io', '555-0201', 1),
(3, 'Emily', 'Brown', 'Operations Manager', 'emily.brown@globalsolutions.com', '555-0301', 1),
(4, 'David', 'Davis', 'Research Director', 'david.davis@innovationlabs.net', '555-0401', 1);

INSERT INTO lead_sources (source_name, description) VALUES
('Website', 'Organic website traffic'),
('Referral', 'Customer referral'),
('Cold Call', 'Outbound cold calling'),
('LinkedIn', 'LinkedIn outreach'),
('Trade Show', 'Industry events'),
('Email Campaign', 'Marketing email campaigns');

INSERT INTO pipeline_stages (stage_name, stage_order, probability, is_closed) VALUES
('PROSPECTING', 1, 10, 0),
('QUALIFICATION', 2, 25, 0),
('PROPOSAL', 3, 50, 0),
('NEGOTIATION', 4, 75, 0),
('CLOSED_WON', 5, 100, 1),
('CLOSED_LOST', 6, 0, 1);

INSERT INTO products (product_code, product_name, description, category, unit_price, cost) VALUES
('PROD-001', 'Enterprise License', 'Annual enterprise software license', 'Software', 50000.00, 5000.00),
('PROD-002', 'Professional Services', 'Implementation and consulting', 'Services', 200.00, 75.00),
('PROD-003', 'Support Package', 'Premium support and maintenance', 'Support', 10000.00, 2000.00),
('PROD-004', 'Training Program', 'On-site training for teams', 'Training', 5000.00, 1000.00);

INSERT INTO opportunities (opportunity_name, company_id, contact_id, amount, stage, probability, expected_close_date, lead_source) VALUES
('Acme Corp - Enterprise Deal', 1, 1, 150000.00, 'NEGOTIATION', 75, '2026-04-15', 'Referral'),
('TechStart - Startup Package', 2, 3, 25000.00, 'PROPOSAL', 50, '2026-03-30', 'Website'),
('Global Solutions - Expansion', 3, 4, 100000.00, 'QUALIFICATION', 25, '2026-05-20', 'Cold Call');

INSERT INTO activities (activity_type, subject, description, company_id, contact_id, status, due_date, priority) VALUES
('CALL', 'Follow-up call with John', 'Discuss contract terms and timeline', 1, 1, 'PENDING', '2026-03-05 14:00:00', 'HIGH'),
('MEETING', 'Product demo for TechStart', 'Demonstrate key features', 2, 3, 'PENDING', '2026-03-08 10:00:00', 'MEDIUM'),
('EMAIL', 'Send proposal to Global Solutions', 'Custom proposal with pricing', 3, 4, 'COMPLETED', '2026-03-01 09:00:00', 'HIGH');

-- Indexes
CREATE INDEX idx_companies_status ON companies(status);
CREATE INDEX idx_contacts_company ON contacts(company_id);
CREATE INDEX idx_opportunities_company ON opportunities(company_id);
CREATE INDEX idx_opportunities_stage ON opportunities(stage);
CREATE INDEX idx_activities_company ON activities(company_id);
CREATE INDEX idx_activities_contact ON activities(contact_id);
CREATE INDEX idx_activities_due_date ON activities(due_date);

-- View for sales pipeline
CREATE VIEW sales_pipeline AS
SELECT 
    o.opportunity_name,
    c.company_name,
    o.stage,
    o.amount,
    o.probability,
    o.expected_close_date,
    p.stage_order,
    o.amount * (o.probability / 100.0) as weighted_amount
FROM opportunities o
JOIN companies c ON o.company_id = c.id
JOIN pipeline_stages p ON o.stage = p.stage_name
WHERE o.stage NOT IN ('CLOSED_WON', 'CLOSED_LOST')
ORDER BY p.stage_order, o.expected_close_date;

-- View for contact directory
CREATE VIEW contact_directory AS
SELECT 
    c.id,
    c.first_name || ' ' || c.last_name as full_name,
    c.title,
    c.email,
    c.phone,
    c.mobile,
    comp.company_name,
    c.is_primary
FROM contacts c
LEFT JOIN companies comp ON c.company_id = comp.id
WHERE c.is_active = 1
ORDER BY c.last_name, c.first_name;
