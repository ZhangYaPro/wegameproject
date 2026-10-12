param([string]$Mode='visual')
$ErrorActionPreference='Stop'
$projectRoot=Split-Path $PSScriptRoot -Parent
$editor='D:/UE_5.5/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$project=Join-Path $projectRoot 'Ski25D.uproject'
if($Mode -eq 'visual'){
    & "$PSScriptRoot/run_revision_visual_checks.ps1" -Mode scenery
    & $editor $project /Game/Ski25D/Maps/SkiChangbaiMap -game -RenderOffscreen -SkiMenuReview -unattended -ResX=1280 -ResY=720 -windowed -NoVSync -ForceRes "-abslog=$projectRoot/Saved/Logs/RemodelPortrait.log"
    if($LASTEXITCODE -ne 0){throw 'Portrait failed'}
}elseif($Mode -eq 'ride'){
    $chapter=0
    foreach($map in @('SkiRimeMap','SkiBeidahuMap','SkiChangbaiMap')){
        $chapter++
        $started=Get-Date
        & $editor $project "/Game/Ski25D/Maps/$map" -game -RenderOffscreen -SkiRideTest -benchmark -fps=60 -unattended -ResX=1280 -ResY=720 -windowed -NoVSync -ForceRes "-abslog=$projectRoot/Saved/Logs/Ride_$map.log"
        if($LASTEXITCODE -ne 0){throw "Ride test failed $map"}
        $report=Get-Item "$projectRoot/Saved/SkiQA/rideable_$chapter.json"
        if($report.LastWriteTime -lt $started -or !(Get-Content $report.FullName -Raw | ConvertFrom-Json).passed){throw "Obstacle assertions failed $map"}
    }
}elseif($Mode -eq 'physics'){
    & "$PSScriptRoot/run_revision_visual_checks.ps1" -Mode physics
}else{throw 'Unknown mode'}
