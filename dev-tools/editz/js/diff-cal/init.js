/**
 * diff-cal/init.js
 * Bootstrap — runs on DOMContentLoaded.
 */

import { initYearDropdown, initMonthDropdown, generateCalendar } from './calendar.js';
import { loadCalendarData } from './storage.js';
import { bindUIEvents, AutoSaveManager } from './ui.js';

document.addEventListener('DOMContentLoaded', async () => {
    // 1. Populate dropdowns
    initYearDropdown();
    initMonthDropdown();

    // 2. Load persisted data (SQLite → localStorage fallback)
    await loadCalendarData();

    // 3. Render the calendar for the current month/year
    const month = parseInt(document.getElementById('month-select').value);
    const year  = parseInt(document.getElementById('year-select').value);
    generateCalendar(month, year);

    // 4. Wire up all UI controls
    bindUIEvents();

    // 5. Start AutoSaveManager (also saves on [contenteditable][data-save-id])
    const autoSave = new AutoSaveManager({
        statusElement: '#save-status',
    });
    window.autoSaveManager = autoSave; // expose for debugging
});
