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
function resolve() {}

const asyncHook = ah.createHook({init, before, after, destroy, resolve});
asyncHook.enable();


fs.writeFileSync("tmp", "data");


setImmediate(() => {
  fs.exists("tmp", (exists) => {
    if (exists) {
      fs.readFile("tmp", (err, data) => {
        if (err) {
          return;
        }
        console.log("Data: " + data);
      });
    }
  })
});


setTimeout(() => {
  fs.unlink("tmp", () => {
  })
});