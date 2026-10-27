. "$PSScriptRoot/common.ps1"
# Identical scene/pose/count comparisons; no captures or motion encoding in timing.
foreach($res in @(480,640)){
 foreach($model in @('off','B0','M1','M2')){
  if($model -eq 'B0' -and !(Test-Path $Legacy)){Write-Warning 'Legacy private kit missing: B0 NOT RUN';continue}
  foreach($count in @(8,16,24)){
   if($model -eq 'off' -and $count -ne 24){continue}
   $flags=@('--r2-model',$model,'--r2-count',"$count",'--r2-ground','patches','--r2-placement','clustered','--resolution',"$res",'--frames','600','--warmup','120','--cap','0','--pacing','precise')
   if($model -eq 'B0'){$flags+=@('--grass-kit',$Legacy)}
   Invoke-Plot "cost-$model-$count-$res" $flags
  }
 }
 foreach($model in @('M1','M2')){foreach($cap in @(30,60)){
  Invoke-Plot "pacing-$model-$res-$cap" @('--r2-model',$model,'--r2-ground','patches','--r2-placement','clustered','--resolution',"$res",'--frames','600','--warmup','120','--cap',"$cap",'--pacing','precise')
 }}
 # Independent ground and layout costs, shadow casting control; bounded24 only.
 foreach($control in @('ground','placement','cast','frustum')){
  $flags=@('--r2-model','M1','--r2-ground','patches','--r2-placement','clustered','--resolution',"$res",'--frames','600','--warmup','120','--cap','0')
  if($control -eq 'ground'){$flags+=@('--r2-ground','original')}
  if($control -eq 'placement'){$flags+=@('--r2-placement','uniform')}
  if($control -eq 'cast'){$flags+=@('--r2-cast','off')}
  if($control -eq 'frustum'){$flags+=@('--r2-frustum','off')}
  Invoke-Plot "control-$control-$res" $flags
 }
}
Get-ChildItem (Join-Path $Root 'assets') -Recurse -File | Get-FileHash -Algorithm SHA256 | Export-Csv (Join-Path $Root 'runs/ASSET_HASHES.csv') -NoTypeInformation
Write-Host 'Finished. These are application/swap API/query/working-set measurements, NOT verified display scanout. Return runs privately.'
