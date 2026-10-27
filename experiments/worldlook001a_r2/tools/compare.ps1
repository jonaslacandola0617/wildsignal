. "$PSScriptRoot/common.ps1"
foreach($res in @(480,640)){
 foreach($model in @('off','B0','M1','M2')){
  if($model -eq 'B0' -and !(Test-Path $Legacy)){Write-Warning 'Legacy PRIVATE atlas missing: B0 NOT RUN';continue}
  foreach($pose in @(0,1,2,3)){
   $flags=@('--r2-model',$model,'--r2-ground','original','--r2-placement','uniform','--resolution',"$res",'--pose',"$pose",'--frames','3','--warmup','0','--cap','0')
   if($model -eq 'B0'){$flags+=@('--grass-kit',$Legacy)}
   Invoke-Plot "shape-$model-$res-pose$pose" $flags
  }
 }
 foreach($ground in @('original','patches')){foreach($place in @('uniform','clustered')){foreach($model in @('M1','M2')){
  Invoke-Plot "independent-$model-$ground-$place-$res" @('--r2-model',$model,'--r2-ground',$ground,'--r2-placement',$place,'--resolution',"$res",'--frames','3','--warmup','0','--cap','0')
 }}}
}
