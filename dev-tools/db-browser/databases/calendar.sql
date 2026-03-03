-- Calendar and Event Management System
-- Scheduling, appointments, and resource booking

-- Calendar Users
CREATE TABLE calendar_users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username VARCHAR(50) NOT NULL UNIQUE,
    email VARCHAR(100) NOT NULL UNIQUE,
    full_name VARCHAR(100) NOT NULL,
    timezone VARCHAR(50) DEFAULT 'UTC',
    is_active BOOLEAN DEFAULT 1,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- Calendars
CREATE TABLE calendars (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    calendar_name VARCHAR(100) NOT NULL,
    description TEXT,
    owner_id INTEGER NOT NULL,
    color VARCHAR(7),
    is_public BOOLEAN DEFAULT 0,
    is_default BOOLEAN DEFAULT 0,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (owner_id) REFERENCES calendar_users(id)
);

-- Events
CREATE TABLE events (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    calendar_id INTEGER NOT NULL,
    title VARCHAR(200) NOT NULL,
    description TEXT,
    location VARCHAR(200),
    start_time DATETIME NOT NULL,
    end_time DATETIME NOT NULL,
    all_day BOOLEAN DEFAULT 0,
    event_type VARCHAR(50) CHECK(event_type IN ('MEETING', 'APPOINTMENT', 'TASK', 'REMINDER', 'BIRTHDAY', 'HOLIDAY', 'OTHER')),
    status VARCHAR(20) DEFAULT 'CONFIRMED' CHECK(status IN ('TENTATIVE', 'CONFIRMED', 'CANCELLED')),
    visibility VARCHAR(20) DEFAULT 'PUBLIC' CHECK(visibility IN ('PUBLIC', 'PRIVATE', 'CONFIDENTIAL')),
    organizer_id INTEGER,
    recurrence_rule TEXT,
    recurrence_parent_id INTEGER,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (calendar_id) REFERENCES calendars(id) ON DELETE CASCADE,
    FOREIGN KEY (organizer_id) REFERENCES calendar_users(id),
    FOREIGN KEY (recurrence_parent_id) REFERENCES events(id)
);

-- Event Attendees
CREATE TABLE event_attendees (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    event_id INTEGER NOT NULL,
    user_id INTEGER,
    email VARCHAR(100),
    response_status VARCHAR(20) DEFAULT 'PENDING' CHECK(response_status IN ('PENDING', 'ACCEPTED', 'DECLINED', 'TENTATIVE')),
    is_organizer BOOLEAN DEFAULT 0,
    is_required BOOLEAN DEFAULT 1,
    notes TEXT,
    responded_at DATETIME,
    FOREIGN KEY (event_id) REFERENCES events(id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES calendar_users(id)
);

-- Event Reminders
CREATE TABLE event_reminders (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    event_id INTEGER NOT NULL,
    minutes_before INTEGER NOT NULL,
    reminder_type VARCHAR(20) CHECK(reminder_type IN ('EMAIL', 'POPUP', 'SMS')),
    sent BOOLEAN DEFAULT 0,
    sent_at DATETIME,
    FOREIGN KEY (event_id) REFERENCES events(id) ON DELETE CASCADE
);

-- Resources (rooms, equipment)
CREATE TABLE resources (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    resource_name VARCHAR(100) NOT NULL,
    resource_type VARCHAR(50) CHECK(resource_type IN ('ROOM', 'EQUIPMENT', 'VEHICLE', 'OTHER')),
    capacity INTEGER,
    location VARCHAR(200),
    description TEXT,
    is_active BOOLEAN DEFAULT 1,
    requires_approval BOOLEAN DEFAULT 0
);

-- Resource Bookings
CREATE TABLE resource_bookings (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    resource_id INTEGER NOT NULL,
    event_id INTEGER,
    booked_by INTEGER NOT NULL,
    start_time DATETIME NOT NULL,
    end_time DATETIME NOT NULL,
    status VARCHAR(20) DEFAULT 'PENDING' CHECK(status IN ('PENDING', 'APPROVED', 'DECLINED', 'CANCELLED')),
    notes TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (resource_id) REFERENCES resources(id),
    FOREIGN KEY (event_id) REFERENCES events(id) ON DELETE CASCADE,
    FOREIGN KEY (booked_by) REFERENCES calendar_users(id)
);

-- Calendar Sharing
CREATE TABLE calendar_sharing (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    calendar_id INTEGER NOT NULL,
    shared_with_user_id INTEGER NOT NULL,
    permission_level VARCHAR(20) DEFAULT 'READ' CHECK(permission_level IN ('READ', 'WRITE', 'ADMIN')),
    shared_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (calendar_id) REFERENCES calendars(id) ON DELETE CASCADE,
    FOREIGN KEY (shared_with_user_id) REFERENCES calendar_users(id)
);

-- Event Categories/Tags
CREATE TABLE event_categories (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    category_name VARCHAR(50) NOT NULL UNIQUE,
    color VARCHAR(7),
    description TEXT
);

-- Event to Category mapping
CREATE TABLE event_category_map (
    event_id INTEGER NOT NULL,
    category_id INTEGER NOT NULL,
    PRIMARY KEY (event_id, category_id),
    FOREIGN KEY (event_id) REFERENCES events(id) ON DELETE CASCADE,
    FOREIGN KEY (category_id) REFERENCES event_categories(id)
);

-- Sample Data
INSERT INTO calendar_users (username, email, full_name, timezone) VALUES
('alice', 'alice@company.com', 'Alice Anderson', 'America/New_York'),
('bob', 'bob@company.com', 'Bob Brown', 'America/Los_Angeles'),
('charlie', 'charlie@company.com', 'Charlie Chen', 'America/Chicago'),
('diana', 'diana@company.com', 'Diana Davis', 'Europe/London');

INSERT INTO calendars (calendar_name, description, owner_id, color, is_default) VALUES
('Work Calendar', 'Primary work calendar', 1, '#4285F4', 1),
('Personal', 'Personal events', 1, '#0B8043', 0),
('Team Calendar', 'Engineering team events', 2, '#E67C73', 0),
('Company Holidays', 'Company-wide holidays', 3, '#F4B400', 0);

INSERT INTO event_categories (category_name, color, description) VALUES
('Meeting', '#FF6B6B', 'Team meetings and discussions'),
('Deadline', '#4ECDC4', 'Project deadlines'),
('Training', '#95E1D3', 'Training and learning sessions'),
('Social', '#F38181', 'Social events and team building'),
('Holiday', '#FFC75F', 'Holidays and time off');

INSERT INTO resources (resource_name, resource_type, capacity, location) VALUES
('Conference Room A', 'ROOM', 10, 'Building 1, Floor 3'),
('Conference Room B', 'ROOM', 20, 'Building 1, Floor 3'),
('Projector', 'EQUIPMENT', 1, 'IT Equipment Room'),
('Company Van', 'VEHICLE', 8, 'Parking Lot');

INSERT INTO events (calendar_id, title, description, location, start_time, end_time, event_type, organizer_id) VALUES
(1, 'Weekly Team Standup', 'Weekly sync with engineering team', 'Conference Room A', '2026-03-03 10:00:00', '2026-03-03 10:30:00', 'MEETING', 1),
(1, 'Project Deadline - Q1 Release', 'Final deadline for Q1 release', 'N/A', '2026-03-31 17:00:00', '2026-03-31 17:00:00', 'REMINDER', 1),
(2, 'Dentist Appointment', 'Regular checkup', 'Downtown Dental', '2026-03-10 14:00:00', '2026-03-10 15:00:00', 'APPOINTMENT', 1),
(3, 'Sprint Planning', 'Plan next sprint items', 'Conference Room B', '2026-03-05 13:00:00', '2026-03-05 15:00:00', 'MEETING', 2),
(4, 'New Year Holiday', 'Company closed', 'N/A', '2026-01-01 00:00:00', '2026-01-01 23:59:59', 'HOLIDAY', 3);

INSERT INTO event_attendees (event_id, user_id, response_status, is_organizer) VALUES
(1, 1, 'ACCEPTED', 1),
(1, 2, 'ACCEPTED', 0),
(1, 3, 'TENTATIVE', 0),
(4, 1, 'ACCEPTED', 0),
(4, 2, 'ACCEPTED', 1),
(4, 3, 'ACCEPTED', 0),
(4, 4, 'PENDING', 0);

INSERT INTO event_reminders (event_id, minutes_before, reminder_type) VALUES
(1, 15, 'EMAIL'),
(1, 5, 'POPUP'),
(3, 60, 'EMAIL'),
(4, 1440, 'EMAIL');

-- Indexes
CREATE INDEX idx_events_calendar ON events(calendar_id);
CREATE INDEX idx_events_start_time ON events(start_time);
CREATE INDEX idx_events_organizer ON events(organizer_id);
CREATE INDEX idx_event_attendees_event ON event_attendees(event_id);
CREATE INDEX idx_event_attendees_user ON event_attendees(user_id);
CREATE INDEX idx_resource_bookings_resource ON resource_bookings(resource_id);
CREATE INDEX idx_resource_bookings_time ON resource_bookings(start_time, end_time);

-- View for upcoming events
CREATE VIEW upcoming_events AS
SELECT 
    e.id,
    e.title,
    e.start_time,
    e.end_time,
    e.location,
    e.event_type,
    c.calendar_name,
    u.full_name as organizer_name,
    COUNT(DISTINCT ea.id) as attendee_count
FROM events e
JOIN calendars c ON e.calendar_id = c.id
LEFT JOIN calendar_users u ON e.organizer_id = u.id
LEFT JOIN event_attendees ea ON e.event_id = ea.event_id
WHERE e.start_time >= datetime('now')
    AND e.status != 'CANCELLED'
GROUP BY e.id
ORDER BY e.start_time
LIMIT 50;

-- View for resource availability
CREATE VIEW resource_availability AS
SELECT 
    r.id,
    r.resource_name,
    r.resource_type,
    r.capacity,
    r.location,
    COUNT(rb.id) as active_bookings
FROM resources r
LEFT JOIN resource_bookings rb ON r.id = rb.resource_id 
    AND rb.start_time <= datetime('now', '+7 days')
    AND rb.end_time >= datetime('now')
    AND rb.status = 'APPROVED'
WHERE r.is_active = 1
GROUP BY r.id;

-- View for my calendar
CREATE VIEW my_calendar_events AS
SELECT 
    e.*,
    c.calendar_name,
    ea.response_status
FROM events e
JOIN calendars c ON e.calendar_id = c.id
JOIN event_attendees ea ON e.id = ea.event_id
WHERE e.status != 'CANCELLED'
ORDER BY e.start_time;
