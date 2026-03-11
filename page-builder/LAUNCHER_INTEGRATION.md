# Page Builder Launcher Integration

The Live CSS Editor launcher (Live CSS Editor.command) now includes a direct option to launch the page builder staging tool.

## How to Use

1. Double-click `Live CSS Editor.command` from the Finder
2. Select option `5` from the main menu: "Page builder"
3. The page builder will open in a full browser window
4. Build and stage your page completely before deployment

## What It Does

- Starts a PHP development server on `localhost:8844`
- Serves the page-builder directory as the root
- Opens the page builder in a full browser window (imgui-browser if available, otherwise system browser)
- Provides a complete staging environment that mimics a live server
- Logs errors to `.page-builder-server.log` if needed

## Features

### Browser

The page builder opens with full browser UI including:
- Address bar showing `http://localhost:8844`
- Full toolbar for navigation
- Developer tools access
- Responsive design preview

### Pages Directory

Default pages are located in:
```
page-builder/pages/
├── landing/
│   ├── page.json      (page manifest)
│   ├── overrides.json (style overrides)
│   └── index.html     (built output)
└── work/
    ├── page.json
    ├── overrides.json
    └── index.html
```

### Available Pages

Access pages at:
- Landing: `http://localhost:8844/pages/landing/index.html`
- Work: `http://localhost:8844/pages/work/index.html`
- Composer (builder): `http://localhost:8844/composer.php?page=landing`

## Workflow: Build to Production

### Stage 1: Local Development
```
Options 1-2: Edit your project locally
```

### Stage 2: Page Builder Staging (Option 5)
```
1. Select "5" from main menu
2. Opens page builder at http://localhost:8844
3. Build, compose, and test pages
4. Verify all sections, images, styling
5. Test on multiple devices/viewports
6. Check all links and assets load correctly
```

### Stage 3: Live Server Push
```
Option p: Push repos to production
- Commits local changes
- Pushes to GitHub/staging server
- Ready for live deployment
```

## Technical Details

### Server Configuration

- **Port**: 8844 (page-builder specific)
- **Root**: `/Users/mac/Documents/live-css/page-builder/`
- **Working Directory**: Same as launcher root
- **Log File**: `.page-builder-server.log`

### Server Detection

The launcher includes:
- Automatic port availability check
- Reuse existing server if already running on port 8844
- Graceful fallback to system browser if imgui-browser unavailable
- Error logging for troubleshooting

### Browser Options

If imgui-browser is available:
- Launches with `--ui-mode full` for full toolbar
- Includes address bar, navigation, dev tools

Fallback:
- Uses system default browser (Safari/Chrome)
- Manual URL entry if needed

## Customization

### Change Port

Edit `Live CSS Editor.command`:
```bash
# Around line 98
local port=8844  # Change this number
```

### Change Browser UI Mode

Edit the _open_page_builder function:
```bash
# Around line 127
--ui-mode full     # Options: full, grab_bar_only, minimal
```

### Disable Browser Auto-Open

Edit the _open_page_builder function:
```bash
# Comment out or remove the imgui-browser or open commands
# bash "$IMGUI_RUN" --url "$URL" --ui-mode full ...
```

## Troubleshooting

### Port Already in Use

If port 8844 is already in use:
1. Kill existing process: `lsof -i :8844 | tail -1 | awk '{print $2}' | xargs kill`
2. Try again from menu option 5

### PHP Server Won't Start

Check logs:
```bash
tail -f .page-builder-server.log
```

Common issues:
- PHP not installed: `brew install php`
- Port firewall rules: Check System Preferences > Security & Privacy
- Permission denied: Run with proper permissions

### Browser Won't Open

If no browser launches:
1. Check imgui-browser availability: `ls -la imgui-browser/run.sh`
2. Check system browser: `which open`
3. Manually visit: `http://localhost:8844`

### Changes Not Reflected

Clear browser cache:
- Hard refresh: Cmd+Shift+R (Chrome) or Cmd+Shift+Delete (Safari)
- Clear page builder cache in composer
- Check PHP error logs in terminal

## Integration with Other Tools

### With Dev Server (Option 3)

You can run BOTH servers simultaneously:
1. Start dev server: Option 3 (nginx/PHP on :8443)
2. Start page builder: Option 5 (PHP on :8844)
3. Edit projects in grab bar (:8443)
4. Build pages in page builder (:8844)
5. Push changes: Option p

### With Agent Flow (Option a)

Separate agent-flow tool runs on :9090
- Page builder: :8844
- Agent flow: :9090
- Dev server: :8443
- All can run concurrently

## Next Steps

After staging pages in the page builder:

1. **Test Thoroughly**
   - Check all pages load
   - Verify responsive design
   - Test navigation
   - Validate images/assets

2. **Build Pages**
   - Use "build" button in composer
   - Generates index.html files
   - Compiles CSS overrides

3. **Push to GitHub**
   - Option p in launcher
   - Commits page changes
   - Pushes to repository

4. **Deploy to Live**
   - Pull from GitHub on production server
   - Run any build steps on live server
   - Verify page-builder/pages/ synced correctly

## See Also

- [DRAG_DROP_GRID_README.md](DRAG_DROP_GRID_README.md) - Image drag-drop features
- [README.md](README.md) - Page builder overview
- [composer.php](composer.php) - Visual page composer interface
