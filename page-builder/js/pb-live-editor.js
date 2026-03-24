(function (global) {
  "use strict";

  var DEFAULT_STATE = {
    appName: "XCM Live Builder",
    pageTitle: "Live Preview Page",
    intro: "Edit fields and content updates instantly with responsive grid layout.",
    cards: [
      { title: "Performance", body: "Fast static output with responsive layout and image assets." },
      { title: "Navigation", body: "Breadcrumb manager compatible shell for public content." },
      { title: "Security", body: "Use XCM Auth role checks and Crystal Auth 2FA for admin actions." }
    ]
  };

  function safe(input) {
    return String(input)
      .replace(/&/g, "&amp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;")
      .replace(/\"/g, "&quot;")
      .replace(/'/g, "&#39;");
  }

  function parseCards(jsonText) {
    try {
      var parsed = JSON.parse(jsonText);
      if (!Array.isArray(parsed)) return DEFAULT_STATE.cards;
      return parsed
        .map(function (x) { return { title: String((x && x.title) || ""), body: String((x && x.body) || "") }; })
        .filter(function (x) { return x.title || x.body; })
        .slice(0, 24);
    } catch (_err) {
      return DEFAULT_STATE.cards;
    }
  }

  function buildResponsiveHtml(state) {
    var cards = state.cards
      .map(function (card) { return "<article class=\"card\"><h3>" + safe(card.title) + "</h3><p>" + safe(card.body) + "</p></article>"; })
      .join("");

    return [
      "<!doctype html>",
      "<html lang=\"en\">",
      "<head>",
      "<meta charset=\"utf-8\" />",
      "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" />",
      "<title>" + safe(state.pageTitle) + "</title>",
      "<style>",
      "*{box-sizing:border-box;margin:0;padding:0}",
      "body{font-family:Segoe UI,Arial,sans-serif;background:#060d18;color:#e2e8f0;padding:16px;line-height:1.5}",
      ".shell{max-width:1180px;margin:0 auto}",
      ".hero{border:1px solid #1e3a5f;border-radius:12px;background:#0d1a2e;padding:18px;margin-bottom:14px}",
      ".hero h1{font-size:clamp(1.4rem,3.2vw,2rem)}",
      ".hero p{margin-top:6px;color:#94a3b8}",
      ".xcm-fl-grid{display:grid;gap:12px;grid-template-columns:repeat(auto-fit,minmax(min(100%,220px),1fr))}",
      ".card{border:1px solid #1e3a5f;border-radius:10px;background:#0d1a2e;padding:14px}",
      ".card h3{font-size:.94rem;margin-bottom:6px}",
      ".card p{font-size:.88rem;color:#94a3b8}",
      "@media (max-width:700px){body{padding:12px}}",
      "</style>",
      "</head>",
      "<body>",
      "<div id=\"xcm-auto-header-root\"></div>",
      "<main class=\"shell\">",
      "<section class=\"hero\"><h1>" + safe(state.pageTitle) + "</h1><p>" + safe(state.intro) + "</p></section>",
      "<section class=\"xcm-fl-grid\">" + cards + "</section>",
      "</main>",
      "<div id=\"xcm-auto-footer-root\"></div>",
      "</body>",
      "</html>"
    ].join("");
  }

  function readState(refs) {
    return {
      appName: refs.appNameInput.value.trim() || DEFAULT_STATE.appName,
      pageTitle: refs.pageTitleInput.value.trim() || DEFAULT_STATE.pageTitle,
      intro: refs.introInput.value.trim() || DEFAULT_STATE.intro,
      cards: parseCards(refs.cardsInput.value)
    };
  }

  function renderPreview(refs) {
    var state = readState(refs);
    refs.iframe.srcdoc = buildResponsiveHtml(state);
    refs.statusEl.textContent = "Preview updated";
  }

  function installLiveEditor(refs) {
    var timer;
    var onChange = function () {
      if (timer) window.clearTimeout(timer);
      timer = window.setTimeout(function () { renderPreview(refs); }, 80);
    };

    refs.appNameInput.addEventListener("input", onChange);
    refs.pageTitleInput.addEventListener("input", onChange);
    refs.introInput.addEventListener("input", onChange);
    refs.cardsInput.addEventListener("input", onChange);

    renderPreview(refs);
  }

  global.PbLiveEditor = {
    installLiveEditor: installLiveEditor,
    renderPreview: renderPreview,
    buildResponsiveHtml: buildResponsiveHtml,
    readState: readState
  };
}(window));
