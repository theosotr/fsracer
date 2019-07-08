const fs = require('fs');
const ah = require('async_hooks');

function init(asyncId, type, trigger, resource) {
  fs.writeSync(1, `Create ${asyncId}, ${type}\n`);
}
function before(asyncID) {
  fs.writeSync(1, `Before ${asyncID}\n`);
}
function after() {}
function destroy() {}
function resolve(asyncId) {
  fs.writeSync(1, `Resolve ${asyncId}\n`);
}

const asyncHook = ah.createHook({init, before, after, destroy, resolve});
asyncHook.enable();


Promise.resolve().then(() => {
  fs.writeFile("foo", "data", () => {});
}).then(() => {
  fs.unlink("foo", () => {})
});

Promise.reject().catch(() => {
  fs.access("foo", () => {});
})
