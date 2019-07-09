const fs = require('fs');

var t;

function foo(cond) {
  if (cond) {
    return Promise.resolve('foo').then(
      function fun2() {
        t = {bar: 1};
        return 'foo';
      }
    ).then(function fun3(value) {
      throw 'baz';
    });
  } else {
    return Promise.resolve('value');
  }
}


var x = Promise.resolve('bar');
x.then(function fun1(value) {
  return foo(true);
}).catch(function fun5(value) {
  t.bar;
  return foo(false)
}).then(function fun7(value) {
  fs.access("foo", () => {});
});
