const fs = require('fs');
const vm = require('vm');
const assert = require('assert/strict');
const html = fs.readFileSync(__dirname + '/WebPage.h', 'utf8');
assert.match(html, /href="\/prometheus"/);
const gzipSource = fs.readFileSync(__dirname + '/WebPageGzip.h', 'utf8');
const compressed = Buffer.from([...gzipSource.matchAll(/0x([0-9a-f]{2})/g)].map(m => parseInt(m[1], 16)));
assert.equal(require('zlib').gunzipSync(compressed).toString('utf8'), html.split('R"rawliteral(')[1].split(')rawliteral"')[0]);
const script = html.split('<script>')[1].split('</script>')[0];
const elements = new Map();
function element(id='') {
  const classes = new Set();
  const el = {id, value:'', textContent:'', hidden:false, disabled:false, dataset:{}, events:{}, attrs:{},
    classList:{toggle(name,on){on?classes.add(name):classes.delete(name)},add(name){classes.add(name)},remove(name){classes.delete(name)}},
    addEventListener(name,fn){this.events[name]=fn},setAttribute(k,v){this.attrs[k]=v},
    appendChild(child){this.children??=[];this.children.push(child)},setCustomValidity(v){this.validation=v},reportValidity(){return !this.validation}};
  Object.defineProperty(el,'innerHTML',{set(value){for(const match of value.matchAll(/id="([^"]+)"/g))get(match[1])}});
  return el;
}
function get(id){if(!elements.has(id))elements.set(id,element(id));return elements.get(id)}
for(const match of html.matchAll(/id="([^"]+)"/g))get(match[1]);
const tabs=['overview','network','weather','settings','sdlog'].map(id=>{const el=get('tab-'+id);el.dataset.tab=id;return el});
const values=[...html.matchAll(/data-value="([^"]+)"/g)].map(m=>{const el=element();el.dataset.value=m[1];return el});
const refreshButtons=[element(),element()];
const document={hidden:false,getElementById:get,createElement:()=>element(),querySelectorAll(selector){
  if(selector==='[data-tab]')return tabs;if(selector==='[data-value]')return values;
  if(selector==='.refresh')return refreshButtons;if(selector==='#ip-fields input')return [1,2,3,4].map(i=>get('ip'+i));return []}};
const data={uptime_segundos:93784,rssi_dbm:-62,wifi_conectado:true,ram_livre:'148 KB',_form_ssid0:'Rede "teste" <script>',clima:{sincronizado:true,previsao_valida:true,temperatura:23.4,umidade:71,minima:18,maxima:27,chuva:35,data:'2026-10-10',condicao:'Nublado',idade_s:90}};
for(let i=1;i<=4;i++){data['_form_ip'+i]='192.168.0.'+(120+i);data['esp'+i+'_online']=i===1}
let fail=false,requests=0;
let logOk=true;
const logText='COMANDO_RX <script>alert(1)</script>\nRESPOSTA_TX ok';
const context=vm.createContext({document,console,AbortSignal,Date,URLSearchParams,setInterval:()=>{},confirm:()=>false,fetch:async(url)=>{requests++;if(fail)throw Error('offline');if(url==='/api/log')return {ok:logOk,text:async()=>logOk?logText:'Cartao SD indisponivel.'};return {ok:true,json:async()=>data}}});
vm.runInContext(script,context);
(async()=>{
  await new Promise(resolve=>setImmediate(resolve));
  assert.equal(get('ssid0').value,data._form_ssid0);
  assert.equal(get('save').disabled,false);
  assert.equal(get('weather-temp').textContent,'23.4 °C');
  assert.equal(get('esp-state1').textContent,'Resposta recebida');
  get('ssid0').value='Edicao em andamento';get('ip1').value='10.0.0.22';
  await vm.runInContext('refresh()',context);
  assert.equal(get('ssid0').value,'Edicao em andamento');
  assert.equal(get('ip1').value,'10.0.0.22');
  tabs[3].events.click();assert.equal(get('settings').hidden,false);assert.equal(get('overview').hidden,true);
  get('ip1').value='999.1.1.1';
  await get('config-form').events.submit({preventDefault(){}});
  assert.match(get('ip1').validation,/IPv4/);
  const before=requests;fail=true;await vm.runInContext('refresh()',context);
  assert.equal(requests,before+1);assert.equal(get('connection').textContent,'Sem comunicação');
  assert.equal(get('weather-temp').textContent,'23.4 °C');assert.equal(get('notice').hidden,false);
  assert.equal(refreshButtons[0].disabled,false);
  fail=false;await vm.runInContext('refresh()',context);assert.equal(get('notice').hidden,true);
  for(let i=0;i<65;i++)vm.runInContext('render('+JSON.stringify(data)+')',context);
  assert.equal(get('signal-line').attrs.points.split(' ').length,60);
  tabs[4].events.click();await new Promise(resolve=>setImmediate(resolve));
  assert.equal(get('sdlog').hidden,false);assert.equal(get('settings').hidden,true);
  assert.equal(get('log-content').textContent,logText);
  logOk=false;await get('log-refresh').events.click();
  assert.match(get('log-status').textContent,/indisponivel/);
  assert.equal(get('log-content').textContent,logText);
  assert.equal(get('log-refresh').disabled,false);
  console.log('OK: leitura, clima, abas, preservacao da edicao, IPv4 invalido, falha/recuperacao e limite do grafico.');
})().catch(error=>{console.error(error);process.exitCode=1});
