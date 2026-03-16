function love.conf(t)
    t.title             = "Code Review  --  Live CSS"
    t.window.width      = 1060
    t.window.height     = 720
    t.window.resizable  = true
    t.window.minwidth   = 700
    t.window.minheight  = 480
    t.window.vsync      = 1
    t.window.highdpi    = false  -- use logical DPI coords; keeps mouse events + rendering consistent
    t.console           = false
end
