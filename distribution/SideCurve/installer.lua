-- @noindex
-- ReaPack owns the files beside this module. Only an explicit installer action
-- writes to the VST3/Effects folder. ReaPack synchronization never executes code.
local M = {}
M.version = '1.0.0'
M.assets = {
  mac = { file = 'SideCurve-0.5.5-macOS-universal.zip', version = '0.5.5',
    sha256 = '68fdb40efa12d1654959b24f68d117fa091d484640935385b5456cb6daaab1a6',
    binary = 'Contents/MacOS/SideCurve' },
  win = { file = 'SideCurve-0.5.5-Windows-x64.zip', version = '0.5.5',
    sha256 = '803d6644cbfefba9c39397d1d59c147d14db334f30490b204c1e6922120fa3be',
    binary = 'Contents/x86_64-win/SideCurve.vst3' },
}
local function fail(message) error(message, 0) end
local function read(path)
  local f, err = io.open(path, 'rb')
  if not f then fail('Cannot read ' .. path .. '\n' .. tostring(err)) end
  local data = f:read('*a'); f:close(); return data
end
local function exists(path)
  local ok, _, code = os.rename(path, path)
  return ok or code == 13
end
local function mkdir(path)
  reaper.RecursiveCreateDirectory(path, 0)
  if not exists(path) then fail('Cannot create folder:\n' .. path) end
end
local function execute(arguments, windows)
  local words = {}
  for _, arg in ipairs(arguments) do
    if arg:find('[\r\n\0]') then fail('Invalid path in installer command.') end
    if windows then
      -- CreateProcess/C-runtime quoting, not cmd.exe or PowerShell evaluation.
      local quoted = arg:gsub('(\\*)"', function(s) return s .. s .. '\\"' end)
      quoted = quoted:gsub('(\\+)$', '%1%1')
      words[#words + 1] = '"' .. quoted .. '"'
    else
      words[#words + 1] = "'" .. arg:gsub("'", "'\\''") .. "'"
    end
  end
  local result = reaper.ExecProcess(table.concat(words, ' '), 60000)
  local code, output = (result or ''):match('^([%-]?%d+)[\r\n]+(.*)$')
  if tonumber(code) ~= 0 then
    fail('Installer command failed.\n' .. (output or result or 'No process result.'))
  end
  return output
end
local function hash(path, windows)
  local output = execute(windows and { 'certutil.exe', '-hashfile', path, 'SHA256' }
    or { '/usr/bin/shasum', '-a', '256', path }, windows)
  for line in output:gmatch('[^\r\n]+') do
    local compact = line:gsub('%s', '')
    local candidate = windows and compact or line:match('^(%x+)')
    if candidate and #candidate == 64 and candidate:match('^%x+$') then
      return candidate:lower()
    end
  end
  fail('Could not read the archive checksum.')
end
local function remove_tree(path, windows)
  -- Only called for a fresh installer-owned staging/backup path. Never follows
  -- the destination or scans other installed plugins.
  if os.remove(path) then return true end -- file or an already empty directory
  while true do
    local f = reaper.EnumerateFiles(path, 0)
    if not f then break end
    if not os.remove(path .. '/' .. f) then return false end
  end
  local folders, n = {}, 0
  while true do
    local d = reaper.EnumerateSubdirectories(path, n)
    if not d then break end
    folders[#folders + 1] = d; n = n + 1
  end
  for _, d in ipairs(folders) do
    if not remove_tree(path .. '/' .. d, windows) then return false end
  end
  if os.remove(path) then return true end
  if windows then
    -- Windows' C runtime cannot remove directories. Delete this empty directory
    -- through .NET; no recursive delete or execution-policy override is used.
    local command = "[System.IO.Directory]::Delete('" .. path:gsub("'", "''") .. "')"
    local ok = pcall(execute, {'powershell.exe', '-NoProfile', '-NonInteractive', '-Command', command}, true)
    return ok and not exists(path)
  end
  return false
end
local function token()
  return (reaper.genGuid and reaper.genGuid() or tostring(reaper.time_precise())):gsub('[^%w]', '')
end
local function replace(staged, destination, backup, windows)
  if exists(backup) then fail('Installer backup already exists:\n' .. backup) end
  local old = exists(destination)
  if old then
    local ok, err = os.rename(destination, backup)
    if not ok then
      fail('SideCurve is in use or its folder is not writable. Restart REAPER with an empty project and run this action again.\n' .. tostring(err))
    end
  end
  local ok, err = os.rename(staged, destination)
  if not ok then
    if old then
      local restored, restore_error = os.rename(backup, destination)
      if not restored then
        fail('Installation failed. The previous version is preserved at:\n' .. backup .. '\nRestore it to:\n' .. destination .. '\n' .. tostring(restore_error))
      end
    end
    fail('Installation failed; the previous version was retained.\n' .. tostring(err))
  end
  if old then remove_tree(backup, windows) end
end
function M.install_jsfx(directory, resource)
  local source = read(directory .. '/files/SideCurve.jsfx')
  local escaped = resource:gsub('\\', '/'):gsub('\\', '\\\\'):gsub('"', '\\"')
  local count
  source, count = source:gsub('#resource_root = "";', function()
    return '#resource_root = "' .. escaped .. '";'
  end, 1)
  if count ~= 1 or not source:find('desc:SideCurve', 1, true) then
    fail('The JSFX package is incomplete. Synchronize ReaPack and reinstall the package.')
  end
  local parent = resource .. '/Effects/SideKick'
  mkdir(parent)
  local id = token()
  local staged = parent .. '/.SideCurve-' .. id .. '.tmp'
  local f, err = io.open(staged, 'wb')
  if not f then fail('Cannot write SideCurve:\n' .. tostring(err)) end
  local ok, write_error = f:write(source)
  local closed, close_error = f:close()
  if not ok or not closed then
    os.remove(staged); fail('Cannot save SideCurve:\n' .. tostring(write_error or close_error))
  end
  local destination = parent .. '/SideKick.jsfx'
  local success, reason = pcall(replace, staged, destination, parent .. '/.SideCurve-' .. id .. '.previous')
  if not success then os.remove(staged); fail(reason) end
  return destination
end
function M.install_vst3(directory, os_name, home, local_app_data)
  local windows = os_name == 'Win64'
  local mac = os_name == 'OSX64' or os_name == 'macOS-arm64'
  if not windows and not mac then fail('The VST3 package supports macOS and Windows x64. Use JSFX on other REAPER platforms.') end
  local root = windows and local_app_data or home
  if not root or root == '' then fail('Cannot locate the current user installation folder.') end
  local asset = M.assets[windows and 'win' or 'mac']
  local archive = directory .. '/files/' .. asset.file
  if not reaper.file_exists(archive) then fail('The VST3 archive is missing. Synchronize ReaPack and reinstall the package.') end
  if hash(archive, windows) ~= asset.sha256 then fail('Archive checksum mismatch. Nothing was installed. Synchronize ReaPack and reinstall the package.') end
  local parent = root .. (windows and '/Programs/Common/VST3' or '/Library/Audio/Plug-Ins/VST3')
  if exists(parent .. '/SideKick.vst3') then
    fail('An early SideKick.vst3 preview is still installed. Save your projects, move that old bundle out of the VST3 folder, then run this action again. Your existing projects use the retained plugin IDs.')
  end
  mkdir(parent)
  local stage = parent .. '/.SideCurve-install-' .. token()
  if exists(stage) then fail('Installer staging folder already exists.') end
  mkdir(stage)
  local destination = parent .. '/SideCurve.vst3'
  local ok, reason = pcall(function()
    execute(windows and { 'tar.exe', '-xf', archive, '-C', stage }
      or { '/usr/bin/ditto', '-x', '-k', archive, stage }, windows)
    local staged = stage .. '/SideCurve.vst3'
    if not reaper.file_exists(staged .. '/' .. asset.binary) then fail('The archive does not contain the expected SideCurve bundle.') end
    -- A same-filesystem rename swaps complete bundles; the installed plugin is
    -- untouched until archive verification and extraction have succeeded.
    replace(staged, destination, stage .. '/previous', windows)
  end)
  -- If rollback failed, the only previous copy is still inside this folder.
  -- Leave it intact and report its exact recovery path.
  if not exists(stage .. '/previous') then remove_tree(stage, windows) end
  if not ok then fail(reason) end
  return destination
end
function M.run(format, directory)
  local ok, result = pcall(function()
    if format == 'jsfx' then return M.install_jsfx(directory, reaper.GetResourcePath()) end
    return M.install_vst3(directory, reaper.GetOS(), os.getenv('HOME'), os.getenv('LOCALAPPDATA'))
  end)
  if not ok then reaper.MB(tostring(result), 'SideCurve installation', 0); return end
  reaper.MB('SideCurve installed or updated.\n\n' .. result .. '\n\nSave your projects and restart REAPER. Refresh the FX list if needed.\n\nFor future updates: ReaPack > Synchronize packages, then run this same action again.', 'SideCurve installation', 0)
end
return M
