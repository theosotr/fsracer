const fs = require('fs');
const http = require('http');

http.get('http://google.com', () => {
  http.get("http://google.com", () => {
    fs.writeFile('foo', 'data', () => {
      fs.unlinkSync("foo");
    });
  })
});
