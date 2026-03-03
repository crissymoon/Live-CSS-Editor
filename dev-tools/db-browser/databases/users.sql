-- Users Database for Web Applications
-- Authentication, profiles, roles, permissions, and sessions

-- Users
CREATE TABLE users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username VARCHAR(50) NOT NULL UNIQUE,
    email VARCHAR(100) NOT NULL UNIQUE,
    password_hash VARCHAR(255) NOT NULL,
    salt VARCHAR(64),
    first_name VARCHAR(50),
    last_name VARCHAR(50),
    display_name VARCHAR(100),
    avatar_url VARCHAR(500),
    phone VARCHAR(20),
    status VARCHAR(20) DEFAULT 'ACTIVE' CHECK(status IN ('ACTIVE', 'INACTIVE', 'SUSPENDED', 'DELETED')),
    email_verified BOOLEAN DEFAULT 0,
    phone_verified BOOLEAN DEFAULT 0,
    two_factor_enabled BOOLEAN DEFAULT 0,
    two_factor_secret VARCHAR(100),
    failed_login_attempts INTEGER DEFAULT 0,
    last_login_at DATETIME,
    last_login_ip VARCHAR(45),
    password_changed_at DATETIME,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- User Profiles
CREATE TABLE user_profiles (
    user_id INTEGER PRIMARY KEY,
    bio TEXT,
    date_of_birth DATE,
    gender VARCHAR(20),
    country VARCHAR(100),
    state VARCHAR(100),
    city VARCHAR(100),
    postal_code VARCHAR(20),
    address TEXT,
    timezone VARCHAR(50) DEFAULT 'UTC',
    language VARCHAR(10) DEFAULT 'en',
    website VARCHAR(200),
    company VARCHAR(200),
    job_title VARCHAR(100),
    social_linkedin VARCHAR(200),
    social_twitter VARCHAR(200),
    social_github VARCHAR(200),
    preferences TEXT,
    metadata TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

-- Roles
CREATE TABLE roles (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    role_name VARCHAR(50) NOT NULL UNIQUE,
    description TEXT,
    is_system BOOLEAN DEFAULT 0,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- Permissions
CREATE TABLE permissions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    permission_name VARCHAR(100) NOT NULL UNIQUE,
    resource VARCHAR(50) NOT NULL,
    action VARCHAR(50) NOT NULL,
    description TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- Role Permissions
CREATE TABLE role_permissions (
    role_id INTEGER NOT NULL,
    permission_id INTEGER NOT NULL,
    granted_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (role_id, permission_id),
    FOREIGN KEY (role_id) REFERENCES roles(id) ON DELETE CASCADE,
    FOREIGN KEY (permission_id) REFERENCES permissions(id) ON DELETE CASCADE
);

-- User Roles
CREATE TABLE user_roles (
    user_id INTEGER NOT NULL,
    role_id INTEGER NOT NULL,
    assigned_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    assigned_by INTEGER,
    PRIMARY KEY (user_id, role_id),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (role_id) REFERENCES roles(id) ON DELETE CASCADE,
    FOREIGN KEY (assigned_by) REFERENCES users(id)
);

-- Sessions
CREATE TABLE sessions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    session_token VARCHAR(255) NOT NULL UNIQUE,
    refresh_token VARCHAR(255),
    ip_address VARCHAR(45),
    user_agent TEXT,
    device_info TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    expires_at DATETIME NOT NULL,
    last_activity DATETIME DEFAULT CURRENT_TIMESTAMP,
    is_active BOOLEAN DEFAULT 1,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

-- Email Verification
CREATE TABLE email_verifications (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    email VARCHAR(100) NOT NULL,
    token VARCHAR(100) NOT NULL UNIQUE,
    expires_at DATETIME NOT NULL,
    verified_at DATETIME,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

-- Password Resets
CREATE TABLE password_resets (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    token VARCHAR(100) NOT NULL UNIQUE,
    expires_at DATETIME NOT NULL,
    used_at DATETIME,
    ip_address VARCHAR(45),
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

-- User Settings
CREATE TABLE user_settings (
    user_id INTEGER PRIMARY KEY,
    theme VARCHAR(20) DEFAULT 'light' CHECK(theme IN ('light', 'dark', 'auto')),
    notifications_enabled BOOLEAN DEFAULT 1,
    email_notifications BOOLEAN DEFAULT 1,
    sms_notifications BOOLEAN DEFAULT 0,
    marketing_emails BOOLEAN DEFAULT 0,
    newsletter BOOLEAN DEFAULT 1,
    privacy_profile VARCHAR(20) DEFAULT 'public' CHECK(privacy_profile IN ('public', 'friends', 'private')),
    language VARCHAR(10) DEFAULT 'en',
    timezone VARCHAR(50) DEFAULT 'UTC',
    date_format VARCHAR(20) DEFAULT 'YYYY-MM-DD',
    time_format VARCHAR(10) DEFAULT '24h',
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

-- User Activity Log
CREATE TABLE user_activity (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    activity_type VARCHAR(50) NOT NULL,
    description TEXT,
    ip_address VARCHAR(45),
    user_agent TEXT,
    metadata TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

-- OAuth Providers
CREATE TABLE oauth_providers (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    provider_name VARCHAR(50) NOT NULL UNIQUE,
    client_id VARCHAR(200),
    is_enabled BOOLEAN DEFAULT 1
);

-- OAuth Connections
CREATE TABLE oauth_connections (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    provider_id INTEGER NOT NULL,
    provider_user_id VARCHAR(200) NOT NULL,
    access_token TEXT,
    refresh_token TEXT,
    token_expires_at DATETIME,
    metadata TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (provider_id) REFERENCES oauth_providers(id),
    UNIQUE(provider_id, provider_user_id)
);

-- Sample Data
INSERT INTO roles (role_name, description, is_system) VALUES
('admin', 'Administrator with full access', 1),
('moderator', 'Moderator with content management access', 1),
('editor', 'Content editor', 0),
('user', 'Standard user', 1),
('guest', 'Guest user with limited access', 1);

INSERT INTO permissions (permission_name, resource, action, description) VALUES
('users.read', 'users', 'read', 'View user information'),
('users.write', 'users', 'write', 'Create and update users'),
('users.delete', 'users', 'delete', 'Delete users'),
('content.read', 'content', 'read', 'View content'),
('content.write', 'content', 'write', 'Create and update content'),
('content.delete', 'content', 'delete', 'Delete content'),
('content.publish', 'content', 'publish', 'Publish content'),
('comments.moderate', 'comments', 'moderate', 'Moderate comments'),
('settings.read', 'settings', 'read', 'View system settings'),
('settings.write', 'settings', 'write', 'Modify system settings');

INSERT INTO role_permissions (role_id, permission_id) VALUES
(1, 1), (1, 2), (1, 3), (1, 4), (1, 5), (1, 6), (1, 7), (1, 8), (1, 9), (1, 10),
(2, 1), (2, 4), (2, 5), (2, 6), (2, 7), (2, 8),
(3, 4), (3, 5), (3, 7),
(4, 1), (4, 4),
(5, 4);

INSERT INTO users (username, email, password_hash, first_name, last_name, display_name, status, email_verified) VALUES
('admin', 'admin@example.com', '$2y$10$abcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJ', 'Admin', 'User', 'Administrator', 'ACTIVE', 1),
('john_doe', 'john@example.com', '$2y$10$abcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJ', 'John', 'Doe', 'John D.', 'ACTIVE', 1),
('jane_smith', 'jane@example.com', '$2y$10$abcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJ', 'Jane', 'Smith', 'Jane S.', 'ACTIVE', 1),
('bob_wilson', 'bob@example.com', '$2y$10$abcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJ', 'Bob', 'Wilson', 'Bob W.', 'ACTIVE', 0);

INSERT INTO user_roles (user_id, role_id, assigned_by) VALUES
(1, 1, 1),
(2, 4, 1),
(3, 3, 1),
(4, 4, 1);

INSERT INTO user_profiles (user_id, bio, country, timezone, language) VALUES
(1, 'System administrator', 'United States', 'America/New_York', 'en'),
(2, 'Software developer and tech enthusiast', 'United States', 'America/Los_Angeles', 'en'),
(3, 'Content writer and editor', 'United Kingdom', 'Europe/London', 'en'),
(4, 'New user', 'Canada', 'America/Toronto', 'en');

INSERT INTO user_settings (user_id, theme, notifications_enabled, privacy_profile) VALUES
(1, 'dark', 1, 'private'),
(2, 'light', 1, 'public'),
(3, 'light', 1, 'friends'),
(4, 'auto', 1, 'public');

INSERT INTO oauth_providers (provider_name, is_enabled) VALUES
('google', 1),
('facebook', 1),
('github', 1),
('twitter', 1),
('linkedin', 1);

-- Indexes
CREATE INDEX idx_users_email ON users(email);
CREATE INDEX idx_users_username ON users(username);
CREATE INDEX idx_users_status ON users(status);
CREATE INDEX idx_sessions_user ON sessions(user_id);
CREATE INDEX idx_sessions_token ON sessions(session_token);
CREATE INDEX idx_sessions_expires ON sessions(expires_at);
CREATE INDEX idx_user_activity_user ON user_activity(user_id);
CREATE INDEX idx_user_activity_created ON user_activity(created_at);
CREATE INDEX idx_password_resets_token ON password_resets(token);
CREATE INDEX idx_email_verifications_token ON email_verifications(token);

-- View for user details
CREATE VIEW user_details AS
SELECT 
    u.id,
    u.username,
    u.email,
    u.first_name,
    u.last_name,
    u.display_name,
    u.status,
    u.email_verified,
    u.two_factor_enabled,
    u.last_login_at,
    u.created_at,
    GROUP_CONCAT(r.role_name) as roles,
    p.country,
    p.timezone,
    p.language
FROM users u
LEFT JOIN user_profiles p ON u.id = p.user_id
LEFT JOIN user_roles ur ON u.id = ur.user_id
LEFT JOIN roles r ON ur.role_id = r.id
GROUP BY u.id;

-- View for active sessions
CREATE VIEW active_sessions AS
SELECT 
    s.id,
    s.user_id,
    u.username,
    u.email,
    s.ip_address,
    s.created_at,
    s.last_activity,
    s.expires_at
FROM sessions s
JOIN users u ON s.user_id = u.id
WHERE s.is_active = 1
    AND s.expires_at > datetime('now')
ORDER BY s.last_activity DESC;

-- View for user permissions
CREATE VIEW user_permissions AS
SELECT DISTINCT
    u.id as user_id,
    u.username,
    p.permission_name,
    p.resource,
    p.action
FROM users u
JOIN user_roles ur ON u.id = ur.user_id
JOIN role_permissions rp ON ur.role_id = rp.role_id
JOIN permissions p ON rp.permission_id = p.id
WHERE u.status = 'ACTIVE';
