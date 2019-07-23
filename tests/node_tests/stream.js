const fs = require('fs');


fs.writeFileSync('foo', 'line1\nline2');
var stream = fs.createReadStream('foo')
  .pipe(fs.createWriteStream('bar'));
stream.on('close', () => {
  fs.unlinkSync('foo');
  fs.unlinkSync('bar');
})
