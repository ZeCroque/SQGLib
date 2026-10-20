param(
	[string]$Mo2Path,
	[string]$Shortcut,
	[string]$Profile,
	[string]$ProcessName = "SkyrimSE",
	[string]$InjectedModule = "skse64_*.dll",
	[int]$TimeoutSeconds = 180,
	[int]$WatchProcessId,
	[int]$VisualStudioProcessId,
	[int]$ScriptProcessId
)

# Launches the game through MO2, then attaches the calling Visual Studio instance to it once SKSE has been injected.
# Attaching earlier (e.g. with the Child Process Debugging Power Tool) makes the SKSE loader fail with "Couldn't inject DLL".
# A detached copy of this script (-WatchProcessId) then kills the game when the debugging session ends with "Stop Debugging"
# (Visual Studio terminates this script) but leaves it running on "Detach All" (this script keeps running).

Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;

public static class RunningObjectTable
{
	[DllImport("ole32.dll")] static extern int GetRunningObjectTable(int reserved, out IRunningObjectTable rot);
	[DllImport("ole32.dll")] static extern int CreateBindCtx(int reserved, out IBindCtx ctx);

	public static object FindVisualStudio(int processId)
	{
		IRunningObjectTable rot;
		IBindCtx ctx;
		GetRunningObjectTable(0, out rot);
		CreateBindCtx(0, out ctx);
		IEnumMoniker monikers;
		rot.EnumRunning(out monikers);
		IMoniker[] current = new IMoniker[1];
		while (monikers.Next(1, current, IntPtr.Zero) == 0)
		{
			string name;
			current[0].GetDisplayName(ctx, null, out name);
			if (name.StartsWith("!VisualStudio.DTE.") && name.EndsWith(":" + processId))
			{
				object instance;
				rot.GetObject(current[0], out instance);
				return instance;
			}
		}
		return null;
	}
}
"@

function Get-VisualStudioProcessId {
	$current = $PID
	while ($current) {
		$process = Get-CimInstance Win32_Process -Filter "ProcessId=$current"
		if (-not $process) { return $null }
		if ($process.Name -eq "devenv.exe") { return $process.ProcessId }
		$current = $process.ParentProcessId
	}
}

function Invoke-WithRetry([scriptblock]$Action) {
	for ($i = 0; $i -lt 20; $i++) {
		try { return & $Action } catch { Start-Sleep -Milliseconds 500 }
	}
	& $Action
}

$dbgDesignMode = 1

if ($WatchProcessId) {
	$dte = [RunningObjectTable]::FindVisualStudio($VisualStudioProcessId)
	while ((Get-Process -Id $WatchProcessId -ErrorAction SilentlyContinue) -and (Get-Process -Id $VisualStudioProcessId -ErrorAction SilentlyContinue)) {
		try {
			if ($dte.Debugger.CurrentMode -eq $dbgDesignMode) { break }
		} catch { }
		Start-Sleep -Milliseconds 500
	}
	# Visual Studio terminates the launched script on "Stop Debugging" but not on "Detach All"
	$scriptProcess = Get-Process -Id $ScriptProcessId -ErrorAction SilentlyContinue
	if ($scriptProcess -and (Get-Process -Id $VisualStudioProcessId -ErrorAction SilentlyContinue)) {
		[void]$scriptProcess.WaitForExit(5000)
		if (-not $scriptProcess.HasExited) { exit }
	}
	Stop-Process -Id $WatchProcessId -Force -ErrorAction SilentlyContinue
	exit
}

if (-not $Mo2Path -or -not $Shortcut) { throw "-Mo2Path and -Shortcut are required" }

$vsPid = Get-VisualStudioProcessId
if (-not $vsPid) { throw "Could not find the Visual Studio instance that launched this script" }
$dte = [RunningObjectTable]::FindVisualStudio($vsPid)
if (-not $dte) { throw "Could not get the DTE object of Visual Studio (pid $vsPid)" }

if (Get-Process -Name $ProcessName -ErrorAction SilentlyContinue) { throw "$ProcessName is already running" }

$arguments = @()
if ($Profile) { $arguments += @("-p", "`"$Profile`"") }
$arguments += "`"$Shortcut`""
Start-Process -FilePath $Mo2Path -ArgumentList $arguments

$deadline = (Get-Date).AddSeconds($TimeoutSeconds)
$game = $null
while ((Get-Date) -lt $deadline) {
	$game = Get-Process -Name $ProcessName -ErrorAction SilentlyContinue | Select-Object -First 1
	if ($game) {
		$injected = $false
		try { $injected = [bool]($game.Modules | Where-Object { $_.ModuleName -like $InjectedModule }) } catch { }
		if ($injected) { break }
		$game = $null
	}
	Start-Sleep -Milliseconds 100
}
if (-not $game) { throw "Timed out waiting for $ProcessName with $InjectedModule loaded" }

Invoke-WithRetry {
	$target = $dte.Debugger.LocalProcesses | Where-Object { $_.ProcessID -eq $game.Id } | Select-Object -First 1
	if (-not $target) { throw "$ProcessName (pid $($game.Id)) is not listed by Visual Studio yet" }
	$target.Attach()
}
Write-Host "Attached to $ProcessName (pid $($game.Id))"

# Created through WMI so the watcher is not a child of this script, which Visual Studio kills when debugging stops
$watcherCommand = "powershell.exe -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File `"$PSCommandPath`" -WatchProcessId $($game.Id) -VisualStudioProcessId $vsPid -ScriptProcessId $PID"
[void](Invoke-CimMethod -ClassName Win32_Process -MethodName Create -Arguments @{ CommandLine = $watcherCommand })

Wait-Process -Id $game.Id -ErrorAction SilentlyContinue
