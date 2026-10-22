// Package gateway provides an authenticated, OpenAI-compatible chat proxy in
// front of one or more model servers: Ollama behind an SSH reverse tunnel, a
// vLLM worker on a rented GPU, llama.cpp, or any other endpoint that speaks
// the /v1 schema.
//
// The gateway holds no user database. It trusts the access tokens issued by
// xcm_auth (shared HS256 secret) and a long-lived service token for scripts
// and MCP clients.
package gateway

import (
	"fmt"
	"log"
	"os"
	"strconv"
	"strings"
	"time"
)

// BackendConfig describes one upstream model server.
type BackendConfig struct {
	Name    string
	BaseURL string // no trailing slash, includes the /v1 prefix
	APIKey  string // optional bearer for hosted endpoints
}

// Config holds every runtime setting. All values come from the environment.
type Config struct {
	Addr           string
	JWTSecret      string
	ServiceToken   string
	AuthBaseURL    string // empty means the UI talks to the same origin
	DefaultBackend string
	CORSOrigins    []string
	RequestTimeout time.Duration
	RatePerMinute  int
	MaxBodyBytes   int64
	Backends       []BackendConfig
}

// LoadConfig reads the environment and validates the result.
func LoadConfig() (*Config, error) {
	cfg := &Config{
		Addr:           env("GATEWAY_ADDR", "127.0.0.1:9101"),
		JWTSecret:      env("JWT_ACCESS_SECRET", ""),
		ServiceToken:   env("GATEWAY_SERVICE_TOKEN", ""),
		AuthBaseURL:    strings.TrimRight(env("GATEWAY_AUTH_BASE", ""), "/"),
		DefaultBackend: env("GATEWAY_DEFAULT_BACKEND", ""),
		CORSOrigins:    splitCSV(env("GATEWAY_CORS_ORIGINS", "")),
		RequestTimeout: time.Duration(envInt("GATEWAY_TIMEOUT_SECONDS", 300)) * time.Second,
		RatePerMinute:  envInt("GATEWAY_RATE_PER_MINUTE", 40),
		MaxBodyBytes:   int64(envInt("GATEWAY_MAX_BODY_KB", 8192)) * 1024,
	}

	keys := parsePairs(env("GATEWAY_BACKEND_KEYS", ""))
	for _, spec := range splitCSV(env("GATEWAY_BACKENDS", "")) {
		name, base, ok := strings.Cut(spec, "=")
		name = strings.TrimSpace(name)
		base = strings.TrimSpace(base)
		if !ok || name == "" || base == "" {
			return nil, fmt.Errorf("GATEWAY_BACKENDS entry %q is not name=baseurl", spec)
		}
		if !strings.HasPrefix(base, "http://") && !strings.HasPrefix(base, "https://") {
			return nil, fmt.Errorf("backend %q base url %q must start with http:// or https://", name, base)
		}
		cfg.Backends = append(cfg.Backends, BackendConfig{
			Name:    name,
			BaseURL: strings.TrimRight(base, "/"),
			APIKey:  keys[name],
		})
	}

	if len(cfg.Backends) == 0 {
		return nil, fmt.Errorf("GATEWAY_BACKENDS is empty")
	}
	if cfg.JWTSecret == "" {
		return nil, fmt.Errorf("JWT_ACCESS_SECRET is required and must match xcm_auth")
	}
	if cfg.DefaultBackend == "" {
		cfg.DefaultBackend = cfg.Backends[0].Name
	}
	found := false
	for _, b := range cfg.Backends {
		if b.Name == cfg.DefaultBackend {
			found = true
		}
	}
	if !found {
		return nil, fmt.Errorf("GATEWAY_DEFAULT_BACKEND %q is not one of the configured backends", cfg.DefaultBackend)
	}
	if cfg.RatePerMinute < 1 {
		cfg.RatePerMinute = 1
	}

	log.Printf("[gateway/config] addr=%s backends=%d default=%s rate=%d/min service_token=%v",
		cfg.Addr, len(cfg.Backends), cfg.DefaultBackend, cfg.RatePerMinute, cfg.ServiceToken != "")
	return cfg, nil
}

func env(key, fallback string) string {
	if v := strings.TrimSpace(os.Getenv(key)); v != "" {
		return v
	}
	return fallback
}

func envInt(key string, fallback int) int {
	raw := strings.TrimSpace(os.Getenv(key))
	if raw == "" {
		return fallback
	}
	n, err := strconv.Atoi(raw)
	if err != nil {
		log.Printf("[gateway/config] %s=%q is not an integer, using %d", key, raw, fallback)
		return fallback
	}
	return n
}

func splitCSV(raw string) []string {
	out := []string{}
	for _, part := range strings.Split(raw, ",") {
		if p := strings.TrimSpace(part); p != "" {
			out = append(out, p)
		}
	}
	return out
}

// parsePairs turns "a=x,b=y" into a map. Only the first "=" splits, so values
// that contain "=" (base64 keys) survive intact.
func parsePairs(raw string) map[string]string {
	out := map[string]string{}
	for _, part := range splitCSV(raw) {
		k, v, ok := strings.Cut(part, "=")
		if !ok {
			continue
		}
		out[strings.TrimSpace(k)] = strings.TrimSpace(v)
	}
	return out
}
