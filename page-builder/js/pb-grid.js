/**
 * pb-grid.js
 * Cross-platform grid alignment and snapping system for the page builder.
 * Provides visual grid, alignment guides, and snap-to-edge functionality.
 */
(function () {
    'use strict';

    var PBGRID = window.PBGRID = {
        gridSize: 8,
        snapEnabled: true,
        showGuides: true,
        alignMode: 'grid', // 'grid' or 'edges'
        elements: [], // tracked elements for alignment
    };

    /* ========== Grid sizing helpers ====================================== */
    function snapToGrid(value, size) {
        size = size || PBGRID.gridSize;
        if (!PBGRID.snapEnabled) return value;
        return Math.round(value / size) * size;
    }

    function isNearEdge(val, target, threshold) {
        threshold = threshold || 4;
        return Math.abs(val - target) < threshold;
    }

    /* ========== Visual grid overlay ====================================== */
    function createGridOverlay(container, cellSize) {
        cellSize = cellSize || PBGRID.gridSize * 4; // visual grid every 32px

        var canvas = document.createElement('canvas');
        canvas.className = 'pbgrid-canvas';
        canvas.width = container.offsetWidth;
        canvas.height = container.offsetHeight;

        var ctx = canvas.getContext('2d');
        ctx.strokeStyle = 'rgba(100, 200, 255, 0.1)';
        ctx.lineWidth = 1;

        // Draw vertical lines
        for (var x = 0; x < canvas.width; x += cellSize) {
            ctx.beginPath();
            ctx.moveTo(x, 0);
            ctx.lineTo(x, canvas.height);
            ctx.stroke();
        }

        // Draw horizontal lines
        for (var y = 0; y < canvas.height; y += cellSize) {
            ctx.beginPath();
            ctx.moveTo(0, y);
            ctx.lineTo(canvas.width, y);
            ctx.stroke();
        }

        return canvas;
    }

    function showGridOverlay(container) {
        if (!PBGRID.showGuides) return;

        var existing = container.querySelector('.pbgrid-canvas');
        if (existing) existing.remove();

        var grid = createGridOverlay(container);
        container.style.position = 'relative';
        container.appendChild(grid);
    }

    function hideGridOverlay(container) {
        var grid = container.querySelector('.pbgrid-canvas');
        if (grid) grid.remove();
    }

    /* ========== Alignment guides (moving elements) ======================= */
    function showAlignmentGuides(element, alignments) {
        var guides = document.createElement('div');
        guides.className = 'pbgrid-guides';

        if (alignments.left !== undefined) {
            var leftGuide = document.createElement('div');
            leftGuide.className = 'pbgrid-guide-v';
            leftGuide.style.left = alignments.left + 'px';
            guides.appendChild(leftGuide);
        }

        if (alignments.top !== undefined) {
            var topGuide = document.createElement('div');
            topGuide.className = 'pbgrid-guide-h';
            topGuide.style.top = alignments.top + 'px';
            guides.appendChild(topGuide);
        }

        element.parentElement.appendChild(guides);
        return guides;
    }

    function hideAlignmentGuides() {
        var guides = document.querySelectorAll('.pbgrid-guides');
        guides.forEach(function (g) { g.remove(); });
    }

    /* ========== Smart alignment (snap to nearby elements/edges) ========== */
    function calculateAlignment(element, allElements) {
        var rect = element.getBoundingClientRect();
        var result = {};

        // Check proximity to other elements
        Array.from(allElements).forEach(function (other) {
            if (other === element) return;

            var otherRect = other.getBoundingClientRect();
            var threshold = 10;

            // Vertical alignment (left edges)
            if (isNearEdge(rect.left, otherRect.left, threshold)) {
                result.left = otherRect.left;
                result.alignEdge = 'left';
            }

            // Vertical alignment (right edges)
            if (isNearEdge(rect.right, otherRect.right, threshold)) {
                result.left = otherRect.right - rect.width;
                result.alignEdge = 'right';
            }

            // Horizontal alignment (top edges)
            if (isNearEdge(rect.top, otherRect.top, threshold)) {
                result.top = otherRect.top;
                result.alignEdge = 'top';
            }

            // Horizontal alignment (bottom edges)
            if (isNearEdge(rect.bottom, otherRect.bottom, threshold)) {
                result.top = otherRect.bottom - rect.height;
                result.alignEdge = 'bottom';
            }
        });

        return result;
    }

    /* ========== Container edge snapping ================================= */
    function constrainToContainer(element, container) {
        var rect = element.getBoundingClientRect();
        var containerRect = container.getBoundingClientRect();

        var constraints = {
            minX: 0,
            maxX: containerRect.width - rect.width,
            minY: 0,
            maxY: containerRect.height - rect.height
        };

        return constraints;
    }

    /* ========== Responsive grid adjustment ============================== */
    function setGridSize(size) {
        PBGRID.gridSize = size;
        console.log('[PBGRID] grid size set to ' + size + 'px');
    }

    function adjustGridForViewport() {
        var w = window.innerWidth;
        
        if (w < 768) {
            setGridSize(4); // mobile: finer grid
        } else if (w < 1024) {
            setGridSize(8); // tablet
        } else {
            setGridSize(16); // desktop: coarser grid
        }
    }

    /* ========== Distribute & align tools ================================ */
    function distributeHorizontally(elements) {
        if (elements.length < 2) return;

        var sorted = Array.from(elements).sort(function (a, b) {
            return a.getBoundingClientRect().left - b.getBoundingClientRect().left;
        });

        var first = sorted[0].getBoundingClientRect();
        var last = sorted[sorted.length - 1].getBoundingClientRect();
        var totalSpace = last.left - first.left;
        var spacing = totalSpace / (sorted.length - 1);

        sorted.forEach(function (el, i) {
            var x = snapToGrid(first.left + (i * spacing));
            el.style.left = x + 'px';
        });
    }

    function distributeVertically(elements) {
        if (elements.length < 2) return;

        var sorted = Array.from(elements).sort(function (a, b) {
            return a.getBoundingClientRect().top - b.getBoundingClientRect().top;
        });

        var first = sorted[0].getBoundingClientRect();
        var last = sorted[sorted.length - 1].getBoundingClientRect();
        var totalSpace = last.top - first.top;
        var spacing = totalSpace / (sorted.length - 1);

        sorted.forEach(function (el, i) {
            var y = snapToGrid(first.top + (i * spacing));
            el.style.top = y + 'px';
        });
    }

    function alignLeft(elements) {
        var minX = Math.min.apply(null, Array.from(elements).map(function (el) {
            return el.getBoundingClientRect().left;
        }));
        elements.forEach(function (el) {
            el.style.left = (minX - el.parentElement.getBoundingClientRect().left) + 'px';
        });
    }

    function alignCenter(elements) {
        var container = elements[0].parentElement;
        var containerWidth = container.offsetWidth;
        elements.forEach(function (el) {
            var x = (containerWidth - el.offsetWidth) / 2;
            el.style.left = snapToGrid(x) + 'px';
        });
    }

    function alignTop(elements) {
        var minY = Math.min.apply(null, Array.from(elements).map(function (el) {
            return el.getBoundingClientRect().top;
        }));
        elements.forEach(function (el) {
            el.style.top = (minY - el.parentElement.getBoundingClientRect().top) + 'px';
        });
    }

    /* ========== Keyboard shortcuts ======================================= */
    function bindKeyboardShortcuts(container) {
        document.addEventListener('keydown', function (e) {
            // Ctrl+G: toggle grid overlay
            if ((e.ctrlKey || e.metaKey) && e.key === 'g') {
                e.preventDefault();
                PBGRID.showGuides = !PBGRID.showGuides;
                if (PBGRID.showGuides) {
                    showGridOverlay(container);
                } else {
                    hideGridOverlay(container);
                }
            }

            // Ctrl+Shift+S: toggle snapping
            if ((e.ctrlKey || e.metaKey) && e.shiftKey && e.key === 's') {
                e.preventDefault();
                PBGRID.snapEnabled = !PBGRID.snapEnabled;
                console.log('[PBGRID] snap: ' + PBGRID.snapEnabled);
            }
        });
    }

    /* ========== Initialize ============================================== */
    function init(container) {
        container = container || document.querySelector('[data-dnd-canvas]');
        if (!container) return;

        // Show initial grid overlay
        showGridOverlay(container);

        // Adjust grid for viewport on resize
        window.addEventListener('resize', adjustGridForViewport);
        adjustGridForViewport();

        // Bind keyboard shortcuts
        bindKeyboardShortcuts(container);
    }

    /* ========== Public API =============================================== */
    PBGRID.init = init;
    PBGRID.snapToGrid = snapToGrid;
    PBGRID.setGridSize = setGridSize;
    PBGRID.showGridOverlay = showGridOverlay;
    PBGRID.hideGridOverlay = hideGridOverlay;
    PBGRID.showAlignmentGuides = showAlignmentGuides;
    PBGRID.hideAlignmentGuides = hideAlignmentGuides;
    PBGRID.calculateAlignment = calculateAlignment;
    PBGRID.distributeHorizontally = distributeHorizontally;
    PBGRID.distributeVertically = distributeVertically;
    PBGRID.alignLeft = alignLeft;
    PBGRID.alignCenter = alignCenter;
    PBGRID.alignTop = alignTop;
    PBGRID.adjustGridForViewport = adjustGridForViewport;

    // Auto-init if DOM ready
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    } else {
        init();
    }

})();
