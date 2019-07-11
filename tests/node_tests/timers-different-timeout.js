const fs = require('fs');


setTimeout(() => {
  fs.access("bar", () => {});
  setTimeout(() => {

  });
}, 10)


setTimeout(() => {
  fs.access("baz", () => {});
}, 11)

setTimeout(() => {
  fs.access("foo", () => {});
}, 10)
