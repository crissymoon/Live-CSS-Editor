/*
 * Crissy's Style Tool
 * Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
 * https://xcaliburmoon.net/
 * MIT License -- see LICENSE file for full text.
 */
/**
 * wireframe.js -- Entry point for the wireframe module.
 *
 * Sub-modules live in js/wireframe/:
 *   constants.js  -- GAP, canvas dimensions, storage key
 *   state.js      -- shared mutable state, localStorage helpers
 *   elements.js   -- element factory, lookup helpers
 *   geometry.js   -- collision math, guide clamping, tryApply
 *   io.js         -- JSON save/load, context builder
 *   props.js      -- properties panel HTML builder, input binding helpers
 *   render.js     -- all DOM rendering, deleteEl
 *   input.js      -- mouse and keyboard handlers
 *   init.js       -- init(), getState(), loadState()
 *
 * Loaded via <script type="module"> so ES import/export is available.
 */
import { init, getState, loadState } from './wireframe/init.js';

window.LiveCSS = window.LiveCSS || {};

window.LiveCSS.wireframe = { init, getState, loadState };
