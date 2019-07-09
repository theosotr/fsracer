const fs = require('fs')

fs.writeFileSync("foo", "data");
var promise = new Promise((resolve) => {
  fs.readFile("foo", (data) => {
    resolve(data);
  })
})

promise.then((data) => {
  fs.unlink("foo", () => {});
}, (err) => {
  fs.unlink(data, () => {});
})
