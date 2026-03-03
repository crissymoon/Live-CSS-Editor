#!/bin/bash
# Create a multi-table smoke test database for Crissy's DB Browser
# Usage: ./create_test_db.sh

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DB="$SCRIPT_DIR/smoke_test.db"

rm -f "$DB"

sqlite3 "$DB" <<'SQL'

-- Users table
CREATE TABLE users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT NOT NULL UNIQUE,
    email TEXT NOT NULL,
    bio TEXT,
    age INTEGER,
    balance REAL DEFAULT 0.0,
    is_active INTEGER DEFAULT 1,
    created_at TEXT DEFAULT (datetime('now'))
);

INSERT INTO users (username, email, bio, age, balance, is_active) VALUES
    ('alice',   'alice@example.com',   'Loves hiking and coffee',          28, 150.75, 1),
    ('bob',     'bob@example.com',     'Software engineer from Portland',  34, 2300.00, 1),
    ('charlie', 'charlie@example.com', NULL,                               22, 0.0,    0),
    ('diana',   'diana@example.com',   'Photographer and traveler',        31, 890.50, 1),
    ('eve',     'eve@example.com',     'Security researcher',              27, 5400.00, 1),
    ('frank',   'frank@example.com',   'Retired teacher',                  65, 12000.00, 1),
    ('grace',   'grace@example.com',   'Student at MIT',                   19, 45.00,  1),
    ('heidi',   'heidi@example.com',   'Chef and food blogger',            40, 3200.00, 1);

-- Products table
CREATE TABLE products (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    description TEXT,
    price REAL NOT NULL,
    stock INTEGER DEFAULT 0,
    category TEXT,
    sku TEXT UNIQUE
);

INSERT INTO products (name, description, price, stock, category, sku) VALUES
    ('Laptop Pro 15',     'High-performance laptop with 16GB RAM',     1299.99, 45,  'Electronics', 'LP-15-001'),
    ('Wireless Mouse',    'Ergonomic wireless mouse with USB-C',       29.99,   200, 'Electronics', 'WM-001'),
    ('Standing Desk',     'Adjustable height standing desk, oak top',  549.00,  12,  'Furniture',   'SD-OAK-01'),
    ('Mechanical Keyboard', 'Cherry MX Blue switches, backlit',        89.95,   78,  'Electronics', 'MK-BLU-01'),
    ('Monitor 27"',       '4K IPS display, USB-C hub built in',        449.00,  30,  'Electronics', 'MON-27-4K'),
    ('Desk Lamp',         'LED desk lamp with adjustable color temp',  34.50,   150, 'Lighting',    'DL-LED-01'),
    ('Office Chair',      'Mesh back, lumbar support, adjustable',     299.00,  25,  'Furniture',   'OC-MESH-01'),
    ('USB-C Hub',         '7-port hub with HDMI and ethernet',         44.99,   300, 'Electronics', 'HUB-7P-01'),
    ('Webcam HD',         '1080p webcam with built-in mic',            59.99,   90,  'Electronics', 'WC-1080-01'),
    ('Notebook Set',      'Pack of 3 ruled notebooks, A5',             12.99,   500, 'Stationery',  'NB-A5-3PK');

-- Orders table
CREATE TABLE orders (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    product_id INTEGER NOT NULL,
    quantity INTEGER NOT NULL DEFAULT 1,
    total_price REAL,
    status TEXT DEFAULT 'pending',
    notes TEXT,
    ordered_at TEXT DEFAULT (datetime('now')),
    FOREIGN KEY (user_id) REFERENCES users(id),
    FOREIGN KEY (product_id) REFERENCES products(id)
);

INSERT INTO orders (user_id, product_id, quantity, total_price, status, notes) VALUES
    (1, 1, 1, 1299.99, 'shipped',    'Gift wrap requested'),
    (1, 6, 2, 69.00,   'delivered',  NULL),
    (2, 3, 1, 549.00,  'pending',    'Deliver to office address'),
    (2, 4, 1, 89.95,   'delivered',  NULL),
    (3, 2, 3, 89.97,   'cancelled',  'Changed mind'),
    (4, 5, 2, 898.00,  'shipped',    'Dual monitor setup'),
    (5, 8, 1, 44.99,   'delivered',  NULL),
    (5, 9, 1, 59.99,   'delivered',  NULL),
    (6, 10, 5, 64.95,  'pending',    'For grandchildren'),
    (7, 7, 1, 299.00,  'shipped',    'Dorm room');

-- Tags table (many-to-many with products)
CREATE TABLE tags (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE
);

INSERT INTO tags (name) VALUES
    ('bestseller'), ('new-arrival'), ('sale'), ('eco-friendly'), ('premium');

CREATE TABLE product_tags (
    product_id INTEGER NOT NULL,
    tag_id INTEGER NOT NULL,
    PRIMARY KEY (product_id, tag_id),
    FOREIGN KEY (product_id) REFERENCES products(id),
    FOREIGN KEY (tag_id) REFERENCES tags(id)
);

INSERT INTO product_tags (product_id, tag_id) VALUES
    (1, 5), (1, 1), (2, 1), (2, 3), (3, 4), (3, 2),
    (4, 5), (5, 1), (5, 5), (6, 3), (6, 4), (7, 2),
    (8, 1), (9, 2), (10, 4);

-- App settings (key-value config table)
CREATE TABLE settings (
    key TEXT PRIMARY KEY,
    value TEXT,
    description TEXT
);

INSERT INTO settings (key, value, description) VALUES
    ('app_name',      'Smoke Test Store',         'Application display name'),
    ('version',       '1.0.3',                    'Current version'),
    ('max_cart_items', '50',                       'Maximum items in shopping cart'),
    ('currency',      'USD',                       'Default currency code'),
    ('maintenance',   '0',                         'Maintenance mode flag (0=off, 1=on)'),
    ('welcome_msg',   'Welcome to our store!',     'Homepage welcome message'),
    ('api_url',       'https://api.example.com/v2', 'Backend API endpoint');

SQL

echo "Created smoke test database: $DB"
echo "Tables:"
sqlite3 "$DB" ".tables"
echo ""
echo "Row counts:"
for t in users products orders tags product_tags settings; do
    count=$(sqlite3 "$DB" "SELECT COUNT(*) FROM $t;")
    printf "  %-15s %s rows\n" "$t" "$count"
done
