-- Security Logging and Audit Trail System
-- Authentication, authorization, and system activity logging

-- Security Users
CREATE TABLE security_users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username VARCHAR(50) NOT NULL UNIQUE,
    email VARCHAR(100) NOT NULL UNIQUE,
    full_name VARCHAR(100),
    user_type VARCHAR(20) CHECK(user_type IN ('ADMIN', 'USER', 'SERVICE', 'API')),
    is_active BOOLEAN DEFAULT 1,
    is_locked BOOLEAN DEFAULT 0,
    failed_login_attempts INTEGER DEFAULT 0,
    last_login_at DATETIME,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- Authentication Logs
CREATE TABLE auth_logs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER,
    username VARCHAR(50),
    event_type VARCHAR(50) NOT NULL CHECK(event_type IN ('LOGIN_SUCCESS', 'LOGIN_FAILED', 'LOGOUT', 'PASSWORD_CHANGE', 'PASSWORD_RESET', 'ACCOUNT_LOCKED', 'ACCOUNT_UNLOCKED', 'MFA_ENABLED', 'MFA_DISABLED')),
    ip_address VARCHAR(45),
    user_agent TEXT,
    session_id VARCHAR(100),
    failure_reason TEXT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES security_users(id)
);

-- System Activity Logs
CREATE TABLE activity_logs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER,
    username VARCHAR(50),
    action VARCHAR(100) NOT NULL,
    resource_type VARCHAR(50),
    resource_id VARCHAR(100),
    details TEXT,
    ip_address VARCHAR(45),
    user_agent TEXT,
    status VARCHAR(20) CHECK(status IN ('SUCCESS', 'FAILURE', 'WARNING')),
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES security_users(id)
);

-- API Access Logs
CREATE TABLE api_logs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER,
    api_key VARCHAR(100),
    endpoint VARCHAR(200) NOT NULL,
    http_method VARCHAR(10) NOT NULL,
    request_body TEXT,
    response_code INTEGER,
    response_time_ms INTEGER,
    ip_address VARCHAR(45),
    user_agent TEXT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES security_users(id)
);

-- Security Events (anomalies, threats)
CREATE TABLE security_events (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    event_type VARCHAR(50) NOT NULL CHECK(event_type IN ('INTRUSION_ATTEMPT', 'BRUTE_FORCE', 'PRIVILEGE_ESCALATION', 'DATA_BREACH', 'MALWARE', 'PHISHING', 'DOS_ATTACK', 'SUSPICIOUS_ACTIVITY')),
    severity VARCHAR(20) NOT NULL CHECK(severity IN ('LOW', 'MEDIUM', 'HIGH', 'CRITICAL')),
    source_ip VARCHAR(45),
    target_resource VARCHAR(200),
    description TEXT NOT NULL,
    details TEXT,
    user_id INTEGER,
    status VARCHAR(20) DEFAULT 'OPEN' CHECK(status IN ('OPEN', 'INVESTIGATING', 'RESOLVED', 'FALSE_POSITIVE')),
    assigned_to INTEGER,
    resolved_at DATETIME,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES security_users(id),
    FOREIGN KEY (assigned_to) REFERENCES security_users(id)
);

-- Data Access Logs (sensitive data access)
CREATE TABLE data_access_logs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    data_type VARCHAR(50) NOT NULL,
    data_id VARCHAR(100),
    access_type VARCHAR(20) CHECK(access_type IN ('READ', 'CREATE', 'UPDATE', 'DELETE', 'EXPORT')),
    field_accessed VARCHAR(100),
    old_value TEXT,
    new_value TEXT,
    ip_address VARCHAR(45),
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES security_users(id)
);

-- Permission Changes Log
CREATE TABLE permission_logs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    changed_by INTEGER NOT NULL,
    affected_user_id INTEGER NOT NULL,
    permission_type VARCHAR(50) NOT NULL,
    action VARCHAR(20) CHECK(action IN ('GRANTED', 'REVOKED', 'MODIFIED')),
    old_permission TEXT,
    new_permission TEXT,
    reason TEXT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (changed_by) REFERENCES security_users(id),
    FOREIGN KEY (affected_user_id) REFERENCES security_users(id)
);

-- Session Management
CREATE TABLE sessions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    session_token VARCHAR(200) NOT NULL UNIQUE,
    ip_address VARCHAR(45),
    user_agent TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    expires_at DATETIME NOT NULL,
    last_activity_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    is_active BOOLEAN DEFAULT 1,
    terminated_at DATETIME,
    FOREIGN KEY (user_id) REFERENCES security_users(id)
);

-- Failed Access Attempts
CREATE TABLE failed_access_attempts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username VARCHAR(50),
    ip_address VARCHAR(45) NOT NULL,
    resource VARCHAR(200),
    attempt_type VARCHAR(50),
    failure_reason TEXT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- IP Blacklist
CREATE TABLE ip_blacklist (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    ip_address VARCHAR(45) NOT NULL UNIQUE,
    reason TEXT NOT NULL,
    added_by INTEGER,
    added_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    expires_at DATETIME,
    is_active BOOLEAN DEFAULT 1,
    FOREIGN KEY (added_by) REFERENCES security_users(id)
);

-- Security Alerts/Notifications
CREATE TABLE security_alerts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    alert_type VARCHAR(50) NOT NULL,
    severity VARCHAR(20) CHECK(severity IN ('INFO', 'WARNING', 'CRITICAL')),
    message TEXT NOT NULL,
    details TEXT,
    related_event_id INTEGER,
    sent_to TEXT,
    acknowledged BOOLEAN DEFAULT 0,
    acknowledged_by INTEGER,
    acknowledged_at DATETIME,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (acknowledged_by) REFERENCES security_users(id)
);

-- Sample Data
INSERT INTO security_users (username, email, full_name, user_type) VALUES
('admin', 'admin@company.com', 'System Administrator', 'ADMIN'),
('security_team', 'security@company.com', 'Security Team', 'ADMIN'),
('api_service', 'api@company.com', 'API Service Account', 'SERVICE'),
('john_doe', 'john@company.com', 'John Doe', 'USER'),
('jane_smith', 'jane@company.com', 'Jane Smith', 'USER');

INSERT INTO auth_logs (user_id, username, event_type, ip_address, user_agent, failure_reason) VALUES
(1, 'admin', 'LOGIN_SUCCESS', '192.168.1.100', 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7)', NULL),
(4, 'john_doe', 'LOGIN_SUCCESS', '192.168.1.105', 'Mozilla/5.0 (Windows NT 10.0; Win64; x64)', NULL),
(5, 'jane_smith', 'LOGIN_FAILED', '192.168.1.110', 'Mozilla/5.0 (X11; Linux x86_64)', 'Invalid password'),
(4, 'john_doe', 'PASSWORD_CHANGE', '192.168.1.105', 'Mozilla/5.0 (Windows NT 10.0; Win64; x64)', NULL);

INSERT INTO activity_logs (user_id, username, action, resource_type, resource_id, status, ip_address) VALUES
(1, 'admin', 'CREATE_USER', 'USER', '5', 'SUCCESS', '192.168.1.100'),
(4, 'john_doe', 'UPDATE_PROFILE', 'USER', '4', 'SUCCESS', '192.168.1.105'),
(1, 'admin', 'DELETE_RECORD', 'DATABASE', 'records_123', 'SUCCESS', '192.168.1.100'),
(5, 'jane_smith', 'EXPORT_DATA', 'REPORT', 'financial_2026', 'SUCCESS', '192.168.1.110');

INSERT INTO security_events (event_type, severity, source_ip, description, status) VALUES
('BRUTE_FORCE', 'HIGH', '203.0.113.45', 'Multiple failed login attempts detected from this IP', 'INVESTIGATING'),
('SUSPICIOUS_ACTIVITY', 'MEDIUM', '198.51.100.23', 'Unusual access pattern detected', 'OPEN'),
('DOS_ATTACK', 'CRITICAL', '203.0.113.67', 'Potential DoS attack - high request rate', 'RESOLVED');

INSERT INTO api_logs (user_id, api_key, endpoint, http_method, response_code, response_time_ms, ip_address) VALUES
(3, 'api_key_abc123', '/api/v1/users', 'GET', 200, 45, '192.168.1.50'),
(3, 'api_key_abc123', '/api/v1/data/export', 'POST', 200, 1250, '192.168.1.50'),
(3, 'api_key_abc123', '/api/v1/users/5', 'PATCH', 200, 89, '192.168.1.50');

INSERT INTO failed_access_attempts (username, ip_address, resource, attempt_type, failure_reason) VALUES
('unknown_user', '203.0.113.45', 'LOGIN', 'AUTHENTICATION', 'Username not found'),
('admin', '203.0.113.45', 'LOGIN', 'AUTHENTICATION', 'Invalid password'),
('admin', '203.0.113.45', 'LOGIN', 'AUTHENTICATION', 'Invalid password');

INSERT INTO ip_blacklist (ip_address, reason, added_by) VALUES
('203.0.113.45', 'Multiple brute force attempts', 1),
('198.51.100.100', 'Known malicious actor', 2);

INSERT INTO security_alerts (alert_type, severity, message, details) VALUES
('BRUTE_FORCE_DETECTED', 'CRITICAL', 'Brute force attack detected', 'IP 203.0.113.45 has made 50+ failed login attempts in 5 minutes'),
('ACCOUNT_LOCKED', 'WARNING', 'User account locked due to failed attempts', 'User: jane_smith, IP: 192.168.1.110'),
('UNUSUAL_ACCESS', 'INFO', 'User accessed system from new location', 'User john_doe logged in from unusual geographic location');

-- Indexes
CREATE INDEX idx_auth_logs_user ON auth_logs(user_id);
CREATE INDEX idx_auth_logs_timestamp ON auth_logs(timestamp);
CREATE INDEX idx_auth_logs_event_type ON auth_logs(event_type);
CREATE INDEX idx_activity_logs_user ON activity_logs(user_id);
CREATE INDEX idx_activity_logs_timestamp ON activity_logs(timestamp);
CREATE INDEX idx_activity_logs_action ON activity_logs(action);
CREATE INDEX idx_api_logs_timestamp ON api_logs(timestamp);
CREATE INDEX idx_api_logs_endpoint ON api_logs(endpoint);
CREATE INDEX idx_security_events_severity ON security_events(severity);
CREATE INDEX idx_security_events_status ON security_events(status);
CREATE INDEX idx_data_access_logs_user ON data_access_logs(user_id);
CREATE INDEX idx_data_access_logs_timestamp ON data_access_logs(timestamp);
CREATE INDEX idx_sessions_user ON sessions(user_id);
CREATE INDEX idx_sessions_token ON sessions(session_token);
CREATE INDEX idx_failed_attempts_ip ON failed_access_attempts(ip_address);

-- View for recent login activity
CREATE VIEW recent_login_activity AS
SELECT 
    u.username,
    u.full_name,
    al.event_type,
    al.ip_address,
    al.timestamp,
    al.failure_reason
FROM auth_logs al
LEFT JOIN security_users u ON al.user_id = u.id
WHERE al.timestamp >= datetime('now', '-7 days')
ORDER BY al.timestamp DESC
LIMIT 100;

-- View for active security threats
CREATE VIEW active_security_threats AS
SELECT 
    event_type,
    severity,
    source_ip,
    description,
    timestamp,
    status
FROM security_events
WHERE status IN ('OPEN', 'INVESTIGATING')
    AND severity IN ('HIGH', 'CRITICAL')
ORDER BY 
    CASE severity
        WHEN 'CRITICAL' THEN 1
        WHEN 'HIGH' THEN 2
    END,
    timestamp DESC;

-- View for audit trail summary
CREATE VIEW audit_trail_summary AS
SELECT 
    date(timestamp) as audit_date,
    COUNT(*) as total_activities,
    COUNT(DISTINCT user_id) as unique_users,
    SUM(CASE WHEN status = 'SUCCESS' THEN 1 ELSE 0 END) as successful_actions,
    SUM(CASE WHEN status = 'FAILURE' THEN 1 ELSE 0 END) as failed_actions
FROM activity_logs
GROUP BY date(timestamp)
ORDER BY audit_date DESC;
