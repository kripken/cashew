function func() {
  print(10);
}
function another(x, y) {
  func();
  return;
}
// hello
/*
world
*/

function varr() {
  var x;
  var a = 5, b, c = func(a, b), d;
  return 20;
}

