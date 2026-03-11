/**
 * pb-dnd.js
 * Drag-and-drop system for images in the page builder canvas.
 * Supports dropping images, auto-fitting, and repositioning with grid snapping.
 */
(function () {
    'use strict';

    var PBDND = window.PBDND = {
        activeImage: null,
        gridSize: 8,
        snapEnabled: true,
        dropZone: null,
    };

    /* ========== DOM helpers ================================================ */
    function el(id) { return document.getElementById(id); }
    function qs(sel) { return document.querySelector(sel); }
    function qsa(sel) { return document.querySelectorAll(sel); }

    /* ========== Grid snapping ============================================= */
    function snapToGrid(value, gridSize) {
        gridSize = gridSize || PBDND.gridSize;
        return Math.round(value / gridSize) * gridSize;
    }

    function snapRect(rect, gridSize) {
        return {
            x: snapToGrid(rect.x, gridSize),
            y: snapToGrid(rect.y, gridSize),
            w: snapToGrid(rect.w, gridSize),
            h: snapToGrid(rect.h, gridSize)
        };
    }

    /* ========== Drag start (image from library or canvas) ================ */
    function onImageDragStart(e) {
        var img = e.target.closest('[data-dnd-image]');
        if (!img) return;

        PBDND.activeImage = img;
        e.dataTransfer.effectAllowed = 'move';
        e.dataTransfer.setData('text/html', img.innerHTML);

        // Visual feedback
        img.style.opacity = '0.6';
        img.classList.add('dragging');
    }

    function onImageDragEnd(e) {
        if (PBDND.activeImage) {
            PBDND.activeImage.style.opacity = '';
            PBDND.activeImage.classList.remove('dragging');
        }
        PBDND.activeImage = null;
    }

    /* ========== Canvas drag handlers ====================================== */
    function onCanvasDragOver(e) {
        e.preventDefault();
        e.dataTransfer.dropEffect = 'move';
        
        var canvas = el('pbc-canvas-wrap');
        if (canvas) {
            canvas.classList.add('dnd-active');
        }
    }

    function onCanvasDragLeave(e) {
        // Only remove if leaving the canvas entirely
        if (e.target.id === 'pbc-canvas-wrap' || !el('pbc-canvas-wrap').contains(e.relatedTarget)) {
            var canvas = el('pbc-canvas-wrap');
            if (canvas) {
                canvas.classList.remove('dnd-active');
            }
        }
    }

    function onCanvasDrop(e) {
        e.preventDefault();
        
        var canvas = el('pbc-canvas-wrap');
        if (canvas) {
            canvas.classList.remove('dnd-active');
        }

        var files = e.dataTransfer.files;
        var images = Array.from(files).filter(function (f) {
            return f.type.startsWith('image/');
        });

        if (images.length > 0) {
            handleImageFiles(images, e);
        }
    }

    /* ========== File handling ============================================= */
    function handleImageFiles(files, dropEvent) {
        var canvas = el('pbc-canvas-wrap');
        if (!canvas) return;

        var dropX = dropEvent ? dropEvent.clientX : 0;
        var dropY = dropEvent ? dropEvent.clientY : 0;
        var canvasRect = canvas.getBoundingClientRect();

        Array.from(files).forEach(function (file, idx) {
            var reader = new FileReader();
            reader.onload = function (e) {
                var imgData = e.target.result;
                var imgEl = createImageElement(imgData, file.name, {
                    x: snapToGrid(dropX - canvasRect.left + (idx * 20)),
                    y: snapToGrid(dropY - canvasRect.top + (idx * 20))
                });
                canvas.appendChild(imgEl);
                
                // Bind drag handlers to newly created image
                imgEl.addEventListener('dragstart', onImageDragStart);
                imgEl.addEventListener('dragend', onImageDragEnd);
                
                // Make it repositionable immediately
                makeImageRepositionable(imgEl);
            };
            reader.readAsDataURL(file);
        });
    }

    /* ========== Create image element wrapper ============================= */
    function createImageElement(src, name, pos) {
        var container = document.createElement('div');
        container.className = 'pbdnd-image-container';
        container.setAttribute('data-dnd-image', 'true');
        container.draggable = true;
        
        // Set position from grid snap
        pos = pos || { x: 0, y: 0 };
        container.style.left = pos.x + 'px';
        container.style.top = pos.y + 'px';

        var img = document.createElement('img');
        img.src = src;
        img.alt = name;
        img.className = 'pbdnd-image';
        
        // Auto-fit: image fills container naturally
        img.addEventListener('load', function () {
            autoFitImage(container, img);
        });

        var handles = document.createElement('div');
        handles.className = 'pbdnd-handles';
        
        // Resize handle (bottom-right corner)
        var resizeHandle = document.createElement('div');
        resizeHandle.className = 'pbdnd-handle resize';
        resizeHandle.title = 'drag to resize';
        handles.appendChild(resizeHandle);

        // Delete button
        var deleteBtn = document.createElement('button');
        deleteBtn.className = 'pbdnd-delete';
        deleteBtn.textContent = 'x';
        deleteBtn.title = 'delete image';
        deleteBtn.onclick = function () {
            container.remove();
            updatePageManifest();
        };
        handles.appendChild(deleteBtn);

        container.appendChild(img);
        container.appendChild(handles);
        
        return container;
    }

    /* ========== Auto-fit sizing ========================================== */
    function autoFitImage(container, img) {
        // Constrain to reasonable sizes while maintaining aspect ratio
        var maxWidth = 400;
        var maxHeight = 300;
        var naturalW = img.naturalWidth;
        var naturalH = img.naturalHeight;
        
        var scale = Math.min(maxWidth / naturalW, maxHeight / naturalH, 1);
        var finalW = Math.round(naturalW * scale);
        var finalH = Math.round(naturalH * scale);
        
        // Snap to grid
        finalW = snapToGrid(finalW);
        finalH = snapToGrid(finalH);

        container.style.width = finalW + 'px';
        container.style.height = finalH + 'px';
        img.style.width = '100%';
        img.style.height = '100%';
        img.style.objectFit = 'contain';
    }

    /* ========== Repositioning (mouse drag) ================================ */
    function makeImageRepositionable(imgEl) {
        var startX, startY, offsetX, offsetY;
        var isDragging = false;

        imgEl.addEventListener('mousedown', function (e) {
            // Don't drag if clicking on handles or delete button
            if (e.target.closest('.pbdnd-handles')) return;
            if (e.target.closest('.pbdnd-delete')) return;

            isDragging = true;
            PBDND.activeImage = imgEl;

            var rect = imgEl.getBoundingClientRect();
            var canvasRect = imgEl.parentElement.getBoundingClientRect();

            startX = e.clientX;
            startY = e.clientY;
            offsetX = rect.left - canvasRect.left;
            offsetY = rect.top - canvasRect.top;

            imgEl.classList.add('repositioning');
            imgEl.style.cursor = 'grabbing';

            document.addEventListener('mousemove', onMouseMove, true);
            document.addEventListener('mouseup', onMouseUp, true);
        });

        function onMouseMove(e) {
            if (!isDragging) return;
            
            var deltaX = e.clientX - startX;
            var deltaY = e.clientY - startY;

            var newX = snapToGrid(offsetX + deltaX);
            var newY = snapToGrid(offsetY + deltaY);

            imgEl.style.left = newX + 'px';
            imgEl.style.top = newY + 'px';
            
            // Show grid guides
            showGridFeedback(imgEl);
        }

        function onMouseUp() {
            isDragging = false;
            imgEl.classList.remove('repositioning');
            imgEl.style.cursor = 'grab';
            
            document.removeEventListener('mousemove', onMouseMove, true);
            document.removeEventListener('mouseup', onMouseUp, true);
            
            hideGridFeedback();
            updatePageManifest();
        }
    }

    /* ========== Resize handling (drag bottom-right corner) =============== */
    function makeImageResizable(imgEl) {
        var resizeHandle = imgEl.querySelector('.pbdnd-handle.resize');
        if (!resizeHandle) return;

        var startX, startY, startW, startH;
        var isDragging = false;

        resizeHandle.addEventListener('mousedown', function (e) {
            e.preventDefault();
            isDragging = true;

            startX = e.clientX;
            startY = e.clientY;
            startW = imgEl.offsetWidth;
            startH = imgEl.offsetHeight;

            imgEl.classList.add('resizing');
            document.addEventListener('mousemove', onMouseMove, true);
            document.addEventListener('mouseup', onMouseUp, true);
        });

        function onMouseMove(e) {
            if (!isDragging) return;

            var deltaX = e.clientX - startX;
            var deltaY = e.clientY - startY;

            var newW = Math.max(40, snapToGrid(startW + deltaX));
            var newH = Math.max(40, snapToGrid(startH + deltaY));

            imgEl.style.width = newW + 'px';
            imgEl.style.height = newH + 'px';
        }

        function onMouseUp() {
            isDragging = false;
            imgEl.classList.remove('resizing');
            
            document.removeEventListener('mousemove', onMouseMove, true);
            document.removeEventListener('mouseup', onMouseUp, true);
            
            updatePageManifest();
        }
    }

    /* ========== Grid visual feedback ===================================== */
    function showGridFeedback(el) {
        var canvas = el.closest('[data-dnd-canvas]');
        if (!canvas) return;

        var existing = canvas.querySelector('.pbdnd-grid-guides');
        if (!existing) {
            var guides = document.createElement('div');
            guides.className = 'pbdnd-grid-guides';
            canvas.appendChild(guides);
        }
    }

    function hideGridFeedback() {
        var guides = document.querySelector('.pbdnd-grid-guides');
        if (guides) {
            guides.remove();
        }
    }

    /* ========== Initialize on DOM ready ================================== */
    function init() {
        var canvas = el('pbc-canvas-wrap');
        if (!canvas) return;

        // Make canvas accept drops
        canvas.addEventListener('dragover', onCanvasDragOver);
        canvas.addEventListener('dragleave', onCanvasDragLeave);
        canvas.addEventListener('drop', onCanvasDrop);
        canvas.setAttribute('data-dnd-canvas', 'true');

        // Bind to existing images on page
        qsa('[data-dnd-image]').forEach(function (imgEl) {
            imgEl.addEventListener('dragstart', onImageDragStart);
            imgEl.addEventListener('dragend', onImageDragEnd);
            makeImageRepositionable(imgEl);
            makeImageResizable(imgEl);
        });
    }

    /* ========== Manifest update (persist to page.json) ==================== */
    function updatePageManifest() {
        if (!window.PBC) return;

        var canvas = el('pbc-canvas-wrap');
        if (!canvas) return;

        var images = Array.from(canvas.querySelectorAll('[data-dnd-image]')).map(function (el) {
            var src = el.querySelector('img').src;
            var alt = el.querySelector('img').alt;
            return {
                src: src,
                alt: alt,
                x: parseInt(el.style.left || 0),
                y: parseInt(el.style.top || 0),
                w: el.offsetWidth,
                h: el.offsetHeight
            };
        });

        // Store in PBC.manifest.images for serialization
        if (!PBC.manifest.images) {
            PBC.manifest.images = [];
        }
        PBC.manifest.images = images;
    }

    /* ========== Public API =============================================== */
    PBDND.init = init;
    PBDND.snapToGrid = snapToGrid;
    PBDND.setGridSize = function (size) {
        PBDND.gridSize = size;
    };
    PBDND.toggleSnap = function () {
        PBDND.snapEnabled = !PBDND.snapEnabled;
        return PBDND.snapEnabled;
    };

    // Auto-init if DOM is ready, else wait
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    } else {
        init();
    }

})();
