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
    return Promise.resolve({
      then: function fun4(res) {
        t = {bar : 1};
        res('bar');
      }
    });
  }
}


var x = Promise.resolve('bar');
x.then(function fun1(value) {
  return foo();
}).then(function fun5(value) {
  t.bar;
}).catch(function fun7(value) {  });
