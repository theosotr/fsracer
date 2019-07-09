const fs = require('fs');

fs.writeFileSync("tmp", "data");


setImmediate(() => {
  fs.exists("tmp", (exists) => {
    if (exists) {
      fs.readFile("tmp", (err, data) => {
        if (err) {
          return;
        }
        console.log("Data: " + data);
      });
    }
  })
});


setTimeout(() => {
  fs.unlink("tmp", () => {
  })
});
