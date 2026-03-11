# XCM Auth Server Troubleshooting

The auth server (xcm_auth) is a Go service that runs on port 9100 and handles user authentication for the page builder admin panel.

## Quick Start

### Method 1: Standalone Script (Recommended)

```bash
bash page-builder/pb_admin/start-xcm-auth-only.sh
```

This script:
- Checks Go installation
- Downloads dependencies
- Builds and runs the server
- Shows detailed logs
- Graceful shutdown on Ctrl+C

### Method 2: Direct Go Command

```bash
cd page-builder/xcm_auth
go run ./cmd/main.go
```

The server will start on `:9100` using config from `.env`.

### Method 3: From Live CSS Editor Launcher

1. Double-click `Live CSS Editor.command`
2. Select option `1` or `2` to start with the admin panel
3. Will attempt to start auth server automatically

## Common Issues

### "Auth server did not start"

#### Issue: Go not installed

```
ERROR: 'go' is not in PATH
```

**Solution:**
```bash
brew install go
```

After installing, close and reopen terminal for PATH to update.

#### Issue: Port 9100 already in use

```
Address already in use
```

**Solution:**
```bash
# Find what's using the port
lsof -i :9100

# Kill the process (replace PID)
kill -9 <PID>

# Or use the script - it auto-kills existing processes
bash page-builder/pb_admin/start-xcm-auth-only.sh
```

#### Issue: Compilation error

```
build ./cmd/main.go: ...
```

**Possible causes:**
- Incomplete Go installation
- Missing dependencies

**Solution:**
```bash
cd page-builder/xcm_auth
go mod download
go mod tidy
go run ./cmd/main.go
```

#### Issue: Database locked

```
database is locked
```

**This happens when:** Multiple processes try to access the SQLite file

**Solution:**
1. Kill all xcm_auth processes: `pkill -f "go run.*xcm_auth"`
2. Wait 2 seconds
3. Restart: `bash page-builder/pb_admin/start-xcm-auth-only.sh`

If problem persists, delete and recreate database:
```bash
cd page-builder/xcm_auth
rm xcm_auth_dev.db xcm_auth_dev.db-shm xcm_auth_dev.db-wal
go run ./cmd/main.go  # Will auto-create fresh DB
```

### "Database migration failed"

**Solution:**
1. Backup current database (optional)
2. Delete database files:
```bash
cd page-builder/xcm_auth
rm xcm_auth_dev.db*
```
3. Restart auth server - it will create new empty DB

### "Cannot connect to auth server" in admin panel

#### Check if server is running

```bash
curl http://localhost:9100/health
```

Should return HTTP 200.

If not running, start it:
```bash
bash page-builder/pb_admin/start-xcm-auth-only.sh
```

#### Check firewall

```bash
# Check if port is listening
lsof -i :9100

# If blank, server didn't start properly
```

#### Check logs

Look for errors in terminal output when starting the script.

### Config issues

The auth server reads configuration from `xcm_auth/.env`:

```bash
# View current config
cat page-builder/xcm_auth/.env

# Edit if needed
nano page-builder/xcm_auth/.env
```

**Key settings:**
- `SERVER_ADDR=:9100` - Port to listen on
- `DB_DSN=./xcm_auth_dev.db` - Database file path
- `TWOFA_ENABLED=false` - 2FA disabled for dev
- `REQUIRE_HTTPS=false` - HTTP allowed for dev

## Verify Server is Running

### Check port

```bash
lsof -i :9100
```

Should show Go process listening.

### Check health endpoint

```bash
curl -v http://localhost:9100/health
```

Expected response:
```
< HTTP/1.1 200 OK
< Content-Type: application/json
<
{"status":"ok"}
```

### Check logs in real-time

When running the script, you should see:
```
[xcm_auth] Starting Go auth server
[main] database migration complete
[main] xcm_auth starting on :9100
```

## Integration with Admin Panel

Once auth server is running on :9100:

1. Admin panel PHP: port 8080 (or :8443 if full admin)
2. Auth server Go: port 9100
3. Both must be running together

When you open admin dashboard at `https://localhost:8443/pb_admin/`, it communicates with the auth server at `localhost:9100`.

## Development Workflow

### All-in-one startup (from launcher)

Launch shows and manages everything:
- Starts auth server (:9100)
- Starts nginx (:8443)  
- Starts PHP dev server if needed
- Opens browser

```bash
# From Live CSS Editor.command option 1 or 2
```

### Manual startup (if launcher has issues)

```bash
# Terminal 1: Start auth server
bash page-builder/pb_admin/start-xcm-auth-only.sh

# Terminal 2: Start web server (optional, if nginx not running via boot)
bash page-builder/server/start.sh

# Terminal 3: Open admin panel in browser
open https://localhost:8443/pb_admin/login.php
```

## Performance Tips

- On first run, `go run` takes time to compile (10-30 seconds depending on machine)
- Subsequent runs are faster (uses cached build)
- For production, use `go build` once to create a binary, then run it

## Reset Everything

If having persistent problems:

```bash
# Kill all auth processes
pkill -f "go run.*xcm_auth"
pkill -f "xcm_auth"

# Reset database
cd page-builder/xcm_auth
rm xcm_auth_dev.db*

# Start fresh
bash page-builder/pb_admin/start-xcm-auth-only.sh
```

## Environment Variables

Override .env settings with environment variables:

```bash
# Change port
SERVER_ADDR=:9999 go run page-builder/xcm_auth/cmd/main.go

# Change database
DB_DSN=/tmp/custom.db go run page-builder/xcm_auth/cmd/main.go

# Custom config file
ENV_FILE=/path/to/.env go run page-builder/xcm_auth/cmd/main.go
```

## See Also

- `.env` - Dev configuration
- `cmd/main.go` - Server entry point
- `api/` - HTTP handlers
- `db/` - Database layer
- `auth/` - Authentication logic
- `AUTHENTICATION.md` - Auth system docs (if exists)

## Getting Help

Check logs for specific error messages:
```bash
bash page-builder/pb_admin/start-xcm-auth-only.sh 2>&1 | tee auth-debug.log
```

Then review the log file for clues about what failed.
