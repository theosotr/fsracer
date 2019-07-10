var t = {
  then: (res, rej) => {
    res(true);
  }
}

Promise.resolve(t).then((val) => {
  return Promise.resolve(true).then((v) => {
    return v;
  })
});

