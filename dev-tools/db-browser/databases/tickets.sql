-- Ticket System / Help Desk
-- Support ticket tracking and management

-- Users (customers and agents)
CREATE TABLE users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username VARCHAR(50) NOT NULL UNIQUE,
    email VARCHAR(100) NOT NULL UNIQUE,
    full_name VARCHAR(100) NOT NULL,
    user_type VARCHAR(20) NOT NULL CHECK(user_type IN ('CUSTOMER', 'AGENT', 'ADMIN')),
    is_active BOOLEAN DEFAULT 1,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- Departments/Teams
CREATE TABLE departments (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    department_name VARCHAR(100) NOT NULL,
    description TEXT,
    is_active BOOLEAN DEFAULT 1
);

-- Ticket Categories
CREATE TABLE categories (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    category_name VARCHAR(100) NOT NULL,
    parent_category_id INTEGER,
    description TEXT,
    FOREIGN KEY (parent_category_id) REFERENCES categories(id)
);

-- Tickets
CREATE TABLE tickets (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    ticket_number VARCHAR(50) NOT NULL UNIQUE,
    subject VARCHAR(200) NOT NULL,
    description TEXT NOT NULL,
    customer_id INTEGER NOT NULL,
    assigned_to INTEGER,
    department_id INTEGER,
    category_id INTEGER,
    priority VARCHAR(20) DEFAULT 'MEDIUM' CHECK(priority IN ('LOW', 'MEDIUM', 'HIGH', 'URGENT')),
    status VARCHAR(20) DEFAULT 'OPEN' CHECK(status IN ('OPEN', 'IN_PROGRESS', 'PENDING', 'RESOLVED', 'CLOSED', 'REOPENED')),
    source VARCHAR(20) CHECK(source IN ('EMAIL', 'PHONE', 'WEB', 'CHAT', 'MOBILE')),
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    resolved_at DATETIME,
    closed_at DATETIME,
    due_date DATETIME,
    FOREIGN KEY (customer_id) REFERENCES users(id),
    FOREIGN KEY (assigned_to) REFERENCES users(id),
    FOREIGN KEY (department_id) REFERENCES departments(id),
    FOREIGN KEY (category_id) REFERENCES categories(id)
);

-- Ticket Comments/Responses
CREATE TABLE ticket_comments (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    ticket_id INTEGER NOT NULL,
    user_id INTEGER NOT NULL,
    comment_text TEXT NOT NULL,
    is_internal BOOLEAN DEFAULT 0,
    is_solution BOOLEAN DEFAULT 0,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (ticket_id) REFERENCES tickets(id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(id)
);

-- Ticket Attachments
CREATE TABLE ticket_attachments (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    ticket_id INTEGER NOT NULL,
    file_name VARCHAR(255) NOT NULL,
    file_path VARCHAR(500),
    file_size INTEGER,
    mime_type VARCHAR(100),
    uploaded_by INTEGER NOT NULL,
    uploaded_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (ticket_id) REFERENCES tickets(id) ON DELETE CASCADE,
    FOREIGN KEY (uploaded_by) REFERENCES users(id)
);

-- Ticket History/Audit Log
CREATE TABLE ticket_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    ticket_id INTEGER NOT NULL,
    user_id INTEGER NOT NULL,
    action VARCHAR(50) NOT NULL,
    field_changed VARCHAR(100),
    old_value TEXT,
    new_value TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (ticket_id) REFERENCES tickets(id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(id)
);

-- SLA (Service Level Agreement) Rules
CREATE TABLE sla_rules (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    rule_name VARCHAR(100) NOT NULL,
    priority VARCHAR(20),
    response_time_hours INTEGER,
    resolution_time_hours INTEGER,
    is_active BOOLEAN DEFAULT 1
);

-- Tags
CREATE TABLE tags (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    tag_name VARCHAR(50) NOT NULL UNIQUE
);

-- Ticket Tags
CREATE TABLE ticket_tags (
    ticket_id INTEGER NOT NULL,
    tag_id INTEGER NOT NULL,
    PRIMARY KEY (ticket_id, tag_id),
    FOREIGN KEY (ticket_id) REFERENCES tickets(id) ON DELETE CASCADE,
    FOREIGN KEY (tag_id) REFERENCES tags(id)
);

-- Sample Data
INSERT INTO users (username, email, full_name, user_type) VALUES
('admin', 'admin@company.com', 'System Administrator', 'ADMIN'),
('agent1', 'agent1@company.com', 'Support Agent One', 'AGENT'),
('agent2', 'agent2@company.com', 'Support Agent Two', 'AGENT'),
('customer1', 'customer1@example.com', 'John Customer', 'CUSTOMER'),
('customer2', 'customer2@example.com', 'Jane Client', 'CUSTOMER');

INSERT INTO departments (department_name, description) VALUES
('Technical Support', 'Technical issues and troubleshooting'),
('Billing', 'Payment and billing inquiries'),
('General Support', 'General questions and assistance');

INSERT INTO categories (category_name, parent_category_id, description) VALUES
('Software', NULL, 'Software related issues'),
('Bug Report', 1, 'Software bugs and errors'),
('Feature Request', 1, 'New feature suggestions'),
('Hardware', NULL, 'Hardware related issues'),
('Network', NULL, 'Network and connectivity'),
('Account', NULL, 'Account management');

INSERT INTO sla_rules (rule_name, priority, response_time_hours, resolution_time_hours) VALUES
('Urgent Priority', 'URGENT', 1, 4),
('High Priority', 'HIGH', 4, 24),
('Medium Priority', 'MEDIUM', 8, 48),
('Low Priority', 'LOW', 24, 120);

INSERT INTO tags (tag_name) VALUES
('bug'), ('security'), ('performance'), ('ui'), ('api'), ('documentation');

INSERT INTO tickets (ticket_number, subject, description, customer_id, assigned_to, department_id, category_id, priority, status, source) VALUES
('TKT-001', 'Login page not loading', 'The login page returns a 500 error when I try to access it.', 4, 2, 1, 2, 'HIGH', 'OPEN', 'EMAIL'),
('TKT-002', 'Request for dark mode feature', 'Would love to have a dark mode option in the application.', 5, 3, 1, 3, 'LOW', 'OPEN', 'WEB'),
('TKT-003', 'Billing discrepancy', 'I was charged twice for my subscription this month.', 4, 2, 2, 6, 'URGENT', 'IN_PROGRESS', 'PHONE');

-- Indexes
CREATE INDEX idx_tickets_customer ON tickets(customer_id);
CREATE INDEX idx_tickets_assigned ON tickets(assigned_to);
CREATE INDEX idx_tickets_status ON tickets(status);
CREATE INDEX idx_tickets_priority ON tickets(priority);
CREATE INDEX idx_tickets_created ON tickets(created_at);
CREATE INDEX idx_ticket_comments_ticket ON ticket_comments(ticket_id);
CREATE INDEX idx_ticket_history_ticket ON ticket_history(ticket_id);

-- View for open tickets
CREATE VIEW open_tickets AS
SELECT 
    t.ticket_number,
    t.subject,
    t.priority,
    t.status,
    c.full_name as customer_name,
    a.full_name as assigned_agent,
    d.department_name,
    cat.category_name,
    t.created_at
FROM tickets t
JOIN users c ON t.customer_id = c.id
LEFT JOIN users a ON t.assigned_to = a.id
LEFT JOIN departments d ON t.department_id = d.id
LEFT JOIN categories cat ON t.category_id = cat.id
WHERE t.status IN ('OPEN', 'IN_PROGRESS', 'PENDING', 'REOPENED')
ORDER BY 
    CASE t.priority
        WHEN 'URGENT' THEN 1
        WHEN 'HIGH' THEN 2
        WHEN 'MEDIUM' THEN 3
        WHEN 'LOW' THEN 4
    END,
    t.created_at;

-- View for agent workload
CREATE VIEW agent_workload AS
SELECT 
    u.id,
    u.full_name,
    COUNT(t.id) as assigned_tickets,
    SUM(CASE WHEN t.status = 'OPEN' THEN 1 ELSE 0 END) as open_tickets,
    SUM(CASE WHEN t.status = 'IN_PROGRESS' THEN 1 ELSE 0 END) as in_progress_tickets,
    SUM(CASE WHEN t.priority = 'URGENT' THEN 1 ELSE 0 END) as urgent_tickets
FROM users u
LEFT JOIN tickets t ON u.id = t.assigned_to AND t.status NOT IN ('RESOLVED', 'CLOSED')
WHERE u.user_type IN ('AGENT', 'ADMIN')
GROUP BY u.id, u.full_name;
