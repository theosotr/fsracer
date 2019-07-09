const fs = require('fs');


fs.writeFile('foo', 'data', () => {
  fs.readFile('foo', () => {
    fs.unlink('foo', () => {
      fs.writeSync(1, 'Done\n');
    })
  })
})
