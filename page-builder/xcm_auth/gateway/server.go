package gateway

import (
	"context"
	"embed"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"strings"
	"time"

	"github.com/go-chi/chi/v5"
	"github.com/go-chi/chi/v5/middleware"
)

//go:embed web/index.html
var webAssets embed.FS

type ctxKey string

const identityKey ctxKey = "identity"

// Server wires the router, the backends and the token check.
type Server struct {
	cfg      *Config
	registry *Registry
	auth     *Authenticator
	limiter  *limiter
	ui       []byte
	started  time.Time
	http     *http.Server
}

// New builds a server from an already validated config.
func New(cfg *Config) *Server {
	ui, err := webAssets.ReadFile("web/index.html")
	if err != nil {
		log.Fatalf("[gateway] embedded UI missing: %v", err)
	}
	page := string(ui)
	page = strings.ReplaceAll(page, "__AUTH_BASE__", cfg.AuthBaseURL)
	page = strings.ReplaceAll(page, "__DEFAULT_BACKEND__", cfg.DefaultBackend)

	return &Server{
		cfg:      cfg,
		registry: NewRegistry(cfg.Backends, cfg.DefaultBackend, cfg.RequestTimeout),
		auth:     NewAuthenticator(cfg.JWTSecret, cfg.ServiceToken),
		limiter:  newLimiter(cfg.RatePerMinute),
		ui:       []byte(page),
		started:  time.Now(),
	}
}

// Handler returns the router.
func (s *Server) Handler() http.Handler {
	r := chi.NewRouter()
	r.Use(middleware.RealIP)
	r.Use(middleware.Recoverer)
	r.Use(middleware.Timeout(s.cfg.RequestTimeout + 30*time.Second))
	r.Use(s.cors)

	r.Get("/", s.handleUI)
	r.Get("/health", s.handleHealth)

	r.Group(func(r chi.Router) {
		r.Use(s.requireAuth)
		r.Get("/v1/models", s.handleModels)
		r.Post("/v1/chat/completions", s.handleChat)
		r.Get("/api/me", s.handleMe)
	})
	return r
}

// Start listens until the process is told to stop.
func (s *Server) Start() error {
	log.Printf("[gateway] listening on %s", s.cfg.Addr)
	s.http = &http.Server{
		Addr:              s.cfg.Addr,
		Handler:           s.Handler(),
		ReadHeaderTimeout: 10 * time.Second,
		WriteTimeout:      0, // streaming replies run as long as the model takes
		IdleTimeout:       120 * time.Second,
	}
	if err := s.http.ListenAndServe(); err != nil && err != http.ErrServerClosed {
		return fmt.Errorf("[gateway] listen: %w", err)
	}
	return nil
}

// Shutdown stops the listener gracefully.
func (s *Server) Shutdown(ctx context.Context) error {
	if s.http == nil {
		return nil
	}
	log.Println("[gateway] shutting down")
	return s.http.Shutdown(ctx)
}

// requireAuth rejects anything without a valid user or service token.
func (s *Server) requireAuth(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		id, err := s.auth.Authenticate(r)
		if err != nil {
			w.Header().Set("WWW-Authenticate", `Bearer realm="xcm-gateway"`)
			writeJSON(w, http.StatusUnauthorized, apiError(err.Error()))
			return
		}
		next.ServeHTTP(w, r.WithContext(context.WithValue(r.Context(), identityKey, id)))
	})
}

func (s *Server) cors(next http.Handler) http.Handler {
	allowed := map[string]bool{}
	for _, o := range s.cfg.CORSOrigins {
		allowed[o] = true
	}
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		origin := r.Header.Get("Origin")
		if origin != "" && (allowed["*"] || allowed[origin]) {
			w.Header().Set("Access-Control-Allow-Origin", origin)
			w.Header().Set("Vary", "Origin")
			w.Header().Set("Access-Control-Allow-Headers", "Authorization, Content-Type, X-Service-Token")
			w.Header().Set("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
			w.Header().Set("Access-Control-Max-Age", "600")
		}
		if r.Method == http.MethodOptions {
			w.WriteHeader(http.StatusNoContent)
			return
		}
		next.ServeHTTP(w, r)
	})
}

func (s *Server) handleUI(w http.ResponseWriter, _ *http.Request) {
	w.Header().Set("Content-Type", "text/html; charset=utf-8")
	w.Header().Set("Cache-Control", "no-cache")
	_, _ = w.Write(s.ui)
}

func (s *Server) handleHealth(w http.ResponseWriter, r *http.Request) {
	backends := s.registry.Health(r.Context())
	ok := false
	for _, b := range backends {
		if b.OK {
			ok = true
		}
	}
	writeJSON(w, http.StatusOK, map[string]any{
		"ok":       ok,
		"uptime_s": int(time.Since(s.started).Seconds()),
		"backends": backends,
	})
}

func (s *Server) handleModels(w http.ResponseWriter, r *http.Request) {
	list := s.registry.Models(r.Context())
	writeJSON(w, http.StatusOK, map[string]any{"object": "list", "data": list})
}

func (s *Server) handleMe(w http.ResponseWriter, r *http.Request) {
	writeJSON(w, http.StatusOK, identityFrom(r))
}

// identityFrom returns the caller set by requireAuth.
func identityFrom(r *http.Request) *Identity {
	if id, ok := r.Context().Value(identityKey).(*Identity); ok {
		return id
	}
	return &Identity{Kind: "anonymous", Username: "anonymous"}
}

func writeJSON(w http.ResponseWriter, status int, v any) {
	w.Header().Set("Content-Type", "application/json; charset=utf-8")
	w.WriteHeader(status)
	if err := json.NewEncoder(w).Encode(v); err != nil {
		log.Printf("[gateway] writeJSON: %v", err)
	}
}
