const fs = require('fs');
const ah = require('async_hooks')
function init(a, b, c) {
  fs.writeSync(1, `${a}, ${b}, ${c}\n`);
}
function before(a) {
  fs.writeSync(1, `Before ${a}\n`);
}
function after() {  }
function destroy() {  }
function resolve() {  }
ah.createHook({ init, before, after, destroy, resolve }).enable()

fs.writeFileSync('foo', 'line1\nline2');
fs.createReadStream('foo')
  .pipe(fs.createWriteStream('bar'));
