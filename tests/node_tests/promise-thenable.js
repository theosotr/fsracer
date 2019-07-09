const fs = require('fs');

var t = {
  then: (res, rej) => {
    res(false);
  }
};
var promise = Promise.resolve(t);
promise.then(() => {
  fs.access('bar', () => {});
})

promise = new Promise ((res) => {
  res(true);
})

promise.then(() => {
  // This I/O is done first because the first promise
  // resolves with a thenable.
  fs.access("foo", () => {});
});

