const fs = require('fs');
const ah = require('async_hooks');

function init(asyncId, type, trigger, resource) {
  fs.writeSync(1, `Create ${asyncId}, ${type}\n`);
}
function before(asyncID) {  }
function after() {}
function destroy() {}
function resolve() {}


fs.copyFile("bar", "foo", () => {})
fs.copyFile("baz", "bar", () => {
  fs.copyFile("bar", "foo", () => {})
})
