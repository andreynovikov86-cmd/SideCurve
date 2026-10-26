-- @description SideCurve - Install or Update VST3
-- @version 1.0.0
-- @author Andrey Novikov
local _, script = reaper.get_action_context()
local directory = assert(script:match('^(.*)[/\\]'))
dofile(directory .. '/installer.lua').run('vst3', directory)
