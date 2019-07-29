const fs = require('fs');

setTimeout(() => {
  setTimeout(() => {
    setTimeout(() => {
      fs.access('foo', () => {

      });
    });
  }, 100);
})
