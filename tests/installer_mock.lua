-- Filesystem integration fixture. Only the host APIs are mocked: hashes,
-- extraction, file writes and transactional renames execute on real test files.
local root, payload, user, resource, osname, scenario, helper, python, digest = table.unpack(arg)
local win = osname == 'Win64'
local function quote(s)
  if win then return '"' .. s:gsub('"', '\\"') .. '"' end
  return "'" .. s:gsub("'", "'\\''") .. "'"
end
local function helper_call(action, path)
  local command = quote(python) .. ' ' .. quote(helper) .. ' ' .. action .. ' ' .. quote(path)
  local f = assert(io.popen(win and ('"' .. command .. '"') or command, 'r'))
  local result = f:read('*a'); local ok = f:close(); assert(ok, result); return result
end
local function list(action, path, index)
  local values = {}
  for line in helper_call(action, path):gmatch('[^\r\n]+') do values[#values + 1] = line end
  return values[index + 1]
end
reaper = {
  RecursiveCreateDirectory = function(p) helper_call('mkdir', p) end,
  EnumerateFiles = function(p, i) return list('files', p, i) end,
  EnumerateSubdirectories = function(p, i) return list('directories', p, i) end,
  file_exists = function(p) local f=io.open(p,'rb'); if f then f:close();return true end;return false end,
  genGuid = function() return '{SIDECURVE-TEST}' end,
  ExecProcess = function(command)
    if scenario == 'extraction-failure' and (command:find('ditto',1,true) or command:find('tar.exe',1,true)) then
      return '1\nSimulated extraction failure'
    end
    -- Product commands use CreateProcess on Windows. io.popen invokes cmd.exe,
    -- so add its required outer quote pair only in this test adapter.
    local f = assert(io.popen(win and ('"' .. command .. '"') or command, 'r'))
    local output = f:read('*a'); local ok, _, code = f:close()
    return tostring(ok and 0 or code or 1) .. '\n' .. output
  end,
}
local installer = dofile(root .. '/distribution/SideCurve/installer.lua')
local asset = installer.assets[win and 'win' or 'mac']
if digest and digest ~= '' then asset.sha256 = digest end
local destination = user .. (win and '/Programs/Common/VST3' or '/Library/Audio/Plug-Ins/VST3') .. '/SideCurve.vst3'
if scenario == 'publish-failure' or scenario == 'rollback-failure' or scenario == 'in-use' then
  local rename = os.rename
  os.rename = function(from, to)
    if from ~= to then
      if scenario == 'in-use' and from == destination then return nil, 'simulated loaded plugin', 13 end
      if to == destination and (scenario == 'rollback-failure' or not from:match('/previous$')) then
        return nil, 'simulated publication failure', 13
      end
    end
    return rename(from, to)
  end
end
local js = scenario:match('^jsfx')
local ok, result = pcall(function()
  if js then return installer.install_jsfx(payload, resource) end
  return installer.install_vst3(payload, scenario == 'unsupported' and 'Linux' or osname, user, user)
end)
local expected_success = scenario == 'fresh' or scenario == 'update' or scenario == 'jsfx-fresh' or scenario == 'jsfx-update'
assert(ok == expected_success, scenario .. ': ' .. tostring(result))
if not ok then print('EXPECTED FAILURE: ' .. tostring(result)) end
print('PASS ' .. scenario)
