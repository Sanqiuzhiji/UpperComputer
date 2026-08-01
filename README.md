# UpperComputer

## Windows portable package

Run the packaging script from PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\package_windows.ps1
```

The script builds the x64 Release configuration, deploys the required Qt and
MSVC runtime files, copies `workspaces`, and creates
`dist\UpperComputer-0.1.0-win64.zip`. The portable build stores its settings in
the `config` directory and user files in `workspaces`, both beside the
executable.

If Windows reports that the Microsoft Visual C++ runtime is missing, run the
included `vc_redist.x64.exe` once and then start `UpperComputer.exe` again.

To package without the current workspace files, add `-ExcludeWorkspaces`.
Use `-QtRoot <path>` when Qt is installed somewhere else.
