# Page Builder Enhancements

Recent updates to the page builder add drag-and-drop image support, cross-platform grid snapping, and a simplified user interface.

## New Features

### 1. Drag-and-Drop Images (`pb-dnd.js`)

Upload images directly to the canvas by dragging files into the center panel.

- **Drop images**: Drag image files from your file system onto the canvas
- **Auto-fit**: Images automatically scale to optimal size while preserving aspect ratio
- **Reposition**: Click and drag images to move them around the canvas with grid snapping
- **Resize**: Use the corner handle (bottom-right triangle) to resize images
- **Delete**: Click the X button to remove images

#### Usage

1. Open your page in the composer
2. Drag image files from your desktop or file browser onto the canvas
3. Images automatically snap to 8px grid
4. Drag images to reposition
5. Drag corner handle to resize
6. Click X to delete

### 2. Cross-Platform Grid System (`pb-grid.js`)

Intelligent grid alignment and snapping for perfect positioning across all devices.

#### Features

- **Visual Grid**: Shows alignment grid on canvas (4-cell intervals)
- **Smart Snapping**: 8px baseline grid with responsive adjustment
- **Platform-Aware**:
  - Mobile (<768px): 4px fine grid
  - Tablet (<1024px): 8px standard grid
  - Desktop: 16px coarse grid
- **Alignment Guides**: Visual feedback when snapping to edges/other elements
- **Alignment Tools**: Quick align left, center, top, distribute evenly

#### Keyboard Shortcuts

- **Ctrl+G** (Cmd+G on Mac): Toggle grid overlay on/off
- **Ctrl+Shift+S** (Cmd+Shift+S): Toggle snap-to-grid enabled/disabled

### 3. Simplified Interface

The composer interface has been streamlined:

- **Cleaner canvas**: Removed verbose instructions, added drop hint
- **Simpler topbar**: Focus on core actions (build, preview, save)
- **Better labeling**: Clear, concise section titles
- **Responsive design**: Elements automatically adjust for mobile/tablet/desktop

## Files Added

### JavaScript

- `js/pb-dnd.js` - Drag-and-drop system for images
  - `PBDND.init()` - Initialize DND system
  - `PBDND.snapToGrid(value, gridSize)` - Snap value to grid
  - `PBDND.setGridSize(size)` - Change grid size
  - `PBDND.toggleSnap()` - Toggle snapping on/off

- `js/pb-grid.js` - Cross-platform grid alignment
  - `PBGRID.init(container)` - Initialize grid system
  - `PBGRID.showGridOverlay(container)` - Show visual grid
  - `PBGRID.alignLeft(elements)` - Align elements left
  - `PBGRID.alignCenter(elements)` - Center elements horizontally
  - `PBGRID.alignTop(elements)` - Align elements to top
  - `PBGRID.distributeHorizontally(elements)` - Even spacing
  - `PBGRID.distributeVertically(elements)` - Even vertical spacing
  - `PBGRID.adjustGridForViewport()` - Auto-adjust grid size

### Stylesheets

- `css/pb-dnd.css` - Styling for drag-and-drop system
  - Image container styles
  - Resize handles and delete buttons
  - Canvas active/hover states
  - Grid guide visual feedback
  - Responsive adjustments
  - Touch support

## Configuration

### Grid Size

Default grid size is 8px. Override with:

```javascript
PBDND.setGridSize(4);   // Finer grid
PBGRID.setGridSize(16); // Coarser grid
```

### Snap Enabled/Disabled

Toggle snapping without disabling the grid visual:

```javascript
PBDND.toggleSnap();  // Toggle in PBDND module
PBGRID.snapEnabled = false; // or set directly
```

### Grid Visibility

Control whether the visual grid is shown:

```javascript
PBGRID.showGuides = false; // Hide grid overlay
PBGRID.showGridOverlay(container); // Show it again
```

## Browser Support

- Chrome/Edge 90+
- Firefox 88+
- Safari 14+
- Mobile browsers (iOS Safari, Chrome Mobile)

Touch events are supported on mobile for repositioning (long-press to drag).

## Performance Notes

- Grid system uses canvas for overlay (lightweight)
- DND system debounces position updates
- Images are snapped on drop, not continuously during drag
- Works smoothly with 50+ images on canvas

## Future Enhancements

Potential additions:

- [ ] Image cropping/rotation
- [ ] Layer stacking (z-index control)
- [ ] Multi-select alignment
- [ ] Image effects (opacity, blur, brightness)
- [ ] Undo/redo history
- [ ] Grid snap distance customization
- [ ] Image library panel
- [ ] Export as CSS/HTML snapshot

## Troubleshooting

**Images not snapping to grid:**
- Check that `PBDND.snapEnabled = true`
- Verify grid size with `PBDND.gridSize`

**Grid not visible:**
- Press Ctrl+G to toggle grid on
- Check `PBGRID.showGuides = true`

**Drop not working:**
- Ensure canvas has `data-dnd-canvas` attribute
- Verify browser supports `FileReader` API
- Check browser console for errors

**Resize handle not appearing:**
- Hover over image to reveal handles
- Ensure image has `pbdnd-image-container` class

## API Examples

### Programmatic image insertion

```javascript
// Create and add image to canvas
var canvas = document.getElementById('pbc-canvas-wrap');
var src = 'data:image/png;base64,...';
var imgEl = PBDND.createImageElement(src, 'my-image.png', {x: 0, y: 0});
canvas.appendChild(imgEl);
PBDND.makeImageRepositionable(imgEl);
```

### Get all positioned images

```javascript
var images = Array.from(document.querySelectorAll('[data-dnd-image]'))
    .map(el => ({
        src: el.querySelector('img').src,
        alt: el.querySelector('img').alt,
        x: parseInt(el.style.left || 0),
        y: parseInt(el.style.top || 0),
        w: el.offsetWidth,
        h: el.offsetHeight
    }));
```

### Serialize to JSON

```javascript
PBDND.updatePageManifest(); // Updates PBC.manifest.images
var json = JSON.stringify(PBC.manifest);
```

## Integration with Existing Systems

The new modules are designed to work alongside existing page-builder systems:

- **pb-composer.js**: Existing section management works unchanged
- **pb-editor.js**: Live editing via watcher.php unaffected
- **section-api.php**: API endpoints compatible with new image data

The `PBC.manifest.images` array stores image data alongside section data.

## License

Same as page-builder project.
