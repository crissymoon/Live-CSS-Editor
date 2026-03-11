# XCM Auth Server Quick Start

The auth server (xcm_auth) is now easy to start with multiple options. Use this if you get "auth server offline" errors.

## Quick Options

### Option 1: From Live CSS Editor Launcher (Easiest)

```bash
# Double-click Live CSS Editor.command
[ Select option: 5 ] "Page builder"
```

This automatically:
- Checks if auth server is running (:9100)
- Starts it if offline
- Opens page builder when ready

### Option 2: Standalone Auth Server

```bash
# From Live CSS Editor Launcher:
[ Select option: s ] "Auth server"
```

Or directly:
```bash
bash page-builder/pb_admin/start-xcm-auth-only.sh
```

Starts ONLY the Go auth service on port 9100. Good for:
- Testing auth without opening page builder
- Running auth in dedicated terminal
- Debugging auth issues
- Keeping it running while working on other tasks

### Option 3: Manual Start

```bash
cd page-builder/xcm_auth
go run ./cmd/main.go
```

Uses the configuration in `.env` file (SERVER_ADDR=:9100).

## What Gets Started

When you start the auth server, it:

1. **Compiles** (first run takes 10-30 seconds)
2. **Migrates database** - creates fresh DB if needed
3. **Listens on :9100** - ready for requests
4. **Shows logs** - all activity printed to terminal

## How to Verify It's Running

### From terminal:
```bash
curl http://localhost:9100/health
```

Should return: `{"status":"ok"}`

### Using lsof:
```bash
lsof -i :9100
```

Should show Go process listening.

### From within launcher:
Page builder (option 5) will auto-detect and show status.

## Troubleshooting Quick Tips

**"port 9100 already in use"**
```bash
lsof -i :9100 | tail -1 | awk '{print $2}' | xargs kill -9
```

**"database is locked"**
```bash
pkill -f "go run.*xcm_auth"
sleep 1
bash page-builder/pb_admin/start-xcm-auth-only.sh
```

**"go: command not found"**
```bash
brew install go
```

**Want to start fresh database**
```bash
cd page-builder/xcm_auth
rm xcm_auth_dev.db*
go run ./cmd/main.go
```

## Startup Scripts

Two scripts are available:

| Script | Purpose |
|--------|---------|
| `start-xcm-auth-only.sh` | (NEW) Standalone Go server starter |
| `start-auth.sh` | Full stack (both PHP and Go) |

Both do the same thing for the auth server, but `start-xcm-auth-only.sh` is simpler and clearer.

## Integration Points

After auth server is running on :9100:

**Admin Panel** (option 1-2) connects to:
- Auth API at `localhost:9100`
- PHP admin at `localhost:8443` (or :8080 if dev)

**Page Builder** (option 5) checks:
- Port 9100 is available before opening
- Auto-starts auth if missing
- Waits for auth to be ready (60 seconds timeout)

**Live Server** (option 3) may use:
- Auth service for user sessions
- Depends on your application setup

## Configuration

The auth server reads from `page-builder/xcm_auth/.env`:

```bash
# Port to listen on
SERVER_ADDR=:9100

# Database file
DB_DSN=./xcm_auth_dev.db

# Dev mode (2FA disabled, HTTPS not required)
TWOFA_ENABLED=false
REQUIRE_HTTPS=false

# Local CORS and API endpoints
CORS_ORIGINS=http://localhost:8080,http://localhost:8443,http://localhost:9000
```

For production, update `.env` with real values and set appropriate flags.

## Performance Notes

- **First run**: 15-30 seconds (Go compiles)
- **Subsequent runs**: 5-10 seconds (uses cached build)
- **Memory**: ~50MB running
- **Port**: Exclusive use of :9100

For production, build once then run the binary:
```bash
cd page-builder/xcm_auth
go build -o xcm_auth ./cmd/main.go
./xcm_auth  # Runs instantly
```

## Files

| File | Purpose |
|------|---------|
| `pb_admin/start-xcm-auth-only.sh` | Standalone auth starter (NEW) |
| `pb_admin/AUTH_SERVER_STARTUP.md` | Detailed troubleshooting guide |
| `xcm_auth/.env` | Auth server configuration |
| `xcm_auth/xcm_auth_dev.db` | Dev database (SQLite) |
| `Live CSS Editor.command` | Main launcher (updated with option 5 & s) |

## Next Steps

1. **Start auth server** - Use launcher option `s`
2. **Open page builder** - Use launcher option `5`
3. **Log in** - Credentials in admin panel or launcher output
4. **Build pages** - Use page builder to compose
5. **Push changes** - Use launcher option `p`

## Getting Help

When auth fails to start, check logs:

```bash
# If started from launcher option s:
tail -f /tmp/xcm-auth-startup.log

# If started manually:
bash page-builder/pb_admin/start-xcm-auth-only.sh 2>&1 | tee auth.log
```

Read [AUTH_SERVER_STARTUP.md](AUTH_SERVER_STARTUP.md) for detailed troubleshooting.
