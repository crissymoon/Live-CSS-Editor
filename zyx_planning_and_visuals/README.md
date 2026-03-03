## PLANNING AND VISUALS FILE, Todo's


#### Model Map / Settings

- [x] Scan with /Users/mac/Documents/live-css/search.py
      Scanned all .py and .php files for hardcoded model strings.
      Confirmed active: claude-haiku-4-5-20251001, gpt-4o-mini, deepseek-chat across 5 directories.

- [x] Put a .JSON for each hardcoded model in the same dir the model is working in (this json needs to call the model-map / settings in the root to get the models)
      c_tools/tui_agent/model-context.json  -- haiku + gpt-4o-mini
      ai/model-context.json                 -- gpt-4o-mini + deepseek-chat + haiku override
      mood_ck/model-context.json            -- EMI local model
      agent-flow/api/model-context.json     -- gpt-4o-mini default
      debug-tool/ai/model-context.json      -- gpt-4o-mini + deepseek fallback
      Each file has a model_map_ref field pointing back to model-map.json at root.

- [x] In the root of live-css - add a model-map (just for the AI APIs outsourced / providers) that has all the locations in a JSON that the models can -
      Created: /Users/mac/Documents/live-css/model-map.json
    **This should include**
    > [x] provider            -- anthropic / openai / deepseek / local
    > [x] model               -- all active + catalog models per provider
    > [x] location (where it is used) -- hardcoded_in arrays with file:line refs
    > [x] use case information (what is it used for and the flow) -- use_cases + flow fields per model
    > [x] token costs         -- cost_per_1k_input + cost_per_1k_output per model
    > [x] etc...              -- tier, context_window_k, max_output_tokens, active_in_project, fallback notes

### Project Reporting Checklist
  **Workflow**
- [x] in this folder - /Users/mac/Documents/live-css/zyx_planning_and_visuals - make a create report py file
      Created: zyx_planning_and_visuals/create_report.py
      Usage: python3 create_report.py [--root /path] [--threshold N] [--open]
    **Uses these files to create a report (same color theme as the other tools)**
    > [x] /Users/mac/Documents/live-css/lines_count.py  -- imports scan() directly
    > [x] /Users/mac/Documents/live-css/security_ck.py  -- imports walk_directory(), scan_file(), severity_of()
    > [x] /Users/mac/Documents/live-css/search.py       -- imports AI_MODELS, WATCH_FILES for model catalog + refs scan
    **Makes a light weight doc**
    > [x] 10/10 Level of severity -- score computed per finding (CRITICAL=10, HIGH=8, MEDIUM=5, LOW=2; large files scaled by line count)
    > [x] Shows items of concern  -- "Items of Concern" pill bar at top of every report
    **The Lightweight HTML/CSS/JS**
    > [x] Should have clickable checkboxes with each file to show completed or not
    > [x] When it is completed it should show timestamp -- saved to localStorage keyed by report ID
    **Make one main file that will auto grab all the reports w/ proper pagination, search, etc..
    > [x] reports/index.html -- auto-regenerated on each run, cards grid, search bar, paginated (12/page)

## Onsite Chatbots

- [x] Add sections for chatbots /Users/mac/Documents/live-css/page-builder/sections
      page-builder/sections/sections/chatbot-widget.json -- section definition (chatbot block type)
      page-builder/sections/chatbot/chatbot-widget.js    -- self-contained widget JS
      page-builder/sections/chatbot/chatbot-widget.css   -- scoped styles, dark + light themes
      page-builder/sections/chatbot/config.json          -- rate-limit, guard, AI settings
      page-builder/sections/chatbot/api/chat.php         -- POST endpoint (validation, rate-limit, guard, AI)
      page-builder/build.php                             -- added case 'chatbot' to renderBlock()

- [x] This should take a flow where companies can add context for their customers /Users/mac/Documents/live-css/agent-flow
      agent-flow/flows/chatbot-company-context.json      -- sample flow with 5 context nodes
      chat.php reads all "context" + "system prompt" nodes from the named flow to build the system prompt
      Companies copy/edit chatbot-company-context.json to set their identity, scope, FAQ, and guardrails

- [x] It should have proper prompt injection security, rate limiting, etc..
      Prompt injection: forwards each message to localhost:8765/classify (prompt_inj_guard server)
        - flagged messages are blocked with HTTP 400 and logged
        - fail_open:true means service degrades gracefully if guard is unreachable
      Rate limiting: file-based per IP (SHA-256 hashed), 15 req / 60s sliding window
        - returns HTTP 429 with retry_after on breach
        - rate-limit data stored in /tmp/pb_chatbot_rl/ (no DB required)
      Input validation: message length cap (4000 chars), history sanitisation, flow_id allowlist (alphanum/dash/underscore)
      All error paths call chatErr() -> writes to error_log + returns structured JSON so the browser always has context

# Notes