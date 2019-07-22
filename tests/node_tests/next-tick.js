const fs = require('fs');

process.nextTick(() => {
  fs.unlink("foo", () => {

  });
})

fs.writeFileSync("foo", "data");
