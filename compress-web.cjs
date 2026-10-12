// Execute depois de editar WebPage.h para atualizar a pagina usada no firmware.
const fs=require('fs'),zlib=require('zlib'),assert=require('assert/strict');
const source=fs.readFileSync(__dirname+'/WebPage.h','utf8');
const html=Buffer.from(source.split('R"rawliteral(')[1].split(')rawliteral"')[0],'utf8');
const gzip=zlib.gzipSync(html,{level:9});
assert.deepEqual(zlib.gunzipSync(gzip),html);
const lines=[];for(let i=0;i<gzip.length;i+=20)lines.push('  '+[...gzip.subarray(i,i+20)].map(n=>'0x'+n.toString(16).padStart(2,'0')).join(',')+',');
fs.writeFileSync(__dirname+'/WebPageGzip.h','#pragma once\n#include <Arduino.h>\n// Gerado por compress-web.cjs a partir de WebPage.h.\nconst uint8_t htmlPageGzip[] PROGMEM = {\n'+lines.join('\n')+'\n};\n');
console.log('Pagina: '+html.length+' bytes -> '+gzip.length+' bytes; conteudo verificado.');
