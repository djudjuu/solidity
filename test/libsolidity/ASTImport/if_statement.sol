contract C {
  function abs(int x) public returns(uint) {
    if (x < 0) {
      return uint(-x);
    } else {
      return uint(x);
    }
  }

  function abs2(int x) public returns(uint) {
    return uint(x < 0 ? -x : x);
  }
}
