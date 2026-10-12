param([string]$Mode='menu')
$ErrorActionPreference='Stop'
$projectRoot=Split-Path $PSScriptRoot -Parent
$editor='D:/UE_5.5/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$project=Join-Path $projectRoot 'Ski25D.uproject'
if($Mode -eq 'menu'){
    $started=Get-Date
    & $editor $project /Game/Ski25D/Maps/SkiRimeMap -game -RenderOffscreen -AudioMixer -SkiExperienceReview -unattended -ResX=1280 -ResY=720 -windowed -NoVSync -ForceRes "-abslog=$projectRoot/Saved/Logs/ExperienceReview.log"
    if($LASTEXITCODE -ne 0){throw 'Experience review failed'}
    $report=Get-Item "$projectRoot/Saved/SkiQA/experience_checks.json"
    if($report.LastWriteTime -lt $started -or !(Get-Content $report.FullName -Raw | ConvertFrom-Json).passed){throw 'Experience assertions failed'}
}elseif($Mode -eq 'finale'){
    $started=Get-Date
    & $editor $project /Game/Ski25D/Maps/SkiRimeMap -game -RenderOffscreen -AudioMixer -SkiFinaleReview -unattended -ResX=1280 -ResY=720 -windowed -NoVSync -ForceRes "-abslog=$projectRoot/Saved/Logs/FinaleReview.log"
    if($LASTEXITCODE -ne 0){throw 'Finale review failed'}
    $report=Get-Item "$projectRoot/Saved/SkiQA/finale_checks.json"
    if($report.LastWriteTime -lt $started -or !(Get-Content $report.FullName -Raw | ConvertFrom-Json).passed){throw 'Finale assertions failed'}
}elseif($Mode -eq 'cabins'){
    & $editor $project /Game/Ski25D/Maps/SkiBeidahuMap -game -RenderOffscreen -SkiPreviewTest -SkiArtPreview -unattended -ResX=1280 -ResY=720 -windowed -NoVSync -ForceRes "-abslog=$projectRoot/Saved/Logs/CabinReview.log"
    if($LASTEXITCODE -ne 0){throw 'Cabin review failed'}
}elseif($Mode -eq 'maps'){
    & $editor $project -run=pythonscript "-script=$projectRoot/Scripts/save_experience_maps.py" -unattended -nullrhi "-abslog=$projectRoot/Saved/Logs/ExperienceMaps.log"
    if($LASTEXITCODE -ne 0){throw 'Map save failed'}
}else{throw 'Unknown mode'}
