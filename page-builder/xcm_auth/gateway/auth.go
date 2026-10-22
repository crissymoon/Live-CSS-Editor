package gateway

import (
	"crypto/subtle"
	"errors"
	"fmt"
	"net/http"
	"strings"

	"github.com/golang-jwt/jwt/v5"
)

// Identity is the caller behind a request: a signed-in user or the service
// account used by scripts and MCP clients.
type Identity struct {
	Kind     string `json:"kind"`
	UserID   int64  `json:"user_id,omitempty"`
	Username string `json:"username"`
	Role     string `json:"role,omitempty"`
}

// Key is the rate limit bucket name.
func (i *Identity) Key() string {
	if i == nil {
		return "anonymous"
	}
	if i.Kind == "service" {
		return "service"
	}
	return fmt.Sprintf("user:%d", i.UserID)
}

// claims mirrors the token payload issued by xcm_auth.
type claims struct {
	UserID   int64  `json:"uid"`
	Username string `json:"usr"`
	Role     string `json:"role"`
	Use      string `json:"use"`
	jwt.RegisteredClaims
}

// Authenticator validates xcm_auth access tokens and the service token.
type Authenticator struct {
	secret       []byte
	serviceToken string
}

// NewAuthenticator builds an authenticator from the shared JWT secret.
func NewAuthenticator(secret, serviceToken string) *Authenticator {
	return &Authenticator{secret: []byte(secret), serviceToken: serviceToken}
}

// Authenticate resolves the caller from the request headers.
func (a *Authenticator) Authenticate(r *http.Request) (*Identity, error) {
	if tok := strings.TrimSpace(r.Header.Get("X-Service-Token")); tok != "" && a.serviceToken != "" {
		if subtle.ConstantTimeCompare([]byte(tok), []byte(a.serviceToken)) == 1 {
			return &Identity{Kind: "service", Username: "service", Role: "service"}, nil
		}
		return nil, errors.New("invalid service token")
	}
	header := strings.TrimSpace(r.Header.Get("Authorization"))
	if header == "" {
		return nil, errors.New("missing Authorization header")
	}
	raw := strings.TrimSpace(strings.TrimPrefix(header, "Bearer "))
	if raw == "" || raw == header {
		return nil, errors.New("Authorization must be Bearer <token>")
	}
	// A service token may arrive either as X-Service-Token or as the bearer,
	// so plain OpenAI-style clients work with one header.
	if a.serviceToken != "" && subtle.ConstantTimeCompare([]byte(raw), []byte(a.serviceToken)) == 1 {
		return &Identity{Kind: "service", Username: "service", Role: "service"}, nil
	}
	return a.verify(raw)
}

func (a *Authenticator) verify(raw string) (*Identity, error) {
	parsed, err := jwt.ParseWithClaims(raw, &claims{}, func(t *jwt.Token) (any, error) {
		if t.Method.Alg() != jwt.SigningMethodHS256.Alg() {
			return nil, fmt.Errorf("unexpected signing method %q", t.Method.Alg())
		}
		return a.secret, nil
	})
	if err != nil {
		return nil, fmt.Errorf("invalid token: %w", err)
	}
	c, ok := parsed.Claims.(*claims)
	if !ok || !parsed.Valid {
		return nil, errors.New("invalid token")
	}
	if c.Use != "" && c.Use != "access" {
		return nil, errors.New("token is not an access token")
	}
	if c.UserID == 0 {
		return nil, errors.New("token has no user")
	}
	return &Identity{Kind: "user", UserID: c.UserID, Username: c.Username, Role: c.Role}, nil
}
