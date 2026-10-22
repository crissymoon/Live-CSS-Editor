package gateway

import (
	"bytes"
	"encoding/json"
	"io"
	"log"
	"net/http"
	"strconv"
	"sync"
	"time"
)

// limiter is a sliding one minute window per identity.
type limiter struct {
	mu     sync.Mutex
	perMin int
	hits   map[string][]time.Time
}

func newLimiter(perMin int) *limiter {
	return &limiter{perMin: perMin, hits: make(map[string][]time.Time)}
}

// Allow records a hit and reports whether the caller is still under budget.
func (l *limiter) Allow(key string) (bool, int) {
	now := time.Now()
	cutoff := now.Add(-time.Minute)

	l.mu.Lock()
	defer l.mu.Unlock()

	kept := l.hits[key][:0]
	for _, t := range l.hits[key] {
		if t.After(cutoff) {
			kept = append(kept, t)
		}
	}
	if len(kept) >= l.perMin {
		l.hits[key] = kept
		retry := int(kept[0].Add(time.Minute).Sub(now).Seconds()) + 1
		return false, retry
	}
	l.hits[key] = append(kept, now)
	return true, 0
}

// flushWriter pushes every chunk to the client so SSE tokens render live.
type flushWriter struct {
	w http.ResponseWriter
	f http.Flusher
}

func (fw flushWriter) Write(p []byte) (int, error) {
	n, err := fw.w.Write(p)
	if fw.f != nil {
		fw.f.Flush()
	}
	return n, err
}

// handleChat proxies /v1/chat/completions to the selected backend, streaming
// the reply back unchanged.
func (s *Server) handleChat(w http.ResponseWriter, r *http.Request) {
	id := identityFrom(r)
	if ok, retry := s.limiter.Allow(id.Key()); !ok {
		w.Header().Set("Retry-After", strconv.Itoa(retry))
		writeJSON(w, http.StatusTooManyRequests, map[string]any{
			"error": map[string]string{"message": "rate limit reached, try again shortly", "type": "rate_limit"},
		})
		return
	}

	raw, err := io.ReadAll(io.LimitReader(r.Body, s.cfg.MaxBodyBytes+1))
	if err != nil {
		writeJSON(w, http.StatusBadRequest, apiError("could not read request body"))
		return
	}
	if int64(len(raw)) > s.cfg.MaxBodyBytes {
		writeJSON(w, http.StatusRequestEntityTooLarge, apiError("request body too large"))
		return
	}

	var payload map[string]any
	if err := json.Unmarshal(raw, &payload); err != nil {
		writeJSON(w, http.StatusBadRequest, apiError("body is not a JSON object"))
		return
	}

	backendName, _ := payload["backend"].(string)
	delete(payload, "backend")
	model, _ := payload["model"].(string)
	if model == "" {
		writeJSON(w, http.StatusBadRequest, apiError("model is required"))
		return
	}
	stream, _ := payload["stream"].(bool)

	backend := s.registry.Resolve(backendName, model)
	body, err := json.Marshal(payload)
	if err != nil {
		writeJSON(w, http.StatusInternalServerError, apiError("could not encode request"))
		return
	}

	start := time.Now()
	resp, err := backend.do(r.Context(), http.MethodPost, "/chat/completions", bytes.NewReader(body), "application/json")
	if err != nil {
		log.Printf("[gateway] chat %s backend=%s: %v", id.Key(), backend.name, err)
		writeJSON(w, http.StatusBadGateway, apiError("backend unavailable: "+err.Error()))
		return
	}
	defer resp.Body.Close()

	if resp.StatusCode >= 400 {
		detail, _ := io.ReadAll(io.LimitReader(resp.Body, 4096))
		log.Printf("[gateway] chat %s backend=%s status=%d", id.Key(), backend.name, resp.StatusCode)
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(resp.StatusCode)
		_, _ = w.Write(detail)
		return
	}

	contentType := resp.Header.Get("Content-Type")
	if contentType == "" {
		contentType = "application/json"
	}
	w.Header().Set("Content-Type", contentType)
	w.Header().Set("Cache-Control", "no-store")
	w.Header().Set("X-Accel-Buffering", "no")
	w.WriteHeader(http.StatusOK)

	var copied int64
	if stream {
		copied, err = io.Copy(flushWriter{w: w, f: flusherFor(w)}, resp.Body)
	} else {
		copied, err = io.Copy(w, resp.Body)
	}
	if err != nil {
		log.Printf("[gateway] chat %s stream broke after %d bytes: %v", id.Key(), copied, err)
	}
	log.Printf("[gateway] chat user=%s backend=%s model=%s stream=%v bytes=%d dur=%s",
		id.Key(), backend.name, model, stream, copied, time.Since(start).Round(time.Millisecond))
}

func flusherFor(w http.ResponseWriter) http.Flusher {
	if f, ok := w.(http.Flusher); ok {
		return f
	}
	return nil
}

func apiError(msg string) map[string]any {
	return map[string]any{"error": map[string]string{"message": msg, "type": "gateway_error"}}
}
