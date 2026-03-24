export type AssistMode = "chat" | "render";

export type AssistRequest = {
  mode: AssistMode;
  prompt: string;
  context: string;
};

export type AssistResponse = {
  ok: boolean;
  model?: string;
  output?: string;
  error?: string;
};

export async function requestAssist(endpoint: string, req: AssistRequest): Promise<AssistResponse> {
  const res = await fetch(endpoint, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(req),
    credentials: "same-origin",
  });

  const data = await res.json().catch(() => ({ ok: false, error: "Invalid JSON response" }));
  if (!res.ok) {
    return { ok: false, error: data?.error || `HTTP ${res.status}` };
  }

  return {
    ok: Boolean(data?.ok),
    model: String(data?.model || ""),
    output: String(data?.output || ""),
    error: data?.error ? String(data.error) : undefined,
  };
}
