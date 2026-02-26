// php_server.rs — Cross-platform PHP built-in server management.
//
// Discovers the PHP binary, starts `php -S 127.0.0.1:<port> -t <www_root>`,
// and exposes a handle that kills the process on drop.

use std::net::TcpListener;
use std::path::Path;
use std::process::{Child, Command, Stdio};

// ---------------------------------------------------------------------------
// Public types
// ---------------------------------------------------------------------------

pub struct PhpServer {
    child: Child,
}

impl PhpServer {
    /// Kill the PHP process and wait for it to exit.
    pub fn stop(&mut self) {
        let _ = self.child.kill();
        let _ = self.child.wait();
    }
}

// Automatically stop the server when the struct is dropped.
impl Drop for PhpServer {
    fn drop(&mut self) {
        self.stop();
    }
}

// ---------------------------------------------------------------------------
// Port discovery
// ---------------------------------------------------------------------------

/// Bind to port 0 to let the OS assign a free port, then return it.
pub fn find_free_port() -> Option<u16> {
    TcpListener::bind("127.0.0.1:0")
        .ok()
        .and_then(|l| l.local_addr().ok())
        .map(|a| a.port())
}

// ---------------------------------------------------------------------------
// PHP binary discovery
// ---------------------------------------------------------------------------

/// Candidate names to search for on each platform.
#[cfg(target_os = "windows")]
const PHP_CANDIDATES: &[&str] = &[
    "php",
    "php.exe",
    "php8",
    "php8.exe",
    "php84",
    "php83",
    "php82",
    // Common Windows install paths — checked via PATH, not hardcoded
];

#[cfg(not(target_os = "windows"))]
const PHP_CANDIDATES: &[&str] = &[
    "php",
    "php8",
    "php8.4",
    "php8.3",
    "php8.2",
    "php8.1",
    "php80",
    "php74",
];

/// Return the first PHP binary name that is executable and responds to --version.
pub fn find_php() -> Option<String> {
    for &name in PHP_CANDIDATES {
        if Command::new(name)
            .arg("--version")
            .stdout(Stdio::null())
            .stderr(Stdio::null())
            .status()
            .map(|s| s.success())
            .unwrap_or(false)
        {
            return Some(name.to_string());
        }
    }
    None
}

/// Return a human-readable description of where PHP binary was found.
pub fn php_version(bin: &str) -> String {
    Command::new(bin)
        .arg("--version")
        .output()
        .map(|o| {
            String::from_utf8_lossy(&o.stdout)
                .lines()
                .next()
                .unwrap_or("unknown")
                .to_string()
        })
        .unwrap_or_else(|_| "unknown".into())
}

// ---------------------------------------------------------------------------
// Server startup
// ---------------------------------------------------------------------------

/// Start PHP's built-in server on `127.0.0.1:<port>` serving from `www_root`.
/// Returns the server handle or an error message.
pub fn start(port: u16, www_root: &Path) -> Result<PhpServer, String> {
    let php = find_php()
        .ok_or_else(|| concat!(
            "PHP could not be found. ",
            "Please install PHP 8+ and ensure the `php` binary is in your PATH."
        ).to_string())?;

    let www_str = www_root
        .to_str()
        .ok_or_else(|| "PHP www root path contains invalid characters.".to_string())?;

    let addr = format!("127.0.0.1:{}", port);

    eprintln!("[php-server] {} | serving {} on {}", php_version(&php), www_str, addr);

    let child = Command::new(&php)
        .args(["-S", &addr, "-t", www_str])
        .stdout(Stdio::null())
        .stderr(Stdio::null())
        .spawn()
        .map_err(|e| format!("Failed to spawn `{php}`: {e}"))?;

    Ok(PhpServer { child })
}

// ---------------------------------------------------------------------------
// Wait helper — poll until the port accepts TCP connections or timeout
// ---------------------------------------------------------------------------

/// Block until the PHP server is accepting connections, or give up after `timeout_ms`.
pub fn wait_for_ready(port: u16, timeout_ms: u64) -> bool {
    use std::net::TcpStream;
    use std::time::{Duration, Instant};

    let addr   = format!("127.0.0.1:{}", port);
    let start  = Instant::now();
    let limit  = Duration::from_millis(timeout_ms);
    let retry  = Duration::from_millis(50);

    while start.elapsed() < limit {
        if TcpStream::connect(&addr).is_ok() {
            return true;
        }
        std::thread::sleep(retry);
    }
    false
}
