// main.rs — Tauri entry point for Live CSS Editor.
//
// Responsibilities:
//   1. Locate a free TCP port.
//   2. Discover the system PHP binary.
//   3. Start PHP's built-in server pointing at the bundled `www/` directory.
//   4. Open the Tauri webview window pointing at http://127.0.0.1:<port>.
//   5. Kill the PHP process cleanly when the window is closed.

// Suppress the extra console window on Windows release builds.
#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

mod php_server;

use std::sync::{Arc, Mutex};
use std::path::PathBuf;
use tauri::{AppHandle, Manager};
use tauri_plugin_dialog::DialogExt;

// ---------------------------------------------------------------------------
// Shared state
// ---------------------------------------------------------------------------

struct PhpState(Arc<Mutex<Option<php_server::PhpServer>>>);

// ---------------------------------------------------------------------------
// Tauri commands
// ---------------------------------------------------------------------------

/// Open DevTools for the main window (debug builds only).
#[tauri::command]
fn open_devtools(window: tauri::WebviewWindow) {
    #[cfg(debug_assertions)]
    window.open_devtools();
    #[cfg(not(debug_assertions))]
    let _ = window;
}

/// Open a native file-picker dialog and return the chosen file's
/// path and text content, or `null` if the user cancelled.
#[tauri::command]
async fn pick_and_read_file(app: tauri::AppHandle) -> Result<Option<serde_json::Value>, String> {
    let (tx, rx) = tokio::sync::oneshot::channel::<Option<PathBuf>>();

    app.dialog()
        .file()
        .add_filter("Web Files", &["html", "htm", "css", "php"])
        .add_filter("All Files", &["*"])
        .pick_file(move |f| {
            let pb = f.and_then(|fp| fp.as_path().map(|p| p.to_path_buf()));
            let _ = tx.send(pb);
        });

    let path_opt = rx.await.map_err(|e| e.to_string())?;

    match path_opt {
        None => Ok(None),
        Some(pb) => {
            let path_str = pb
                .to_str()
                .ok_or_else(|| "Path contains non-UTF-8 characters".to_string())?
                .to_string();
            let content = std::fs::read_to_string(&pb)
                .map_err(|e| format!("Failed to read file: {e}"))?;
            Ok(Some(serde_json::json!({
                "path":    path_str,
                "content": content
            })))
        }
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Return the www directory that PHP should serve.
///
/// - In debug builds:  <workspace_root>/src-tauri/www
///   (populated by `npm run copy-www` before `tauri dev`)
/// - In release builds: the `www` folder bundled as a Tauri resource.
fn resolve_www_root(app: &AppHandle) -> std::path::PathBuf {
    #[cfg(debug_assertions)]
    {
        // CARGO_MANIFEST_DIR is set at compile-time to src-tauri/
        let _ = app; // unused in debug builds
        let manifest = std::path::PathBuf::from(env!("CARGO_MANIFEST_DIR"));
        manifest.join("www")
    }

    #[cfg(not(debug_assertions))]
    {
        app.path()
            .resource_dir()
            .expect("Tauri resource dir unavailable")
            .join("www")
    }
}

/// Show a native error dialog and return, without panicking the whole app.
fn show_error(app: &AppHandle, title: &str, body: &str) {
    eprintln!("[error] {}: {}", title, body);
    // Use a plain injected window as a fallback error display
    let html = format!(
        r#"<!DOCTYPE html><html><head><meta charset="UTF-8">
<style>
body{{margin:0;background:#0c071c;font-family:sans-serif;
     display:flex;align-items:center;justify-content:center;height:100vh;color:#eceaf6;}}
.box{{text-align:center;padding:40px;max-width:500px;}}
h2{{font-size:18px;margin-bottom:12px;}}
p{{font-size:14px;color:#9e93c0;line-height:1.6;}}
</style></head><body>
<div class="box"><h2>{title}</h2><p>{body}</p></div>
</body></html>"#,
        title = title,
        body  = body
    );

    let _ = tauri::WebviewWindowBuilder::new(
        app,
        "error",
        tauri::WebviewUrl::App("error".into()),
    )
    .title("Live CSS Editor — Error")
    .inner_size(540.0, 280.0)
    .resizable(false)
    .build()
    .map(|w| {
        let src = format!("data:text/html;charset=utf-8,{}", urlencoding(&html));
        let _ = w.navigate(src.parse().unwrap());
    });
}

fn urlencoding(s: &str) -> String {
    // Minimal percent-encoder for the data URI
    s.chars()
        .flat_map(|c| match c {
            ' ' => vec!['%', '2', '0'],
            '"' => vec!['%', '2', '2'],
            '#' => vec!['%', '2', '3'],
            '<' => vec!['%', '3', 'C'],
            '>' => vec!['%', '3', 'E'],
            _   => vec![c],
        })
        .collect()
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

fn main() {
    // Pick the port up front so it is captured by the move closure.
    // In debug we pin port 7777 to match tauri.conf.json devUrl so the dev
    // window auto-navigates there without extra plumbing.
    let port: u16 = if cfg!(debug_assertions) {
        7777
    } else {
        php_server::find_free_port().expect("No free TCP port available")
    };

    tauri::Builder::default()
        .plugin(tauri_plugin_dialog::init())
        .invoke_handler(tauri::generate_handler![open_devtools, pick_and_read_file])
        .setup(move |app| {
            let www_root = resolve_www_root(app.handle());

            // Validate www root exists
            if !www_root.exists() {
                show_error(
                    app.handle(),
                    "Startup Error",
                    &format!(
                        "PHP app directory not found:\n{}\n\nRun `npm run copy-www` first.",
                        www_root.display()
                    ),
                );
                return Ok(());
            }

            // Start PHP server
            match php_server::start(port, &www_root) {
                Err(e) => {
                    show_error(app.handle(), "PHP Not Found", &e);
                }
                Ok(server) => {
                    // Store server handle so it lives for the app lifetime
                    app.manage(PhpState(Arc::new(Mutex::new(Some(server)))));

                    // Wait up to 3 s for PHP to be ready
                    let ready = php_server::wait_for_ready(port, 3000);
                    if !ready {
                        eprintln!("[warn] PHP server did not respond within 3 s — continuing anyway");
                    }

                    // In release builds the window starts at devUrl placeholder;
                    // navigate it to the actual PHP server.
                    #[cfg(not(debug_assertions))]
                    if let Some(win) = app.get_webview_window("main") {
                        let url = format!("http://127.0.0.1:{}", port);
                        let _ = win.navigate(url.parse().expect("Invalid PHP server URL"));
                    }
                }
            }

            Ok(())
        })
        // Kill PHP when the last window is closed
        .on_window_event(|window, event| {
            if let tauri::WindowEvent::Destroyed = event {
                if let Some(state) = window.try_state::<PhpState>() {
                    if let Ok(mut guard) = state.0.lock() {
                        if let Some(mut server) = guard.take() {
                            server.stop();
                            eprintln!("[php-server] stopped");
                        }
                    }
                }
            }
        })
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
