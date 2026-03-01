package api

import (
	"log"
	"net/http"
	"strconv"

	"xcaliburmoon.net/xcm_auth/config"
	"xcaliburmoon.net/xcm_auth/db"
)

// UserHandlers provides authenticated user management endpoints.
type UserHandlers struct {
	store db.Store
	cfg   *config.Config
}

// NewUserHandlers creates a UserHandlers.
func NewUserHandlers(store db.Store, cfg *config.Config) *UserHandlers {
	return &UserHandlers{store: store, cfg: cfg}
}

// ── GET /user/me ──────────────────────────────────────────────────────────────

// Me returns the profile of the currently authenticated user.
func (h *UserHandlers) Me(w http.ResponseWriter, r *http.Request) {
	claims := claimsFromCtx(r)
	if claims == nil {
		jsonErr(w, http.StatusUnauthorized, "authentication required")
		return
	}
	user, err := h.store.GetUserByID(r.Context(), claims.UserID)
	if err != nil {
		log.Printf("[api/user] Me: GetUserByID %d: %v", claims.UserID, err)
		jsonErr(w, http.StatusInternalServerError, "could not fetch user")
		return
	}
	if user == nil {
		jsonErr(w, http.StatusNotFound, "user not found")
		return
	}
	jsonOK(w, user.Safe())
}

// ── GET /user/sessions ────────────────────────────────────────────────────────

// ListSessions returns all active sessions for the authenticated user.
func (h *UserHandlers) ListSessions(w http.ResponseWriter, r *http.Request) {
	claims := claimsFromCtx(r)
	if claims == nil {
		jsonErr(w, http.StatusUnauthorized, "authentication required")
		return
	}
	sessions, err := h.store.ListSessionsByUser(r.Context(), claims.UserID)
	if err != nil {
		log.Printf("[api/user] ListSessions: %v", err)
		jsonErr(w, http.StatusInternalServerError, "could not fetch sessions")
		return
	}
	jsonOK(w, sessions)
}

// ── GET /user/devices ─────────────────────────────────────────────────────────

// ListDevices returns all known devices for the authenticated user.
func (h *UserHandlers) ListDevices(w http.ResponseWriter, r *http.Request) {
	claims := claimsFromCtx(r)
	if claims == nil {
		jsonErr(w, http.StatusUnauthorized, "authentication required")
		return
	}
	devices, err := h.store.ListDevicesByUser(r.Context(), claims.UserID)
	if err != nil {
		log.Printf("[api/user] ListDevices: %v", err)
		jsonErr(w, http.StatusInternalServerError, "could not fetch devices")
		return
	}
	jsonOK(w, devices)
}

// ── GET /user/audit ───────────────────────────────────────────────────────────

// AuditLog returns recent audit log entries for the authenticated user.
func (h *UserHandlers) AuditLog(w http.ResponseWriter, r *http.Request) {
	claims := claimsFromCtx(r)
	if claims == nil {
		jsonErr(w, http.StatusUnauthorized, "authentication required")
		return
	}
	limit  := queryInt(r, "limit", 20)
	offset := queryInt(r, "offset", 0)
	entries, err := h.store.ListAuditLogByUser(r.Context(), claims.UserID, limit, offset)
	if err != nil {
		log.Printf("[api/user] AuditLog: %v", err)
		jsonErr(w, http.StatusInternalServerError, "could not fetch audit log")
		return
	}
	jsonOK(w, entries)
}

// ── GET /admin/users (admin only) ─────────────────────────────────────────────

// ListUsers returns all users. Admin role required (enforced by RequireRole middleware).
func (h *UserHandlers) ListUsers(w http.ResponseWriter, r *http.Request) {
	limit  := queryInt(r, "limit", 50)
	offset := queryInt(r, "offset", 0)
	users, err := h.store.ListUsers(r.Context(), limit, offset)
	if err != nil {
		log.Printf("[api/user] ListUsers: %v", err)
		jsonErr(w, http.StatusInternalServerError, "could not fetch users")
		return
	}
	type row struct {
		ID          int64  `json:"id"`
		Username    string `json:"username"`
		Email       string `json:"email"`
		Role        string `json:"role"`
		IsActive    bool   `json:"is_active"`
		IsVerified  bool   `json:"is_verified"`
	}
	var out []row
	for _, u := range users {
		out = append(out, row{
			ID: u.ID, Username: u.Username, Email: u.Email,
			Role: u.Role, IsActive: u.IsActive, IsVerified: u.IsVerified,
		})
	}
	jsonOK(w, out)
}

// ── helpers ───────────────────────────────────────────────────────────────────

func queryInt(r *http.Request, key string, def int) int {
	v := r.URL.Query().Get(key)
	if v == "" {
		return def
	}
	i, err := strconv.Atoi(v)
	if err != nil || i < 0 {
		return def
	}
	return i
}
