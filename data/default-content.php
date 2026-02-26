<?php
/**
 * Default HTML and CSS loaded into the editors on first visit or after reset.
 */
$defaultHtml = '<div class="page">

  <header class="site-header">
    <img
      src="https://xcaliburmoon.net/xcm-logo-light.webp"
      alt="XCaliburMoon"
      class="logo"
    />
    <p class="tagline">Get in touch</p>
  </header>

  <main class="card">
    <form id="contactForm" novalidate>

      <div class="field-row">
        <div class="field">
          <label for="fname">Name</label>
          <input type="text" id="fname" name="name" placeholder="Your name" autocomplete="off" />
        </div>
        <div class="field">
          <label for="femail">Email</label>
          <input type="email" id="femail" name="email" placeholder="you@example.com" autocomplete="off" />
        </div>
      </div>

      <div class="field">
        <label for="fsubject">Subject</label>
        <input type="text" id="fsubject" name="subject" placeholder="What is this about?" autocomplete="off" />
      </div>

      <div class="field">
        <label for="fmessage">Message</label>
        <textarea id="fmessage" name="message" rows="5" placeholder="Write your message here..."></textarea>
      </div>

      <div class="form-footer">
        <button type="submit" class="btn-send">Send Message</button>
        <div id="formStatus" class="form-status" aria-live="polite"></div>
      </div>

    </form>
  </main>

</div>';

$defaultCss = <<<'CSSEOF'
/* ── Variables ─────────────────────────────── */
:root {
  --bg:           #1a1a2e;
  --bg-card:      #16162b;
  --bg-input:     #13131f;
  --shadow-light: #252540;
  --shadow-dark:  #0d0d18;
  --text:         #e8e8f0;
  --text-muted:   #8888a0;
  --accent:       #6366f1;
  --accent-dim:   rgba(99, 102, 241, 0.20);
  --accent-glow:  rgba(99, 102, 241, 0.45);
  --success:      #10b981;
  --error:        #ef4444;
  --border:       rgba(255, 255, 255, 0.06);
  --font:         'JetBrains Mono', 'Fira Code', monospace;
}

/* ── Reset ─────────────────────────────────── */
*, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

/* ── Page ──────────────────────────────────── */
body {
  background: var(--bg);
  color: var(--text);
  font-family: var(--font);
  font-size: 14px;
  line-height: 1.6;
  min-height: 100vh;
  display: flex;
  justify-content: center;
  align-items: flex-start;
  padding: 40px 16px;
}

.page {
  width: 100%;
  max-width: 640px;
}

/* ── Header ────────────────────────────────── */
.site-header {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 10px;
  margin-bottom: 32px;
}

.logo {
  height: 54px;
  width: auto;
  display: block;
}

.tagline {
  color: var(--text-muted);
  font-size: 12px;
  letter-spacing: 0.12em;
  text-transform: uppercase;
}

/* ── Card ──────────────────────────────────── */
.card {
  background: var(--bg-card);
  border: 1px solid var(--border);
  padding: 32px;
  box-shadow:
    8px 8px 16px var(--shadow-dark),
    -8px -8px 16px var(--shadow-light);
}

/* ── Fields ────────────────────────────────── */
.field-row {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 16px;
}

.field {
  display: flex;
  flex-direction: column;
  gap: 6px;
  margin-bottom: 16px;
}

label {
  font-size: 11px;
  letter-spacing: 0.10em;
  text-transform: uppercase;
  color: var(--text-muted);
}

input,
textarea {
  background: var(--bg-input);
  color: var(--text);
  border: 1px solid var(--border);
  font-family: var(--font);
  font-size: 13px;
  padding: 10px 12px;
  outline: none;
  resize: vertical;
  transition: border-color 0.18s, box-shadow 0.18s;
  box-shadow:
    inset 3px 3px 6px var(--shadow-dark),
    inset -3px -3px 6px var(--shadow-light);
}

input:focus,
textarea:focus {
  border-color: var(--accent);
  box-shadow:
    inset 3px 3px 6px var(--shadow-dark),
    inset -3px -3px 6px var(--shadow-light),
    0 0 0 2px var(--accent-dim);
}

input::placeholder,
textarea::placeholder {
  color: #44445a;
}

/* ── Footer / Submit ───────────────────────── */
.form-footer {
  display: flex;
  align-items: center;
  gap: 16px;
  margin-top: 8px;
  flex-wrap: wrap;
}

.btn-send {
  background: var(--bg-card);
  color: var(--accent);
  border: 1px solid var(--accent);
  font-family: var(--font);
  font-size: 13px;
  letter-spacing: 0.08em;
  text-transform: uppercase;
  padding: 10px 28px;
  cursor: pointer;
  transition: background 0.18s, box-shadow 0.18s, color 0.18s;
  box-shadow:
    4px 4px 8px var(--shadow-dark),
    -4px -4px 8px var(--shadow-light);
}

.btn-send:hover {
  background: var(--accent);
  color: #ffffff;
  box-shadow:
    0 0 14px var(--accent-glow),
    4px 4px 8px var(--shadow-dark);
}

.btn-send:active {
  box-shadow:
    inset 3px 3px 6px var(--shadow-dark),
    inset -3px -3px 6px var(--shadow-light);
}

/* ── Status ────────────────────────────────── */
.form-status {
  font-size: 12px;
  letter-spacing: 0.06em;
  padding: 6px 12px;
  flex: 1;
  min-width: 0;
}

.form-status.is-success {
  color: var(--success);
  border-left: 2px solid var(--success);
}

.form-status.is-error {
  color: var(--error);
  border-left: 2px solid var(--error);
}

.form-status.is-sending {
  color: var(--text-muted);
  border-left: 2px solid var(--text-muted);
}
CSSEOF;

$defaultJs = <<<'JSEOF'
document.addEventListener("DOMContentLoaded", function () {
  var form   = document.getElementById("contactForm");
  var status = document.getElementById("formStatus");

  function setStatus(msg, type) {
    status.textContent = msg;
    status.className   = "form-status " + (type ? "is-" + type : "");
  }

  function clearStatus() {
    setStatus("", "");
  }

  function validate(data) {
    if (!data.name.trim())                        { return "Name is required."; }
    if (!data.email.trim())                       { return "Email is required."; }
    if (!/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(data.email)) { return "Enter a valid email address."; }
    if (!data.subject.trim())                     { return "Subject is required."; }
    if (!data.message.trim())                     { return "Message cannot be empty."; }
    return null;
  }

  form.addEventListener("submit", function (e) {
    e.preventDefault();
    clearStatus();

    var data = {
      name:    form.querySelector("[name=name]").value,
      email:   form.querySelector("[name=email]").value,
      subject: form.querySelector("[name=subject]").value,
      message: form.querySelector("[name=message]").value
    };

    var err = validate(data);
    if (err) {
      setStatus(err, "error");
      return;
    }

    setStatus("Sending...", "sending");

    // Simulate async send (replace with fetch() for a real endpoint)
    setTimeout(function () {
      setStatus("Message sent. We will be in touch soon.", "success");
      form.reset();
    }, 1200);
  });

  // Clear status when user starts editing any field
  var fields = form.querySelectorAll("input, textarea");
  for (var i = 0; i < fields.length; i++) {
    fields[i].addEventListener("input", clearStatus);
  }
});
JSEOF;
