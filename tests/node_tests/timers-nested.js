const fs = require('fs');

setTimeout(function foo() {
  setTimeout(function bar() {
    setTimeout(function baz() {
      fs.access("foo", () => {  });
    });
    x.foo // No error.
    x = undefined;
  });
  x = {foo : 1};
});
