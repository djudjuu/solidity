contract C {

  function f(uint x) public returns(uint, uint, uint) {
    return (x, 2*x, 3*x);
  }

  function ff(uint x) public returns(uint) {
    uint a;
    uint b;
    uint c;

    (a,b,c) = f(x);
    return c;
  }
}
