/**
 * db_bridge/db-client.js
 *
 * Lightweight browser client for the db_bridge API layer.
 * Works with both page-builder/db_api.php and pb_admin/api_proxy.php.
 *
 * Usage (page-builder):
 *   import { DbClient } from '/db_bridge/db-client.js';
 *   const db = new DbClient({ endpoint: '/page-builder/db_api.php', token: stageToken });
 *   const { ok, data } = await db.query('pages.list', { limit: 20 });
 *
 * Usage (pb_admin — uses session cookie, no token needed):
 *   import { DbClient } from '/db_bridge/db-client.js';
 *   const db = new DbClient({ endpoint: '/page-builder/pb_admin/api_proxy.php?action=db' });
 *   const { ok, data } = await db.query('admin.table_list', {}, 'dev-tools/db-browser/databases/pages');
 */

export class DbClient {
    /**
     * @param {object}  opts
     * @param {string}  opts.endpoint  Full path to the PHP endpoint.
     * @param {string}  [opts.token]   Bearer token (stage-token or xcm_auth).
     * @param {string}  [opts.db]      Default database key.
     * @param {boolean} [opts.debug]   Log requests to console when true.
     */
    constructor({ endpoint, token = '', db = '', debug = false } = {}) {
        if (!endpoint) throw new Error('DbClient: endpoint is required.');
        this._endpoint = endpoint;
        this._token    = token;
        this._db       = db;
        this._debug    = debug;
    }

    /**
     * Execute an allowlisted action.
     *
     * @param  {string} action     e.g. "pages.list"
     * @param  {object} [params]   Action parameters.
     * @param  {string} [db]       Override the default database key.
     * @returns {Promise<{ok: boolean, data: any, error: string|null}>}
     */
    async query(action, params = {}, db = '') {
        const body = {
            action,
            params,
            db: db || this._db,
        };

        const headers = { 'Content-Type': 'application/json' };
        if (this._token) {
            headers['Authorization'] = `Bearer ${this._token}`;
        }

        if (this._debug) {
            console.debug('[DbClient] →', action, body);
        }

        let resp;
        try {
            resp = await fetch(this._endpoint, {
                method:      'POST',
                credentials: 'same-origin', // send session cookie for xcm_auth
                headers,
                body:        JSON.stringify(body),
            });
        } catch (err) {
            return { ok: false, data: null, error: 'Network error: ' + err.message };
        }

        let json;
        try {
            json = await resp.json();
        } catch {
            return { ok: false, data: null, error: 'Invalid JSON response (HTTP ' + resp.status + ')' };
        }

        if (this._debug) {
            console.debug('[DbClient] ←', action, json);
        }

        return json;
    }

    // ---- Convenience wrappers for common page-builder operations -----------

    /** List pages with optional pagination */
    listPages(limit = 50, offset = 0) {
        return this.query('pages.list', { limit, offset });
    }

    /** Get a single page by slug */
    getPage(slug) {
        return this.query('pages.get', { slug });
    }

    /**
     * Create or update a page.
     * @param {string} slug
     * @param {object} data  { title?, status?, css_overrides?, meta? }
     */
    upsertPage(slug, data = {}) {
        return this.query('pages.upsert', { slug, ...data });
    }

    /** Delete a page by slug */
    deletePage(slug) {
        return this.query('pages.delete', { slug });
    }

    /** Fetch save history for a page */
    pageHistory(slug, limit = 20) {
        return this.query('pages.history', { slug, limit });
    }

    // ---- Admin convenience wrappers ----------------------------------------

    /** List all workspace databases (admin only) */
    listDatabases() {
        return this.query('admin.db_list', {});
    }

    /** List tables in the currently selected database */
    listTables() {
        return this.query('admin.table_list', {});
    }

    /**
     * Read rows from a table.
     * @param {string} table
     * @param {number} limit
     * @param {number} offset
     */
    readTable(table, limit = 50, offset = 0) {
        return this.query('admin.table_read', { table, limit, offset });
    }

    /** Count rows in a table */
    countTable(table) {
        return this.query('admin.table_count', { table });
    }
}

// ---------------------------------------------------------------------------
// Module-level singleton factory for direct use without a class instance
// ---------------------------------------------------------------------------

/**
 * Quick singleton for the page-builder context.
 * Auto-reads the stage token from a meta tag:
 *   <meta name="stage-token" content="...">
 *
 * @returns {DbClient}
 */
export function getPageBuilderClient() {
    const meta  = document.querySelector('meta[name="stage-token"]');
    const token = meta ? meta.getAttribute('content') : '';
    return new DbClient({
        endpoint: '/page-builder/db_api.php',
        token,
        debug: window._DB_DEBUG === true,
    });
}

/**
 * Quick singleton for pb_admin — relies on the session cookie.
 *
 * @returns {DbClient}
 */
export function getAdminClient() {
    return new DbClient({
        endpoint: '/page-builder/pb_admin/api_proxy.php?action=db',
        debug: window._DB_DEBUG === true,
    });
}
