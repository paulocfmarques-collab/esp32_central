// Verificacao de compilacao dos fontes com o compilador ESP32 instalado.
const fs = require('fs'), path = require('path'), cp = require('child_process');
const packages = 'C:/Users/paulo/AppData/Local/Arduino15/packages/esp32';
const core = packages + '/hardware/esp32/3.3.12';
const sdk = packages + '/tools/esp32-libs/3.3.12';
const compiler = packages + '/tools/esp-x32/2601/bin/xtensa-esp-elf-g++.exe';
const args = ['@'+sdk+'/flags/cpp_flags','@'+sdk+'/flags/defines','-iprefix',sdk+'/include/','@'+sdk+'/flags/includes',
  '-I'+sdk+'/qio_qspi/include','-I'+core+'/cores/esp32','-I'+core+'/variants/esp32','-I'+__dirname,
  '-DF_CPU=240000000L','-DARDUINO=10819','-DARDUINO_ARCH_ESP32','-DARDUINO_ESP32_DEV','-DESP32','-DCORE_DEBUG_LEVEL=0','-DARDUINO_USB_CDC_ON_BOOT=0','-fsyntax-only','-x','c++'];
for(const root of [core+'/libraries','C:/Users/paulo/OneDrive/Documentos/Arduino/libraries'])
  for(const entry of fs.readdirSync(root,{withFileTypes:true}))if(entry.isDirectory()){
    const lib=path.join(root,entry.name);args.push('-I'+lib);if(fs.existsSync(lib+'/src'))args.push('-I'+lib+'/src');
  }
const files = process.argv.length > 2 ? process.argv.slice(2) : fs.readdirSync(__dirname).filter(n=>n.endsWith('.cpp')||n.endsWith('.ino'));
for(const name of files){
  const result=cp.spawnSync(compiler,[...args,path.join(__dirname,name)],{encoding:'utf8',timeout:180000,windowsHide:true});
  if(result.status!==0){console.error(name+' FALHOU\n'+result.stderr);if(result.error)console.error(result.error.message);process.exit(1)}
  console.log(name+': OK');
}
