const fs = require('fs');

Promise.resolve().then(() => {
  fs.writeFile("foo", "data", () => {
    fs.unlink("foo", () => {})
  });
}).then(() => {
  fs.readFile("foo", () => {

  })
});

Promise.reject().catch(() => {
  fs.access("foo", () => {});
})
