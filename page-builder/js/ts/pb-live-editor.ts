export type LiveCard = {
  title: string;
  body: string;
};

export type LivePageState = {
  appName: string;
  pageTitle: string;
  intro: string;
  cards: LiveCard[];
};

export type EditorRefs = {
  appNameInput: HTMLInputElement;
  pageTitleInput: HTMLInputElement;
  introInput: HTMLTextAreaElement;
  cardsInput: HTMLTextAreaElement;
  iframe: HTMLIFrameElement;
  statusEl: HTMLElement;
};

const DEFAULT_STATE: LivePageState = {
  appName: "XCM Live Builder",
  pageTitle: "Live Preview Page",
  intro: "Edit fields and content updates instantly with responsive grid layout.",
  cards: [
    { title: "Performance", body: "Fast static output with responsive layout and image assets." },
    { title: "Navigation", body: "Breadcrumb manager compatible shell for public content." },
    { title: "Security", body: "Use XCM Auth role checks and Crystal Auth 2FA for admin actions." },
  ],
};

function safe(input: string): string {
  return String(input)
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/\"/g, "&quot;")
    .replace(/'/g, "&#39;");
}

function parseCards(jsonText: string): LiveCard[] {
  try {
    const parsed = JSON.parse(jsonText);
    if (!Array.isArray(parsed)) return DEFAULT_STATE.cards;
    return parsed
      .map((x) => ({ title: String(x?.title ?? ""), body: String(x?.body ?? "") }))
      .filter((x) => x.title || x.body)
      .slice(0, 24);
  } catch {
    return DEFAULT_STATE.cards;
  }
}

export function buildResponsiveHtml(state: LivePageState): string {
  const cards = state.cards
    .map((card) => `<article class="card"><h3>${safe(card.title)}</h3><p>${safe(card.body)}</p></article>`)
    .join("");

  return [
    "<!doctype html>",
    "<html lang=\"en\">",
    "<head>",
    "<meta charset=\"utf-8\" />",
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" />",
    `<title>${safe(state.pageTitle)}</title>`,
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
    `<section class=\"hero\"><h1>${safe(state.pageTitle)}</h1><p>${safe(state.intro)}</p></section>`,
    `<section class=\"xcm-fl-grid\">${cards}</section>`,
    "</main>",
    "<div id=\"xcm-auto-footer-root\"></div>",
    "</body>",
    "</html>",
  ].join("");
}

export function readState(refs: EditorRefs): LivePageState {
  return {
    appName: refs.appNameInput.value.trim() || DEFAULT_STATE.appName,
    pageTitle: refs.pageTitleInput.value.trim() || DEFAULT_STATE.pageTitle,
    intro: refs.introInput.value.trim() || DEFAULT_STATE.intro,
    cards: parseCards(refs.cardsInput.value),
  };
}

export function renderPreview(refs: EditorRefs): void {
  const state = readState(refs);
  refs.iframe.srcdoc = buildResponsiveHtml(state);
  refs.statusEl.textContent = "Preview updated";
}

export function installLiveEditor(refs: EditorRefs): void {
  let timer: number | undefined;
  const onChange = () => {
    if (timer) window.clearTimeout(timer);
    timer = window.setTimeout(() => renderPreview(refs), 80);
  };

  refs.appNameInput.addEventListener("input", onChange);
  refs.pageTitleInput.addEventListener("input", onChange);
  refs.introInput.addEventListener("input", onChange);
  refs.cardsInput.addEventListener("input", onChange);

  renderPreview(refs);
}
