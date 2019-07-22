const fs = require('fs')
const ah = require('async_hooks')
function init(a, b, c) {
  fs.writeSync(1, `${a}, ${b}, ${c}\n`);
}
function before(a) {
  fs.writeSync(1, `Before ${a}\n`);
}
function after(a) {

  fs.writeSync(1, `After ${a}\n`);
}
function destroy() {  }
function resolve() {  }
ah.createHook({ init, before, after, destroy, resolve }).enable()

const http = require('http');
http.get('http://google.com', () => {
  fs.access("foo", () => {

  });
});
