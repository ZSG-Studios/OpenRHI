param([Parameter(Mandatory = $true)][string[]]$Names)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName Microsoft.VisualBasic
$repositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
foreach ($name in ($Names -split ',')) {
    if ($name -notin @('build', '_Bin', '_Build', '_Shaders', '_Tmp', '_OpenRHI_SDK')) {
        throw "Unsupported generated directory: $name"
    }
    $targetPath = [IO.Path]::GetFullPath((Join-Path $repositoryRoot $name))
    if ([IO.Path]::GetDirectoryName($targetPath) -ne $repositoryRoot) {
        throw "Target is outside the repository: $targetPath"
    }
    if (Test-Path -LiteralPath $targetPath) {
        Write-Host "Recycling $targetPath"
        [Microsoft.VisualBasic.FileIO.FileSystem]::DeleteDirectory($targetPath,
            [Microsoft.VisualBasic.FileIO.UIOption]::OnlyErrorDialogs,
            [Microsoft.VisualBasic.FileIO.RecycleOption]::SendToRecycleBin,
            [Microsoft.VisualBasic.FileIO.UICancelOption]::ThrowException)
        if (Test-Path -LiteralPath $targetPath) { throw "Directory remains: $targetPath" }
    }
}
