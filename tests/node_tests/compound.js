const fs = require('fs');

fs.writeFileSync("bar", "data");

fs.copyFile("bar", "foo", () => {})
fs.copyFile("baz", "bar", () => {
  fs.copyFile("bar", "foo", () => {})
})
