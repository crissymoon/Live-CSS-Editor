package gateway

import (
	"context"
	"encoding/json"
	"fmt"
	"io"
	"log"
	"net/http"
	"strings"
	"sync"
	"time"
)

const modelCacheTTL = 20 * time.Second

// ModelInfo is one model as advertised by a backend, in OpenAI list shape plus
// the backend it came from.
type ModelInfo struct {
	ID      string `json:"id"`
	Object  string `json:"object"`
	Created int64  `json:"created"`
	OwnedBy string `json:"owned_by"`
	Backend string `json:"backend"`
}

// BackendHealth is the probe result for one backend.
type BackendHealth struct {
	Name      string `json:"name"`
	OK        bool   `json:"ok"`
	Error     string `json:"error,omitempty"`
	LatencyMS int64  `json:"latency_ms"`
	Models    int    `json:"models"`
}

// Backend is a single upstream model server.
type Backend struct {
	name    string
	baseURL string
	apiKey  string
	client  *http.Client

	mu       sync.Mutex
	models   []ModelInfo
	modelsAt time.Time
}

func newBackend(c BackendConfig, timeout time.Duration) *Backend {
	return &Backend{
		name:    c.Name,
		baseURL: c.BaseURL,
		apiKey:  c.APIKey,
		client:  &http.Client{Timeout: timeout},
	}
}

// do issues a request against the backend, adding its key when configured.
func (b *Backend) do(ctx context.Context, method, path string, body io.Reader, contentType string) (*http.Response, error) {
	req, err := http.NewRequestWithContext(ctx, method, b.baseURL+path, body)
	if err != nil {
		return nil, err
	}
	if contentType != "" {
		req.Header.Set("Content-Type", contentType)
	}
	if b.apiKey != "" {
		req.Header.Set("Authorization", "Bearer "+b.apiKey)
	}
	return b.client.Do(req)
}

// fetchModels reads /models from the backend without touching the cache.
func (b *Backend) fetchModels(ctx context.Context) ([]ModelInfo, error) {
	resp, err := b.do(ctx, http.MethodGet, "/models", nil, "")
	if err != nil {
		return nil, fmt.Errorf("%s: %w", b.name, err)
	}
	defer resp.Body.Close()
	if resp.StatusCode != http.StatusOK {
		body, _ := io.ReadAll(io.LimitReader(resp.Body, 512))
		return nil, fmt.Errorf("%s: HTTP %d: %s", b.name, resp.StatusCode, strings.TrimSpace(string(body)))
	}
	var payload struct {
		Data []struct {
			ID      string `json:"id"`
			Created int64  `json:"created"`
			OwnedBy string `json:"owned_by"`
		} `json:"data"`
	}
	if err := json.NewDecoder(io.LimitReader(resp.Body, 1<<20)).Decode(&payload); err != nil {
		return nil, fmt.Errorf("%s: decode models: %w", b.name, err)
	}
	out := make([]ModelInfo, 0, len(payload.Data))
	for _, m := range payload.Data {
		if m.ID == "" {
			continue
		}
		out = append(out, ModelInfo{ID: m.ID, Object: "model", Created: m.Created, OwnedBy: m.OwnedBy, Backend: b.name})
	}
	return out, nil
}

// Models returns the cached list, refreshing it when stale. A failed refresh
// leaves a previous list in place so a short backend wobble does not empty the
// picker.
func (b *Backend) Models(ctx context.Context) ([]ModelInfo, error) {
	b.mu.Lock()
	fresh := time.Since(b.modelsAt) < modelCacheTTL && b.models != nil
	cached := b.models
	b.mu.Unlock()
	if fresh {
		return cached, nil
	}

	list, err := b.fetchModels(ctx)
	if err != nil {
		if cached != nil {
			return cached, nil
		}
		return nil, err
	}
	b.mu.Lock()
	b.models = list
	b.modelsAt = time.Now()
	b.mu.Unlock()
	return list, nil
}

func (b *Backend) hasModel(model string) bool {
	b.mu.Lock()
	defer b.mu.Unlock()
	for _, m := range b.models {
		if m.ID == model {
			return true
		}
	}
	return false
}

// Health probes the backend with a short deadline.
func (b *Backend) Health(ctx context.Context) BackendHealth {
	start := time.Now()
	ctx, cancel := context.WithTimeout(ctx, 4*time.Second)
	defer cancel()
	list, err := b.fetchModels(ctx)
	h := BackendHealth{Name: b.name, LatencyMS: time.Since(start).Milliseconds()}
	if err != nil {
		h.Error = err.Error()
		return h
	}
	h.OK = true
	h.Models = len(list)
	return h
}

// Registry owns every backend and resolves which one serves a request.
type Registry struct {
	order  []*Backend
	byName map[string]*Backend
	def    *Backend
}

// NewRegistry builds the registry and warms the model caches.
func NewRegistry(cfgs []BackendConfig, defaultName string, timeout time.Duration) *Registry {
	r := &Registry{byName: make(map[string]*Backend, len(cfgs))}
	for _, c := range cfgs {
		b := newBackend(c, timeout)
		r.order = append(r.order, b)
		r.byName[b.name] = b
		if b.name == defaultName {
			r.def = b
		}
	}
	if r.def == nil {
		r.def = r.order[0]
	}
	r.warm()
	return r
}

// warm fills every cache once so model routing works from the first request.
func (r *Registry) warm() {
	ctx, cancel := context.WithTimeout(context.Background(), 6*time.Second)
	defer cancel()
	var wg sync.WaitGroup
	for _, b := range r.order {
		wg.Add(1)
		go func(b *Backend) {
			defer wg.Done()
			if _, err := b.Models(ctx); err != nil {
				log.Printf("[gateway] warm %s: %v", b.name, err)
			}
		}(b)
	}
	wg.Wait()
}

// Default returns the backend used when nothing else matches.
func (r *Registry) Default() *Backend { return r.def }

// Get looks a backend up by name.
func (r *Registry) Get(name string) *Backend { return r.byName[name] }

// All returns the backends in configured order.
func (r *Registry) All() []*Backend { return r.order }

// Resolve picks the backend for a request: an explicit name wins, then the
// backend that advertises the model, then the default.
func (r *Registry) Resolve(backendName, model string) *Backend {
	if backendName != "" {
		if b, ok := r.byName[backendName]; ok {
			return b
		}
	}
	if model != "" {
		if r.def.hasModel(model) {
			return r.def
		}
		for _, b := range r.order {
			if b.hasModel(model) {
				return b
			}
		}
	}
	return r.def
}

// Models merges the model lists of every backend.
func (r *Registry) Models(ctx context.Context) []ModelInfo {
	out := []ModelInfo{}
	for _, b := range r.order {
		list, err := b.Models(ctx)
		if err != nil {
			log.Printf("[gateway] models %s: %v", b.name, err)
			continue
		}
		out = append(out, list...)
	}
	return out
}

// Health probes every backend concurrently.
func (r *Registry) Health(ctx context.Context) []BackendHealth {
	out := make([]BackendHealth, len(r.order))
	var wg sync.WaitGroup
	for i, b := range r.order {
		wg.Add(1)
		go func(i int, b *Backend) {
			defer wg.Done()
			out[i] = b.Health(ctx)
		}(i, b)
	}
	wg.Wait()
	return out
}
