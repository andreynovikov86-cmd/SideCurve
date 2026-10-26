-- One-time installer. SideCurve itself is a standalone JSFX, with no Lua runtime.
local _, script = reaper.get_action_context()
local package_dir = script:match('^(.*)[/\\]')
local source_path = package_dir .. '/Effects/SideKick/SideKick.jsfx'
local input, err = io.open(source_path, 'rb')
if not input then reaper.MB('Cannot read SideKick.jsfx:\n' .. tostring(err), 'SideCurve installation', 0); return end
local source = input:read('*a'); input:close()
local resource = reaper.GetResourcePath():gsub('\\', '/')
local quoted = resource:gsub('\\', '\\\\'):gsub('"', '\\"')
local count
source, count = source:gsub('#resource_root = "";', function() return '#resource_root = "' .. quoted .. '";' end, 1)
if count ~= 1 then reaper.MB('The package is incomplete or has already been configured.', 'SideCurve installation', 0); return end
local destination = resource .. '/Effects/SideKick'
reaper.RecursiveCreateDirectory(destination, 0)
local output
output, err = io.open(destination .. '/SideKick.jsfx', 'wb')
if not output then reaper.MB('Cannot install SideCurve:\n' .. tostring(err), 'SideCurve installation', 0); return end
output:write(source); output:close()
reaper.MB('Installed SideCurve (Sidechain Shaper).\n\nAdd it from the track FX browser. Press F5 in the browser if its listing needs a refresh.\n\nRESET restores the preset saved with “Save preset as default…”. This installer does not need to remain running.', 'SideCurve installation', 0)
