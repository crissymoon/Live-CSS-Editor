/**
 * agent-flow/js/flow.js
 * Drag-and-drop AI agent canvas using moon-lang for execution.
 * EXPERIMENTAL
 *
 * Architecture:
 *   state.nodes   -- array of {id, type, label, x, y, props}
 *   state.edges   -- array of {id, from:{nodeId,port}, to:{nodeId,port}}
 *   state.selected -- nodeId or null
 *
 * All mutations go through state helpers so the SVG edge layer stays in sync.
 *
 * Fallback error handling: every async call has a .catch(); every try/catch logs
 * to console.error with a "agent-flow:" prefix so problems are always visible.
 */

/* ============================================================
   Constants
   ============================================================ */

const NODE_TYPES = {
    prompt:    { icon: 'P', label: 'Prompt',    color: '#7ab3e0' },
    'ai-call': { icon: 'A', label: 'AI Call',   color: '#c084fc' },
    condition: { icon: '?', label: 'Condition', color: '#fbbf24' },
    loop:      { icon: 'L', label: 'Loop',      color: '#34d399' },
    memory:    { icon: 'M', label: 'Memory',    color: '#f97316' },
    tool:      { icon: 'T', label: 'Tool',      color: '#818cf8' },
    output:    { icon: 'O', label: 'Output',    color: '#4ade80' },
    guard:     { icon: 'G', label: 'Guard',     color: '#ef4444' },
    'agent-task': { icon: 'V', label: 'VS Agent Task', color: '#22d3ee' },
};

/* Default extra props per node type */
const TYPE_DEFAULTS = {
    prompt:    { varName: 'prompt', text: '' },
    'ai-call': { provider: 'openai', model: 'gpt-4o-mini', varName: 'response', inputVar: 'prompt', system: 'You are a helpful assistant.' },
    condition: { varName: '', trueLabel: 'yes', falseLabel: 'no' },
    loop:      { count: 5, indexVar: 'i' },
    memory:    { op: 'keep', varName: 'result', inputVar: '' },
    tool:      { command: '', varName: 'result' },
    output:    { varName: '' },
    guard:     { inputVar: 'prompt', varName: 'guard_result', guardUrl: 'http://localhost:8765/classify', blockOnFlag: 'true' },
    'agent-task': { taskId: 'xcm_auth_guard_login_alt_server', timeoutSec: 240, failFlowOnError: 'true' },
};

/* ============================================================
   State
   ============================================================ */

let state = {
    nodes: [],
    edges: [],
    selected: null,
    nextId: 1,
    wireFrom: null,  // {nodeId, port, portType} when dragging a new edge
};

let taskCatalog = [];

function findTaskMeta(taskId) {
    return taskCatalog.find(t => t.id === taskId) || null;
}

/* ============================================================
   DOM refs
   ============================================================ */

const canvas       = document.getElementById('canvas');
const edgeSvg      = document.getElementById('edge-svg');
const propsForm    = document.getElementById('props-form');
const propsHint    = document.getElementById('props-hint');
const propId       = document.getElementById('prop-id');
const propLabel    = document.getElementById('prop-label');
const propDynamic  = document.getElementById('prop-dynamic');
const outputPanel  = document.getElementById('output-panel');
const outputStdout = document.getElementById('output-stdout');
const outputStderr = document.getElementById('output-stderr');
const outputMoon   = document.getElementById('output-moon');
const outputClose  = document.getElementById('output-close');
const toolbarStatus = document.getElementById('toolbar-status');
const wireOverlay  = document.getElementById('wire-overlay');
const savedList    = document.getElementById('saved-flow-list');

/* ============================================================
   ID helpers
   ============================================================ */

function genId() { return 'n' + (state.nextId++); }
function genEdgeId() { return 'e' + Date.now() + Math.random().toString(36).slice(2, 5); }

/* ============================================================
   Node creation
   ============================================================ */

function createNode(type, x, y) {
    try {
        const def = NODE_TYPES[type];
        if (!def) { console.error('agent-flow: unknown node type', type); return null; }
        const node = {
            id:    genId(),
            type,
            label: def.label,
            x:     Math.max(0, x),
            y:     Math.max(0, y),
            props: Object.assign({}, TYPE_DEFAULTS[type] || {}),
        };
        state.nodes.push(node);
        renderNode(node);
        return node;
    } catch(err) {
        console.error('agent-flow: createNode failed', err);
        return null;
    }
}

function removeNode(id) {
    try {
        state.nodes = state.nodes.filter(n => n.id !== id);
        state.edges = state.edges.filter(e => e.from.nodeId !== id && e.to.nodeId !== id);
        const el = document.getElementById('fn-' + id);
        if (el) el.remove();
        if (state.selected === id) deselect();
        renderAllEdges();
    } catch(err) {
        console.error('agent-flow: removeNode failed', err);
    }
}

/* ============================================================
   Node rendering
   ============================================================ */

function nodeEl(id) { return document.getElementById('fn-' + id); }

function renderNode(node) {
    try {
        let el = nodeEl(node.id);
        if (!el) {
            el = document.createElement('div');
            el.id = 'fn-' + node.id;
            el.className = 'flow-node fn-type-' + node.type;
            canvas.appendChild(el);
            attachNodeEvents(el, node);
        }
        el.style.left = node.x + 'px';
        el.style.top  = node.y + 'px';

        const def = NODE_TYPES[node.type] || { icon: '?', label: node.type };
        const summary = buildSummary(node);

        el.innerHTML = `
            <div class="fn-header">
                <div class="fn-icon">${def.icon}</div>
                <div class="fn-title">${escHtml(node.label)}</div>
                <button class="fn-del" data-del="${node.id}" title="Remove" tabindex="-1">x</button>
            </div>
            <div class="fn-body">${escHtml(summary)}</div>
            <div class="fn-ports">
                <div class="ports-in">
                    <div class="port port-in" data-node="${node.id}" data-port="in" title="Input"></div>
                </div>
                <div class="ports-out">
                    <div class="port port-out" data-node="${node.id}" data-port="out" title="Output"></div>
                </div>
            </div>`;

        /* re-attach port events after innerHTML replacement */
        el.querySelectorAll('.port').forEach(p => attachPortEvents(p));
        el.querySelector('.fn-del').addEventListener('click', e => {
            e.stopPropagation();
            removeNode(node.id);
        });
    } catch(err) {
        console.error('agent-flow: renderNode failed', err);
    }
}

function buildSummary(node) {
    try {
        const p = node.props;
        switch(node.type) {
            case 'prompt':    return p.varName ? `${p.varName} = "${(p.text||'').slice(0,24)}"` : '';
            case 'ai-call':   return `ava.${p.provider||'anthropic'}(${p.inputVar||'...'})`;
            case 'condition': return p.varName ? `if ${p.varName}` : 'if ...';
            case 'loop':      return `loop ${p.count||'N'} times`;
            case 'memory':    return `${p.op||'keep'}: ${p.varName||'...'}`;
            case 'tool':      return p.command ? p.command.slice(0,30) : 'command...';
            case 'output':    return `p(${p.varName||'...'})`;
            case 'guard':     return `guard(${p.inputVar||'...'}) -> ${p.varName||'...'}`;
            case 'agent-task': return `task: ${p.taskId||'...'} (${p.timeoutSec||240}s)`;
            default:          return '';
        }
    } catch(err) {
        console.error('agent-flow: buildSummary failed', err);
        return '';
    }
}

function refreshNodeBody(nodeId) {
    try {
        const node = state.nodes.find(n => n.id === nodeId);
        if (!node) return;
        const el = nodeEl(nodeId);
        if (!el) return;
        const body = el.querySelector('.fn-body');
        if (body) body.textContent = buildSummary(node);
    } catch(err) {
        console.error('agent-flow: refreshNodeBody failed', err);
    }
}

/* ============================================================
   Node drag (move on canvas)
   ============================================================ */

function attachNodeEvents(el, node) {
    let dragging = false, ox = 0, oy = 0;

    el.addEventListener('mousedown', e => {
        if (e.button !== 0) return;
        if (e.target.classList.contains('port')) return;
        if (e.target.classList.contains('fn-del')) return;
        e.preventDefault();
        dragging = true;
        const rect = el.getBoundingClientRect();
        ox = e.clientX - rect.left;
        oy = e.clientY - rect.top;
        selectNode(node.id);
        el.style.zIndex = 50;
        document.addEventListener('mousemove', onMove);
        document.addEventListener('mouseup', onUp);
    });

    function onMove(e) {
        if (!dragging) return;
        const canvasRect = canvas.getBoundingClientRect();
        node.x = Math.max(0, e.clientX - canvasRect.left - ox);
        node.y = Math.max(0, e.clientY - canvasRect.top  - oy);
        el.style.left = node.x + 'px';
        el.style.top  = node.y + 'px';
        renderAllEdges();
    }

    function onUp() {
        dragging = false;
        el.style.zIndex = '';
        document.removeEventListener('mousemove', onMove);
        document.removeEventListener('mouseup', onUp);
    }

    el.addEventListener('click', e => {
        if (e.target.classList.contains('port')) return;
        if (e.target.classList.contains('fn-del')) return;
        selectNode(node.id);
    });
}

/* ============================================================
   Port / edge wiring
   ============================================================ */

let wirePreviewPath = null;

function attachPortEvents(portEl) {
    portEl.addEventListener('mousedown', e => {
        if (e.button !== 0) return;
        e.stopPropagation();
        e.preventDefault();
        const nodeId   = portEl.dataset.node;
        const portName = portEl.dataset.port;
        startWire(nodeId, portName, portEl);
    });

    portEl.addEventListener('mouseup', e => {
        if (!state.wireFrom) return;
        e.stopPropagation();
        const nodeId   = portEl.dataset.node;
        const portName = portEl.dataset.port;
        finishWire(nodeId, portName);
    });
}

function startWire(nodeId, portName, portEl) {
    try {
        state.wireFrom = { nodeId, port: portName };
        portEl.classList.add('active');

        /* draw preview line */
        wirePreviewPath = document.createElementNS('http://www.w3.org/2000/svg', 'path');
        wirePreviewPath.classList.add('flow-edge-preview');
        edgeSvg.appendChild(wirePreviewPath);

        wireOverlay.style.display = 'block';

        wireOverlay.onmousemove = e => {
            if (!state.wireFrom || !wirePreviewPath) return;
            const start = getPortCenter(state.wireFrom.nodeId, state.wireFrom.port);
            if (!start) return;
            const canvasRect = canvas.getBoundingClientRect();
            const end = { x: e.clientX - canvasRect.left, y: e.clientY - canvasRect.top };
            wirePreviewPath.setAttribute('d', bezierPath(start, end));
        };

        wireOverlay.onmouseup = () => cancelWire();
        wireOverlay.onmouseleave = () => cancelWire();
    } catch(err) {
        console.error('agent-flow: startWire failed', err);
        cancelWire();
    }
}

function finishWire(nodeId, portName) {
    try {
        if (!state.wireFrom) return;
        const from = state.wireFrom;
        cancelWire();

        /* do not allow self-loops */
        if (from.nodeId === nodeId) { return; }
        /* validate direction: out -> in */
        const fromPort = from.port;
        const toPort   = portName;
        if (fromPort === toPort) return;
        const [outNode, outPort, inNode, inPort] =
            fromPort === 'out'
                ? [from.nodeId, 'out', nodeId, 'in']
                : [nodeId, 'out', from.nodeId, 'in'];

        /* avoid duplicate edges */
        if (state.edges.find(ex => ex.from.nodeId === outNode && ex.to.nodeId === inNode)) return;

        state.edges.push({ id: genEdgeId(), from: { nodeId: outNode, port: outPort }, to: { nodeId: inNode, port: inPort } });
        renderAllEdges();
    } catch(err) {
        console.error('agent-flow: finishWire failed', err);
        cancelWire();
    }
}

function cancelWire() {
    try {
        if (wirePreviewPath) { wirePreviewPath.remove(); wirePreviewPath = null; }
        wireOverlay.style.display = 'none';
        wireOverlay.onmousemove = null;
        wireOverlay.onmouseup  = null;
        wireOverlay.onmouseleave = null;
        state.wireFrom = null;
        /* remove active class from all ports */
        document.querySelectorAll('.port.active').forEach(p => p.classList.remove('active'));
    } catch(err) {
        console.error('agent-flow: cancelWire failed', err);
    }
}

/* ============================================================
   Edge rendering
   ============================================================ */

function getPortCenter(nodeId, portName) {
    try {
        const node = state.nodes.find(n => n.id === nodeId);
        if (!node) return null;
        const el = nodeEl(nodeId);
        if (!el) return null;
        const portEl = el.querySelector(portName === 'in' ? '.port-in' : '.port-out');
        if (!portEl) return null;
        const canvasRect = canvas.getBoundingClientRect();
        const portRect   = portEl.getBoundingClientRect();
        return {
            x: portRect.left + portRect.width  / 2 - canvasRect.left,
            y: portRect.top  + portRect.height / 2 - canvasRect.top,
        };
    } catch(err) {
        console.error('agent-flow: getPortCenter failed', err);
        return null;
    }
}

function bezierPath(start, end) {
    const cx = (start.x + end.x) / 2;
    return `M ${start.x} ${start.y} C ${cx} ${start.y}, ${cx} ${end.y}, ${end.x} ${end.y}`;
}

function renderAllEdges() {
    try {
        /* remove old static paths */
        edgeSvg.querySelectorAll('.flow-edge').forEach(p => p.remove());

        state.edges.forEach(edge => {
            const from = getPortCenter(edge.from.nodeId, edge.from.port);
            const to   = getPortCenter(edge.to.nodeId,   edge.to.port);
            if (!from || !to) return;

            const path = document.createElementNS('http://www.w3.org/2000/svg', 'path');
            path.classList.add('flow-edge');
            path.setAttribute('d', bezierPath(from, to));
            path.dataset.edgeId = edge.id;
            /* click to remove edge */
            path.style.pointerEvents = 'stroke';
            path.style.cursor = 'pointer';
            path.addEventListener('click', () => {
                state.edges = state.edges.filter(e => e.id !== edge.id);
                renderAllEdges();
            });
            edgeSvg.appendChild(path);
        });
    } catch(err) {
        console.error('agent-flow: renderAllEdges failed', err);
    }
}

/* ============================================================
   Selection / properties panel
   ============================================================ */

function selectNode(id) {
    try {
        deselect();
        state.selected = id;
        const el = nodeEl(id);
        if (el) el.classList.add('selected');
        openProps(id);
    } catch(err) {
        console.error('agent-flow: selectNode failed', err);
    }
}

function deselect() {
    try {
        if (state.selected) {
            const el = nodeEl(state.selected);
            if (el) el.classList.remove('selected');
        }
        state.selected = null;
        propsHint.style.display  = '';
        propsForm.style.display  = 'none';
    } catch(err) {
        console.error('agent-flow: deselect failed', err);
    }
}

function openProps(id) {
    try {
        const node = state.nodes.find(n => n.id === id);
        if (!node) return;
        propsHint.style.display = 'none';
        propsForm.style.display = '';
        propId.value    = id;
        propLabel.value = node.label;
        buildDynamicProps(node);
    } catch(err) {
        console.error('agent-flow: openProps failed', err);
    }
}

function buildDynamicProps(node) {
    try {
        propDynamic.innerHTML = '';
        const p = node.props;
        const field = (label, inputHtml) => {
            const lbl = document.createElement('label');
            lbl.textContent = label;
            propDynamic.appendChild(lbl);
            const wrap = document.createElement('div');
            wrap.innerHTML = inputHtml;
            propDynamic.appendChild(wrap);
        };

        switch(node.type) {
            case 'prompt':
                field('Variable name', `<input type="text" data-prop="varName" value="${escAttr(p.varName||'')}">`);
                field('Text / value', `<textarea data-prop="text">${escHtml(p.text||'')}</textarea>`);
                break;
            case 'ai-call':
                field('Provider', `<select data-prop="provider">
                    ${['openai','anthropic','gemini'].map(v => `<option value="${v}"${p.provider===v?' selected':''}>${v}</option>`).join('')}
                </select>`);
                field('Model', `<input type="text" data-prop="model" value="${escAttr(p.model||'gpt-4o-mini')}" placeholder="gpt-4o-mini">`);
                field('System prompt', `<textarea data-prop="system">${escHtml(p.system||'You are a helpful assistant.')}</textarea>`);
                field('Input variable', `<input type="text" data-prop="inputVar" value="${escAttr(p.inputVar||'prompt')}">`);
                field('Output variable', `<input type="text" data-prop="varName" value="${escAttr(p.varName||'response')}">`);
                break;
            case 'condition':
                field('Variable to test', `<input type="text" data-prop="varName" value="${escAttr(p.varName||'')}">`);
                field('True branch label', `<input type="text" data-prop="trueLabel" value="${escAttr(p.trueLabel||'yes')}">`);
                field('False branch label', `<input type="text" data-prop="falseLabel" value="${escAttr(p.falseLabel||'no')}">`);
                break;
            case 'loop':
                field('Count', `<input type="number" data-prop="count" value="${Number(p.count)||5}" min="1" max="999">`);
                field('Index variable', `<input type="text" data-prop="indexVar" value="${escAttr(p.indexVar||'i')}">`);
                break;
            case 'memory':
                field('Operation', `<select data-prop="op">
                    ${['keep','recall'].map(v => `<option value="${v}"${p.op===v?' selected':''}>${v}</option>`).join('')}
                </select>`);
                field('Variable name', `<input type="text" data-prop="varName" value="${escAttr(p.varName||'result')}">`);
                field('Input variable', `<input type="text" data-prop="inputVar" value="${escAttr(p.inputVar||'')}">`);
                break;
            case 'tool':
                field('Shell command', `<input type="text" data-prop="command" value="${escAttr(p.command||'')}">`);
                field('Output variable', `<input type="text" data-prop="varName" value="${escAttr(p.varName||'result')}">`);
                break;
            case 'output':
                field('Variable to print', `<input type="text" data-prop="varName" value="${escAttr(p.varName||'response')}">`);
                break;
            case 'guard':
                field('Input variable', `<input type="text" data-prop="inputVar" value="${escAttr(p.inputVar||'prompt')}">`);
                field('Output variable', `<input type="text" data-prop="varName" value="${escAttr(p.varName||'guard_result')}">`);
                field('Guard URL', `<input type="text" data-prop="guardUrl" value="${escAttr(p.guardUrl||'http://localhost:8765/classify')}">`);
                field('Block on flag', `<select data-prop="blockOnFlag">
                    <option value="true"${(p.blockOnFlag||'true')==='true'?' selected':''}>yes -- halt flow if flagged</option>
                    <option value="false"${p.blockOnFlag==='false'?' selected':''}>no -- continue even if flagged</option>
                </select>`);
                break;
            case 'agent-task':
                field('Task ID (from smoke-tools.json)', renderTaskIdSelect(p.taskId || 'xcm_auth_guard_login_alt_server'));
                field('Timeout (seconds)', `<input type="number" data-prop="timeoutSec" value="${Number(p.timeoutSec)||240}" min="5" max="1800">`);
                field('Fail flow on error', `<select data-prop="failFlowOnError">
                    <option value="true"${(p.failFlowOnError||'true')==='true'?' selected':''}>yes -- halt flow if task fails</option>
                    <option value="false"${p.failFlowOnError==='false'?' selected':''}>no -- continue flow</option>
                </select>`);
                break;
        }
    } catch(err) {
        console.error('agent-flow: buildDynamicProps failed', err);
    }
}

function renderTaskIdSelect(selectedId) {
    const fallback = escAttr(selectedId || '');
    if (!Array.isArray(taskCatalog) || taskCatalog.length === 0) {
        return `<input type="text" data-prop="taskId" value="${fallback}" placeholder="task id from smoke-tools.json">`;
    }

    const options = taskCatalog.map(task => {
        const id = escAttr(task.id || '');
        const name = escHtml(task.name || task.id || '');
        const selected = task.id === selectedId ? ' selected' : '';
        return `<option value="${id}"${selected}>${name} (${id})</option>`;
    }).join('');

    const selectedExists = taskCatalog.some(task => task.id === selectedId);
    const customOpt = (!selectedExists && selectedId)
        ? `<option value="${fallback}" selected>custom (${fallback})</option>`
        : '';

    return `<select data-prop="taskId">${customOpt}${options}</select>`;
}

async function loadTaskCatalog() {
    try {
        const resp = await fetch('api/tasks.php').catch(err => {
            throw new Error('Network error: ' + err.message);
        });
        if (!resp.ok) {
            throw new Error('HTTP ' + resp.status);
        }
        const data = await resp.json();
        if (!data.ok || !Array.isArray(data.tasks)) {
            throw new Error(data.error || 'Invalid task catalog response');
        }
        taskCatalog = data.tasks;
    } catch (err) {
        console.warn('agent-flow: task catalog unavailable', err);
        taskCatalog = [];
    }
}

propsForm.addEventListener('submit', e => {
    try {
        e.preventDefault();
        const id = propId.value;
        const node = state.nodes.find(n => n.id === id);
        if (!node) return;
        node.label = propLabel.value.trim() || NODE_TYPES[node.type].label;
        propDynamic.querySelectorAll('[data-prop]').forEach(el => {
            const key = el.dataset.prop;
            node.props[key] = el.type === 'number' ? Number(el.value) : el.value;
        });
        renderNode(node);
        renderAllEdges();
    } catch(err) {
        console.error('agent-flow: props submit failed', err);
    }
});

/* deselect on canvas click */
canvas.addEventListener('click', e => {
    if (e.target === canvas) deselect();
});

/* ============================================================
   Palette drag to canvas
   ============================================================ */

document.querySelectorAll('.palette-node').forEach(palNode => {
    palNode.addEventListener('dragstart', e => {
        const type = palNode.dataset.type;
        e.dataTransfer.setData('text/plain', type);
        e.dataTransfer.effectAllowed = 'copy';
    });
});

canvas.addEventListener('dragover', e => {
    e.preventDefault();
    e.dataTransfer.dropEffect = 'copy';
    canvas.classList.add('drop-active');
});

canvas.addEventListener('dragleave', () => canvas.classList.remove('drop-active'));

canvas.addEventListener('drop', e => {
    try {
        e.preventDefault();
        canvas.classList.remove('drop-active');
        const type = e.dataTransfer.getData('text/plain');
        if (!type || !NODE_TYPES[type]) {
            console.warn('agent-flow: drop with unknown type', type);
            return;
        }
        const rect = canvas.getBoundingClientRect();
        const x = e.clientX - rect.left - 70;
        const y = e.clientY - rect.top  - 30;
        const node = createNode(type, x, y);
        if (node) selectNode(node.id);
    } catch(err) {
        console.error('agent-flow: canvas drop failed', err);
    }
});

/* ============================================================
   Moon codegen -- topological walk
   ============================================================ */

function generateMoon() {
    try {
        const lines = [];
        lines.push('::: Auto-generated by agent-flow -- do not edit manually :::');
        lines.push('');
        lines.push('__MAIN_LOGIC__');
        lines.push('');

        /* build adjacency map: nodeId -> list of successor nodeIds */
        const successors = {};
        state.nodes.forEach(n => { successors[n.id] = []; });
        state.edges.forEach(e => {
            if (successors[e.from.nodeId]) successors[e.from.nodeId].push(e.to.nodeId);
        });

        /* topological sort (Kahn's algorithm) */
        const inDegree = {};
        state.nodes.forEach(n => { inDegree[n.id] = 0; });
        state.edges.forEach(e => { inDegree[e.to.nodeId] = (inDegree[e.to.nodeId] || 0) + 1; });

        const queue   = state.nodes.filter(n => (inDegree[n.id] || 0) === 0).map(n => n.id);
        const ordered = [];
        const visited = new Set();

        while (queue.length) {
            const id = queue.shift();
            if (visited.has(id)) continue;
            visited.add(id);
            ordered.push(id);
            (successors[id] || []).forEach(sid => {
                inDegree[sid]--;
                if (inDegree[sid] === 0) queue.push(sid);
            });
        }

        /* emit any nodes not reached (disconnected) */
        state.nodes.forEach(n => { if (!visited.has(n.id)) ordered.push(n.id); });

        ordered.forEach(id => {
            const node = state.nodes.find(n => n.id === id);
            if (!node) return;
            const p = node.props;
            lines.push(`# ${node.label}`);
            switch(node.type) {
                case 'prompt':
                    lines.push(`${safeName(p.varName||'prompt')} is "${escapeMoonStr(p.text||'')}"`);
                    break;
                case 'ai-call': {
                    const provider = p.provider || 'anthropic';
                    const fn = provider === 'anthropic' ? 'ava.anthropic' : provider === 'openai' ? 'ava.openai' : 'ava.make';
                    lines.push(`${safeName(p.varName||'response')} is ${fn}(${safeName(p.inputVar||'prompt')})`);
                    break;
                }
                case 'condition':
                    lines.push(`If ${safeName(p.varName||'result')} is not none then {`);
                    lines.push(`    p("${escapeMoonStr(p.trueLabel||'yes')}")`);
                    lines.push(`}`);
                    lines.push(`Is-none then {`);
                    lines.push(`    p("${escapeMoonStr(p.falseLabel||'no')}")`);
                    lines.push(`}`);
                    break;
                case 'loop':
                    lines.push(`loop thru ${safeName(p.indexVar||'i')} to ${Number(p.count)||5}:`);
                    lines.push(`    p(${safeName(p.indexVar||'i')})`);
                    break;
                case 'memory':
                    if (p.op === 'recall') {
                        lines.push(`${safeName(p.varName||'result')} is ava.recall("${escapeMoonStr(p.inputVar||'')}")`);
                    } else {
                        lines.push(`saved is ava.keep(${safeName(p.inputVar||'result')})`);
                    }
                    break;
                case 'tool':
                    /* moon does not have shell exec natively; emit a comment + p() */
                    lines.push(`# tool: ${escapeMoonStr(p.command||'')}`);
                    lines.push(`p("Running: ${escapeMoonStr(p.command||'')}")`);
                    break;
                case 'output':
                    lines.push(`p(${safeName(p.varName||'response')})`);
                    break;
                case 'guard':
                    /* guard node: runtime HTTP call -- emit a comment in moon source */
                    lines.push(`# guard: check ${safeName(p.inputVar||'prompt')} via ${escapeMoonStr(p.guardUrl||'http://localhost:8765/classify')}`);
                    lines.push(`p("guard node -- use Run AI Direct for live guard checks")`);
                    break;
                case 'agent-task':
                    /* task node: runtime task execution only available in direct runner */
                    lines.push(`# agent-task: ${escapeMoonStr(p.taskId||'')} timeout=${Number(p.timeoutSec)||240}s`);
                    lines.push(`p("agent-task node -- use Run AI Direct for task execution")`);
                    break;
            }
            lines.push('');
        });

        return lines.join('\n');
    } catch(err) {
        console.error('agent-flow: generateMoon failed', err);
        return '# codegen error -- check console';
    }
}

function safeName(s) { return (s || 'x').replace(/[^a-zA-Z0-9_]/g, '_'); }
function escapeMoonStr(s) { return String(s).replace(/\\/g, '\\\\').replace(/"/g, '\\"'); }

/* ============================================================
   Run via API
   ============================================================ */

async function runFlow() {
    try {
        setStatus('running...', '');
        showOutput();
        outputStdout.textContent = '';
        outputStderr.textContent = 'Waiting for output...';
        outputMoon.textContent   = '';

        const moonSrc  = generateMoon();
        const flowJson = JSON.stringify({ nodes: state.nodes, edges: state.edges });

        const resp = await fetch('api/run.php', {
            method:  'POST',
            headers: { 'Content-Type': 'application/json' },
            body:    JSON.stringify({ moon: moonSrc, flow: flowJson }),
        }).catch(err => { throw new Error('Network error: ' + err.message); });

        if (!resp.ok) {
            const text = await resp.text().catch(() => '(no body)');
            throw new Error(`HTTP ${resp.status}: ${text}`);
        }

        const data = await resp.json().catch(err => { throw new Error('Invalid JSON from api/run.php: ' + err.message); });

        outputStdout.textContent = data.stdout || '(no output)';
        outputStderr.textContent = data.stderr || '';
        outputMoon.textContent   = moonSrc;
        if (data.stderr && data.stderr.trim()) {
            console.warn('agent-flow: moon stderr:', data.stderr);
        }
        outputStderr.classList.toggle('hidden', !data.stderr || !data.stderr.trim());
        setStatus(data.exit_code === 0 ? 'OK' : 'error (exit ' + data.exit_code + ')', data.exit_code === 0 ? 'ok' : 'err');
    } catch(err) {
        console.error('agent-flow: runFlow failed', err);
        outputStdout.textContent = '';
        outputStderr.textContent = 'Run error: ' + err.message + '\n\nCheck the browser console for details.';
        outputStderr.classList.remove('hidden');
        setStatus('error', 'err');
    }
}

/* ============================================================
   Run directly via AI provider (no moon binary)
   ============================================================ */

async function runAiDirect() {
    try {
        if (state.nodes.length === 0) {
            setStatus('no nodes -- load a flow first', 'err');
            return;
        }

        // Warn if any ai-call node uses a provider other than openai.
        const unsupported = state.nodes.filter(
            n => n.type === 'ai-call' && (n.props.provider || 'openai') !== 'openai'
        );
        if (unsupported.length) {
            const labels = unsupported.map(n => n.label).join(', ');
            console.warn('agent-flow: runAiDirect -- node(s) use a provider other than openai and will be skipped:', labels);
        }

        setStatus('running AI...', '');
        showOutput();
        outputStdout.textContent = 'Waiting for GPT-4o-mini...';
        outputStderr.textContent = '';
        outputStderr.classList.add('hidden');
        outputMoon.textContent   = '(direct AI run -- no moon source)';

        const resp = await fetch('api/ai_run.php', {
            method:  'POST',
            headers: { 'Content-Type': 'application/json' },
            body:    JSON.stringify({ nodes: state.nodes, edges: state.edges }),
        }).catch(err => { throw new Error('Network error: ' + err.message); });

        if (!resp.ok) {
            const text = await resp.text().catch(() => '(no body)');
            throw new Error('HTTP ' + resp.status + ': ' + text);
        }

        const data = await resp.json().catch(err => {
            throw new Error('Invalid JSON from api/ai_run.php: ' + err.message);
        });

        if (!data.ok) {
            throw new Error(data.error || 'ai_run.php returned ok:false');
        }

        // Main output pane.
        outputStdout.textContent = data.output || '(no output node in flow)';

        // Steps detail in the stderr/detail pane.
        const stepsText = (data.steps || []).map((s, i) => {
            const num    = String(i + 1).padStart(2, ' ');
            const header = `[${num}] ${s.type.toUpperCase().padEnd(10)} ${s.label}`;
            const result = s.error
                ? 'ERROR: ' + s.error
                : (s.result ? s.result.slice(0, 200) + (s.result.length > 200 ? '...' : '') : '(empty)');
            return header + '\n     ' + result;
        }).join('\n\n');

        outputStderr.textContent = stepsText || '(no steps)';
        const hasErrors = (data.steps || []).some(s => s.error);
        outputStderr.classList.toggle('hidden', !stepsText.trim());

        if (data.halted) {
            setStatus('guard halted flow -- input flagged', 'err');
            console.warn('agent-flow: flow was halted by a guard node');
        } else {
            setStatus(hasErrors ? 'AI done (with errors)' : 'AI done', hasErrors ? 'err' : 'ok');
        }

    } catch(err) {
        console.error('agent-flow: runAiDirect failed', err);
        outputStdout.textContent = '';
        outputStderr.textContent = 'AI run error: ' + err.message + '\n\nCheck the browser console for details.';
        outputStderr.classList.remove('hidden');
        setStatus('AI error', 'err');
    }
}

/* ============================================================
   Save / load flows
   ============================================================ */

async function saveFlow() {
    try {
        const name = prompt('Flow name (no spaces):');
        if (!name || !name.trim()) return;
        const safeName = name.trim().replace(/[^a-zA-Z0-9_-]/g, '-');
        const payload  = { name: safeName, flow: { nodes: state.nodes, edges: state.edges, nextId: state.nextId } };
        const resp = await fetch('api/save.php', {
            method:  'POST',
            headers: { 'Content-Type': 'application/json' },
            body:    JSON.stringify(payload),
        }).catch(err => { throw new Error('Network error: ' + err.message); });

        if (!resp.ok) throw new Error(`HTTP ${resp.status}`);
        const data = await resp.json();
        if (data.ok) { setStatus('Saved: ' + safeName, 'ok'); loadFlowList(); }
        else throw new Error(data.error || 'Save failed');
    } catch(err) {
        console.error('agent-flow: saveFlow failed', err);
        setStatus('Save error: ' + err.message, 'err');
    }
}

async function loadFlow(name) {
    try {
        const resp = await fetch('api/load.php?name=' + encodeURIComponent(name))
            .catch(err => { throw new Error('Network error: ' + err.message); });
        if (!resp.ok) throw new Error(`HTTP ${resp.status}`);
        const data = await resp.json();
        if (!data.ok) throw new Error(data.error || 'Load failed');
        clearCanvas();
        const flow = data.flow;
        state.nextId = flow.nextId || 1;
        (flow.nodes || []).forEach(n => {
            state.nodes.push(n);
            renderNode(n);
        });
        state.edges = flow.edges || [];
        renderAllEdges();
        setStatus('Loaded: ' + name, 'ok');
    } catch(err) {
        console.error('agent-flow: loadFlow failed', err);
        setStatus('Load error: ' + err.message, 'err');
    }
}

async function loadFlowList() {
    try {
        const resp = await fetch('api/list.php')
            .catch(err => { throw new Error('Network error: ' + err.message); });
        if (!resp.ok) throw new Error(`HTTP ${resp.status}`);
        const data = await resp.json();
        savedList.innerHTML = '';
        if (!data.flows || data.flows.length === 0) {
            savedList.innerHTML = '<span class="muted">No saved flows</span>';
            return;
        }
        data.flows.forEach(name => {
            const item = document.createElement('div');
            item.className = 'saved-flow-item';
            item.innerHTML = `<span>${escHtml(name)}</span><button class="del-btn" data-name="${escAttr(name)}" title="Delete">x</button>`;
            item.querySelectorAll('span').forEach(s => s.addEventListener('click', () => loadFlow(name)));
            item.querySelector('.del-btn').addEventListener('click', e => {
                e.stopPropagation();
                deleteFlow(name);
            });
            savedList.appendChild(item);
        });
    } catch(err) {
        console.error('agent-flow: loadFlowList failed', err);
        savedList.innerHTML = '<span class="muted">Could not load list</span>';
    }
}

async function deleteFlow(name) {
    try {
        if (!confirm('Delete flow "' + name + '"?')) return;
        const resp = await fetch('api/save.php?delete=' + encodeURIComponent(name), { method: 'DELETE' })
            .catch(err => { throw new Error('Network error: ' + err.message); });
        if (!resp.ok) throw new Error(`HTTP ${resp.status}`);
        loadFlowList();
    } catch(err) {
        console.error('agent-flow: deleteFlow failed', err);
        setStatus('Delete error: ' + err.message, 'err');
    }
}

/* ============================================================
   Export .moon file
   ============================================================ */

function exportMoon() {
    try {
        const src  = generateMoon();
        const blob = new Blob([src], { type: 'text/plain' });
        const url  = URL.createObjectURL(blob);
        const a    = document.createElement('a');
        a.href     = url;
        a.download = 'agent.moon';
        a.click();
        URL.revokeObjectURL(url);
        setStatus('Exported agent.moon', 'ok');
    } catch(err) {
        console.error('agent-flow: exportMoon failed', err);
        setStatus('Export error: ' + err.message, 'err');
    }
}

/* ============================================================
   Toolbar
   ============================================================ */

document.getElementById('btn-run').addEventListener('click', runFlow);
document.getElementById('btn-run-ai').addEventListener('click', runAiDirect);
document.getElementById('btn-export').addEventListener('click', exportMoon);
document.getElementById('btn-save').addEventListener('click', saveFlow);
document.getElementById('btn-clear').addEventListener('click', () => {
    if (!state.nodes.length || confirm('Clear all nodes and edges?')) clearCanvas();
});

document.querySelectorAll('.quick-node-btn').forEach(btn => {
    btn.addEventListener('click', () => {
        const taskId = btn.dataset.taskId || '';
        const node = createNode('agent-task', 140, 120 + (state.nodes.length % 8) * 70);
        if (!node) return;
        node.props.taskId = taskId;
        const meta = findTaskMeta(taskId);
        node.label = meta ? 'Task: ' + (meta.name || taskId) : 'Task: ' + taskId;
        renderNode(node);
        selectNode(node.id);
        renderAllEdges();
    });
});

outputClose.addEventListener('click', () => outputPanel.classList.add('hidden'));

document.querySelectorAll('.out-tab').forEach(tab => {
    tab.addEventListener('click', () => {
        const target = tab.dataset.target;
        document.querySelectorAll('.out-tab').forEach(t => t.classList.remove('active'));
        tab.classList.add('active');
        [outputStdout, outputStderr, outputMoon].forEach(el => el.classList.add('hidden'));
        document.getElementById(target).classList.remove('hidden');
    });
});

/* ============================================================
   Helpers
   ============================================================ */

function clearCanvas() {
    try {
        state.nodes = [];
        state.edges = [];
        state.selected = null;
        state.nextId = 1;
        state.wireFrom = null;
        canvas.innerHTML = '';
        edgeSvg.innerHTML = '';
        deselect();
    } catch(err) {
        console.error('agent-flow: clearCanvas failed', err);
    }
}

function showOutput() { outputPanel.classList.remove('hidden'); }

function setStatus(msg, cls) {
    toolbarStatus.textContent = msg;
    toolbarStatus.className   = 'tb-status' + (cls ? ' ' + cls : '');
}

function escHtml(s) {
    return String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;');
}

function escAttr(s) {
    return String(s).replace(/"/g,'&quot;').replace(/'/g,'&#39;');
}

/* ============================================================
   Init
   ============================================================ */

(function init() {
    try {
        loadFlowList();
        loadTaskCatalog();

        /* validate moon binary on load */
        fetch('api/validate.php')
            .then(r => r.json())
            .then(d => {
                if (!d.ok) {
                    setStatus('moon binary not found -- Run will fail', 'err');
                    console.warn('agent-flow: validate.php says moon binary unavailable:', d.error);
                } else {
                    setStatus('moon ' + (d.version||'ok'), 'ok');
                }
            })
            .catch(err => {
                console.warn('agent-flow: validate.php unreachable', err);
                setStatus('Could not validate moon binary', 'err');
            });

        /* keep edge layer sized to canvas */
        const ro = new ResizeObserver(() => renderAllEdges());
        ro.observe(canvas);
    } catch(err) {
        console.error('agent-flow: init failed', err);
    }
})();
