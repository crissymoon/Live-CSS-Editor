(function (global) {
  "use strict";

  async function requestAssist(endpoint, req) {
    var res = await fetch(endpoint, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(req),
      credentials: "same-origin"
    });

    var data;
    try {
      data = await res.json();
    } catch (_err) {
      data = { ok: false, error: "Invalid JSON response" };
    }

    if (!res.ok) {
      return { ok: false, error: (data && data.error) || ("HTTP " + res.status) };
    }

    return {
      ok: Boolean(data && data.ok),
      model: String((data && data.model) || ""),
      output: String((data && data.output) || ""),
      error: data && data.error ? String(data.error) : undefined
    };
  }

  global.PbAiAssist = {
    requestAssist: requestAssist
  };
}(window));
