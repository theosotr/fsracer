const fs = require('fs')


setImmediate(() => {
  setTimeout(() => {
    fs.unlinkSync("foo");
  });
});


process.nextTick(() => {
  fs.writeFileSync("foo", "data");
});
