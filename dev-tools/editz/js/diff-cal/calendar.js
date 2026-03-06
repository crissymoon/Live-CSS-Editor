/**
 * diff-cal/calendar.js
 * Calendar rendering logic.
 */

const MONTH_NAMES = [
    'January', 'February', 'March', 'April', 'May', 'June',
    'July', 'August', 'September', 'October', 'November', 'December'
];

/**
 * Populate the year <select> with a range around the current year.
 */
export function initYearDropdown() {
    const yearSelect = document.getElementById('year-select');
    const currentYear = new Date().getFullYear();
    for (let y = currentYear - 10; y <= currentYear + 10; y++) {
        const opt = document.createElement('option');
        opt.value = y;
        opt.textContent = y;
        if (y === currentYear) opt.selected = true;
        yearSelect.appendChild(opt);
    }
}

/**
 * Set the month <select> to the current calendar month.
 */
export function initMonthDropdown() {
    document.getElementById('month-select').value = new Date().getMonth();
}

/**
 * Build the calendar grid for the given month/year.
 * @param {number} month  0-based month index
 * @param {number} year   4-digit year
 */
export function generateCalendar(month, year) {
    const calendarBody = document.getElementById('calendar-body');
    const monthYearDisplay = document.getElementById('current-month-year');

    // Clear previous rows
    calendarBody.innerHTML = '';

    // Update header display
    monthYearDisplay.textContent = `${MONTH_NAMES[month]} ${year}`;

    const firstDay     = new Date(year, month, 1).getDay();      // 0=Sun … 6=Sat
    const daysInMonth  = new Date(year, month + 1, 0).getDate();
    const prevLastDay  = new Date(year, month, 0).getDate();

    const today = new Date();
    let date          = 1;
    let prevDate      = prevLastDay - firstDay + 1;
    let nextDate      = 1;

    for (let row = 0; row < 6; row++) {
        // Stop adding rows once we've placed all days of the month
        if (date > daysInMonth && row > 0) break;

        const tr = document.createElement('tr');

        for (let col = 0; col < 7; col++) {
            const td = document.createElement('td');

            if (row === 0 && col < firstDay) {
                // Leading days from previous month
                td.innerHTML = `<div class="day-number">${prevDate++}</div>`;
                td.classList.add('other-month');
            } else if (date > daysInMonth) {
                // Trailing days from next month
                td.innerHTML = `<div class="day-number">${nextDate++}</div>`;
                td.classList.add('other-month');
                date++; // advance so we can track trailing count
            } else {
                td.innerHTML = `<div class="day-number">${date}</div>`;

                if (
                    date === today.getDate() &&
                    month === today.getMonth() &&
                    year  === today.getFullYear()
                ) {
                    td.classList.add('today');
                }

                date++;
            }

            tr.appendChild(td);
        }

        calendarBody.appendChild(tr);
    }
}
