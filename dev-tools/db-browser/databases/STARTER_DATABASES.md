# Starter Database Quick Reference

## Available Databases

| Database | File Size | Tables | Primary Use Case |
|----------|-----------|--------|------------------|
| accounting.db | 48K | 4 | Double entry bookkeeping, financial reporting |
| payroll.db | 80K | 8 | Employee payroll, time tracking, benefits |
| tickets.db | 104K | 11 | Help desk, support ticket management |
| crm.db | 96K | 13 | Sales pipeline, customer relationships |
| calendar.db | 100K | 11 | Event scheduling, resource booking |
| security_logs.db | 140K | 11 | Security auditing, authentication logs |
| users.db | 156K | 16 | Web app authentication, RBAC, profiles |

## Database Schemas

### accounting.db
```
accounts (27 sample records)
├── journal_entries
└── journal_lines
fiscal_periods (4 quarters)
```

### payroll.db
```
employees (4 sample records)
├── time_entries
├── payroll_runs
├── employee_benefits
├── pto_requests
└── pto_balances
benefits (5 types)
pay_periods (4 periods)
```

### tickets.db
```
users (5 records: 2 agents, 2 customers, 1 admin)
departments (3)
categories (6)
tickets (3 sample tickets)
├── ticket_comments
├── ticket_attachments
├── ticket_history
└── ticket_tags
sla_rules (4 priority levels)
tags (6 common tags)
```

### crm.db
```
companies (4)
├── contacts (5)
├── opportunities (3 deals)
│   └── opportunity_products
└── activities (3)
products (4)
lead_sources (6)
pipeline_stages (6)
email_templates
notes
```

### calendar.db
```
calendar_users (4)
├── calendars (4)
│   └── events (5)
│       ├── event_attendees
│       ├── event_reminders
│       └── event_category_map
└── sessions
resources (4: rooms, equipment)
└── resource_bookings
event_categories (5)
calendar_sharing
```

### security_logs.db
```
security_users (5)
├── auth_logs (4 login events)
├── activity_logs (4 actions)
├── sessions
└── permission_logs
api_logs (3 API calls)
security_events (3 incidents)
data_access_logs
failed_access_attempts (3 failures)
ip_blacklist (2 blocked IPs)
security_alerts (3 alerts)
```

### users.db
```
users (4 users with roles)
├── user_profiles (4)
├── user_settings (4)
├── user_roles
├── sessions
├── email_verifications
├── password_resets
├── user_activity
└── oauth_connections
roles (5: admin, moderator, editor, user, guest)
permissions (10 granular permissions)
role_permissions (mapping)
oauth_providers (5 providers)
```

## Common Views Included

### Accounting
- trial_balance - Account balances summary

### Payroll
- active_employees - Current staff only
- employee_summary - YTD compensation totals

### Tickets
- open_tickets - Active tickets by priority
- agent_workload - Tickets per agent

### CRM
- sales_pipeline - Active opportunities weighted
- contact_directory - All contacts with companies

### Calendar
- upcoming_events - Next 50 events
- resource_availability - Resource booking status
- my_calendar_events - Personal calendar view

### Security Logs
- recent_login_activity - Last 7 days of logins
- active_security_threats - Open critical/high events
- audit_trail_summary - Daily activity summary

### Users
- user_details - Complete user info with roles
- active_sessions - Current logged-in sessions
- user_permissions - Flattened permission list

## Sample Queries

### Find all high-priority open tickets
```sql
SELECT * FROM open_tickets WHERE priority = 'HIGH';
```

### Check user permissions
```sql
SELECT * FROM user_permissions WHERE username = 'john_doe';
```

### View sales pipeline value
```sql
SELECT SUM(weighted_amount) as pipeline_value FROM sales_pipeline;
```

### Audit recent user activity
```sql
SELECT * FROM recent_login_activity WHERE event_type = 'LOGIN_FAILED';
```

### Calculate account balance
```sql
SELECT * FROM trial_balance WHERE account_type = 'ASSET';
```

## Integration Examples

### Web Application User Auth
Use users.db for authentication, session management, and RBAC.

### Business Intelligence
Connect accounting.db and crm.db for financial reporting.

### Security Monitoring
Connect security_logs.db to monitoring dashboards.

### Employee Portal
Combine payroll.db and calendar.db for employee self-service.

### Support System
Use tickets.db with users.db for customer support portal.

## Customization Tips

1. Add custom fields to tables as needed
2. Create additional indexes for your query patterns
3. Extend permission system with your resource types
4. Add custom views for reporting
5. Implement soft deletes where appropriate
6. Add created_by/updated_by audit columns
7. Customize status values for your workflow

## Data Integrity

All databases include:
- Foreign key constraints
- Check constraints for valid values
- Unique constraints where appropriate
- Not null constraints on required fields
- Default values for common fields
- Indexes on frequently queried columns
