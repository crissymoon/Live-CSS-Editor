// lib.rs — Tauri library entry point.
// Required by Tauri v2 for mobile build targets.
// Desktop logic lives in main.rs; this file wires it up for mobile contexts.

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    // Re-use the app builder from main.rs by calling main() logic.
    // For now desktop-only; extend here when adding iOS / Android support.
}
