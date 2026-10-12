// Previa local com dados ficticios; nao faz parte do firmware.
const fs = require('fs');
const vm = require('vm');
const http = require('http');
const source = fs.readFileSync(__dirname + '/WebPage.h', 'utf8');
const html = source.split('R"rawliteral(')[1].split(')rawliteral"')[0];
new vm.Script(html.split('<script>')[1].split('</script>')[0]);
const data = {hostname:'ESP32_CENTRAL',firmware:'261010.10.30',modo_rede:'Wi-Fi conectado',rede_ssid:'Rede de casa',ip_local:'192.168.0.110',sinal_rssi:'-61 dBm',rssi_dbm:-61,wifi_conectado:true,ram_livre:'148 KB',temp_cpu:'43.2 C',cpu_mhz:'240 MHz',uptime_segundos:93841,flash_livre:'1.8 MB',motivo_reset:'POWER-ON (Tomada/VCC)',mac:'AA:BB:CC:DD:EE:FF',gateway:'192.168.0.1',dns:'192.168.0.1',build:'Oct 10 2026 10:30:00',clima:{sincronizado:true,previsao_valida:true,ultima_falhou:false,temperatura:23.4,umidade:71,minima:18.2,maxima:26.8,chuva:35,idade_s:90,data:'2026-10-10',condicao:'Parc. nublado'}};
for(let i=0;i<5;i++)data['_form_ssid'+i]=i===0?'Rede de casa':'';
for(let i=1;i<=4;i++){data['_form_ip'+i]='192.168.0.'+(115+i*5);data['esp'+i+'_online']=i<=2}
http.createServer((req,res)=>{
  if(req.url==='/api/info'){res.setHeader('Content-Type','application/json');res.end(JSON.stringify(data))}
  else if(req.url==='/salvar'){res.statusCode=400;res.end('Previa: configuracoes nao sao salvas.')}
  else{res.setHeader('Content-Type','text/html; charset=utf-8');res.end(html)}
}).listen(8765,'127.0.0.1',()=>console.log('Previa: http://127.0.0.1:8765 / JavaScript valido'));
