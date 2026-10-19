// Compila todos os fontes da Central e do display com a configuracao local.
// As dependencias que nao usam TFT sao copiadas do cache, sem modifica-lo.
const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');
const {args,compiler}=require('./check-firmware.cjs');
const cache='C:/Users/paulo/AppData/Local/arduino/sketches/F4E009820E6B852A96415F494CEEE602';
const sdk='C:/Users/paulo/AppData/Local/Arduino15/packages/esp32/tools/esp32-libs/3.3.12';
const output=path.join(__dirname,'build-fonts'),deps=path.join(output,'deps');fs.mkdirSync(deps,{recursive:true});
const options=fs.readFileSync(path.join(__dirname,'build_opt.h'),'utf8').trim().split(/\s+/);
const headers=fs.readdirSync(__dirname).filter(n=>n.endsWith('.h')).sort().map(n=>fs.readFileSync(path.join(__dirname,n)));
const headerHash=crypto.createHash('sha256').update(Buffer.concat(headers)).digest('hex');
async function run(exe,argv){return await new Promise((resolve,reject)=>{const p=cp.spawn(exe,argv,{windowsHide:true});let out='',err='';const timer=setTimeout(()=>p.kill(),600000);p.stdout.on('data',s=>out+=s);p.stderr.on('data',s=>{if(err.length<8000)err+=s});p.on('error',e=>{clearTimeout(timer);reject(e)});p.on('close',n=>{clearTimeout(timer);n===0?resolve(out):reject(Error(err||out||'Falha no compilador'))})});}
function files(dir){return fs.readdirSync(dir,{withFileTypes:true}).flatMap(e=>e.isDirectory()?files(path.join(dir,e.name)):[path.join(dir,e.name)]);}
function snapshot(source,target){fs.mkdirSync(path.dirname(target),{recursive:true});for(let i=0;i<3;i++){const before=fs.statSync(source);fs.copyFileSync(source,target);const after=fs.statSync(source);if(before.size===after.size&&before.mtimeMs===after.mtimeMs&&fs.statSync(target).size===after.size)return target;}throw Error('Dependencia em alteracao: '+source);}
async function main(){
 const libraryRoot=path.join(cache,'libraries');
 const libraries=fs.readdirSync(libraryRoot,{withFileTypes:true}).filter(e=>e.isDirectory()&&!['TFT_eSPI','BluetoothSerial','HTTPClient'].includes(e.name));
 const libObjects=libraries.flatMap(e=>files(path.join(libraryRoot,e.name)).filter(p=>p.endsWith('.o')).map(p=>snapshot(p,path.join(deps,path.relative(libraryRoot,p)))));
 const mapPath=path.join(cache,'Central.ino.map'),map=fs.existsSync(mapPath)?fs.readFileSync(mapPath,'utf8'):'';
 let core=[...map.matchAll(/^LOAD (.+?\.a)\s*$/gm)].map(m=>m[1].trim()).find(p=>/core/i.test(p)&&fs.existsSync(p));
 if(!core){const base='C:/Users/paulo/AppData/Local/arduino/cores';core=fs.readdirSync(base).map(n=>path.join(base,n,'core.a')).filter(p=>fs.existsSync(p)).sort((a,b)=>fs.statSync(b).mtimeMs-fs.statSync(a).mtimeMs).find(p=>fs.readFileSync(p).includes(Buffer.from('/3.3.12/cores/esp32')));}
 if(!core)throw Error('Nucleo ESP32 3.3.12 nao encontrado');core=snapshot(core,path.join(deps,'core.a'));
 const sources=fs.readdirSync(__dirname).filter(n=>n.endsWith('.cpp')||n==='Central.ino').map(n=>path.join(__dirname,n));
 sources.push('C:/Users/paulo/OneDrive/Documentos/Arduino/libraries/TFT_eSPI/TFT_eSPI.cpp');
 const objects=sources.map(p=>path.join(output,path.basename(p)+'.o'));
 const jobs=sources.map((source,i)=>({source,object:objects[i]}));
 let failure=null;
 async function worker(){while(jobs.length&&!failure){const {source,object}=jobs.shift();try{const hash=crypto.createHash('sha256').update(fs.readFileSync(source)).update(headerHash).update(options.join(' ')).digest('hex');const stamp=object+'.sha256';if(!fs.existsSync(stamp)||fs.readFileSync(stamp,'utf8')!==hash){await run(compiler,[...args.filter(x=>x!=='-fsyntax-only'),'-Os',...options,'-c',source,'-o',object]);fs.writeFileSync(stamp,hash);}console.log(path.basename(source)+': OK');}catch(e){failure=e}}}
 await Promise.allSettled([worker(),worker(),worker()]);if(failure)throw failure;
 const elf=path.join(output,'Central.ino.elf'),wraps=['esp_panic_handler','esp_bt_mem_release','esp_bt_controller_mem_release','_ZN4chip11DeviceLayer8Internal10ESP32Utils13InitWiFiStackEv','_Z21esp_matter_mem_callocjj','_Z22esp_matter_mem_reallocPvj','_Z19esp_matter_mem_freePv'];
 await run(compiler,[`-Wl,--Map=${path.join(output,'Central.ino.map')}`,`-L${sdk}/lib`,`-L${sdk}/ld`,`-L${sdk}/qio_qspi`,...wraps.map(s=>'-Wl,--wrap='+s),'@'+sdk+'/flags/ld_flags','@'+sdk+'/flags/ld_scripts','-Wl,--start-group',...objects,...libObjects,core,'@'+sdk+'/flags/ld_libs','-Wl,--end-group','-Wl,-EL','-o',elf]);
 console.log('Linker: OK (IRAM dentro do limite).');const bin=path.join(output,'Central.ino.bin');
 console.log(await run('C:/Users/paulo/AppData/Local/Arduino15/packages/esp32/tools/esptool_py/5.3.1/esptool.exe',['--chip','esp32','elf2image','--flash-mode','dio','--flash-freq','80m','--flash-size','4MB','--elf-sha256-offset','0xb0','-o',bin,elf]));
 const size=fs.statSync(bin).size,limit=1966080;console.log(`Firmware: ${size} bytes. Limite OTA: ${limit}. Folga: ${limit-size}.`);if(size>limit)process.exitCode=2;
}
main().catch(e=>{console.error(e.message);process.exitCode=1});
